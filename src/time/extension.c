// Copyright (c) 2024 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// SQLite extension for working with time.

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT3

#include "time/timex.h"

#pragma region Helpers

// result_blob converts a Time value to a blob and sets it as the result.
static void result_blob(sqlite3_context* context, Time t) {
    uint8_t buf[TIMEX_BLOB_SIZE];
    time_to_blob(t, buf);
    sqlite3_result_blob(context, buf, sizeof(buf), SQLITE_TRANSIENT);
}

// result_unknown_field sets an "unknown field" error naming the field.
static void result_unknown_field(sqlite3_context* context, const char* field) {
    char* msg = sqlite3_mprintf("unknown field: %s", field);
    if (msg == NULL) {
        sqlite3_result_error_nomem(context);
        return;
    }
    sqlite3_result_error(context, msg, -1);
    sqlite3_free(msg);
}

// has_null reports whether any of the arguments is NULL.
static bool has_null(int argc, sqlite3_value** argv) {
    for (int i = 0; i < argc; i++) {
        if (sqlite3_value_type(argv[i]) == SQLITE_NULL) {
            return true;
        }
    }
    return false;
}

// arg_time decodes a time blob argument into out.
// Returns false if the value is not a valid time blob.
static bool arg_time(sqlite3_value* value, Time* out) {
    if (sqlite3_value_type(value) != SQLITE_BLOB) {
        return false;
    }
    const uint8_t* buf = sqlite3_value_blob(value);
    if (sqlite3_value_bytes(value) != TIMEX_BLOB_SIZE) {
        return false;
    }
    return time_blob(buf, out);
}

// arg_int64 decodes an integer argument into out, converting numeric text.
// Returns false if the value is not an integer.
static bool arg_int64(sqlite3_value* value, int64_t* out) {
    if (sqlite3_value_numeric_type(value) != SQLITE_INTEGER) {
        return false;
    }
    *out = sqlite3_value_int64(value);
    return true;
}

// arg_int decodes an integer argument into out, converting numeric text.
// Returns false if the value is not an integer or does not fit in an int.
static bool arg_int(sqlite3_value* value, int* out) {
    int64_t n;
    if (!arg_int64(value, &n) || n < INT_MIN || n > INT_MAX) {
        return false;
    }
    *out = (int)n;
    return true;
}

// arg_field decodes a field name argument. Field names are spec arguments,
// so an invalid one is an error, not NULL. Sets the error and returns NULL
// if the value is not a field name.
static const char* arg_field(sqlite3_context* context, sqlite3_value* value) {
    if (sqlite3_value_type(value) == SQLITE_BLOB) {
        sqlite3_result_error(context, "field name should be text", -1);
        return NULL;
    }
    const char* field = (const char*)sqlite3_value_text(value);
    if (field == NULL) {
        sqlite3_result_error_nomem(context);
        return NULL;
    }
    return field;
}

#pragma endregion

#pragma region Constructors

// time_now()
static void fn_now(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 0);
    Time t = time_now();
    result_blob(context, t);
}

// time_date(year, month, day[, hour, min, sec[, nsec[, offset_sec]]])
static void fn_date(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 3 || argc == 6 || argc == 7 || argc == 8);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }

    // year, month, day, hour, min, sec, nsec, offset_sec;
    // the optional ones default to 0.
    int parts[8] = {0};
    for (int i = 0; i < argc; i++) {
        if (!arg_int(argv[i], &parts[i])) {
            sqlite3_result_null(context);
            return;
        }
    }

    Time t =
        time_date(parts[0], parts[1], parts[2], parts[3], parts[4], parts[5], parts[6], parts[7]);
    result_blob(context, t);
}

#pragma endregion

#pragma region Time parts

// Adapters with the exact signature fn_extract calls the time part functions
// through: calling a function through an incompatible pointer type is UB.

static int get_month(Time t) {
    return time_get_month(t);
}

static int get_weekday(Time t) {
    return time_get_weekday(t);
}

static int get_isoyear(Time t) {
    int year, week;
    time_get_isoweek(t, &year, &week);
    return year;
}

static int get_isoweek(Time t) {
    int year, week;
    time_get_isoweek(t, &year, &week);
    return week;
}

