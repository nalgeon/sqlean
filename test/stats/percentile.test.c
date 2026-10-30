// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Percentile tests. The golden values match NumPy's percentile()
// with the default (linear) method.

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#include "stats/stats.h"

#define LEN(a) (sizeof(a) / sizeof(a[0]))

// near reports whether got is within 1e-12 of want.
static bool near(double got, double want) {
    return fabs(got - want) <= 1e-12 * (1 + fabs(want));
}

// make returns a sample of n values.
static Sample make(const double* xs, size_t n) {
    Sample s = {0};
    for (size_t i = 0; i < n; i++) {
        bool ok = sample_add(&s, xs[i]);
        assert(ok);
    }
    return s;
}

typedef struct {
    double p;
    double want;
} PercTest;

static void test_sample_perc(void) {
    printf("test_sample_perc...");
    // unsorted, with duplicates: 1, 2, 3, 3, 5, 8, 13
    static const double xs[] = {8, 3, 1, 13, 5, 3, 2};
    static const PercTest tests[] = {
        {0, 1},     {100, 13},    {50, 3}, {25, 2.5}, {75, 6.5}, {90, 10},
        {99, 12.7}, {12.5, 1.75}, {-5, 1}, {105, 13}, {NAN, 1},
    };
    Sample s = make(xs, LEN(xs));
    assert(!s.sorted);
    for (size_t i = 0; i < LEN(tests); i++) {
        double got;
        assert(sample_perc(&s, tests[i].p, &got));
        assert(near(got, tests[i].want));
    }
    assert(s.sorted);
    sample_free(&s);
    assert(s.values == NULL && s.len == 0);

    double got;
    assert(!sample_perc(&s, 50, &got));
    printf("OK\n");
}

static void test_sample_perc_extremes(void) {
    printf("test_sample_perc_extremes...");
    double got;

    // the interpolation doesn't overflow
    static const double big[] = {1e308, -1e308};
    Sample s = make(big, LEN(big));
    assert(sample_perc(&s, 50, &got) && got == 0);
    assert(sample_perc(&s, 75, &got) && near(got, 5e307));
    sample_free(&s);

    static const double inf[] = {1, INFINITY, INFINITY};
    s = make(inf, LEN(inf));
    assert(sample_perc(&s, 25, &got) && got == INFINITY);
    assert(sample_perc(&s, 0, &got) && got == 1);
    sample_free(&s);

    // -Inf and +Inf interpolate to NaN
    static const double both[] = {INFINITY, -INFINITY};
    s = make(both, LEN(both));
    assert(sample_perc(&s, 50, &got) && isnan(got));
    assert(sample_perc(&s, 0, &got) && got == -INFINITY);
    sample_free(&s);
    printf("OK\n");
}

// The sample grows past its initial capacity, and stays sorted
// while the values come in order.
static void test_sample_grow(void) {
    printf("test_sample_grow...");
    Sample s = {0};
    for (int i = 1; i <= 1000; i++) {
        assert(sample_add(&s, i));
    }
    assert(s.len == 1000 && s.cap >= 1000 && s.sorted);
    double got;
    assert(sample_perc(&s, 50, &got) && got == 500.5);
    assert(sample_add(&s, 0));
    assert(!s.sorted);
    assert(sample_perc(&s, 0, &got) && got == 0);
    sample_free(&s);
    printf("OK\n");
}

static void test_sample_remove(void) {
    printf("test_sample_remove...");
    double got;

    // unsorted
    static const double xs[] = {5, 1, 4, 1, 3};
    Sample s = make(xs, LEN(xs));
    assert(!s.sorted);
    sample_remove(&s, 1);
    sample_remove(&s, 42);  // not there
    assert(s.len == 4);
    // 1, 3, 4, 5
    assert(sample_perc(&s, 50, &got) && got == 3.5);

    // sorted: removing keeps it sorted
    assert(s.sorted);
    sample_remove(&s, 5);
    sample_remove(&s, 1);
    sample_remove(&s, 2);  // not there
    assert(s.len == 2 && s.sorted);
    assert(sample_perc(&s, 0, &got) && got == 3);
    assert(sample_perc(&s, 100, &got) && got == 4);
    sample_remove(&s, 3);
    sample_remove(&s, 4);
    assert(s.len == 0 && !sample_perc(&s, 50, &got));
    sample_remove(&s, 4);
    assert(s.len == 0);
    sample_free(&s);
    printf("OK\n");
}

static void test_perc_valid(void) {
    printf("test_perc_valid...");
    assert(perc_valid(0) && perc_valid(50) && perc_valid(100) && perc_valid(99.999));
    assert(!perc_valid(-0.001) && !perc_valid(100.001));
    assert(!perc_valid(NAN) && !perc_valid(INFINITY) && !perc_valid(-INFINITY));
    assert(perc_same(50, 50) && perc_same(50, 50.001) && perc_same(50.001, 50));
    assert(!perc_same(50, 50.002) && !perc_same(50, NAN));
    printf("OK\n");
}

int main(void) {
    test_sample_perc();
    test_sample_perc_extremes();
    test_sample_grow();
    test_sample_remove();
    test_perc_valid();
    return 0;
}
