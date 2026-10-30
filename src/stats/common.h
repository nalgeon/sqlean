// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Conversions shared by the domain and binding files.

#ifndef STATS_COMMON_H
#define STATS_COMMON_H

#include <stdint.h>

// uint64_to_int64 interprets u modulo 2^64 as a signed 64-bit value.
// Unlike a direct cast, this is fully defined when u > INT64_MAX.
static inline int64_t uint64_to_int64(uint64_t u) {
    if (u <= INT64_MAX) {
        return (int64_t)u;
    }
    return -1 - (int64_t)(UINT64_MAX - u);
}

#endif /* STATS_COMMON_H */
