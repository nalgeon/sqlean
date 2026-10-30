// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Series tests.

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "stats/stats.h"

#define LEN(a) (sizeof(a) / sizeof(a[0]))

typedef struct {
    int64_t start, stop, step;
    uint64_t last;   // the index of the last value
    int64_t first;   // the value at index 0
    int64_t second;  // the value at index 1, if any
    int64_t end;     // the value at index last
} SeriesTest;

static void test_series(void) {
    printf("test_series...");
    static const SeriesTest tests[] = {
        {1, 3, 1, 2, 1, 2, 3},
        {5, 5, 1, 0, 5, 0, 5},
        {-3, -1, 1, 2, -3, -2, -1},
        {0, 99, 5, 19, 0, 5, 95},
        // a step of 0 is 1
        {1, 3, 0, 2, 1, 2, 3},
        // a negative step counts down from the last value
        {1, 10, -3, 3, 10, 7, 1},
        {1, 9, -3, 2, 7, 4, 1},
        // extremes
        {INT64_MIN, INT64_MAX, 1, UINT64_MAX, INT64_MIN, INT64_MIN + 1, INT64_MAX},
        {INT64_MIN, INT64_MAX, -1, UINT64_MAX, INT64_MAX, INT64_MAX - 1, INT64_MIN},
        {INT64_MIN, INT64_MAX, INT64_MAX, 2, INT64_MIN, -1, INT64_MAX - 1},
        {INT64_MIN, INT64_MAX, INT64_MIN, 1, 0, INT64_MIN, INT64_MIN},
        {INT64_MAX - 1, INT64_MAX, 1, 1, INT64_MAX - 1, INT64_MAX, INT64_MAX},
        {INT64_MAX - 1, INT64_MAX, -1, 1, INT64_MAX, INT64_MAX - 1, INT64_MAX - 1},
        {1, 3, INT64_MAX, 0, 1, 0, 1},
        {1, 3, INT64_MIN, 0, 1, 0, 1},
        {INT64_MAX, INT64_MAX, INT64_MIN, 0, INT64_MAX, 0, INT64_MAX},
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        const SeriesTest* t = &tests[i];
        Series s = series_make(t->start, t->stop, t->step);
        assert(!s.empty);
        assert(s.last == t->last);
        assert(series_at(s, 0) == t->first);
        if (s.last > 0) {
            assert(series_at(s, 1) == t->second);
        }
        assert(series_at(s, s.last) == t->end);
    }
    printf("OK\n");
}

static void test_series_empty(void) {
    printf("test_series_empty...");
    assert(series_make(5, 1, 1).empty);
    assert(series_make(5, 1, -1).empty);
    assert(series_make(INT64_MAX, INT64_MIN, 1).empty);
    assert(!series_make(1, 1, 1).empty);
    printf("OK\n");
}

int main(void) {
    test_series();
    test_series_empty();
    return 0;
}
