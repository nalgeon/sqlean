// Copyright (c) 2024 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Time tests.

#include <assert.h>
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
    int year;
    enum Month month;
    int day;
    int hour;
    int min;
    int sec;
    int nsec;
    enum Weekday weekday;
} ParsedTime;

typedef struct {
    int64_t sec;
    int64_t nsec;
    ParsedTime golden;
} TimeTest;

#pragma region Constructors.

typedef struct {
    int year, month, day, hour, min, sec, nsec, offset_sec;
    int64_t epoch;
} DateTest;

static DateTest date_tests[] = {
    {2011, 11, 6, 8, 0, 0, 0, TIMEX_UTC, 1320566400},    // 8:00:00 UTC
    {2011, 11, 6, 8, 59, 59, 0, TIMEX_UTC, 1320569999},  // 8:59:59 UTC
    {2011, 11, 6, 10, 0, 0, 0, TIMEX_UTC, 1320573600},   // 10:00:00 UTC

    {2011, 3, 13, 9, 0, 0, 0, TIMEX_UTC, 1300006800},    // 9:00:00 UTC
    {2011, 3, 13, 9, 59, 59, 0, TIMEX_UTC, 1300010399},  // 9:59:59 UTC
    {2011, 3, 13, 10, 0, 0, 0, TIMEX_UTC, 1300010400},   // 10:00:00 UTC
    {2011, 3, 13, 9, 30, 0, 0, TIMEX_UTC, 1300008600},   // 9:30:00 UTC
    {2012, 12, 24, 8, 0, 0, 0, TIMEX_UTC, 1356336000},   // Leap year

    // Many names for 2011-11-18 15:56:35.0 UTC
    {2011, 11, 18, 15, 56, 35, 0, TIMEX_UTC, 1321631795},             // Nov 18 15:56:35
    {2011, 11, 19, -9, 56, 35, 0, TIMEX_UTC, 1321631795},             // Nov 19 -9:56:35
    {2011, 11, 17, 39, 56, 35, 0, TIMEX_UTC, 1321631795},             // Nov 17 39:56:35
    {2011, 11, 18, 14, 116, 35, 0, TIMEX_UTC, 1321631795},            // Nov 18 14:116:35
    {2011, 10, 49, 15, 56, 35, 0, TIMEX_UTC, 1321631795},             // Oct 49 15:56:35
    {2011, 11, 18, 15, 55, 95, 0, TIMEX_UTC, 1321631795},             // Nov 18 15:55:95
    {2011, 11, 18, 15, 56, 34, 1e9, TIMEX_UTC, 1321631795},           // Nov 18 15:56:34 + 10⁹ns
    {2011, 12, -12, 15, 56, 35, 0, TIMEX_UTC, 1321631795},            // Dec -12 15:56:35
    {2012, 1, -43, 15, 56, 35, 0, TIMEX_UTC, 1321631795},             // 2012 Jan -43 15:56:35
    {2012, January - 2, 18, 15, 56, 35, 0, TIMEX_UTC, 1321631795},    // 2012 (Jan-2) 18 15:56:35
    {2010, December + 11, 18, 15, 56, 35, 0, TIMEX_UTC, 1321631795},  // 2010 (Dec+11) 18 15:56:35
    {1970, 1, 15297, 15, 56, 35, 0, TIMEX_UTC, 1321631795},           // large number of days

    {2011, 11, 18, 10, 56, 35, 0, -5 * 3600, 1321631795},  // UTC-5
    {2011, 11, 18, 3, 56, 35, 0, -12 * 3600, 1321631795},  // UTC-12
    {2011, 11, 18, 16, 56, 35, 0, 1 * 3600, 1321631795},   // UTC+1
    {2011, 11, 19, 3, 56, 35, 0, 12 * 3600, 1321631795},   // UTC+12

    {1970, 1, -25508, 8, 0, 0, 0, TIMEX_UTC, -2203948800},  // negative Unix time
};

static void test_date(void) {
    printf("test_date...");
    for (size_t i = 0; i < sizeof(date_tests) / sizeof(date_tests[0]); i++) {
        DateTest test = date_tests[i];
        Time t = time_date(test.year, test.month, test.day, test.hour, test.min, test.sec,
                           test.nsec, test.offset_sec);
        assert(time_to_unix(t) == test.epoch);
    }
    printf("OK\n");
}

#pragma endregion

#pragma region Time parts.

static TimeTest unix_tests[] = {
    {0, 0, {1970, January, 1, 0, 0, 0, 0, Thursday}},
    {1221681866, 0, {2008, September, 17, 20, 4, 26, 0, Wednesday}},
    {-1221681866, 0, {1931, April, 16, 3, 55, 34, 0, Thursday}},
    {-11644473600, 0, {1601, January, 1, 0, 0, 0, 0, Monday}},
    {599529660, 0, {1988, December, 31, 0, 1, 0, 0, Saturday}},
    {978220860, 0, {2000, December, 31, 0, 1, 0, 0, Sunday}},
    {0, 1e8, {1970, January, 1, 0, 0, 0, 1e8, Thursday}},
    {1221681866, 2e8, {2008, September, 17, 20, 4, 26, 2e8, Wednesday}},
};

