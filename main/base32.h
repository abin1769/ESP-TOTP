#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Decode Base32 (RFC4648) into raw bytes.
 *
 * - Accepts upper/lowercase.
 * - Ignores spaces and hyphens.
 * - Supports '=' padding.
 *
 * Returns 0 on success, negative on error.
 */
int base32_decode(const char *input, uint8_t *output, size_t output_size, size_t *output_len);

#ifdef __cplusplus
}
#endif
