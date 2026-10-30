// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Variance tests.

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#include "stats/stats.h"

#define LEN(a) (sizeof(a) / sizeof(a[0]))

// near reports whether got is within a relative 1e-12 of want.
static bool near(double got, double want) {
    return got == want || fabs(got - want) <= 1e-12 * fabs(want);
}

// add adds n values to v.
static void add(Variance* v, const double* xs, size_t n) {
    for (size_t i = 0; i < n; i++) {
        var_add(v, xs[i]);
    }
}

typedef struct {
    double xs[8];
    size_t n;
    double samp;  // NAN if undefined
    double pop;   // NAN if undefined
} VarTest;

static void test_var(void) {
    printf("test_var...");
    static const VarTest tests[] = {
        {{0}, 0, NAN, NAN},
        {{5}, 1, NAN, 0},
        {{1, 2}, 2, 0.5, 0.25},
        {{2, 4, 4, 4, 5, 5, 7, 9}, 8, 32.0 / 7, 4},
        {{-3, -3, -3}, 3, 0, 0},
        // a large offset doesn't lose precision
        {{1e9 + 4, 1e9 + 7, 1e9 + 13, 1e9 + 16}, 4, 30, 22.5},
        // overflow gives +Inf: the true variances are 2e616 and 1e616
        {{1e308, -1e308}, 2, INFINITY, INFINITY},
        // an infinite value makes the variance undefined
        {{1, INFINITY}, 2, NAN, NAN},
        {{-INFINITY}, 1, NAN, NAN},
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        const VarTest* t = &tests[i];
        Variance v = {0};
        add(&v, t->xs, t->n);
        double got;
        if (isnan(t->samp)) {
            assert(!var_samp(v, &got));
            assert(!var_stddev_samp(v, &got));
        } else {
            assert(var_samp(v, &got) && near(got, t->samp));
            assert(var_stddev_samp(v, &got) && near(got, sqrt(t->samp)));
        }
        if (isnan(t->pop)) {
            assert(!var_pop(v, &got));
            assert(!var_stddev_pop(v, &got));
        } else {
            assert(var_pop(v, &got) && near(got, t->pop));
            assert(var_stddev_pop(v, &got) && near(got, sqrt(t->pop)));
        }
    }
    printf("OK\n");
}

// var_remove gives the same result as adding only the values left.
static void test_var_remove(void) {
    printf("test_var_remove...");
    Variance v = {0};
    static const double xs[] = {1, 2, 3, 4, 5};
    add(&v, xs, LEN(xs));
    var_remove(&v, 1);
    var_remove(&v, 2);
    double got;
    // 3, 4, 5
    assert(var_samp(v, &got) && near(got, 1));
    assert(var_pop(v, &got) && near(got, 2.0 / 3));

    // removing the last values empties it
    var_remove(&v, 3);
    var_remove(&v, 4);
    assert(var_pop(v, &got) && got == 0);
    var_remove(&v, 5);
    assert(v.n == 0 && !var_pop(v, &got));

    // an infinity taken back out no longer makes the variance undefined
    var_add(&v, 1);
    var_add(&v, INFINITY);
    var_add(&v, 3);
    assert(!var_samp(v, &got));
    var_remove(&v, INFINITY);
    assert(var_samp(v, &got) && near(got, 2));

    // removing the same value it added leaves no rounding residue below 0
    Variance w = {0};
    var_add(&w, 0.1);
    var_add(&w, 0.1);
    var_add(&w, 0.7);
    var_remove(&w, 0.7);
    assert(var_pop(w, &got) && got >= 0 && got < 1e-15);
    printf("OK\n");
}

int main(void) {
    test_var();
    test_var_remove();
    return 0;
}