static void test_get_part(void) {
    printf("test_get_part...");
    for (size_t i = 0; i < sizeof(unix_tests) / sizeof(unix_tests[0]); i++) {
        TimeTest test = unix_tests[i];
        Time t = time_unix(test.sec, test.nsec);
        assert(time_get_year(t) == test.golden.year);
        assert(time_get_month(t) == test.golden.month);
        assert(time_get_day(t) == test.golden.day);
        assert(time_get_hour(t) == test.golden.hour);
        assert(time_get_minute(t) == test.golden.min);
        assert(time_get_second(t) == test.golden.sec);
        assert(time_get_nano(t) == test.golden.nsec);
        assert(time_get_weekday(t) == test.golden.weekday);
    }
    for (size_t i = 0; i < 8; i++) {
        DateTest test = date_tests[i];
        Time t = time_unix(test.epoch, 0);
        assert(time_get_year(t) == test.year);
        assert((int)time_get_month(t) == test.month);
        assert(time_get_day(t) == test.day);
        assert(time_get_hour(t) == test.hour);
        assert(time_get_minute(t) == test.min);
        assert(time_get_second(t) == test.sec);
        assert(time_get_nano(t) == test.nsec);
    }
    printf("OK\n");
}

typedef struct {
    int year;
    int month;
    int day;
    int yex;  // expected year
    int wex;  // expected week
} ISOWeekTest;

static ISOWeekTest isoweek_tests[] = {
    {2014, 1, 1, 2014, 1},  {2014, 1, 5, 2014, 1},  {2014, 1, 6, 2014, 2},  {2015, 1, 1, 2015, 1},
    {2016, 1, 1, 2015, 53}, {2017, 1, 1, 2016, 52}, {2018, 1, 1, 2018, 1},  {2019, 1, 1, 2019, 1},
    {2020, 1, 1, 2020, 1},  {2021, 1, 1, 2020, 53}, {2022, 1, 1, 2021, 52}, {2023, 1, 1, 2022, 52},
    {2024, 1, 1, 2024, 1},  {2025, 1, 1, 2025, 1},  {2026, 1, 1, 2026, 1},  {2027, 1, 1, 2026, 53},
    {2028, 1, 1, 2027, 52}, {2029, 1, 1, 2029, 1},  {2030, 1, 1, 2030, 1},  {2031, 1, 1, 2031, 1},
    {2032, 1, 1, 2032, 1},  {2033, 1, 1, 2032, 53}, {2034, 1, 1, 2033, 52}, {2035, 1, 1, 2035, 1},
    {2036, 1, 1, 2036, 1},  {2037, 1, 1, 2037, 1},  {2038, 1, 1, 2037, 53}, {2039, 1, 1, 2038, 52},
    {2040, 1, 1, 2039, 52},
};

static void test_get_isoweek(void) {
    printf("test_get_isoweek...");
    for (size_t i = 0; i < sizeof(isoweek_tests) / sizeof(isoweek_tests[0]); i++) {
        ISOWeekTest test = isoweek_tests[i];
        Time t = time_date(test.year, test.month, test.day, 0, 0, 0, 0, TIMEX_UTC);
        int year, week;
        time_get_isoweek(t, &year, &week);
        // printf("%d %d %d, want %d %d, got %d %d\n", test.year, test.month, test.day, test.yex,
        //        test.wex, year, week);
        assert(year == test.yex && week == test.wex);
    }
    printf("OK\n");
}

typedef struct {
    int year, month, day;
    int yday;
} YearDayTest;

// Test YearDay in several different scenarios and corner cases
static YearDayTest yearday_tests[] = {
    // Non-leap-year tests
    {2007, 1, 1, 1},
    {2007, 1, 15, 15},
    {2007, 2, 1, 32},
    {2007, 2, 15, 46},
    {2007, 3, 1, 60},
    {2007, 3, 15, 74},
    {2007, 4, 1, 91},
    {2007, 12, 31, 365},

    // Leap-year tests
    {2008, 1, 1, 1},
    {2008, 1, 15, 15},
    {2008, 2, 1, 32},
    {2008, 2, 15, 46},
    {2008, 3, 1, 61},
    {2008, 3, 15, 75},
    {2008, 4, 1, 92},
    {2008, 12, 31, 366},

    // Looks like leap-year (but isn't) tests
    {1900, 1, 1, 1},
    {1900, 1, 15, 15},
    {1900, 2, 1, 32},
    {1900, 2, 15, 46},
    {1900, 3, 1, 60},
    {1900, 3, 15, 74},
    {1900, 4, 1, 91},
    {1900, 12, 31, 365},

    // Year one tests (non-leap)
    {1, 1, 1, 1},
    {1, 1, 15, 15},
    {1, 2, 1, 32},
    {1, 2, 15, 46},
    {1, 3, 1, 60},
    {1, 3, 15, 74},
    {1, 4, 1, 91},
    {1, 12, 31, 365},

    // Year minus one tests (non-leap)
    {-1, 1, 1, 1},
    {-1, 1, 15, 15},
    {-1, 2, 1, 32},
    {-1, 2, 15, 46},
    {-1, 3, 1, 60},
    {-1, 3, 15, 74},
    {-1, 4, 1, 91},
    {-1, 12, 31, 365},

    // 400 BC tests (leap-year)
    {-400, 1, 1, 1},
    {-400, 1, 15, 15},
    {-400, 2, 1, 32},
    {-400, 2, 15, 46},
    {-400, 3, 1, 61},
    {-400, 3, 15, 75},
    {-400, 4, 1, 92},
    {-400, 12, 31, 366},

    // Special Cases

    // Gregorian calendar change (no effect)
    {1582, 10, 4, 277},
    {1582, 10, 15, 288},
};

