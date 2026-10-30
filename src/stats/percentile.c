// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Based on percentile.c by D. Richard Hipp, Public Domain
// https://sqlite.org/src/file/ext/misc/percentile.c

// Percentiles.

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "stats/stats.h"

// SAMPLE_INITIAL_CAP is the initial capacity of a sample.
#define SAMPLE_INITIAL_CAP 64

#pragma region Sample

// compare_doubles compares two doubles for qsort. There are no NaNs.
static int compare_doubles(const void* pa, const void* pb) {
    double a = *(const double*)pa;
    double b = *(const double*)pb;
    return (a > b) - (a < b);
}

// sample_add adds x to the sample. x must not be NaN.
// Returns false if out of memory, leaving the sample as it was.
bool sample_add(Sample* s, double x) {
    if (s->len == s->cap) {
        // Overflow of the doubled capacity, or its size in bytes.
        if (s->cap > SIZE_MAX / 2 / sizeof(double)) {
            return false;
        }
        size_t cap = s->cap == 0 ? SAMPLE_INITIAL_CAP : s->cap * 2;
        double* values = realloc(s->values, cap * sizeof(double));
        if (values == NULL) {
            return false;
        }
        s->values = values;
        s->cap = cap;
    }
    // Appending in order keeps the sample sorted, which is common
    // with ordered input and window frames.
    s->sorted = s->len == 0 || (s->sorted && x >= s->values[s->len - 1]);
    s->values[s->len++] = x;
    return true;
}

// sample_remove removes one occurrence of x from the sample.
// Does nothing if there is no such value.
// A sorted sample is searched in O(log n) and stays sorted.
void sample_remove(Sample* s, double x) {
    if (s->sorted) {
        size_t lo = 0, hi = s->len;
        while (lo < hi) {
            size_t mid = lo + (hi - lo) / 2;
            if (s->values[mid] < x) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        if (lo == s->len || s->values[lo] != x) {
            return;
        }
        memmove(&s->values[lo], &s->values[lo + 1], (s->len - lo - 1) * sizeof(double));
        s->len--;
        return;
    }
    for (size_t i = 0; i < s->len; i++) {
        if (s->values[i] == x) {
            s->values[i] = s->values[s->len - 1];
            s->len--;
            return;
        }
    }
}

// sample_perc returns the p-th percentile of the sample through out.
// Returns false if the sample is empty.
//
// Interpolates linearly between the closest ranks, as Excel's PERCENTILE.INC
// and NumPy's default method do: the result is at the zero-based index
// p/100 * (n-1) in the sorted values. p is clamped to [0, 100].
// Sorts the sample if it isn't sorted.
bool sample_perc(Sample* s, double p, double* out) {
    if (s->len == 0) {
        return false;
    }
    if (!s->sorted) {
        qsort(s->values, s->len, sizeof(double), compare_doubles);
        s->sorted = true;
    }
    // !(p >= 0) is also true for NaN.
    if (!(p >= 0)) {
        p = 0;
    } else if (p > 100) {
        p = 100;
    }

    // Dividing last, p * (n-1) / 100 is exact for p = 100 and integer p
    // with moderate n, so the ends and whole ranks need no interpolation.
    double idx = p * (double)(s->len - 1) / 100;
    size_t lo = (size_t)idx;
    if (lo >= s->len - 1) {
        *out = s->values[s->len - 1];
        return true;
    }
    double frac = idx - (double)lo;
    double v1 = s->values[lo];
    double v2 = s->values[lo + 1];
    if (frac == 0) {
        *out = v1;
        return true;
    }
    double diff = v2 - v1;
    if (isfinite(diff)) {
        *out = v1 + diff * frac;
    } else {
        // v2 - v1 overflowed (or there is an infinity),
        // and weighting each value separately doesn't.
        *out = v1 * (1 - frac) + v2 * frac;
    }
    return true;
}

// sample_free releases the memory of the sample and makes it empty.
void sample_free(Sample* s) {
    free(s->values);
    memset(s, 0, sizeof(*s));
}

#pragma endregion

#pragma region Percentile

// perc_valid reports whether p is a valid percentile: a number from 0 to 100.
bool perc_valid(double p) {
    return p >= 0 && p <= 100;
}

// perc_same reports whether two percentiles are the same,
// within PERC_SAME_EPSILON.
bool perc_same(double p1, double p2) {
    return fabs(p1 - p2) <= PERC_SAME_EPSILON;
}

#pragma endregion
