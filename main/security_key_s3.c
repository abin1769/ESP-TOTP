#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include "base32.h"
#include "totp.h"
#include "wifi_time.h"
#include "nvs_time.h"

static const char *TAG = "totp_app";

void app_main(void)
{
	esp_err_t nvs_rc = nvs_flash_init();
	if (nvs_rc == ESP_ERR_NVS_NO_FREE_PAGES || nvs_rc == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		nvs_rc = nvs_flash_init();
	}
	ESP_ERROR_CHECK(nvs_rc);

	if (wifi_connect_sta() != 0) {
		ESP_LOGE(TAG, "WiFi connect gagal. Cek SSID/PW di menuconfig.");
		// Fallback: coba load waktu dari NVS
		time_t saved_time = 0;
		if (nvs_load_time(&saved_time) == 0 && saved_time > 0) {
			ESP_LOGI(TAG, "Using time from NVS (offline mode)");
			struct timeval tv;
			tv.tv_sec = saved_time;
			tv.tv_usec = 0;
			settimeofday(&tv, NULL);
		} else {
			ESP_LOGE(TAG, "Tidak ada waktu tersimpan di NVS. WiFi diperlukan untuk sinkronisasi awal.");
			return;
		}
	} else {
		// WiFi konek, sync SNTP
		if (sync_time_via_sntp() != 0) {
			ESP_LOGW(TAG, "╔══════════════════════════════════════╗");
			ESP_LOGW(TAG, "║  SNTP SYNC FAILED (OFFLINE MODE)    ║");
			ESP_LOGW(TAG, "║  Menggunakan waktu terakhir (cache) ║");
			ESP_LOGW(TAG, "║  TOTP mungkin salah jika cache lama ║");
			ESP_LOGW(TAG, "╚══════════════════════════════════════╝");
			
			time_t saved_time = 0;
			if (nvs_load_time(&saved_time) == 0 && saved_time > 0) {
				ESP_LOGI(TAG, "Using time from NVS: %ld", (long)saved_time);
				struct timeval tv;
				tv.tv_sec = saved_time;
				tv.tv_usec = 0;
				settimeofday(&tv, NULL);
				
				// Calculate how old the cached time is
				time_t now;
				time(&now);
				int age_sec = (int)(now - saved_time);
				if (age_sec > 300) {
					ESP_LOGW(TAG, "⚠️  Cache age: %d seconds - TOTP mungkin inaccurate!", age_sec);
				}
			} else {
				ESP_LOGE(TAG, "SNTP gagal & tidak ada backup waktu di NVS. WiFi + Internet diperlukan untuk sinkronisasi awal.");
				ESP_LOGE(TAG, "Solusi: 1) Cek SSID/Password di menuconfig 2) Cek koneksi internet 3) Cek port 123 (NTP)");
				return;
			}
		} else {
			// SNTP sukses, simpan waktu ke NVS
			time_t now = 0;
			time(&now);
			nvs_save_time(now);
		}
	}

	if (strlen(CONFIG_TOTP_SECRET_BASE32) == 0) {
		ESP_LOGE(TAG, "Secret Base32 kosong. Set CONFIG_TOTP_SECRET_BASE32 via menuconfig.");
		return;
	}

	uint8_t secret[64];
	size_t secret_len = 0;
	int dec_rc = base32_decode(CONFIG_TOTP_SECRET_BASE32, secret, sizeof(secret), &secret_len);
	if (dec_rc != 0 || secret_len == 0) {
		ESP_LOGE(TAG, "Base32 decode gagal (rc=%d). Pastikan secret valid.", dec_rc);
		return;
	}

	ESP_LOGI(TAG, "TOTP ready (digits=%d, step=%d).", CONFIG_TOTP_DIGITS, CONFIG_TOTP_TIME_STEP);

	uint64_t last_counter = UINT64_MAX;
	while (1) {
		time_t now = 0;
		time(&now);
		if (now <= 0) {
			vTaskDelay(pdMS_TO_TICKS(1000));
			continue;
		}

		uint64_t counter = ((uint64_t)now - (uint64_t)CONFIG_TOTP_T0) / (uint32_t)CONFIG_TOTP_TIME_STEP;
		if (counter != last_counter) {
			last_counter = counter;

			char code[10];
			int totp_rc = totp_generate_sha256(secret, secret_len, (uint64_t)now,
									  CONFIG_TOTP_DIGITS,
									  (uint32_t)CONFIG_TOTP_TIME_STEP,
									  (uint64_t)CONFIG_TOTP_T0,
									  code, sizeof(code));
			if (totp_rc == 0) {
				int remain = CONFIG_TOTP_TIME_STEP - (int)(((uint64_t)now - (uint64_t)CONFIG_TOTP_T0) % (uint32_t)CONFIG_TOTP_TIME_STEP);
				ESP_LOGI(TAG, "TOTP: %s (valid ~%ds)", code, remain);
			} else {
				ESP_LOGE(TAG, "TOTP generate gagal (rc=%d)", totp_rc);
			}
		}

		vTaskDelay(pdMS_TO_TICKS(200));
	}
}