static void test_get_yearday(void) {
    printf("test_get_yearday...");
    for (size_t i = 0; i < sizeof(yearday_tests) / sizeof(yearday_tests[0]); i++) {
        YearDayTest test = yearday_tests[i];
        Time t = time_date(test.year, test.month, test.day, 0, 0, 0, 0, TIMEX_UTC);
        int yday = time_get_yearday(t);
        // printf("%d %d %d, want %d, got %d\n", test.year, test.month, test.day, test.yday, yday);
        assert(yday == test.yday);
    }
    printf("OK\n");
}

#pragma endregion

#pragma region Unix time.

static bool same(Time t, ParsedTime u) {
    int year, day;
    enum Month month;
    time_get_date(t, &year, &month, &day);
    int hour, min, sec;
    time_get_clock(t, &hour, &min, &sec);
    enum Weekday weekday = time_get_weekday(t);
    return year == u.year && month == u.month && day == u.day && hour == u.hour && min == u.min &&
           sec == u.sec && t.nsec == u.nsec && weekday == u.weekday;
}

static void test_unix(void) {
    printf("test_unix...");
    for (size_t i = 0; i < sizeof(unix_tests) / sizeof(unix_tests[0]); i++) {
        TimeTest test = unix_tests[i];
        Time t = time_unix(test.sec, test.nsec);
        assert(same(t, test.golden));
    }
    printf("OK\n");
}

static void test_milli(void) {
    printf("test_milli...");
    for (size_t i = 0; i < sizeof(unix_tests) / sizeof(unix_tests[0]); i++) {
        TimeTest test = unix_tests[i];
        int64_t msec = test.sec * 1000 + test.nsec / 1000000;
        Time t = time_milli(msec);
        assert(same(t, test.golden));
    }
    printf("OK\n");
}

static void test_micro(void) {
    printf("test_micro...");
    for (size_t i = 0; i < sizeof(unix_tests) / sizeof(unix_tests[0]); i++) {
        TimeTest test = unix_tests[i];
        int64_t usec = test.sec * 1000000 + test.nsec / 1000;
        Time t = time_micro(usec);
        assert(same(t, test.golden));
    }
    printf("OK\n");
}

static void test_to_unix(void) {
    printf("test_to_unix...");
    for (size_t i = 0; i < sizeof(unix_tests) / sizeof(unix_tests[0]); i++) {
        TimeTest test = unix_tests[i];
        Time t = time_unix(test.sec, test.nsec);
        assert(time_to_unix(t) == test.sec);
    }
    printf("OK\n");
}

static void test_to_milli(void) {
    printf("test_to_milli...");
    for (size_t i = 0; i < sizeof(unix_tests) / sizeof(unix_tests[0]); i++) {
        TimeTest test = unix_tests[i];
        Time t = time_unix(test.sec, test.nsec);
        int64_t msec = test.sec * 1000 + test.nsec / 1000000;
        assert(time_to_milli(t) == msec);
    }
    printf("OK\n");
}

static void test_to_micro(void) {
    printf("test_to_micro...");
    for (size_t i = 0; i < sizeof(unix_tests) / sizeof(unix_tests[0]); i++) {
        TimeTest test = unix_tests[i];
        Time t = time_unix(test.sec, test.nsec);
        int64_t usec = test.sec * 1000000 + test.nsec / 1000;
        assert(time_to_micro(t) == usec);
    }
    printf("OK\n");
}

static void test_to_nano(void) {
    printf("test_to_nano...");
    for (size_t i = 0; i < sizeof(unix_tests) / sizeof(unix_tests[0]); i++) {
        TimeTest test = unix_tests[i];
        Time t = time_unix(test.sec, test.nsec);
        assert(time_to_nano(t) == test.sec * 1000000000 + test.nsec);
    }
    printf("OK\n");
}

#pragma endregion

#pragma region Calendar time.

typedef struct {
    int year, month, day, hour, min, sec, offset_sec;
    int tm_year, tm_mon, tm_mday, tm_hour, tm_min, tm_sec;
} CalendarTest;

static CalendarTest calendar_tests[] = {
    {2011, 11, 18, 15, 56, 35, TIMEX_UTC, 111, 10, 18, 15, 56, 35},
    {1901, 11, 18, 15, 56, 35, TIMEX_UTC, 1, 10, 18, 15, 56, 35},
    {1900, 11, 18, 15, 56, 35, TIMEX_UTC, 0, 10, 18, 15, 56, 35},
    {1800, 11, 18, 15, 56, 35, TIMEX_UTC, -100, 10, 18, 15, 56, 35},
    {1, 1, 1, 0, 0, 0, TIMEX_UTC, -1899, 0, 1, 0, 0, 0},

    {2011, 11, 18, 15, 56, 35, -5 * 3600, 111, 10, 18, 10, 56, 35},
    {2011, 11, 18, 15, 56, 35, 1 * 3600, 111, 10, 18, 16, 56, 35},
    {2011, 11, 18, 15, 56, 35, 12 * 3600, 111, 10, 19, 3, 56, 35},
    {2011, 11, 18, 15, 56, 35, -12 * 3600, 111, 10, 18, 3, 56, 35},
};

