// Copyright (c) 2024 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Based on Go's time package, BSD 3-Clause License
// https://github.com/golang/go

// Time functions and methods.

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "time/common.h"
#include "time/timex.h"

// Some platforms do not support timespec_get() from time.h.
#if defined(_WIN32)
#include <sys/timeb.h>
#elif !defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L || \
    (!defined(TIME_UTC) && (!defined(_POSIX_TIMERS) || _POSIX_TIMERS <= 0))
#include <sys/time.h>
#endif

#pragma region Private

static const int64_t seconds_per_minute = 60;
static const int64_t seconds_per_hour = 60 * seconds_per_minute;
static const int64_t seconds_per_day = 24 * seconds_per_hour;
static const int64_t seconds_per_week = 7 * seconds_per_day;
static const int64_t days_per_400_years = 365 * 400 + 97;
static const int64_t days_per_100_years = 365 * 100 + 24;
static const int64_t days_per_4_years = 365 * 4 + 1;

// The unsigned zero year for internal calculations.
// Must be 1 mod 400, and times before it will not compute correctly,
// but otherwise can be changed at will.
static const int64_t absolute_zero_year = -292277022399LL;

// Offsets to convert between internal and absolute or Unix times.
// = (absoluteZeroYear - internalYear) * 365.2425 * secondsPerDay
static const int64_t absolute_to_internal = -9223371966579724800LL;
static const int64_t internal_to_absolute = -absolute_to_internal;

static const int64_t unix_to_internal =
    (1969 * 365 + 1969 / 4 - 1969 / 100 + 1969 / 400) * seconds_per_day;
static const int64_t internal_to_unix = -unix_to_internal;

// days_before[m] counts the number of days in a non-leap year
// before month m begins. There is an entry for m=12, counting
// the number of days before January of next year (365).
static const int days_before[] = {
    0,
    31,
    31 + 28,
    31 + 28 + 31,
    31 + 28 + 31 + 30,
    31 + 28 + 31 + 30 + 31,
    31 + 28 + 31 + 30 + 31 + 30,
    31 + 28 + 31 + 30 + 31 + 30 + 31,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + 31,
};

// norm returns nhi, nlo such that
//
//	hi * base + lo == nhi * base + nlo
//	0 <= nlo < base
//
// The arithmetic is done in int64_t so every combination of int inputs can
// normalize without overflowing an intermediate value.
static void norm(int64_t hi, int64_t lo, int64_t base, int64_t* nhi, int64_t* nlo) {
    if (lo < 0) {
        int64_t n = (-lo - 1) / base + 1;
        hi -= n;
        lo += n * base;
    }
    if (lo >= base) {
        int64_t n = lo / base;
        hi += n;
        lo -= n * base;
    }
    *nhi = hi;
    *nlo = lo;
}

// days_since_epoch takes a year and returns the number of days from
// the absolute epoch to the start of that year.
// This is basically (year - zeroYear) * 365, but accounting for leap days.
static uint64_t days_since_epoch(int64_t year) {
    uint64_t y = (uint64_t)(year - absolute_zero_year);

    // Add in days from 400-year cycles.
    uint64_t n = y / 400;
    y -= 400 * n;
    uint64_t d = days_per_400_years * n;

    // Add in 100-year cycles.
    n = y / 100;
    y -= 100 * n;
    d += days_per_100_years * n;

    // Add in 4-year cycles.
    n = y / 4;
    y -= 4 * n;
    d += days_per_4_years * n;

    // Add in non-leap years.
    n = y;
    d += 365 * n;

    return d;
}

