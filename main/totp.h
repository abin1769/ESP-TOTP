#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Generate a TOTP code according to RFC6238 using HMAC-SHA256.
 *
 * timestamp: Unix time (seconds).
 * digits: typically 6.
 * timestep: typically 30.
 * t0: typically 0.
 *
 * out_code must have space for digits + 1 (NUL).
 * Returns 0 on success, negative on error.
 */
int totp_generate_sha256(const uint8_t *key, size_t key_len,
                         uint64_t timestamp,
                         int digits,
                         uint32_t timestep,
                         uint64_t t0,
                         char *out_code,
                         size_t out_code_size);

#ifdef __cplusplus
}
#endif