static void test_tm(void) {
    printf("test_tm...");
    for (size_t i = 0; i < sizeof(calendar_tests) / sizeof(calendar_tests[0]); i++) {
        CalendarTest test = calendar_tests[i];
        Time want =
            time_date(test.year, test.month, test.day, test.hour, test.min, test.sec, 0, TIMEX_UTC);
        struct tm tm = {.tm_year = test.tm_year,
                        .tm_mon = test.tm_mon,
                        .tm_mday = test.tm_mday,
                        .tm_hour = test.tm_hour,
                        .tm_min = test.tm_min,
                        .tm_sec = test.tm_sec};
        Time got = time_tm(tm, test.offset_sec);
        // printf("%d-%d-%d %d:%d:%d +%d, want %lld, got %lld\n", test.tm_year, test.tm_mon,
        //        test.tm_mday, test.tm_hour, test.tm_min, test.tm_sec, test.offset_sec / 3600,
        //        time_to_unix(want), time_to_unix(got));
        assert(time_equal(want, got));
    }
    printf("OK\n");
}

static void test_to_tm(void) {
    printf("test_to_tm...");
    for (size_t i = 0; i < sizeof(calendar_tests) / sizeof(calendar_tests[0]); i++) {
        CalendarTest test = calendar_tests[i];
        Time t =
            time_date(test.year, test.month, test.day, test.hour, test.min, test.sec, 0, TIMEX_UTC);
        struct tm tm = time_to_tm(t, test.offset_sec);
        assert(tm.tm_year == test.tm_year);
        assert(tm.tm_mon == test.tm_mon);
        assert(tm.tm_mday == test.tm_mday);
        assert(tm.tm_hour == test.tm_hour);
        assert(tm.tm_min == test.tm_min);
        assert(tm.tm_sec == test.tm_sec);
    }
    printf("OK\n");
}

#pragma endregion

#pragma region Comparisons.

typedef struct {
    ParsedTime t1, t2;
    int cmp;
} CompareTest;

static CompareTest compare_tests[] = {
    {
        {2011, 11, 18, 15, 56, 35, 1321631795},
        {2011, 11, 18, 15, 56, 35, 1321631795},
        0,
    },
    {
        {2011, 11, 18, 15, 56, 35, 0},
        {2011, 11, 18, 15, 56, 35, 1321631795},
        -1,
    },
    {
        {2011, 11, 18, 15, 56, 35, 1321631795},
        {2011, 11, 18, 15, 56, 35, 0},
        1,
    },
    {
        {2011, 11, 18, 15, 56, 25, 1321631795},
        {2011, 11, 18, 15, 56, 35, 1321631795},
        -1,
    },
    {
        {2011, 11, 18, 15, 56, 45, 1321631795},
        {2011, 11, 18, 15, 56, 35, 1321631795},
        1,
    },
    {
        {2011, 11, 18, 15, 55, 35, 1321631795},
        {2011, 11, 18, 15, 56, 35, 1321631795},
        -1,
    },
    {
        {2011, 11, 18, 15, 57, 35, 1321631795},
        {2011, 11, 18, 15, 56, 35, 1321631795},
        1,
    },
    {
        {2011, 11, 18, 14, 56, 35, 1321631795},
        {2011, 11, 18, 15, 56, 35, 1321631795},
        -1,
    },
    {
        {2011, 11, 18, 16, 56, 35, 1321631795},
        {2011, 11, 18, 15, 56, 35, 1321631795},
        1,
    },
    {
        {2011, 11, 17, 15, 56, 35, 1321631795},
        {2011, 11, 18, 15, 56, 35, 1321631795},
        -1,
    },
    {
        {2011, 11, 19, 15, 56, 35, 1321631795},
        {2011, 11, 18, 15, 56, 35, 1321631795},
        1,
    },
    {
        {2011, 10, 18, 15, 56, 35, 1321631795},
        {2011, 11, 18, 15, 56, 35, 1321631795},
        -1,
    },
    {
        {2011, 12, 18, 15, 56, 35, 1321631795},
        {2011, 11, 18, 15, 56, 35, 1321631795},
        1,
    },
    {
        {2010, 11, 18, 15, 56, 35, 1321631795},
        {2011, 11, 18, 15, 56, 35, 1321631795},
        -1,
    },
    {
        {2012, 11, 18, 15, 56, 35, 1321631795},
        {2011, 11, 18, 15, 56, 35, 1321631795},
        1,
    },
};

static void test_compare(void) {
    printf("test_compare...");
    for (size_t i = 0; i < sizeof(compare_tests) / sizeof(compare_tests[0]); i++) {
        CompareTest test = compare_tests[i];
        Time t1 = time_date(test.t1.year, test.t1.month, test.t1.day, test.t1.hour, test.t1.min,
                            test.t1.sec, test.t1.nsec, TIMEX_UTC);
        Time t2 = time_date(test.t2.year, test.t2.month, test.t2.day, test.t2.hour, test.t2.min,
                            test.t2.sec, test.t2.nsec, TIMEX_UTC);
        int cmp = time_compare(t1, t2);
        // printf("{%lld %lld} vs {%lld %lld}: want %d, got %d\n", t1.sec, t1.nsec, t2.sec, t2.nsec,
        //        test.cmp, cmp);
        assert(cmp == test.cmp);
        if (cmp == 0) {
            assert(time_equal(t1, t2));
            assert(!time_before(t1, t2));
            assert(!time_after(t1, t2));
        } else if (cmp < 0) {
            assert(!time_equal(t1, t2));
            assert(time_before(t1, t2));
            assert(!time_after(t1, t2));
        } else {
            assert(!time_equal(t1, t2));
            assert(!time_before(t1, t2));
            assert(time_after(t1, t2));
        }
    }
    printf("OK\n");
}