// is_leap reports whether the year is a leap year.
static bool is_leap(int64_t year) {
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

// uint32_to_int32 interprets u modulo 2^32 as a signed 32-bit value, used for
// calendar years returned through the public int API.
static int32_t uint32_to_int32(uint32_t u) {
    if (u <= INT32_MAX) {
        return (int32_t)u;
    }
    return -1 - (int32_t)(UINT32_MAX - u);
}

// The epoch-offset conversions below are computed in uint64_t on purpose:
// the results are only meaningful modulo 2^64 (Go computes them in unsigned
// arithmetic as well), and plain int64_t addition would overflow for extreme
// time values, which is undefined behavior in C.
static int64_t unix_sec(Time t) {
    return uint64_to_int64((uint64_t)t.sec + (uint64_t)internal_to_unix);
}

static Time unix_time(int64_t sec, int32_t nsec) {
    return (Time){uint64_to_int64((uint64_t)sec + (uint64_t)unix_to_internal), nsec};
}

// abs_time returns the time t as an absolute time, adjusted by the zone offset.
// It is called when computing a presentation property like Month or Hour.
static uint64_t abs_time(Time t) {
    return (uint64_t)t.sec + (uint64_t)internal_to_absolute;
}

// abs_weekday is like Weekday but operates on an absolute time.
static enum Weekday abs_weekday(uint64_t abs) {
    // January 1 of the absolute year, like January 1 of 2001, was a Monday.
    uint64_t sec = (abs + Monday * seconds_per_day) % seconds_per_week;
    return sec / seconds_per_day;
}

static void abs_date(uint64_t abs, int* year, int* yday) {
    // Split into time and day.
    uint64_t d = abs / seconds_per_day;

    // Account for 400 year cycles.
    uint64_t n = d / days_per_400_years;
    uint64_t y = 400 * n;
    d -= days_per_400_years * n;

    // Cut off 100-year cycles.
    // The last cycle has one extra leap year, so on the last day
    // of that year, day / days_per_100_years will be 4 instead of 3.
    // Cut it back down to 3 by subtracting n>>2.
    n = d / days_per_100_years;
    n -= n >> 2;
    y += 100 * n;
    d -= days_per_100_years * n;

    // Cut off 4-year cycles.
    // The last cycle has a missing leap year, which does not
    // affect the computation.
    n = d / days_per_4_years;
    y += 4 * n;
    d -= days_per_4_years * n;

    // Cut off years within a 4-year cycle.
    // The last year is a leap year, so on the last day of that year,
    // day / 365 will be 4 instead of 3. Cut it back down to 3
    // by subtracting n>>2.
    n = d / 365;
    n -= n >> 2;
    y += n;
    d -= 365 * n;

    // The true calendar year does not fit into int for extreme time values
    // (roughly beyond year ±2 billion); wrapping mirrors Go's int conversion.
    *year = uint32_to_int32((uint32_t)((uint64_t)y + (uint64_t)absolute_zero_year));
    *yday = d;
}

static void abs_date_full(uint64_t abs, int* year, enum Month* month, int* day, int* yday) {
    abs_date(abs, year, yday);

    *day = *yday;
    if (is_leap(*year)) {
        // Leap year
        if (*day > 31 + 29 - 1) {
            // After leap day; pretend it wasn't there.
            *day -= 1;
        } else if (*day == 31 + 29 - 1) {
            // Leap day. The branches must be mutually exclusive: otherwise
            // March 1 (yday 60) would be decremented to 59 and then
            // misreported as February 29.
            *month = February;
            *day = 29;
            return;
        }
    }

    // Estimate month on assumption that every month has 31 days.
    // The estimate may be too low by at most one month, so adjust.
    *month = *day / 31;
    int end = days_before[(int)(*month) + 1];
    int begin;
    if (*day >= end) {
        *month += 1;
        begin = end;
    } else {
        begin = days_before[(int)(*month)];
    }

    *month += 1;  // because January is 1
    *day = *day - begin + 1;
}

static void abs_clock(uint64_t abs, int* hour, int* min, int* sec) {
    *sec = abs % seconds_per_day;
    *hour = *sec / seconds_per_hour;
    *sec -= *hour * seconds_per_hour;
    *min = *sec / seconds_per_minute;
    *sec -= *min * seconds_per_minute;
}

// tless_than_half reports whether x+x < y but avoids overflow,
// assuming x and y are both positive (Duration is signed).
static bool tless_than_half(Duration x, Duration y) {
    return (uint64_t)x + (uint64_t)x < (uint64_t)y;
}

// time_div divides t by d and returns the remainder.
// Only supports d which is a multiple of 1 second.
static Duration time_div(Time t, Duration d) {
    if (d % Second != 0) {
        return 0;
    }

    bool neg = false;
    uint64_t sec = t.sec;
    int64_t nsec = t.nsec;
    if (t.sec < 0) {
        // Operate on absolute value. Unsigned negation also works
        // for t.sec == INT64_MIN, where -t.sec would overflow.
        neg = true;
        sec = -sec;
        nsec = -nsec;
        if (nsec < 0) {
            nsec += 1e9;
            sec--;  // sec >= 1 before the -- so safe
        }
    }

    // d is a multiple of 1 second.
    uint64_t d1 = d / Second;
    Duration r = (int64_t)((sec % d1) * (uint64_t)Second + (uint64_t)nsec);

    if (neg && r != 0) {
        r = d - r;
    }
    return r;
}

// timespec_now returns the current time with nanosecond precision.
static struct timespec timespec_now(void) {
    struct timespec ts;
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && defined(TIME_UTC) && \
    !defined(__ANDROID__)
    // C11.
    timespec_get(&ts, TIME_UTC);
#elif defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)
    // POSIX.
    clock_gettime(CLOCK_REALTIME, &ts);
#elif defined(_WIN32)
    // Windows.
    struct __timeb64 tb;
    _ftime64(&tb);
    ts.tv_sec = (time_t)tb.time;
    ts.tv_nsec = tb.millitm * 1000000;
#else
    // Fallback for older systems.
    struct timeval tv;
    gettimeofday(&tv, NULL);
    ts.tv_sec = tv.tv_sec;
    ts.tv_nsec = tv.tv_usec * 1000;
#endif
    return ts;
}

#pragma endregion

#pragma region Constructors

// time_now returns the current time in UTC.
Time time_now(void) {
    struct timespec ts = timespec_now();
    return unix_time(ts.tv_sec, ts.tv_nsec);
}

// time_date returns the Time corresponding to
// yyyy-mm-dd hh:mm:ss + nsec nanoseconds
//
// The month, day, hour, min, sec, and nsec values may be outside
// their usual ranges and will be normalized during the conversion.
// For example, October 32 converts to November 1.
//
// The time is converted to UTC using offset_sec in seconds east of UTC.
static Time time_date64(int64_t year,
                        int64_t month,
                        int64_t day,
                        int64_t hour,
                        int64_t min,
                        int64_t sec,
                        int64_t nsec,
                        int64_t offset_sec) {
    // Normalize month, overflowing into year.
    int64_t year64, month64;
    norm(year, month - 1, 12, &year64, &month64);
    month = (int)(month64 + 1);

    // Normalize nsec, sec, min, hour, overflowing into day.
    int64_t day64, hour64, min64, sec64, nsec64;
    norm(sec, nsec, 1000000000, &sec64, &nsec64);
    norm(min, sec64, 60, &min64, &sec64);
    norm(hour, min64, 60, &hour64, &min64);
    norm(day, hour64, 24, &day64, &hour64);

    // Compute days since the absolute epoch.
    uint64_t d = days_since_epoch(year64);

    // Add in days before this month.
    d += days_before[month - 1];
    if (is_leap(year64) && month >= March) {
        d++;  // February 29
    }

    // Add in days before today.
    d += (uint64_t)(day64 - 1);

    // Add in time elapsed today.
    uint64_t abs = d * seconds_per_day;
    abs += (uint64_t)(hour64 * seconds_per_hour + min64 * seconds_per_minute + sec64);

    // Convert to UTC.
    abs -= offset_sec;

    return (Time){uint64_to_int64(abs + (uint64_t)absolute_to_internal), (int32_t)nsec64};
}

Time time_date(int year,
               int month,
               int day,
               int hour,
               int min,
               int sec,
               int nsec,
               int offset_sec) {
    return time_date64(year, month, day, hour, min, sec, nsec, offset_sec);
}

#pragma endregion

#pragma region Time parts

// time_get_date returns the year, month, and day in which t occurs.
void time_get_date(Time t, int* year, enum Month* month, int* day) {
    uint64_t abs = abs_time(t);
    int yday;
    abs_date_full(abs, year, month, day, &yday);
}

// time_get_year returns the year in which t occurs.
int time_get_year(Time t) {
    uint64_t abs = abs_time(t);
    int year, yday;
    abs_date(abs, &year, &yday);
    return year;
}

// time_get_month returns the month of the year specified by t.
enum Month time_get_month(Time t) {
    uint64_t abs = abs_time(t);
    int year, day, yday;
    enum Month month;
    abs_date_full(abs, &year, &month, &day, &yday);
    return month;
}

// time_get_day returns the day of the month specified by t.
int time_get_day(Time t) {
    uint64_t abs = abs_time(t);
    int year, day, yday;
    enum Month month;
    abs_date_full(abs, &year, &month, &day, &yday);
    return day;
}

// time_get_clock returns the hour, minute, and second within the day specified by t.
void time_get_clock(Time t, int* hour, int* min, int* sec) {
    uint64_t abs = abs_time(t);
    abs_clock(abs, hour, min, sec);
}

// time_get_hour returns the hour within the day specified by t, in the range [0, 23].
int time_get_hour(Time t) {
    uint64_t abs = abs_time(t);
    return (abs % seconds_per_day) / seconds_per_hour;
}

// time_get_minute returns the minute offset within the hour specified by t, in the range [0, 59].
int time_get_minute(Time t) {
    uint64_t abs = abs_time(t);
    return (abs % seconds_per_hour) / seconds_per_minute;
}

// time_get_second returns the second offset within the minute specified by t, in the range [0, 59].
int time_get_second(Time t) {
    uint64_t abs = abs_time(t);
    return abs % seconds_per_minute;
}

// time_get_nano returns the nanosecond offset within the second specified by t,
// in the range [0, 999999999].
int time_get_nano(Time t) {
    return t.nsec;
}

// time_get_weekday returns the day of the week specified by t.
enum Weekday time_get_weekday(Time t) {
    uint64_t abs = abs_time(t);
    return abs_weekday(abs);
}

// time_get_yearday returns the day of the year specified by t, in the range [1,365] for non-leap
// years, and [1,366] in leap years.
int time_get_yearday(Time t) {
    uint64_t abs = abs_time(t);
    int year, yday;
    abs_date(abs, &year, &yday);
    return yday + 1;
}

// time_get_isoweek returns the ISO 8601 year and week number in which t occurs.
// Week ranges from 1 to 53. Jan 01 to Jan 03 of year n might belong to
// week 52 or 53 of year n-1, and Dec 29 to Dec 31 might belong to week 1 of year n+1.
void time_get_isoweek(Time t, int* year, int* week) {
    // According to the rule that the first calendar week of a calendar year is
    // the week including the first Thursday of that year, and that the last one is
    // the week immediately preceding the first calendar week of the next calendar year.
    // See https://www.iso.org/obp/ui#iso:std:iso:8601:-1:ed-1:v1:en:term:3.1.1.23 for details.

    // weeks start with Monday
    // Monday Tuesday Wednesday Thursday Friday Saturday Sunday
    // 1      2       3         4        5      6        7
    // +3     +2      +1        0        -1     -2       -3
    // the offset to Thursday
    uint64_t abs = abs_time(t);
    int d = (Thursday - abs_weekday(abs));
    // handle Sunday
    if (d == 4) {
        d = -3;
    }
    // find the Thursday of the calendar week
    int yday;
    abs += d * seconds_per_day;
    abs_date(abs, year, &yday);
    *week = yday / 7 + 1;
}

#pragma endregion

#pragma region Unix time

// time_unix returns the Time corresponding to the given Unix time,
// sec seconds and nsec nanoseconds since January 1, 1970 UTC.
// It is valid to pass nsec outside the range [0, 999999999].
// Not all sec values have a corresponding time value. One such
// value is 1<<63-1 (the largest int64 value).
Time time_unix(int64_t sec, int64_t nsec) {
    if (nsec < 0 || nsec >= 1000000000) {
        int64_t n = nsec / 1000000000;
        // Wraparound is intentional (as in Go): unsigned arithmetic keeps
        // the overflow well-defined for extreme arguments.
        sec = uint64_to_int64((uint64_t)sec + (uint64_t)n);
        nsec -= n * 1000000000;
        if (nsec < 0) {
            nsec += 1000000000;
            sec = uint64_to_int64((uint64_t)sec - 1);
        }
    }
    return unix_time(sec, (int32_t)nsec);
}

// time_milli returns the Time corresponding to the given Unix time,
// msec milliseconds since January 1, 1970 UTC.
Time time_milli(int64_t msec) {
    return time_unix(msec / 1000, (msec % 1000) * 1000000);
}

// time_micro returns the Time corresponding to the given Unix time,
// usec microseconds since January 1, 1970 UTC.
Time time_micro(int64_t usec) {
    return time_unix(usec / 1000000, (usec % 1000000) * 1000);
}

// time_nano returns the Time corresponding to the given Unix time,
// nsec nanoseconds since January 1, 1970 UTC.
Time time_nano(int64_t nsec) {
    return time_unix(0, nsec);
}

// time_to_unix returns t as a Unix time, the number of seconds elapsed
// since January 1, 1970 UTC.
// Unix-like operating systems often record time as a 32-bit
// count of seconds, but since the method here returns a 64-bit
// value it is valid for billions of years into the past or future.
int64_t time_to_unix(Time t) {
    return unix_sec(t);
}

// time_to_milli returns t as a Unix time, the number of milliseconds elapsed since
// January 1, 1970 UTC. The result wraps around modulo 2^64 if the Unix time in
// milliseconds cannot be represented by an int64 (a date more than 292 million
// years before or after 1970).
int64_t time_to_milli(Time t) {
    return uint64_to_int64((uint64_t)unix_sec(t) * 1000 + (uint64_t)(t.nsec / 1000000));
}

// time_to_micro returns t as a Unix time, the number of microseconds elapsed since
// January 1, 1970 UTC. The result wraps around modulo 2^64 if the Unix time in
// microseconds cannot be represented by an int64 (a date before year -290307 or
// after year 294246).
int64_t time_to_micro(Time t) {
    return uint64_to_int64((uint64_t)unix_sec(t) * 1000000 + (uint64_t)(t.nsec / 1000));
}

// time_to_nano returns t as a Unix time, the number of nanoseconds elapsed
// since January 1, 1970 UTC. The result wraps around modulo 2^64 if the Unix
// time in nanoseconds cannot be represented by an int64 (a date before the year
// 1678 or after 2262).
int64_t time_to_nano(Time t) {
    return uint64_to_int64((uint64_t)unix_sec(t) * 1000000000 + (uint64_t)t.nsec);
}

#pragma endregion

#pragma region Calendar time

// clamp_int narrows v to the int range.
static int clamp_int(int64_t v) {
    if (v > INT32_MAX) {
        return INT32_MAX;
    }
    if (v < INT32_MIN) {
        return INT32_MIN;
    }
    return (int)v;
}

// time_tm returns the Time corresponding to the given calendar time at the given timezone offset.
Time time_tm(struct tm tm, int offset_sec) {
    return time_date64((int64_t)tm.tm_year + 1900, (int64_t)tm.tm_mon + 1, tm.tm_mday,
                       tm.tm_hour, tm.tm_min, tm.tm_sec, 0, offset_sec);
}

// time_to_tm returns t in the given timezone offset as a calendar time.
struct tm time_to_tm(Time t, int offset_sec) {
    Time loc_t = time_add(t, offset_sec * Second);
    int year, day, hour, min, sec;
    enum Month month;
    time_get_date(loc_t, &year, &month, &day);
    time_get_clock(loc_t, &hour, &min, &sec);
    struct tm tm = {
        // Clamped: year - 1900 would overflow int for extreme time values.
        .tm_year = clamp_int((int64_t)year - 1900),
        .tm_mon = month - 1,
        .tm_mday = day,
        .tm_hour = hour,
        .tm_min = min,
        .tm_sec = sec,
        .tm_isdst = -1,
    };
    return tm;
}

#pragma endregion

#pragma region Comparison

// time_after reports whether the time instant t is after u.
bool time_after(Time t, Time u) {
    return t.sec > u.sec || (t.sec == u.sec && t.nsec > u.nsec);
}

// time_before reports whether the time instant t is before u.
bool time_before(Time t, Time u) {
    return t.sec < u.sec || (t.sec == u.sec && t.nsec < u.nsec);
}

// time_compare compares the time instant t with u. If t is before u, it returns -1;
// if t is after u, it returns +1; if they're the same, it returns 0.
int time_compare(Time t, Time u) {
    if (time_before(t, u)) {
        return -1;
    }
    if (time_after(t, u)) {
        return +1;
    }
    return 0;
}

// time_equal reports whether t and u represent the same time instant.
bool time_equal(Time t, Time u) {
    return t.sec == u.sec && t.nsec == u.nsec;
}

// time_is_zero reports whether t represents the zero time instant,
// January 1, year 1, 00:00:00 UTC.
bool time_is_zero(Time t) {
    return t.sec == 0 && t.nsec == 0;
}

#pragma endregion

#pragma region Arithmetic

// time_add returns the time t+d. If the result is outside the range of
// representable times, it saturates to the maximum (or minimum) time.
Time time_add(Time t, Duration d) {
    int64_t dsec = d / Second;
    int64_t nsec = t.nsec + d % 1000000000;
    if (nsec >= 1e9) {
        dsec++;
        nsec -= 1e9;
    } else if (nsec < 0) {
        dsec--;
        nsec += 1e9;
    }
    // The bounds are checked before adding, so no signed overflow can occur.
    int64_t sec;
    if (dsec > 0 && t.sec > INT64_MAX - dsec) {
        sec = INT64_MAX;
        nsec = 999999999;
    } else if (dsec < 0 && t.sec < INT64_MIN - dsec) {
        sec = INT64_MIN;
        nsec = 0;
    } else {
        sec = t.sec + dsec;
    }
    return (Time){sec, (int32_t)nsec};
}

// time_sub returns the duration t-u. If the result exceeds the maximum (or minimum)
// value that can be stored in a Duration, the maximum (or minimum) duration
// will be returned.
Duration time_sub(Time t, Time u) {
    // The candidate difference is computed modulo 2^64 (as in Go): unsigned
    // arithmetic keeps the overflow well-defined. The roundtrip check then
    // detects the wrap: because time_add saturates instead of wrapping,
    // an out-of-range difference cannot reconstruct t, and the result
    // clamps to MIN/MAX_DURATION.
    uint64_t sec = (uint64_t)t.sec - (uint64_t)u.sec;
    int64_t d =
        uint64_to_int64(sec * (uint64_t)Second + (uint64_t)(int64_t)(t.nsec - u.nsec));
    if (time_equal(time_add(u, d), t)) {
        return d;  // d is correct
    }
    if (time_before(t, u)) {
        return MIN_DURATION;  // t - u is negative out of range
    }
    return MAX_DURATION;  // t - u is positive out of range
}

// time_since returns the time elapsed since t.
// It is shorthand for time_sub(time_now(), t).
Duration time_since(Time t) {
    return time_sub(time_now(), t);
}

// time_until returns the duration until t.
// It is shorthand for time_sub(t, time_now()).
Duration time_until(Time t) {
    return time_sub(t, time_now());
}

// time_add_date returns the time corresponding to adding the
// given number of years, months, and days to t.
// For example, time_add_date(-1, 2, 3) applied to January 1, 2011
// returns March 4, 2010.
//
// time_add_date normalizes its result in the same way that Date does,
// so, for example, adding one month to October 31 yields
// December 1, the normalized form for November 31.
Time time_add_date(Time t, int years, int months, int days) {
    int year, day;
    enum Month month;
    time_get_date(t, &year, &month, &day);
    int hour, min, sec;
    time_get_clock(t, &hour, &min, &sec);
    return time_date64((int64_t)year + years, (int64_t)month + months, (int64_t)day + days, hour,
                       min, sec, t.nsec, TIMEX_UTC);
}

#pragma endregion

#pragma region Rounding

// time_truncate returns the result of rounding t down to a multiple of d (since the zero time).
// Only supports d which is a multiple of 1 second. If d <= 0, returns t unchanged.
Time time_truncate(Time t, Duration d) {
    if (d <= 0) {
        return t;
    }
    Duration r = time_div(t, d);
    return time_add(t, -r);
}

// time_round returns the result of rounding t to the nearest multiple of d (since the zero time).
// The rounding behavior for halfway values is to round up.
// If d <= 0, returns t unchanged.
Time time_round(Time t, Duration d) {
    if (d <= 0) {
        return t;
    }
    Duration r = time_div(t, d);
    if (tless_than_half(r, d)) {
        return time_add(t, -r);
    }
    return time_add(t, d - r);
}

#pragma endregion

#pragma region Formatting

// fmt_year renders the year as Go does: a minus sign for negative years,
// then the magnitude zero-padded to at least four digits, printing all
// digits for wider years ("%04d" alone renders year -20 as "-020", where
// Go prints "-0020"). buf must hold at least 12 bytes ('-' plus 10 digits
// plus NUL, for year == INT32_MIN).
static const char* fmt_year(char buf[12], int year) {
    if (year < 0) {
        // int64_t because -year would overflow int for year == INT32_MIN.
        unsigned int magnitude = (unsigned int)(-(int64_t)year);
        snprintf(buf, 12, "-%04u", magnitude);
    } else {
        snprintf(buf, 12, "%04d", year);
    }
    return buf;
}

// time_fmt_iso returns an ISO 8601 time string for the given time value.
// Converts the time value to the given timezone offset before formatting.
// Chooses the most compact representation:
//  - 2006-01-02T15:04:05.999999999+07:00
//  - 2006-01-02T15:04:05.999999999Z
//  - 2006-01-02T15:04:05+07:00
//  - 2006-01-02T15:04:05Z
size_t time_fmt_iso(char* buf, size_t size, Time t, int offset_sec) {
    int year, day, hour, min, sec;
    enum Month month;
    const char* layout;
    size_t n = 0;

    char ybuf[12];
    if (offset_sec == 0) {
        time_get_date(t, &year, &month, &day);
        time_get_clock(t, &hour, &min, &sec);
        if (t.nsec == 0) {
            layout = "%s-%02d-%02dT%02d:%02d:%02dZ";
            n = snprintf(buf, size, layout, fmt_year(ybuf, year), month, day, hour, min, sec);
        } else {
            layout = "%s-%02d-%02dT%02d:%02d:%02d.%09dZ";
            n = snprintf(buf, size, layout, fmt_year(ybuf, year), month, day, hour, min, sec,
                         t.nsec);
        }
    } else {
        Time loc_t = time_add(t, offset_sec * Second);
        time_get_date(loc_t, &year, &month, &day);
        time_get_clock(loc_t, &hour, &min, &sec);
        // Normalize the sign before splitting hours/minutes (as Go does):
        // otherwise an offset like -30 minutes prints as "+00:30".
        // int64_t because -offset_sec would overflow int for INT_MIN.
        char ofsign = '+';
        int64_t ofsec = offset_sec;
        if (ofsec < 0) {
            ofsign = '-';
            ofsec = -ofsec;
        }
        int ofhour = (int)(ofsec / 3600);
        int ofmin = (int)((ofsec % 3600) / 60);
        // Go drops the offset sign when the offset is less than a minute:
        // e.g. -59 seconds formats as "+00:00". The time fields are still
        // shifted by the full offset.
        if (ofhour == 0 && ofmin == 0) {
            ofsign = '+';
        }
        if (loc_t.nsec == 0) {
            layout = "%s-%02d-%02dT%02d:%02d:%02d%c%02d:%02d";
            n = snprintf(buf, size, layout, fmt_year(ybuf, year), month, day, hour, min, sec,
                         ofsign, ofhour, ofmin);
        } else {
            layout = "%s-%02d-%02dT%02d:%02d:%02d.%09d%c%02d:%02d";
            n = snprintf(buf, size, layout, fmt_year(ybuf, year), month, day, hour, min, sec,
                         loc_t.nsec, ofsign, ofhour, ofmin);
        }
    }
    return n;
}

// time_fmt_datetime returns a datetime string
// (2006-01-02 15:04:05) for the given time value.
// Converts the time value to the given timezone offset before formatting.
size_t time_fmt_datetime(char* buf, size_t size, Time t, int offset_sec) {
    int year, day, hour, min, sec;
    enum Month month;
    if (offset_sec == 0) {
        time_get_date(t, &year, &month, &day);
        time_get_clock(t, &hour, &min, &sec);
    } else {
        Time loc_t = time_add(t, offset_sec * Second);
        time_get_date(loc_t, &year, &month, &day);
        time_get_clock(loc_t, &hour, &min, &sec);
    }
    char ybuf[12];
    return snprintf(buf, size, "%s-%02d-%02d %02d:%02d:%02d", fmt_year(ybuf, year), month, day,
                    hour, min, sec);
}

// time_fmt_date returns a date string
// (2006-01-02) for the given time value.
// Converts the time value to the given timezone offset before formatting.
size_t time_fmt_date(char* buf, size_t size, Time t, int offset_sec) {
    int year, day;
    enum Month month;
    if (offset_sec == 0) {
        time_get_date(t, &year, &month, &day);
    } else {
        Time loc_t = time_add(t, offset_sec * Second);
        time_get_date(loc_t, &year, &month, &day);
    }
    char ybuf[12];
    return snprintf(buf, size, "%s-%02d-%02d", fmt_year(ybuf, year), month, day);
}

// time_fmt_time returns a time string
// (15:04:05) for the given time value.
// Converts the time value to the given timezone offset before formatting.
size_t time_fmt_time(char* buf, size_t size, Time t, int offset_sec) {
    int hour, min, sec;
    if (offset_sec == 0) {
        time_get_clock(t, &hour, &min, &sec);
    } else {
        Time loc_t = time_add(t, offset_sec * Second);
        time_get_clock(loc_t, &hour, &min, &sec);
    }
    return snprintf(buf, size, "%02d:%02d:%02d", hour, min, sec);
}

// is_space reports whether c is whitespace, matching SQLite's sqlite3Isspace.
static bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

// scan_digits reads exactly n digits.
// Returns a pointer to the character after the digits, or NULL on failure.
static const char* scan_digits(const char* p, int n, int* out) {
    int val = 0;
    for (int i = 0; i < n; i++) {
        if (*p < '0' || *p > '9') {
            return NULL;
        }
        val = val * 10 + (*p - '0');
        p++;
    }
    *out = val;
    return p;
}

// scan_date reads a date (2006-01-02).
// Returns a pointer to the character after the date, or NULL on failure.
static const char* scan_date(const char* p, int* year, int* month, int* day) {
    p = scan_digits(p, 4, year);
    if (p == NULL || *p != '-') {
        return NULL;
    }
    p = scan_digits(p + 1, 2, month);
    if (p == NULL || *p != '-') {
        return NULL;
    }
    return scan_digits(p + 1, 2, day);
}

// scan_frac reads a fractional second (.999999999) and scales it to nanoseconds.
// Digits past the ninth are consumed and discarded, as Go's time.Parse does.
// Returns a pointer to the character after the fraction, or NULL on failure.
static const char* scan_frac(const char* p, int* nsec) {
    p++;  // skip the '.'
    if (*p < '0' || *p > '9') {
        // a '.' must be followed by at least one digit
        return NULL;
    }
    int val = 0, ndigits = 0;
    for (; *p >= '0' && *p <= '9'; p++) {
        if (ndigits < 9) {
            val = val * 10 + (*p - '0');
            ndigits++;
        }
    }
    for (; ndigits < 9; ndigits++) {
        val *= 10;
    }
    *nsec = val;
    return p;
}

// scan_zone reads a timezone offset (Z or +07:00).
// Returns a pointer to the character after the offset, or NULL on failure.
static const char* scan_zone(const char* p, int* offset_sec) {
    // RFC 3339 permits lowercase "z".
    if (*p == 'Z' || *p == 'z') {
        *offset_sec = TIMEX_UTC;
        return p + 1;
    }
    int sign = (*p == '-') ? -1 : 1;
    int hour, min;
    p = scan_digits(p + 1, 2, &hour);
    if (p == NULL || *p != ':') {
        return NULL;
    }
    p = scan_digits(p + 1, 2, &min);
    if (p == NULL) {
        return NULL;
    }
    *offset_sec = sign * (hour * 3600 + min * 60);
    return p;
}

// scan_clock reads a time (15:04:05) with an optional fractional second
// and an optional timezone offset.
// Returns a pointer to the character after the time, or NULL on failure.
static const char* scan_clock(const char* p,
                              int* hour,
                              int* min,
                              int* sec,
                              int* nsec,
                              int* offset_sec) {
    p = scan_digits(p, 2, hour);
    if (p == NULL || *p != ':') {
        return NULL;
    }
    p = scan_digits(p + 1, 2, min);
    if (p == NULL || *p != ':') {
        return NULL;
    }
    p = scan_digits(p + 1, 2, sec);
    if (p == NULL) {
        return NULL;
    }
    if (*p == '.') {
        p = scan_frac(p, nsec);
        if (p == NULL) {
            return NULL;
        }
    }
    if (*p == 'Z' || *p == 'z' || *p == '+' || *p == '-') {
        p = scan_zone(p, offset_sec);
        if (p == NULL) {
            return NULL;
        }
    }
    return p;
}

// time_parse parses either date [separator clock] or clock, where date is
// "2006-01-02", separator is "T"/"t" or one or more ASCII whitespace characters,
// and clock is "15:04:05" with optional fraction and zone ("Z"/"z" or ±07:00).
// Numeric date, clock, and offset fields have fixed widths but no range checks;
// time_date normalizes them. Fractions beyond nine digits are truncated.
// Returns false and leaves out unchanged on invalid syntax or NULL.
bool time_parse(const char* value, Time* out) {
    if (value == NULL) {
        return false;
    }

    int year = 1, month = 1, day = 1;
    int hour = 0, min = 0, sec = 0, nsec = 0, offset_sec = TIMEX_UTC;

    const char* p = scan_date(value, &year, &month, &day);
    if (p != NULL) {
        if (*p == 'T' || *p == 't') {
            p = scan_clock(p + 1, &hour, &min, &sec, &nsec, &offset_sec);
        } else if (is_space(*p)) {
            while (is_space(*p)) {
                p++;
            }
            p = scan_clock(p, &hour, &min, &sec, &nsec, &offset_sec);
        }
    } else {
        p = scan_clock(value, &hour, &min, &sec, &nsec, &offset_sec);
    }
    if (p == NULL || *p != '\0') {
        return false;
    }

    *out = time_date(year, month, day, hour, min, sec, nsec, offset_sec);
    return true;
}

#pragma endregion

#pragma region Marshaling

// time_blob decodes the time instant represented by the binary data into out.
// The blob must have been created by time_to_blob and be TIMEX_BLOB_SIZE bytes long.
// Returns false and leaves out unchanged if the blob is invalid: an unknown version
// or an out-of-range nanosecond field (as Go's Time.UnmarshalBinary rejects them).
bool time_blob(const uint8_t* buf, Time* out) {
    const uint8_t version = buf[0];
    if (version != 1) {
        return false;
    }

    // Assemble in unsigned arithmetic: shifting e.g. 0xff by 56 places is not
    // representable in int64_t, which would be undefined behavior (reachable
    // for any time before year 1, where sec is negative).
    uint64_t sec = (uint64_t)buf[8] | (uint64_t)buf[7] << 8 | (uint64_t)buf[6] << 16 |
                   (uint64_t)buf[5] << 24 | (uint64_t)buf[4] << 32 | (uint64_t)buf[3] << 40 |
                   (uint64_t)buf[2] << 48 | (uint64_t)buf[1] << 56;

    uint32_t nsec = (uint32_t)buf[12] | (uint32_t)buf[11] << 8 | (uint32_t)buf[10] << 16 |
                    (uint32_t)buf[9] << 24;

    if (nsec > 999999999) {
        // Invalid blob: the nanosecond field is outside [0, 999999999].
        return false;
    }
    *out = (Time){uint64_to_int64(sec), (int32_t)nsec};
    return true;
}

// time_to_blob returns the binary representation of the time instant t.
// The result is a byte slice with the following layout:
// 0: version (currently 1)
// 1-8: seconds
// 9-12: nanoseconds
void time_to_blob(Time t, uint8_t* buf) {
    const uint8_t version = 1;
    // Unsigned shifts: well-defined for negative values of sec.
    uint64_t sec = t.sec;
    uint32_t nsec = t.nsec;
    buf[0] = version;
    buf[1] = sec >> 56;  // bytes 1-8: seconds
    buf[2] = sec >> 48;
    buf[3] = sec >> 40;
    buf[4] = sec >> 32;
    buf[5] = sec >> 24;
    buf[6] = sec >> 16;
    buf[7] = sec >> 8;
    buf[8] = sec;
    buf[9] = nsec >> 24;  // bytes 9-12: nanoseconds
    buf[10] = nsec >> 16;
    buf[11] = nsec >> 8;
    buf[12] = nsec;
}

#pragma endregion
