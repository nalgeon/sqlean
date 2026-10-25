// Copyright (c) 2025 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Helpers shared by the encoders and decoders.

#ifndef CRYPTO_COMMON_H
#define CRYPTO_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// groups_size returns the size of n_groups groups of group_size bytes each,
// where n_groups = ceil(len / group_len), or SIZE_MAX if it does not fit
// into size_t. SIZE_MAX can never be allocated, so the caller fails cleanly.
static inline size_t groups_size(size_t len, size_t group_len, size_t group_size) {
    size_t n_groups = len / group_len + (len % group_len != 0);
    if (n_groups > SIZE_MAX / group_size) {
        return SIZE_MAX;
    }
    return n_groups * group_size;
}

// hex_value sets value to the value of a hex digit (either case),
// or returns false if c is not one.
static inline bool hex_value(uint8_t c, uint8_t* value) {
    if (c >= '0' && c <= '9') {
        *value = c - '0';
    } else if (c >= 'A' && c <= 'F') {
        *value = c - 'A' + 10;
    } else if (c >= 'a' && c <= 'f') {
        *value = c - 'a' + 10;
    } else {
        return false;
    }
    return true;
}

#endif /* CRYPTO_COMMON_H */