// time_get_year(t)
// time_get_month(t)
// time_get_day(t)
// time_get_hour(t)
// time_get_minute(t)
// time_get_second(t)
// time_get_nano(t)
// time_get_weekday(t)
// time_get_yearday(t)
// time_get_isoyear(t)
// time_get_isoweek(t)
static void fn_extract(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    Time t;
    if (!arg_time(argv[0], &t)) {
        sqlite3_result_null(context);
        return;
    }
    int (*extract)(Time t) = (int (*)(Time t))sqlite3_user_data(context);
    sqlite3_result_int(context, extract(t));
}

// get_field returns a part of the t according to a given field
static void get_field(sqlite3_context* context, Time t, const char* field) {
    // millennium, century, decade
    if (strcmp(field, "millennium") == 0) {
        int millennium = time_get_year(t) / 1000;
        sqlite3_result_int(context, millennium);
        return;
    }
    if (strcmp(field, "century") == 0) {
        int century = time_get_year(t) / 100;
        sqlite3_result_int(context, century);
        return;
    }
    if (strncmp(field, "decade", 6) == 0) {
        int decade = time_get_year(t) / 10;
        sqlite3_result_int(context, decade);
        return;
    }

    // year, quarter, month, day
    if (strcmp(field, "year") == 0 || strcmp(field, "years") == 0) {
        sqlite3_result_int(context, time_get_year(t));
        return;
    }
    if (strncmp(field, "quarter", 7) == 0) {
        int quarter = (time_get_month(t) - 1) / 3 + 1;
        sqlite3_result_int(context, quarter);
        return;
    }
    if (strncmp(field, "month", 5) == 0) {
        sqlite3_result_int(context, time_get_month(t));
        return;
    }
    if (strcmp(field, "day") == 0 || strcmp(field, "days") == 0) {
        sqlite3_result_int(context, time_get_day(t));
        return;
    }

    // hour, minute, second
    if (strncmp(field, "hour", 4) == 0) {
        sqlite3_result_int(context, time_get_hour(t));
        return;
    }
    if (strncmp(field, "minute", 6) == 0) {
        sqlite3_result_int(context, time_get_minute(t));
        return;
    }
    if (strncmp(field, "second", 6) == 0) {
        // including fractional part
        double sec = time_get_second(t) + t.nsec / 1e9;
        sqlite3_result_double(context, sec);
        return;
    }

    // millisecond, microsecond, nanosecond
    if (strncmp(field, "milli", 5) == 0) {
        int msec = time_get_nano(t) / 1000000;
        sqlite3_result_int(context, msec);
        return;
    }
    if (strncmp(field, "micro", 5) == 0) {
        int usec = time_get_nano(t) / 1000;
        sqlite3_result_int(context, usec);
        return;
    }
    if (strncmp(field, "nano", 4) == 0) {
        sqlite3_result_int(context, time_get_nano(t));
        return;
    }

    // isoyear, isoweek, isodow, yearday, weekday
    if (strcmp(field, "isoyear") == 0) {
        sqlite3_result_int(context, get_isoyear(t));
        return;
    }
    if (strcmp(field, "isoweek") == 0 || strcmp(field, "week") == 0) {
        sqlite3_result_int(context, get_isoweek(t));
        return;
    }
    if (strcmp(field, "isodow") == 0) {
        int isodow = time_get_weekday(t) == 0 ? 7 : time_get_weekday(t);
        sqlite3_result_int(context, isodow);
        return;
    }
    if (strcmp(field, "yearday") == 0 || strcmp(field, "doy") == 0 ||
        strcmp(field, "dayofyear") == 0) {
        sqlite3_result_int(context, time_get_yearday(t));
        return;
    }
    if (strcmp(field, "weekday") == 0 || strcmp(field, "dow") == 0 ||
        strcmp(field, "dayofweek") == 0) {
        sqlite3_result_int(context, time_get_weekday(t));
        return;
    }

    // epoch
    if (strcmp(field, "epoch") == 0) {
        // including fractional part
        double epoch = time_to_unix(t) + t.nsec / 1e9;
        sqlite3_result_double(context, epoch);
        return;
    }

    result_unknown_field(context, field);
}

