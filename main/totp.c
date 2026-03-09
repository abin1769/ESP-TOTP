#include "totp.h"

#include <inttypes.h>
#include <stdio.h>

#include "mbedtls/md.h"

static uint32_t pow10_u32(int digits)
{
    uint32_t p = 1;
    for (int i = 0; i < digits; ++i) {
        p *= 10u;
    }
    return p;
}

int totp_generate_sha256(const uint8_t *key, size_t key_len,
                         uint64_t timestamp,
                         int digits,
                         uint32_t timestep,
                         uint64_t t0,
                         char *out_code,
                         size_t out_code_size)
{
    if (!key || key_len == 0 || !out_code) return -1;
    if (digits < 6 || digits > 8) return -2;
    if (timestep == 0) return -3;
    if (out_code_size < (size_t)digits + 1) return -4;
    if (timestamp < t0) return -5;

    uint64_t counter = (timestamp - t0) / timestep;

    uint8_t msg[8];
    for (int i = 7; i >= 0; --i) {
        msg[i] = (uint8_t)(counter & 0xFFu);
        counter >>= 8;
    }

    const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!md_info) return -6;

    uint8_t hmac[32];
    int rc = mbedtls_md_hmac(md_info, key, key_len, msg, sizeof(msg), hmac);
    if (rc != 0) return -7;

    int offset = hmac[31] & 0x0F;
    uint32_t bin_code = ((uint32_t)(hmac[offset] & 0x7F) << 24) |
                        ((uint32_t)(hmac[offset + 1]) << 16) |
                        ((uint32_t)(hmac[offset + 2]) << 8) |
                        (uint32_t)(hmac[offset + 3]);

    uint32_t otp = bin_code % pow10_u32(digits);

    // Zero-pad to digits
    // Example for digits=6: "%06" PRIu32
    char fmt[16];
    snprintf(fmt, sizeof(fmt), "%%0%du", digits);
    snprintf(out_code, out_code_size, fmt, otp);
    return 0;
}
