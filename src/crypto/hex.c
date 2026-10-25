// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Hex encoding/decoding

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "crypto/common.h"
#include "crypto/hex.h"

static const char hex_chars[] = "0123456789abcdef";

size_t hex_encoded_size(size_t len) {
    // Each byte takes 2 characters.
    return groups_size(len, 1, 2);
}

size_t hex_encode(const uint8_t* src, size_t len, uint8_t* dst) {
    for (size_t i = 0; i < len; i++) {
        dst[i * 2] = hex_chars[src[i] >> 4];
        dst[i * 2 + 1] = hex_chars[src[i] & 0x0F];
    }
    return len * 2;
}

size_t hex_decoded_size(const uint8_t* src, size_t len) {
    (void)src;
    return len / 2;
}

bool hex_decode(const uint8_t* src, size_t len, uint8_t* dst, size_t* out_len) {
    if (len % 2 != 0) {
        return false;
    }
    for (size_t i = 0; i < len / 2; i++) {
        uint8_t hi, lo;
        if (!hex_value(src[i * 2], &hi) || !hex_value(src[i * 2 + 1], &lo)) {
            return false;
        }
        dst[i] = (uint8_t)(hi << 4 | lo);
    }
    *out_len = len / 2;
    return true;
}
