// Copyright (c) 2025 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Helpers shared by the encoders, decoders and hash functions.

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

#pragma region Bits

// UNROLL asks the compiler to fully unroll the loop that follows. The hash
// block functions use it on their fixed-size loops: unrolled, the working
// variables stay in registers instead of being shuffled on every step,
// which makes the hashes 1.3-1.8x faster. Other compilers ignore it.
#if defined(__GNUC__) || defined(__clang__)
#define UNROLL _Pragma("GCC unroll 80")
#else
#define UNROLL
#endif

static inline uint32_t rotl32(uint32_t x, unsigned n) {
    return (x << n) | (x >> (32 - n));
}

static inline uint32_t rotr32(uint32_t x, unsigned n) {
    return (x >> n) | (x << (32 - n));
}

static inline uint64_t rotr64(uint64_t x, unsigned n) {
    return (x >> n) | (x << (64 - n));
}

#pragma endregion

#pragma region Byte order

static inline uint32_t load_le32(const uint8_t* p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static inline void store_le32(uint8_t* p, uint32_t x) {
    p[0] = (uint8_t)x;
    p[1] = (uint8_t)(x >> 8);
    p[2] = (uint8_t)(x >> 16);
    p[3] = (uint8_t)(x >> 24);
}

static inline void store_le64(uint8_t* p, uint64_t x) {
    store_le32(p, (uint32_t)x);
    store_le32(p + 4, (uint32_t)(x >> 32));
}

static inline uint32_t load_be32(const uint8_t* p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | (uint32_t)p[3];
}

static inline uint64_t load_be64(const uint8_t* p) {
    return (uint64_t)load_be32(p) << 32 | load_be32(p + 4);
}

static inline void store_be32(uint8_t* p, uint32_t x) {
    p[0] = (uint8_t)(x >> 24);
    p[1] = (uint8_t)(x >> 16);
    p[2] = (uint8_t)(x >> 8);
    p[3] = (uint8_t)x;
}

static inline void store_be64(uint8_t* p, uint64_t x) {
    store_be32(p, (uint32_t)(x >> 32));
    store_be32(p + 4, (uint32_t)x);
}

#pragma endregion

#endif /* CRYPTO_COMMON_H */
