#pragma once

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Save current time to NVS.
 * Returns 0 on success, negative on error.
 */
int nvs_save_time(time_t now);

/**
 * Load time from NVS.
 * Returns 0 on success (time_out set), negative on error/not found.
 */
int nvs_load_time(time_t *time_out);

#ifdef __cplusplus
}
#endif
