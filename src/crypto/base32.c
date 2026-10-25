// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Base32 encoding/decoding (RFC 4648)

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "crypto/base32.h"
#include "crypto/common.h"

static const char base32_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

size_t base32_encoded_size(size_t len) {
    // Each group of 5 bytes takes 8 characters, the last one padded with '='.
    return groups_size(len, 5, 8);
}

size_t base32_encode(const uint8_t* src, size_t len, uint8_t* dst) {
    size_t j = 0;
    for (size_t i = 0; i < len;) {
        uint32_t octet0 = i < len ? src[i++] : 0;
        uint32_t octet1 = i < len ? src[i++] : 0;
        uint32_t octet2 = i < len ? src[i++] : 0;
        uint32_t octet3 = i < len ? src[i++] : 0;
        uint32_t octet4 = i < len ? src[i++] : 0;

        dst[j++] = base32_chars[octet0 >> 3];
        dst[j++] = base32_chars[((octet0 & 0x07) << 2) | (octet1 >> 6)];
        dst[j++] = base32_chars[(octet1 >> 1) & 0x1F];
        dst[j++] = base32_chars[((octet1 & 0x01) << 4) | (octet2 >> 4)];
        dst[j++] = base32_chars[((octet2 & 0x0F) << 1) | (octet3 >> 7)];
        dst[j++] = base32_chars[(octet3 >> 2) & 0x1F];
        dst[j++] = base32_chars[((octet3 & 0x03) << 3) | (octet4 >> 5)];
        dst[j++] = base32_chars[octet4 & 0x1F];
    }

    // Replace the characters that encode only zero padding with '='.
    if (len % 5 != 0) {
        size_t padding = 7 - (len % 5) * 8 / 5;
        for (size_t i = 0; i < padding; i++) {
            dst[j - padding + i] = '=';
        }
    }
    return j;
}

size_t base32_decoded_size(const uint8_t* src, size_t len) {
    (void)src;
    // Every character carries 5 bits.
    return len / 8 * 5 + len % 8 * 5 / 8;
}

bool base32_decode(const uint8_t* src, size_t len, uint8_t* dst, size_t* out_len) {
    // Padding is optional, so strip it and decode the rest.
    while (len > 0 && src[len - 1] == '=') {
        len--;
    }

    uint32_t value = 0;
    size_t bits = 0, count = 0;
    for (size_t i = 0; i < len; i++) {
        uint8_t c = src[i];
        if (c >= 'A' && c <= 'Z') {
            c -= 'A';
        } else if (c >= '2' && c <= '7') {
            c -= '2' - 26;
        } else {
            return false;
        }
        // Only the lowest bits that are not yet written out are kept,
        // so the value never exceeds 12 bits.
        value = ((value << 5) | c) & 0xFFF;
        bits += 5;
        if (bits >= 8) {
            dst[count++] = (uint8_t)(value >> (bits - 8));
            bits -= 8;
        }
    }

    // The leftover bits must be the zero padding of the last character,
    // and a whole leftover character means a truncated group.
    if (bits >= 5 || (value & ((1u << bits) - 1)) != 0) {
        return false;
    }
    *out_len = count;
    return true;
}
