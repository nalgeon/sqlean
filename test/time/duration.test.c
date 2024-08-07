// Copyright (c) 2024 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Duration tests.

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

#include "time/timex.h"

typedef struct {
    int64_t nsec;
    int64_t usec;
    int64_t msec;
    double sec;
    double min;
    double hour;
} ParsedDuration;

typedef struct {
    int64_t nsec;
    ParsedDuration golden;
} ToTest;

static ToTest to_tests[] = {
    {0, {.nsec = 0, .usec = 0, .msec = 0, .sec = 0.000000, .min = 0.000000, .hour = 0.000000}},
    {1, {.nsec = 1, .usec = 0, .msec = 0, .sec = 0.000000, .min = 0.000000, .hour = 0.000000}},
    {1100,
     {.nsec = 1100, .usec = 1, .msec = 0, .sec = 0.000001, .min = 0.000000, .hour = 0.000000}},
    {2200000,
     {.nsec = 2200000,
      .usec = 2200,
      .msec = 2,
      .sec = 0.002200,
      .min = 0.000037,
      .hour = 0.000001}},
    {3300000000,
     {.nsec = 3300000000,
      .usec = 3300000,
      .msec = 3300,
      .sec = 3.300000,
      .min = 0.055000,
      .hour = 0.000917}},
    {245000000000,
     {.nsec = 245000000000,
      .usec = 245000000,
      .msec = 245000,
      .sec = 245.000000,
      .min = 4.083333,
      .hour = 0.068056}},
    {245001000000,
     {.nsec = 245001000000,
      .usec = 245001000,
      .msec = 245001,
      .sec = 245.001000,
      .min = 4.083350,
      .hour = 0.068056}},
    {18367001000000,
     {.nsec = 18367001000000,
      .usec = 18367001000,
      .msec = 18367001,
      .sec = 18367.001000,
      .min = 306.116683,
      .hour = 5.101945}},
    {480000000001,
     {.nsec = 480000000001,
      .usec = 480000000,
      .msec = 480000,
      .sec = 480.000000,
      .min = 8.000000,
      .hour = 0.133333}},
    {9223372036854775807,
     {.nsec = 9223372036854775807,
      .usec = 9223372036854775,
      .msec = 9223372036854,
      .sec = 9223372036.854776,
      .min = 153722867.280913,
      .hour = 2562047.788015}},
    {-9223372036854775807,
     {.nsec = -9223372036854775807,
      .usec = -9223372036854775,
      .msec = -9223372036854,
      .sec = -9223372036.854776,
      .min = -153722867.280913,
      .hour = -2562047.788015}},
};

static void test_to_x(void) {
    printf("test_to_x...");
    for (size_t i = 0; i < sizeof(to_tests) / sizeof(to_tests[0]); i++) {
        ToTest test = to_tests[i];
        Duration d = test.nsec;
        // printf("%lld\n", d);
        assert(dur_to_micro(d) == test.golden.usec);
        assert(dur_to_milli(d) == test.golden.msec);
        assert(round(dur_to_seconds(d)) == round(test.golden.sec));
        assert(round(dur_to_minutes(d)) == round(test.golden.min));
        assert(round(dur_to_hours(d)) == round(test.golden.hour));
    }
    printf("OK\n");
}

typedef struct {
    Duration d;
    double want;
} ToMinuteTest;

static ToMinuteTest to_minute_tests[] = {
    {-60000000000, -1}, {-1, -1 / 60e9}, {1, 1 / 60e9}, {60000000000, 1}, {3000, 5e-8},
};

static void test_to_minutes(void) {
    printf("test_to_minutes...");
    for (size_t i = 0; i < sizeof(to_minute_tests) / sizeof(to_minute_tests[0]); i++) {
        ToMinuteTest test = to_minute_tests[i];
        double got = dur_to_minutes(test.d);
        // printf("want %f, got %f\n", test.want, got);
        assert(got == test.want);
    }
    printf("OK\n");
}

typedef struct {
    Duration d;
    double want;
} ToHourTest;

static ToHourTest to_hour_tests[] = {
    {-3600000000000, -1}, {-1, -1 / 3600e9}, {1, 1 / 3600e9}, {3600000000000, 1}, {36, 1e-11},
};

static void test_to_hours(void) {
    printf("test_to_hours...");
    for (size_t i = 0; i < sizeof(to_hour_tests) / sizeof(to_hour_tests[0]); i++) {
        ToHourTest test = to_hour_tests[i];
        double got = dur_to_hours(test.d);
        // printf("want %f, got %f\n", test.want, got);
        assert(got == test.want);
    }
    printf("OK\n");
}

typedef struct {
    Duration d, m, want;
} RoundTest;