// get decodes the time and field arguments and returns the field value.
static void get(sqlite3_context* context, sqlite3_value* time_arg, sqlite3_value* field_arg) {
    const char* field = arg_field(context, field_arg);
    if (field == NULL) {
        return;
    }
    Time t;
    if (!arg_time(time_arg, &t)) {
        sqlite3_result_null(context);
        return;
    }
    get_field(context, t, field);
}

// time_get(t, field)
static void fn_get(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    get(context, argv[0], argv[1]);
}

// date_part(field, t)
// Postgres-compatible.
static void fn_date_part(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    get(context, argv[1], argv[0]);
}

#pragma endregion

#pragma region Unix time

// time_unix(sec[, nsec])
static void fn_unix(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1 || argc == 2);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    int64_t sec;
    if (!arg_int64(argv[0], &sec)) {
        sqlite3_result_null(context);
        return;
    }
    int64_t nsec = 0;
    if (argc == 2 && !arg_int64(argv[1], &nsec)) {
        sqlite3_result_null(context);
        return;
    }
    Time t = time_unix(sec, nsec);
    result_blob(context, t);
}

// time_milli(msec)
// time_micro(usec)
// time_nano(nsec)
static void fn_unix_n(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    int64_t n;
    if (!arg_int64(argv[0], &n)) {
        sqlite3_result_null(context);
        return;
    }
    Time (*convert)(int64_t n) = (Time(*)(int64_t))sqlite3_user_data(context);
    Time t = convert(n);
    result_blob(context, t);
}

// time_to_unix(t)
// time_to_milli(t)
// time_to_micro(t)
// time_to_nano(t)
static void fn_convert(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    Time t;
    if (!arg_time(argv[0], &t)) {
        sqlite3_result_null(context);
        return;
    }
    int64_t (*convert)(Time t) = (int64_t (*)(Time t))sqlite3_user_data(context);
    sqlite3_result_int64(context, convert(t));
}

#pragma endregion

#pragma region Comparison

// Adapters with the exact signature fn_compare calls the comparison functions
// through: calling a function through an incompatible pointer type is UB.

static int after(Time t, Time u) {
    return time_after(t, u);
}

static int before(Time t, Time u) {
    return time_before(t, u);
}

static int equal(Time t, Time u) {
    return time_equal(t, u);
}

// time_after(t, u)
// time_before(t, u)
// time_compare(t, u)
// time_equal(t, u)
static void fn_compare(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    Time t, u;
    if (!arg_time(argv[0], &t) || !arg_time(argv[1], &u)) {
        sqlite3_result_null(context);
        return;
    }
    int (*compare)(Time t, Time u) = (int (*)(Time, Time))sqlite3_user_data(context);
    sqlite3_result_int(context, compare(t, u));
}

#pragma endregion

#pragma region Arithmetic

// time_add(t, d)
static void fn_add(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    Time t;
    Duration d;
    if (!arg_time(argv[0], &t) || !arg_int64(argv[1], &d)) {
        sqlite3_result_null(context);
        return;
    }
    Time r = time_add(t, d);
    result_blob(context, r);
}

// time_sub(t, u)
static void fn_sub(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    Time t, u;
    if (!arg_time(argv[0], &t) || !arg_time(argv[1], &u)) {
        sqlite3_result_null(context);
        return;
    }
    Duration d = time_sub(t, u);
    sqlite3_result_int64(context, d);
}

// time_since(t)
// time_until(t)
static void fn_elapsed(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    Time t;
    if (!arg_time(argv[0], &t)) {
        sqlite3_result_null(context);
        return;
    }
    Duration (*elapsed)(Time t) = (Duration(*)(Time))sqlite3_user_data(context);
    sqlite3_result_int64(context, elapsed(t));
}

// time_add_date(t, years[, months[, days]])
static void fn_add_date(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2 || argc == 3 || argc == 4);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    Time t;
    if (!arg_time(argv[0], &t)) {
        sqlite3_result_null(context);
        return;
    }

    // years, months, days; the optional ones default to 0.
    int parts[3] = {0};
    for (int i = 1; i < argc; i++) {
        if (!arg_int(argv[i], &parts[i - 1])) {
            sqlite3_result_null(context);
            return;
        }
    }

    Time r = time_add_date(t, parts[0], parts[1], parts[2]);
    result_blob(context, r);
}

#pragma endregion

#pragma region Rounding