static void test_is_zero(void) {
    printf("test_is_zero...");
    Time t1 = time_date(2011, 11, 18, 15, 56, 35, 1321631795, TIMEX_UTC);
    assert(!time_is_zero(t1));
    Time t2 = time_date(1, 1, 1, 0, 0, 0, 0, TIMEX_UTC);
    assert(time_is_zero(t2));
    Time zero = {0, 0};
    assert(time_is_zero(zero));
    printf("OK\n");
}

#pragma endregion

#pragma region Arithmetic.

typedef struct {
    Time t;
    Duration d;
    Time r;
} AddTest;

static AddTest add_tests[] = {
    {{0, 0}, 0, {0, 0}},
    // 2009-11-23 00:00:00 +1ns
    {{1258916400, 0}, 1, {1258916400, 1}},
    // 2009-11-23 + 24h = 2009-11-24
    {{1258916400, 0}, 24 * 3600e9, {1259002800, 0}},
    // 2009-11-24 - 24h = 2009-11-23
    {{1259002800, 0}, -24 * 3600e9, {1258916400, 0}},
    // -2009-11-23 +24h = -2009-11-24
    {{-1259002800, 0}, 24 * 3600e9, {-1258916400, 0}},
    // 2000-01-01 + (290*365*24h + 71*24h) = 2290-01-01
    {{946684800, 0}, (290 * 365 + 71) * (24 * 3600e9), {10098259200, 0}},
    // 2290-01-01 - 290*365*24h + 71*24h = 2000-01-01
    {{10098259200, 0}, -(290 * 365 + 71) * (24 * 3600e9), {946684800, 0}},
    // 2019-08-16 02:29:30.268436582 + 9223372036795099414 = 2311-11-26 02:16:47.63535996
    {{1565922570, 268436582}, 9223372036795099414, {10789294607, 63535996}},
};

static void test_add(void) {
    printf("test_add...");
    for (size_t i = 0; i < sizeof(add_tests) / sizeof(add_tests[0]); i++) {
        AddTest test = add_tests[i];
        Time t = time_add(test.t, test.d);
        // printf("want %lld, got %lld\n", test.r.sec, t.sec);
        assert(time_equal(t, test.r));
    }
    printf("OK\n");
}

static void test_add_to_exact_second(void) {
    // Add an amount to the current time to round it up to the next exact second.
    // This test checks that the nsec field still lies within the range [0, 999999999].
    printf("test_add_to_exact_second...");
    Time t1 = time_now();
    Time t2 = time_add(t1, Second - t1.nsec);
    int sec = (t1.sec + 1) % 60;
    assert(time_get_second(t2) == sec && t2.nsec == 0);
    printf("OK\n");
}

typedef struct {
    Time t;
    Time u;
    Duration d;
} SubTest;

static SubTest sub_tests[] = {
    {{0, 0}, {0, 0}, 0},
    // 2009-11-23 00:00:00 -1ns
    {{1258916400, 1}, {1258916400, 0}, 1},
    // 2009-11-23 + 24h = 2009-11-24
    {{1258916400, 0}, {1259002800, 0}, -24 * 3600e9},
    // 2009-11-24 - 24h = 2009-11-23
    {{1259002800, 0}, {1258916400, 0}, 24 * 3600e9},
    // -2009-11-24 -24h = -2009-11-23
    {{-1258916400, 0}, {-1259002800, 0}, 24 * 3600e9},
    // 0001-01-01 - min = 2109-11-23
    {{-9223372036, -854775808}, {4414590000, 0}, MIN_DURATION},
    // 2109-11-23 - max = 0001-01-01
    {{4414590000, 0}, {-9223372036, -854775808}, MAX_DURATION},
    // 0001-01-01 - max = -2109-11-23
    {{-9223372036, -854775808}, {-128692627200, 0}, MAX_DURATION},
    // 2290-01-01 - (290*365*24h + 71*24h) = 2000-01-01
    {{10098259200, 0}, {946684800, 0}, (290 * 365 + 71) * (24 * 3600e9)},
    // 2300-01-01 - max = 2000-01-01
    {{10413792000, 0}, {946684800, 0}, MAX_DURATION},
    // 2000-01-01 + 290*365*24h + 71*24h = 2290-01-01
    {{946684800, 0}, {10098259200, 0}, (-290 * 365 - 71) * (24 * 3600e9)},
    // 2000-01-01 - min = 2300-01-01
    {{946684800, 0}, {10413792000, 0}, MIN_DURATION},
    // 2311-11-26 02:16:47.63535996 - 9223372036795099414 = 2019-08-16 02:29:30.268436582
    {{10789294607, 63535996}, {1565922570, 268436582}, 9223372036795099414},
};

