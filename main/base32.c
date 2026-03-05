#include "base32.h"

#include <ctype.h>

static int base32_value(int c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a';
    if (c >= '2' && c <= '7') return 26 + (c - '2');
    return -1;
}

int base32_decode(const char *input, uint8_t *output, size_t output_size, size_t *output_len)
{
    if (!input || !output || !output_len) {
        return -1;
    }

    size_t out_len = 0;
    uint32_t buffer = 0;
    int bits_left = 0;

    for (const char *p = input; *p; ++p) {
        unsigned char ch = (unsigned char)*p;
        if (ch == '=') {
            break;
        }
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '-') {
            continue;
        }

        int val = base32_value(ch);
        if (val < 0) {
            return -2;
        }

        buffer = (buffer << 5) | (uint32_t)val;
        bits_left += 5;

        while (bits_left >= 8) {
            bits_left -= 8;
            if (out_len >= output_size) {
                return -3;
            }
            output[out_len++] = (uint8_t)((buffer >> bits_left) & 0xFFu);
        }
    }

    *output_len = out_len;
    return 0;
}