// trunc_field truncates t according to a given field
static void trunc_field(sqlite3_context* context, Time t, const char* field) {
    // millennium, century, decade
    if (strcmp(field, "millennium") == 0) {
        int year = time_get_year(t);
        int millennium = year / 1000 * 1000;
        Time r = time_date(millennium, January, 1, 0, 0, 0, 0, TIMEX_UTC);
        result_blob(context, r);
        return;
    }
    if (strcmp(field, "century") == 0) {
        int year = time_get_year(t);
        int century = year / 100 * 100;
        Time r = time_date(century, January, 1, 0, 0, 0, 0, TIMEX_UTC);
        result_blob(context, r);
        return;
    }
    if (strcmp(field, "decade") == 0) {
        int year = time_get_year(t);
        int decade = year / 10 * 10;
        Time r = time_date(decade, January, 1, 0, 0, 0, 0, TIMEX_UTC);
        result_blob(context, r);
        return;
    }

    // year, quarter, month, week, day
    if (strcmp(field, "year") == 0) {
        Time r = time_date(time_get_year(t), January, 1, 0, 0, 0, 0, TIMEX_UTC);
        result_blob(context, r);
        return;
    }
    if (strcmp(field, "quarter") == 0) {
        int quarter = (time_get_month(t) - 1) / 3;
        Time r = time_date(time_get_year(t), quarter * 3 + 1, 1, 0, 0, 0, 0, TIMEX_UTC);
        result_blob(context, r);
        return;
    }
    if (strcmp(field, "month") == 0) {
        Time r = time_date(time_get_year(t), time_get_month(t), 1, 0, 0, 0, 0, TIMEX_UTC);
        result_blob(context, r);
        return;
    }
    if (strcmp(field, "week") == 0) {
        // Truncate to the Monday of the current (ISO) week: Jan 1 of the ISO
        // year is not necessarily a Monday, so (week - 1) * 7 days from
        // Jan 1 does not land on week boundaries.
        int year, day;
        enum Month month;
        time_get_date(t, &year, &month, &day);
        enum Weekday weekday = time_get_weekday(t);
        int isodow = weekday == Sunday ? 7 : weekday;
        Time r = time_date(year, month, day - (isodow - 1), 0, 0, 0, 0, TIMEX_UTC);
        result_blob(context, r);
        return;
    }
    if (strcmp(field, "day") == 0) {
        Time r =
            time_date(time_get_year(t), time_get_month(t), time_get_day(t), 0, 0, 0, 0, TIMEX_UTC);
        result_blob(context, r);
        return;
    }

    // hour, minute, second, millisecond, microsecond
    if (strcmp(field, "hour") == 0) {
        Time r = time_truncate(t, Hour);
        result_blob(context, r);
        return;
    }
    if (strcmp(field, "minute") == 0) {
        Time r = time_truncate(t, Minute);
        result_blob(context, r);
        return;
    }
    if (strcmp(field, "second") == 0) {
        Time r = time_truncate(t, Second);
        result_blob(context, r);
        return;
    }
    if (strncmp(field, "milli", 5) == 0) {
        int64_t nsec = (t.nsec / 1000000) * 1000000;
        Time r = time_unix(time_to_unix(t), nsec);
        result_blob(context, r);
        return;
    }
    if (strncmp(field, "micro", 5) == 0) {
        int64_t nsec = (t.nsec / 1000) * 1000;
        Time r = time_unix(time_to_unix(t), nsec);
        result_blob(context, r);
        return;
    }

    result_unknown_field(context, field);
}

// time_trunc(t, field)
// time_trunc(t, d)
static void fn_trunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }

    // The second argument is either a duration or a field name.
    // Check for an integer first, so that numeric text is a duration.
    Duration d;
    bool is_dur = arg_int64(argv[1], &d);
    const char* field = NULL;
    if (!is_dur && sqlite3_value_type(argv[1]) == SQLITE_TEXT) {
        field = arg_field(context, argv[1]);
        if (field == NULL) {
            return;
        }
    }
    if (!is_dur && field == NULL) {
        // Neither: a real or a blob is a bad duration value.
        sqlite3_result_null(context);
        return;
    }

    Time t;
    if (!arg_time(argv[0], &t)) {
        sqlite3_result_null(context);
        return;
    }

    if (is_dur) {
        Time r = time_truncate(t, d);
        result_blob(context, r);
    } else {
        trunc_field(context, t, field);
    }
}

