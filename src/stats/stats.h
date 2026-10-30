// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Package stats provides descriptive statistics (variance, standard
// deviation, percentiles) and arithmetic series of integers.

#ifndef STATS_STATS_H
#define STATS_STATS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// --- Variance ---

// Variance accumulates a sample one value at a time, keeping the moments
// needed for its variance and standard deviation. Uses Welford's algorithm,
// which doesn't lose precision when the values are large compared to their
// spread. The zero value is an empty sample.
typedef struct {
    int64_t n;          // number of finite values
    int64_t nonfinite;  // number of infinite values
    double mean;        // mean of the finite values
    double m2;          // sum of squared deviations of the finite values from the mean
} Variance;

// var_add adds x to the sample.
void var_add(Variance* v, double x);

// var_remove removes x, previously added, from the sample.
void var_remove(Variance* v, double x);

// var_samp returns the sample variance through out.
// Returns false if there are fewer than two values or any value is infinite.
bool var_samp(Variance v, double* out);

// var_pop returns the population variance through out.
// Returns false if there are no values or any value is infinite.
bool var_pop(Variance v, double* out);

// var_stddev_samp returns the sample standard deviation through out.
// Returns false if there are fewer than two values or any value is infinite.
bool var_stddev_samp(Variance v, double* out);

// var_stddev_pop returns the population standard deviation through out.
// Returns false if there are no values or any value is infinite.
bool var_stddev_pop(Variance v, double* out);

// --- Percentile ---

// PERC_SAME_EPSILON is the largest difference between two percentiles
// that are considered the same, as in SQLite's percentile().
#define PERC_SAME_EPSILON 0.001

// Sample keeps the values of a sample to compute its percentiles.
// The zero value is an empty sample. Release the memory with sample_free.
typedef struct {
    double* values;  // the values, allocated with malloc
    size_t len;      // the number of values
    size_t cap;      // the capacity of values
    bool sorted;     // whether values are sorted in ascending order
} Sample;

// sample_add adds x to the sample. x must not be NaN.
// Returns false if out of memory, leaving the sample as it was.
bool sample_add(Sample* s, double x);

// sample_remove removes one occurrence of x from the sample.
// Does nothing if there is no such value.
void sample_remove(Sample* s, double x);

// sample_perc returns the p-th percentile of the sample through out.
// Returns false if the sample is empty.
bool sample_perc(Sample* s, double p, double* out);

// sample_free releases the memory of the sample and makes it empty.
void sample_free(Sample* s);

// perc_valid reports whether p is a valid percentile: a number from 0 to 100.
bool perc_valid(double p);

// perc_same reports whether two percentiles are the same,
// within PERC_SAME_EPSILON.
bool perc_same(double p1, double p2);

// --- Series ---

// Series is an arithmetic series of 64-bit integers.
// The values are numbered from 0 to last.
typedef struct {
    int64_t first;  // the first value
    uint64_t step;  // the distance between the values, at least 1
    uint64_t last;  // the index of the last value
    bool desc;      // whether the values decrease
    bool empty;     // whether the series has no values
} Series;

// series_make returns the series from start to stop (inclusive), step apart.
// A step of 0 is 1.
Series series_make(int64_t start, int64_t stop, int64_t step);

// series_at returns the value at index i of a non-empty series (i <= s.last).
int64_t series_at(Series s, uint64_t i);

#endif /* STATS_STATS_H */
