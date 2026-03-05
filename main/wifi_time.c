#include "wifi_time.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_wifi.h"

static const char *TAG = "wifi_time";

static EventGroupHandle_t s_wifi_event_group;
static const int WIFI_CONNECTED_BIT = BIT0;
static const int WIFI_FAIL_BIT = BIT1;

static int s_retry_num;

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 10) {
            s_retry_num++;
            ESP_LOGW(TAG, "WiFi disconnected; retry %d", s_retry_num);
            esp_wifi_connect();
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

int wifi_connect_sta(void)
{
    if (strlen(CONFIG_TOTP_WIFI_SSID) == 0) {
        ESP_LOGE(TAG, "CONFIG_TOTP_WIFI_SSID kosong. Set via menuconfig.");
        return -1;
    }

    s_wifi_event_group = xEventGroupCreate();
    if (!s_wifi_event_group) return -2;

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, CONFIG_TOTP_WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, CONFIG_TOTP_WIFI_PASSWORD, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = (strlen(CONFIG_TOTP_WIFI_PASSWORD) == 0) ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                          WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                          pdFALSE,
                                          pdFALSE,
                                          pdMS_TO_TICKS(20000));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi connected");
        return 0;
    }
    if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "WiFi failed");
        return -3;
    }

    ESP_LOGE(TAG, "WiFi timeout");
    return -4;
}

int sync_time_via_sntp(void)
{
    setenv("TZ", "UTC0", 1);
    tzset();

    // Try primary server first
    if (strlen(CONFIG_TOTP_SNTP_SERVER) > 0) {
        ESP_LOGI(TAG, "SNTP init (primary: %s, timeout: %d sec)", CONFIG_TOTP_SNTP_SERVER, CONFIG_TOTP_SNTP_TIMEOUT_SEC);
        esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
        esp_sntp_setservername(0, CONFIG_TOTP_SNTP_SERVER);
        esp_sntp_init();

        // Calculate retry count based on timeout config
        int max_retries = CONFIG_TOTP_SNTP_TIMEOUT_SEC * 2;  // 500ms per retry
        for (int i = 0; i < max_retries; ++i) {
            time_t now = 0;
            struct tm timeinfo = {0};
            time(&now);
            localtime_r(&now, &timeinfo);
            
            if (timeinfo.tm_year >= (2020 - 1900)) {
                ESP_LOGI(TAG, "Time synced from primary server: %ld", (long)now);
                return 0;
            }
            
            if (i % 20 == 0 && i > 0) {  // Log every 10 seconds
                ESP_LOGD(TAG, "SNTP waiting... (%d/%d sec)", i / 2, CONFIG_TOTP_SNTP_TIMEOUT_SEC);
            }
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        ESP_LOGW(TAG, "Primary SNTP server timeout");
    }

    // Try backup server if configured
    if (strlen(CONFIG_TOTP_SNTP_SERVER_BACKUP) > 0) {
        ESP_LOGI(TAG, "Trying backup server: %s", CONFIG_TOTP_SNTP_SERVER_BACKUP);
        esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
        esp_sntp_setservername(0, CONFIG_TOTP_SNTP_SERVER_BACKUP);
        esp_sntp_init();

        int max_retries = CONFIG_TOTP_SNTP_TIMEOUT_SEC * 2;
        for (int i = 0; i < max_retries; ++i) {
            time_t now = 0;
            struct tm timeinfo = {0};
            time(&now);
            localtime_r(&now, &timeinfo);
            
            if (timeinfo.tm_year >= (2020 - 1900)) {
                ESP_LOGI(TAG, "Time synced from backup server: %ld", (long)now);
                return 0;
            }
            
            if (i % 20 == 0 && i > 0) {
                ESP_LOGD(TAG, "SNTP backup waiting... (%d/%d sec)", i / 2, CONFIG_TOTP_SNTP_TIMEOUT_SEC);
            }
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        ESP_LOGW(TAG, "Backup SNTP server timeout");
    }

    ESP_LOGE(TAG, "SNTP sync failed (all servers timeout)");
    return -2;
}