// date_trunc(field, t)
// Postgres-compatible.
static void fn_date_trunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* field = arg_field(context, argv[0]);
    if (field == NULL) {
        return;
    }
    Time t;
    if (!arg_time(argv[1], &t)) {
        sqlite3_result_null(context);
        return;
    }
    trunc_field(context, t, field);
}

// time_round(t, d)
static void fn_round(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    Time t;
    Duration d;
    if (!arg_time(argv[0], &t) || !arg_int64(argv[1], &d)) {
        sqlite3_result_null(context);
        return;
    }
    Time r = time_round(t, d);
    result_blob(context, r);
}

#pragma endregion

#pragma region Formatting

// time_fmt_iso(t[, offset_sec])
// time_fmt_datetime(t[, offset_sec])
// time_fmt_date(t[, offset_sec])
// time_fmt_time(t[, offset_sec])
static void fn_format(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1 || argc == 2);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    Time t;
    if (!arg_time(argv[0], &t)) {
        sqlite3_result_null(context);
        return;
    }
    int offset_sec = TIMEX_UTC;
    if (argc == 2 && !arg_int(argv[1], &offset_sec)) {
        sqlite3_result_null(context);
        return;
    }

    char buf[TIMEX_FMT_BUF_SIZE];
    size_t (*format)(char* buf, size_t size, Time t, int offset_sec) =
        (size_t (*)(char*, size_t, Time, int))sqlite3_user_data(context);
    format(buf, sizeof(buf), t, offset_sec);
    sqlite3_result_text(context, buf, -1, SQLITE_TRANSIENT);
}

// time_parse(s)
static void fn_parse(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    if (sqlite3_value_type(argv[0]) == SQLITE_BLOB) {
        sqlite3_result_null(context);
        return;
    }
    const char* val = (const char*)sqlite3_value_text(argv[0]);
    if (val == NULL) {
        sqlite3_result_error_nomem(context);
        return;
    }
    int nbytes = sqlite3_value_bytes(argv[0]);
    if (strlen(val) != (size_t)nbytes) {
        // time_parse accepts NUL-terminated strings, so embedded NUL bytes are invalid.
        sqlite3_result_null(context);
        return;
    }
    Time t;
    if (!time_parse(val, &t)) {
        sqlite3_result_null(context);
        return;
    }
    result_blob(context, t);
}

#pragma endregion

#pragma region Duration constants

// dur_h(), dur_m(), dur_s(), dur_ms(), dur_us(), dur_ns()
static void fn_dur_const(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 0);
    // The user data points to the constant: an int64_t does not fit into
    // a pointer on 32-bit platforms.
    const Duration* d = sqlite3_user_data(context);
    sqlite3_result_int64(context, *d);
}

#pragma endregion

#define FLAGS (SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC)
#define FLAGS_ND (SQLITE_UTF8 | SQLITE_INNOCUOUS)

// Function describes an SQL function to register.
typedef struct {
    const char* name;
    int nargs;
    int flags;
    void* user_data;
    void (*fn)(sqlite3_context* context, int argc, sqlite3_value** argv);
} Function;