static RoundTest truncate_tests[] = {
    {0, 1e9, 0},                                             // 0 / 1s = 0
    {60 * 1e9, -7 * 1e9, 60 * 1e9},                          // 1m / -7s = 1m
    {60 * 1e9, 0, 60 * 1e9},                                 // 1m / 0 = 1m
    {60 * 1e9, 1, 60 * 1e9},                                 // 1m / 1 = 1m
    {60 * 1e9 + 10 * 1e9, 10 * 1e9, 60 * 1e9 + 10 * 1e9},    // 1m + 10s / 10s = 1m + 10s
    {2 * 60 * 1e9 + 10 * 1e9, 60 * 1e9, 2 * 60 * 1e9},       // 2m + 10s / 1m = 2m
    {10 * 60 * 1e9 + 10 * 1e9, 3 * 60 * 1e9, 9 * 60 * 1e9},  // 10m + 10s / 3m = 9m
    {60 * 1e9 + 10 * 1e9, 60 * 1e9 + 10 * 1e9 + 1, 0},       // 1m + 10s / 1m + 10s + 1 = 0
    {60 * 1e9 + 10 * 1e9, 3600 * 1e9, 0},                    // 1m + 10s / 1h = 0
    {-60 * 1e9, 1e9, -60 * 1e9},                             // -1m / 1s = -1m
    {-10 * 60 * 1e9, 3 * 60 * 1e9, -9 * 60 * 1e9},           // -10m / 3m = -9m
    {-10 * 60 * 1e9, 3600 * 1e9, 0},                         // -10m / 1h = 0
};

static void test_truncate(void) {
    printf("test_truncate...");
    for (size_t i = 0; i < sizeof(truncate_tests) / sizeof(truncate_tests[0]); i++) {
        RoundTest test = truncate_tests[i];
        Duration got = dur_truncate(test.d, test.m);
        // printf("want %lld, got %lld\n", test.want, got);
        assert(got == test.want);
    }
    printf("OK\n");
}

static RoundTest round_tests[] = {
    {0, 1e9, 0},                                          // 0 / 1s = 0
    {60 * 1e9, -11 * 1e9, 60 * 1e9},                      // 1m / -11s = 1m
    {60 * 1e9, 0, 60 * 1e9},                              // 1m / 0 = 1m
    {60 * 1e9, 1, 60 * 1e9},                              // 1m / 1 = 1m
    {2 * 60 * 1e9, 60 * 1e9, 2 * 60 * 1e9},               // 2m / 1m = 2m
    {2 * 60 * 1e9 + 10 * 1e9, 60 * 1e9, 2 * 60 * 1e9},    // 2m + 10s / 1m = 2m
    {2 * 60 * 1e9 + 30 * 1e9, 60 * 1e9, 3 * 60 * 1e9},    // 2m + 30s / 1m = 3m
    {2 * 60 * 1e9 + 50 * 1e9, 60 * 1e9, 3 * 60 * 1e9},    // 2m + 50s / 1m = 3m
    {-60 * 1e9, 1, -60 * 1e9},                            // -1m / 1 = -1m
    {-2 * 60 * 1e9, 60 * 1e9, -2 * 60 * 1e9},             // -2m / 1m = -2m
    {-2 * 60 * 1e9 - 10 * 1e9, 60 * 1e9, -2 * 60 * 1e9},  // -2m - 10s / 1m = -2m
    {-2 * 60 * 1e9 - 30 * 1e9, 60 * 1e9, -3 * 60 * 1e9},  // -2m - 30s / 1m = -3m,
    {-2 * 60 * 1e9 - 50 * 1e9, 60 * 1e9, -3 * 60 * 1e9},  // -2m - 50s / 1m = -3m,
    {8e18, 3e18, 9e18},
    {9e18, 5e18, 9223372036854775807},
    {-8e18, 3e18, -9e18},
    {-9e18, 5e18, LLONG_MIN},
    {(3LL << 61) - 1, 3LL << 61, 3LL << 61},
};

static void test_round(void) {
    printf("test_round...");
    for (size_t i = 0; i < sizeof(round_tests) / sizeof(round_tests[0]); i++) {
        RoundTest test = round_tests[i];
        Duration got = dur_round(test.d, test.m);
        // printf("want %lld, got %lld\n", test.want, got);
        assert(got == test.want);
    }
    printf("OK\n");
}

typedef struct {
    Duration d, want;
} AbsTest;

static AbsTest abs_tests[] = {
    {0, 0},
    {1, 1},
    {-1, 1},
    {1 * 60 * 1e9, 1 * 60 * 1e9},
    {-1 * 60 * 1e9, 1 * 60 * 1e9},
    {MIN_DURATION, MAX_DURATION},
    {MIN_DURATION + 1, MAX_DURATION},
    {MIN_DURATION + 2, MAX_DURATION - 1},
    {MAX_DURATION, MAX_DURATION},
    {MAX_DURATION - 1, MAX_DURATION - 1},
};

static void test_abs(void) {
    printf("test_abs...");
    for (size_t i = 0; i < sizeof(abs_tests) / sizeof(abs_tests[0]); i++) {
        AbsTest test = abs_tests[i];
        Duration got = dur_abs(test.d);
        // printf("want %lld, got %lld\n", test.want, got);
        assert(got == test.want);
    }
    printf("OK\n");
}

int main(void) {
    test_to_x();
    test_to_minutes();
    test_to_hours();
    test_truncate();
    test_round();
    test_abs();
}