static void test_sub(void) {
    printf("test_sub...");
    for (size_t i = 0; i < sizeof(sub_tests) / sizeof(sub_tests[0]); i++) {
        SubTest test = sub_tests[i];
        Duration d = time_sub(test.t, test.u);
        // printf("want %lld, got %lld\n", test.d, d);
        assert(d == test.d);
    }
    printf("OK\n");
}

typedef struct AddDateTest {
    int years, months, days;
    int y, m, d;
} AddDateTest;

// Several ways of getting from
// Fri Nov 18 7:56:35 PST 2011
// to
// Thu Mar 19 7:56:35 PST 2016
static AddDateTest add_date_tests[] = {
    {4, 4, 1, 2016, 3, 19},
    {3, 16, 1, 2016, 3, 19},
    {3, 15, 30, 2016, 3, 19},
    {5, -6, -18 - 30 - 12, 2016, 3, 19},
};

static void test_add_date(void) {
    printf("test_add_date...");
    Time t0 = time_date(2011, November, 18, 7, 56, 35, 0, TIMEX_UTC);
    Time t1 = time_date(2016, March, 19, 7, 56, 35, 0, TIMEX_UTC);
    for (size_t i = 0; i < sizeof(add_date_tests) / sizeof(add_date_tests[0]); i++) {
        AddDateTest test = add_date_tests[i];
        Time t = time_add_date(t0, test.years, test.months, test.days);
        assert(time_equal(t, t1));
    }
    printf("OK\n");
}

#pragma endregion

#pragma region Rounding.

typedef struct {
    ParsedTime t;
    Duration d;
    ParsedTime want;
} RoundTest;

static RoundTest truncate_tests[] = {
    // 1 second
    {{2011, 11, 18, 15, 56, 35, 777888999}, 1e9, {2011, 11, 18, 15, 56, 35, 0}},
    // 10 seconds
    {{2011, 11, 18, 15, 56, 35, 0}, 10e9, {2011, 11, 18, 15, 56, 30, 0}},
    // 30 seconds
    {{2011, 11, 18, 15, 56, 35, 0}, 30e9, {2011, 11, 18, 15, 56, 30, 0}},
    // 1 minute
    {{2011, 11, 18, 15, 56, 35, 0}, 60e9, {2011, 11, 18, 15, 56, 0, 0}},
    // 5 minutes
    {{2011, 11, 18, 15, 56, 35, 0}, 5 * 60e9, {2011, 11, 18, 15, 55, 0, 0}},
    // 30 minutes
    {{2011, 11, 18, 15, 56, 35, 0}, 30 * 60e9, {2011, 11, 18, 15, 30, 0, 0}},
    // 1 hour
    {{2011, 11, 18, 15, 56, 35, 0}, 3600e9, {2011, 11, 18, 15, 0, 0, 0}},
    // 6 hours
    {{2011, 11, 18, 15, 56, 35, 0}, 6 * 3600e9, {2011, 11, 18, 12, 0, 0, 0}},
    // 1 day (= 24 hours)
    {{2011, 11, 18, 15, 56, 35, 0}, 86400e9, {2011, 11, 18, 0, 0, 0, 0}},
};

static void test_truncate(void) {
    printf("test_truncate...");
    for (size_t i = 0; i < sizeof(truncate_tests) / sizeof(truncate_tests[0]); i++) {
        RoundTest test = truncate_tests[i];
        Time t = time_date(test.t.year, test.t.month, test.t.day, test.t.hour, test.t.min,
                           test.t.sec, test.t.nsec, TIMEX_UTC);
        Time got = time_truncate(t, test.d);
        Time want = time_date(test.want.year, test.want.month, test.want.day, test.want.hour,
                              test.want.min, test.want.sec, test.want.nsec, TIMEX_UTC);
        // printf("want: {%lld %lld}, got: {%lld %lld}\n", want.sec, want.nsec, got.sec, got.nsec);
        assert(time_equal(got, want));
    }
    printf("OK\n");
}

static RoundTest round_tests[] = {
    // 1 second
    {{2011, 11, 18, 15, 56, 35, 777888999}, 1e9, {2011, 11, 18, 15, 56, 36, 0}},
    // 10 seconds
    {{2011, 11, 18, 15, 56, 35, 0}, 10e9, {2011, 11, 18, 15, 56, 40, 0}},
    // 30 seconds
    {{2011, 11, 18, 15, 56, 35, 0}, 30e9, {2011, 11, 18, 15, 56, 30, 0}},
    // 1 minute
    {{2011, 11, 18, 15, 56, 35, 0}, 60e9, {2011, 11, 18, 15, 57, 0, 0}},
    // 5 minutes
    {{2011, 11, 18, 15, 56, 35, 0}, 5 * 60e9, {2011, 11, 18, 15, 55, 0, 0}},
    // 30 minutes
    {{2011, 11, 18, 15, 56, 35, 0}, 30 * 60e9, {2011, 11, 18, 16, 0, 0, 0}},
    // 1 hour
    {{2011, 11, 18, 15, 56, 35, 0}, 3600e9, {2011, 11, 18, 16, 0, 0, 0}},
    // 6 hours
    {{2011, 11, 18, 15, 56, 35, 0}, 6 * 3600e9, {2011, 11, 18, 18, 0, 0, 0}},
    // 1 day (= 24 hours)
    {{2011, 11, 18, 15, 56, 35, 0}, 86400e9, {2011, 11, 19, 0, 0, 0, 0}},
};

