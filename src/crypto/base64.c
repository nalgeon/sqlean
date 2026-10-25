// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Base64 encoding/decoding (RFC 4648)

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "crypto/base64.h"
#include "crypto/common.h"

static const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Maps every byte to its base64 value plus one, so that the bytes
// left out (the zeros) are the ones that are not base64 characters.
static const uint8_t base64_table[256] = {
    ['A'] = 1,  ['B'] = 2,  ['C'] = 3,  ['D'] = 4,  ['E'] = 5,  ['F'] = 6,  ['G'] = 7,  ['H'] = 8,
    ['I'] = 9,  ['J'] = 10, ['K'] = 11, ['L'] = 12, ['M'] = 13, ['N'] = 14, ['O'] = 15, ['P'] = 16,
    ['Q'] = 17, ['R'] = 18, ['S'] = 19, ['T'] = 20, ['U'] = 21, ['V'] = 22, ['W'] = 23, ['X'] = 24,
    ['Y'] = 25, ['Z'] = 26, ['a'] = 27, ['b'] = 28, ['c'] = 29, ['d'] = 30, ['e'] = 31, ['f'] = 32,
    ['g'] = 33, ['h'] = 34, ['i'] = 35, ['j'] = 36, ['k'] = 37, ['l'] = 38, ['m'] = 39, ['n'] = 40,
    ['o'] = 41, ['p'] = 42, ['q'] = 43, ['r'] = 44, ['s'] = 45, ['t'] = 46, ['u'] = 47, ['v'] = 48,
    ['w'] = 49, ['x'] = 50, ['y'] = 51, ['z'] = 52, ['0'] = 53, ['1'] = 54, ['2'] = 55, ['3'] = 56,
    ['4'] = 57, ['5'] = 58, ['6'] = 59, ['7'] = 60, ['8'] = 61, ['9'] = 62, ['+'] = 63, ['/'] = 64,
};

size_t base64_encoded_size(size_t len) {
    // Each group of 3 bytes takes 4 characters, the last one padded with '='.
    return groups_size(len, 3, 4);
}

size_t base64_encode(const uint8_t* src, size_t len, uint8_t* dst) {
    size_t j = 0;
    for (size_t i = 0; i < len; i += 3) {
        uint32_t octets = ((uint32_t)src[i] << 16) |
                          ((uint32_t)(i + 1 < len ? src[i + 1] : 0) << 8) |
                          (uint32_t)(i + 2 < len ? src[i + 2] : 0);
        dst[j++] = base64_chars[(octets >> 18) & 0x3f];
        dst[j++] = base64_chars[(octets >> 12) & 0x3f];
        dst[j++] = base64_chars[(octets >> 6) & 0x3f];
        dst[j++] = base64_chars[octets & 0x3f];
    }

    if (len % 3 == 1) {
        dst[j - 1] = '=';
        dst[j - 2] = '=';
    } else if (len % 3 == 2) {
        dst[j - 1] = '=';
    }
    return j;
}

size_t base64_decoded_size(const uint8_t* src, size_t len) {
    (void)src;
    // Each group of 4 characters decodes into at most 3 bytes.
    return len / 4 * 3;
}

bool base64_decode(const uint8_t* src, size_t len, uint8_t* dst, size_t* out_len) {
    // The input consists of full groups, with the last one padded with '='.
    if (len % 4 != 0) {
        return false;
    }
    size_t padding = 0;
    if (len > 0 && src[len - 1] == '=') {
        padding++;
        if (src[len - 2] == '=') {
            padding++;
        }
    }

    // Every character but the padding must be a base64 one
    // (so '=' anywhere else is invalid).
    size_t data_len = len - padding;
    size_t j = 0;
    for (size_t i = 0; i < len; i += 4) {
        uint32_t block = 0;
        for (size_t k = 0; k < 4; k++) {
            block <<= 6;
            if (i + k >= data_len) {
                continue;
            }
            uint8_t value = base64_table[src[i + k]];
            if (value == 0) {
                return false;
            }
            block |= value - 1u;
        }

        // A padded group holds fewer bytes. The padding bits
        // of its last character are ignored.
        size_t n_bytes = i + 4 <= data_len ? 3 : 3 - padding;
        dst[j++] = (uint8_t)(block >> 16);
        if (n_bytes > 1) {
            dst[j++] = (uint8_t)(block >> 8);
        }
        if (n_bytes > 2) {
            dst[j++] = (uint8_t)block;
        }
    }

    *out_len = j;
    return true;
}
