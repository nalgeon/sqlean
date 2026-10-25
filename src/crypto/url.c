// Originally by Fränz Friederes, MIT License
// https://github.com/cryptii/cryptii/blob/main/src/Encoder/URL.js

// Modified by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean/

// URL-escape encoding/decoding

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "crypto/common.h"
#include "crypto/url.h"

static const char hex_chars[] = "0123456789ABCDEF";

// is_unreserved reports whether c is an RFC 3986 unreserved character,
// which is left as is by the encoder.
static bool is_unreserved(uint8_t c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' ||
           c == '_' || c == '.' || c == '~';
}

size_t url_encoded_size(size_t len) {
    // Each byte takes at most 3 characters (%XX).
    return groups_size(len, 1, 3);
}

size_t url_encode(const uint8_t* src, size_t len, uint8_t* dst) {
    size_t pos = 0;
    for (size_t i = 0; i < len; i++) {
        if (is_unreserved(src[i])) {
            dst[pos++] = src[i];
        } else {
            dst[pos++] = '%';
            dst[pos++] = hex_chars[src[i] >> 4];
            dst[pos++] = hex_chars[src[i] & 0x0F];
        }
    }
    return pos;
}

size_t url_decoded_size(const uint8_t* src, size_t len) {
    (void)src;
    // Each character decodes into at most one byte.
    return len;
}

bool url_decode(const uint8_t* src, size_t len, uint8_t* dst, size_t* out_len) {
    size_t pos = 0;
    for (size_t i = 0; i < len; i++) {
        if (src[i] == '%') {
            // An escape must be followed by two hex digits.
            uint8_t hi, lo;
            if (len - i < 3 || !hex_value(src[i + 1], &hi) || !hex_value(src[i + 2], &lo)) {
                return false;
            }
            dst[pos++] = (uint8_t)(hi << 4 | lo);
            i += 2;
        } else if (src[i] == '+') {
            // Form encoding (application/x-www-form-urlencoded) uses '+' for a space.
            dst[pos++] = ' ';
        } else {
            // Other characters are taken as is, even if they should have been escaped.
            dst[pos++] = src[i];
        }
    }
    *out_len = pos;
    return true;
}