static void test_round(void) {
    printf("test_round...");
    for (size_t i = 0; i < sizeof(round_tests) / sizeof(round_tests[0]); i++) {
        RoundTest test = round_tests[i];
        Time t = time_date(test.t.year, test.t.month, test.t.day, test.t.hour, test.t.min,
                           test.t.sec, test.t.nsec, TIMEX_UTC);
        Time got = time_round(t, test.d);
        Time want = time_date(test.want.year, test.want.month, test.want.day, test.want.hour,
                              test.want.min, test.want.sec, test.want.nsec, TIMEX_UTC);
        // printf("want: {%lld %lld}, got: {%lld %lld}\n", want.sec, want.nsec, got.sec, got.nsec);
        assert(time_equal(got, want));
    }
    printf("OK\n");
}

#pragma endregion

#pragma region Formatting.

typedef struct {
    int year, month, day, hour, min, sec, nsec;
    const char* want;
    int offset_sec;
} FormatTest;

FormatTest fmt_iso_tests[] = {
    {2011, 11, 18, 15, 56, 35, 0, "2011-11-18T15:56:35Z", TIMEX_UTC},
    {2011, 11, 18, 15, 56, 35, 666777888, "2011-11-18T15:56:35.666777888Z", TIMEX_UTC},
    {2011, 11, 18, 15, 56, 35, 0, "2011-11-18T20:56:35+05:00", 5 * 3600},
    {2011, 11, 18, 15, 56, 35, 0, "2011-11-18T21:26:35+05:30", 5 * 3600 + 30 * 60},
    {2011, 11, 18, 15, 56, 35, 0, "2011-11-18T10:56:35-05:00", -5 * 3600},
    {2011, 11, 18, 15, 56, 35, 0, "2011-11-18T10:26:35-05:30", -5 * 3600 - 30 * 60},
    {2011, 11, 18, 15, 56, 35, 666777888, "2011-11-18T20:56:35.666777888+05:00", 5 * 3600},
    {2011, 11, 18, 15, 56, 35, 666777888, "2011-11-18T10:56:35.666777888-05:00", -5 * 3600},
};

static void test_fmt_iso(void) {
    printf("test_fmt_iso...");
    for (size_t i = 0; i < sizeof(fmt_iso_tests) / sizeof(fmt_iso_tests[0]); i++) {
        FormatTest test = fmt_iso_tests[i];
        Time t = time_date(test.year, test.month, test.day, test.hour, test.min, test.sec,
                           test.nsec, TIMEX_UTC);
        char got[64];
        time_fmt_iso(got, sizeof(got), t, test.offset_sec);
        // printf("want: %s, got: %s\n", test.want, got);
        assert(strcmp(got, test.want) == 0);
    }
    printf("OK\n");
}

FormatTest fmt_dt_tests[] = {
    {2011, 11, 18, 15, 56, 35, 0, "2011-11-18 15:56:35", TIMEX_UTC},
    {2011, 11, 18, 15, 56, 35, 666777888, "2011-11-18 15:56:35", TIMEX_UTC},
    {2011, 11, 18, 15, 56, 35, 0, "2011-11-18 20:56:35", 5 * 3600},
    {2011, 11, 18, 15, 56, 35, 0, "2011-11-18 21:26:35", 5 * 3600 + 30 * 60},
    {2011, 11, 18, 15, 56, 35, 0, "2011-11-18 10:56:35", -5 * 3600},
    {2011, 11, 18, 15, 56, 35, 0, "2011-11-18 10:26:35", -5 * 3600 - 30 * 60},
    {2011, 11, 18, 15, 56, 35, 666777888, "2011-11-18 20:56:35", 5 * 3600},
    {2011, 11, 18, 15, 56, 35, 666777888, "2011-11-18 10:56:35", -5 * 3600},
};

static void test_fmt_datetime(void) {
    printf("test_fmt_datetime...");
    for (size_t i = 0; i < sizeof(fmt_dt_tests) / sizeof(fmt_dt_tests[0]); i++) {
        FormatTest test = fmt_dt_tests[i];
        Time t = time_date(test.year, test.month, test.day, test.hour, test.min, test.sec,
                           test.nsec, TIMEX_UTC);
        char got[64];
        time_fmt_datetime(got, sizeof(got), t, test.offset_sec);
        // printf("want: %s, got: %s\n", test.want, got);
        assert(strcmp(got, test.want) == 0);
    }
    printf("OK\n");
}

FormatTest fmt_date_tests[] = {
    {2011, 11, 18, 15, 56, 35, 0, "2011-11-18", TIMEX_UTC},
    {2011, 11, 18, 15, 56, 35, 0, "2011-11-18", 5 * 3600},
    {2011, 11, 18, 15, 56, 35, 0, "2011-11-19", 12 * 3600},
    {2011, 11, 18, 15, 56, 35, 0, "2011-11-18", -5 * 3600},
    {2011, 11, 18, 15, 56, 35, 0, "2011-11-17", -20 * 3600},
};

