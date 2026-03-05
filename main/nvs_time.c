#include "nvs_time.h"

#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "nvs_time";
static const char *NVS_NAMESPACE = "totp";
static const char *NVS_KEY_TIME = "last_time";

int nvs_save_time(time_t now)
{
    nvs_handle_t handle;
    esp_err_t rc = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (rc != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open failed: %s", esp_err_to_name(rc));
        return -1;
    }

    rc = nvs_set_i64(handle, NVS_KEY_TIME, (int64_t)now);
    if (rc != ESP_OK) {
        ESP_LOGE(TAG, "nvs_set_i64 failed: %s", esp_err_to_name(rc));
        nvs_close(handle);
        return -2;
    }

    rc = nvs_commit(handle);
    if (rc != ESP_OK) {
        ESP_LOGE(TAG, "nvs_commit failed: %s", esp_err_to_name(rc));
        nvs_close(handle);
        return -3;
    }

    nvs_close(handle);
    ESP_LOGI(TAG, "Time saved to NVS: %ld", (long)now);
    return 0;
}

int nvs_load_time(time_t *time_out)
{
    if (!time_out) return -1;

    nvs_handle_t handle;
    esp_err_t rc = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (rc != ESP_OK) {
        ESP_LOGW(TAG, "nvs_open failed: %s (maybe first boot?)", esp_err_to_name(rc));
        return -2;
    }

    int64_t saved_time = 0;
    rc = nvs_get_i64(handle, NVS_KEY_TIME, &saved_time);
    if (rc != ESP_OK) {
        ESP_LOGW(TAG, "nvs_get_i64 failed: %s (key not found)", esp_err_to_name(rc));
        nvs_close(handle);
        return -3;
    }

    nvs_close(handle);
    *time_out = (time_t)saved_time;
    ESP_LOGI(TAG, "Time loaded from NVS: %ld", (long)saved_time);
    return 0;
}