static const Function functions[] = {
    // constructors
    {"time_now", 0, FLAGS_ND, 0, fn_now},
    {"time_date", 3, FLAGS, 0, fn_date},
    {"time_date", 6, FLAGS, 0, fn_date},
    {"time_date", 7, FLAGS, 0, fn_date},
    {"time_date", 8, FLAGS, 0, fn_date},

    // time parts
    {"time_get_year", 1, FLAGS, time_get_year, fn_extract},
    {"time_get_month", 1, FLAGS, get_month, fn_extract},
    {"time_get_day", 1, FLAGS, time_get_day, fn_extract},
    {"time_get_hour", 1, FLAGS, time_get_hour, fn_extract},
    {"time_get_minute", 1, FLAGS, time_get_minute, fn_extract},
    {"time_get_second", 1, FLAGS, time_get_second, fn_extract},
    {"time_get_nano", 1, FLAGS, time_get_nano, fn_extract},
    {"time_get_weekday", 1, FLAGS, get_weekday, fn_extract},
    {"time_get_yearday", 1, FLAGS, time_get_yearday, fn_extract},
    {"time_get_isoyear", 1, FLAGS, get_isoyear, fn_extract},
    {"time_get_isoweek", 1, FLAGS, get_isoweek, fn_extract},
    {"time_get", 2, FLAGS, 0, fn_get},

    // unix time
    {"time_unix", 1, FLAGS, 0, fn_unix},
    {"time_unix", 2, FLAGS, 0, fn_unix},
    {"time_milli", 1, FLAGS, time_milli, fn_unix_n},
    {"time_micro", 1, FLAGS, time_micro, fn_unix_n},
    {"time_nano", 1, FLAGS, time_nano, fn_unix_n},
    {"time_to_unix", 1, FLAGS, time_to_unix, fn_convert},
    {"time_to_milli", 1, FLAGS, time_to_milli, fn_convert},
    {"time_to_micro", 1, FLAGS, time_to_micro, fn_convert},
    {"time_to_nano", 1, FLAGS, time_to_nano, fn_convert},

    // comparison
    {"time_after", 2, FLAGS, after, fn_compare},
    {"time_before", 2, FLAGS, before, fn_compare},
    {"time_compare", 2, FLAGS, time_compare, fn_compare},
    {"time_equal", 2, FLAGS, equal, fn_compare},

    // arithmetic
    {"time_add", 2, FLAGS, 0, fn_add},
    {"time_sub", 2, FLAGS, 0, fn_sub},
    {"time_since", 1, FLAGS_ND, time_since, fn_elapsed},
    {"time_until", 1, FLAGS_ND, time_until, fn_elapsed},
    {"time_add_date", 2, FLAGS, 0, fn_add_date},
    {"time_add_date", 3, FLAGS, 0, fn_add_date},
    {"time_add_date", 4, FLAGS, 0, fn_add_date},

    // rounding
    {"time_trunc", 2, FLAGS, 0, fn_trunc},
    {"time_round", 2, FLAGS, 0, fn_round},

    // formatting
    {"time_fmt_iso", 1, FLAGS, time_fmt_iso, fn_format},
    {"time_fmt_iso", 2, FLAGS, time_fmt_iso, fn_format},
    {"time_fmt_datetime", 1, FLAGS, time_fmt_datetime, fn_format},
    {"time_fmt_datetime", 2, FLAGS, time_fmt_datetime, fn_format},
    {"time_fmt_date", 1, FLAGS, time_fmt_date, fn_format},
    {"time_fmt_date", 2, FLAGS, time_fmt_date, fn_format},
    {"time_fmt_time", 1, FLAGS, time_fmt_time, fn_format},
    {"time_fmt_time", 2, FLAGS, time_fmt_time, fn_format},
    {"time_parse", 1, FLAGS, 0, fn_parse},

    // duration constants
    {"dur_h", 0, FLAGS, (void*)&Hour, fn_dur_const},
    {"dur_m", 0, FLAGS, (void*)&Minute, fn_dur_const},
    {"dur_s", 0, FLAGS, (void*)&Second, fn_dur_const},
    {"dur_ms", 0, FLAGS, (void*)&Millisecond, fn_dur_const},
    {"dur_us", 0, FLAGS, (void*)&Microsecond, fn_dur_const},
    {"dur_ns", 0, FLAGS, (void*)&Nanosecond, fn_dur_const},

    // postgres compatibility layer
    {"age", 2, FLAGS, 0, fn_sub},
    {"date_add", 2, FLAGS, 0, fn_add},
    {"date_part", 2, FLAGS, 0, fn_date_part},
    {"date_trunc", 2, FLAGS, 0, fn_date_trunc},
    {"make_date", 3, FLAGS, 0, fn_date},
    {"make_timestamp", 6, FLAGS, 0, fn_date},
    {"now", 0, FLAGS_ND, 0, fn_now},
    {"to_timestamp", 1, FLAGS, 0, fn_unix},
};

int time_init(sqlite3* db) {
    for (size_t i = 0; i < sizeof(functions) / sizeof(functions[0]); i++) {
        const Function* f = &functions[i];
        int rc =
            sqlite3_create_function(db, f->name, f->nargs, f->flags, f->user_data, f->fn, 0, 0);
        if (rc != SQLITE_OK) {
            return rc;
        }
    }
    return SQLITE_OK;
}