static void test_fmt_date(void) {
    printf("test_fmt_date...");
    for (size_t i = 0; i < sizeof(fmt_date_tests) / sizeof(fmt_date_tests[0]); i++) {
        FormatTest test = fmt_date_tests[i];
        Time t = time_date(test.year, test.month, test.day, test.hour, test.min, test.sec,
                           test.nsec, TIMEX_UTC);
        char got[64];
        time_fmt_date(got, sizeof(got), t, test.offset_sec);
        // printf("want: %s, got: %s\n", test.want, got);
        assert(strcmp(got, test.want) == 0);
    }
    printf("OK\n");
}

FormatTest fmt_time_tests[] = {
    {2011, 11, 18, 15, 56, 35, 0, "15:56:35", TIMEX_UTC},
    {2011, 11, 18, 15, 56, 35, 0, "20:56:35", 5 * 3600},
    {2011, 11, 18, 15, 56, 35, 0, "03:56:35", 12 * 3600},
    {2011, 11, 18, 15, 56, 35, 0, "10:56:35", -5 * 3600},
    {2011, 11, 18, 15, 56, 35, 0, "19:56:35", -20 * 3600},
};

static void test_fmt_time(void) {
    printf("test_fmt_time...");
    for (size_t i = 0; i < sizeof(fmt_time_tests) / sizeof(fmt_time_tests[0]); i++) {
        FormatTest test = fmt_time_tests[i];
        Time t = time_date(test.year, test.month, test.day, test.hour, test.min, test.sec,
                           test.nsec, TIMEX_UTC);
        char got[64];
        time_fmt_time(got, sizeof(got), t, test.offset_sec);
        // printf("want: %s, got: %s\n", test.want, got);
        assert(strcmp(got, test.want) == 0);
    }
    printf("OK\n");
}

FormatTest parse_tests[] = {
    {2011, 11, 18, 15, 56, 35, 0, "2011-11-18T15:56:35Z", TIMEX_UTC},
    {2011, 11, 18, 15, 56, 35, 666777888, "2011-11-18T15:56:35.666777888Z", TIMEX_UTC},
    {2011, 11, 18, 15, 56, 35, 0, "2011-11-18T20:56:35+05:00", 5 * 3600},
    {2011, 11, 18, 15, 56, 35, 0, "2011-11-18T21:26:35+05:30", 5 * 3600 + 30 * 60},
    {2011, 11, 18, 15, 56, 35, 0, "2011-11-18T10:56:35-05:00", -5 * 3600},
    {2011, 11, 18, 15, 56, 35, 0, "2011-11-18T10:26:35-05:30", -5 * 3600 - 30 * 60},
    {2011, 11, 18, 15, 56, 35, 666777888, "2011-11-18T20:56:35.666777888+05:00", 5 * 3600},
    {2011, 11, 18, 15, 56, 35, 666777888, "2011-11-18T10:56:35.666777888-05:00", -5 * 3600},
    {2011, 11, 18, 15, 56, 35, 0, "2011-11-18 15:56:35", TIMEX_UTC},
    {2011, 11, 18, 0, 0, 0, 0, "2011-11-18", TIMEX_UTC},
    {1, 1, 1, 15, 56, 35, 0, "15:56:35", TIMEX_UTC},
    {1, 1, 1, 0, 0, 0, 0, "2011-11-18 10:56", TIMEX_UTC},
};

static void test_parse(void) {
    printf("test_parse...");
    for (size_t i = 0; i < sizeof(parse_tests) / sizeof(parse_tests[0]); i++) {
        FormatTest test = parse_tests[i];
        Time want = time_date(test.year, test.month, test.day, test.hour, test.min, test.sec,
                              test.nsec, TIMEX_UTC);
        Time got = time_parse(test.want);
        // printf("want: {%lld %d}, got: {%lld %d}\n", time_to_unix(want), want.nsec,
        //        time_to_unix(got), got.nsec);
        assert(time_equal(got, want));
    }
    printf("OK\n");
}

#pragma endregion

#pragma region Marshaling.

static void test_marshal_blob(void) {
    printf("test_marshal_blob...");
    uint8_t buf[15];
    for (size_t i = 0; i < sizeof(unix_tests) / sizeof(unix_tests[0]); i++) {
        TimeTest test = unix_tests[i];
        Time want = time_unix(test.sec, test.nsec);
        time_to_blob(want, buf);
        Time got = time_blob(buf);
        // printf("want: {%lld %lld}, got: {%lld %lld}\n", want.sec, want.nsec, got.sec, got.nsec);
        assert(time_equal(got, want));
    }
    printf("OK\n");
}

#pragma endregion

int main(void) {
    // Constructors.
    test_date();

    // Time parts.
    test_get_part();
    test_get_isoweek();
    test_get_yearday();

    // Unix time.
    test_unix();
    test_milli();
    test_micro();
    test_to_unix();
    test_to_milli();
    test_to_micro();
    test_to_nano();

    // Calendar time.
    test_tm();
    test_to_tm();

    // Comparisons.
    test_compare();
    test_is_zero();

    // Arithmetic.
    test_add();
    test_add_to_exact_second();
    test_sub();
    test_add_date();

    // Rounding.
    test_truncate();
    test_round();

    // Formatting.
    test_fmt_iso();
    test_fmt_datetime();
    test_fmt_date();
    test_fmt_time();
    test_parse();

    // Marshaling.
    test_marshal_blob();
}
