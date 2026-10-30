// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Based on extension-functions.c by Liam Healy, Public Domain
// https://sqlite.org/contrib/

// Variance and standard deviation.

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "stats/stats.h"

#pragma region Variance

// var_add adds x to the sample.
// An infinite x is only counted: it makes the variance undefined,
// and keeping it out of the moments lets var_remove take it back out.
void var_add(Variance* v, double x) {
    if (!isfinite(x)) {
        v->nonfinite++;
        return;
    }
    v->n++;
    double delta = x - v->mean;
    if (isfinite(delta)) {
        v->mean += delta / v->n;
    } else {
        // x - mean overflowed, but the mean itself doesn't.
        v->mean += x / v->n - v->mean / v->n;
    }
    // If delta overflowed, so does the true m2, and this gives +Inf.
    v->m2 += delta * (x - v->mean);
}

// var_remove removes x, previously added, from the sample.
// Reverses var_add. Once m2 overflows to +Inf, removing values
// doesn't bring it back.
void var_remove(Variance* v, double x) {
    if (!isfinite(x)) {
        if (v->nonfinite > 0) {
            v->nonfinite--;
        }
        return;
    }
    if (v->n <= 1) {
        v->n = 0;
        v->mean = 0;
        v->m2 = 0;
        return;
    }
    double delta = x - v->mean;
    v->n--;
    if (isfinite(delta)) {
        v->mean -= delta / v->n;
    } else {
        // x - mean overflowed, but the mean itself doesn't.
        v->mean += v->mean / v->n - x / v->n;
    }
    v->m2 -= delta * (x - v->mean);
    // Rounding can take a near-zero m2 below zero.
    if (v->m2 < 0) {
        v->m2 = 0;
    }
}

// var_samp returns the sample variance through out.
// Returns false if there are fewer than two values or any value is infinite.
bool var_samp(Variance v, double* out) {
    if (v.n < 2 || v.nonfinite > 0) {
        return false;
    }
    *out = v.m2 / (double)(v.n - 1);
    return true;
}

// var_pop returns the population variance through out.
// Returns false if there are no values or any value is infinite.
bool var_pop(Variance v, double* out) {
    if (v.n < 1 || v.nonfinite > 0) {
        return false;
    }
    *out = v.m2 / (double)v.n;
    return true;
}

// var_stddev_samp returns the sample standard deviation through out.
// Returns false if there are fewer than two values or any value is infinite.
bool var_stddev_samp(Variance v, double* out) {
    double var;
    if (!var_samp(v, &var)) {
        return false;
    }
    *out = sqrt(var);
    return true;
}

// var_stddev_pop returns the population standard deviation through out.
// Returns false if there are no values or any value is infinite.
bool var_stddev_pop(Variance v, double* out) {
    double var;
    if (!var_pop(v, &var)) {
        return false;
    }
    *out = sqrt(var);
    return true;
}

#pragma endregion
