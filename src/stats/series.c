// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Based on series.c by D. Richard Hipp, Public Domain
// https://sqlite.org/src/file/ext/misc/series.c

// Arithmetic series.

#include <stdbool.h>
#include <stdint.h>

#include "stats/common.h"
#include "stats/stats.h"

#pragma region Series

// series_make returns the series from start to stop (inclusive), step apart.
// A step of 0 is 1. The series is empty if start > stop.
//
// A negative step gives the same values as its absolute value,
// in decreasing order: (1, 10, -3) is 10, 7, 4, 1.
//
// The values never overflow: the series ends at the last value
// that is not past stop.
Series series_make(int64_t start, int64_t stop, int64_t step) {
    Series s = {0};
    if (start > stop) {
        s.empty = true;
        return s;
    }
    // -step overflows for INT64_MIN, so negate in unsigned.
    if (step == 0) {
        s.step = 1;
    } else if (step > 0) {
        s.step = (uint64_t)step;
    } else {
        s.step = 0 - (uint64_t)step;
    }
    // stop - start overflows int64 when they are far apart, but it
    // always fits uint64, and the wrapping subtraction gives it exactly.
    uint64_t span = (uint64_t)stop - (uint64_t)start;
    s.last = span / s.step;
    s.desc = step < 0;
    if (s.desc) {
        // last * step <= span, so the last value is within [start, stop].
        s.first = uint64_to_int64((uint64_t)start + s.last * s.step);
    } else {
        s.first = start;
    }
    return s;
}

// series_at returns the value at index i of a non-empty series (i <= s.last).
int64_t series_at(Series s, uint64_t i) {
    // Signed arithmetic would overflow in the intermediate steps for
    // far-apart values, so compute in unsigned. The result is within
    // [start, stop], so the conversion back is exact.
    uint64_t offset = i * s.step;
    if (s.desc) {
        return uint64_to_int64((uint64_t)s.first - offset);
    }
    return uint64_to_int64((uint64_t)s.first + offset);
}

#pragma endregion
