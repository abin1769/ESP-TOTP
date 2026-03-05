#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** Connect to WiFi (STA) using CONFIG_TOTP_WIFI_*.
 * Returns 0 on success, negative on error.
 */
int wifi_connect_sta(void);

/** Sync time via SNTP using CONFIG_TOTP_SNTP_SERVER.
 * Returns 0 on success, negative on error.
 */
int sync_time_via_sntp(void);

#ifdef __cplusplus
}
#endif
