// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// SQLite extension for working with text.

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sqlite3ext.h"
#include "text/text.h"

SQLITE_EXTENSION_INIT1

// Extracts a substring starting at the `start` position (1-based).
// text_substring(str, start)
static void sqlite3_substring2(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    if (sqlite3_value_type(argv[1]) != SQLITE_INTEGER) {
        sqlite3_result_error(context, "start parameter should be integer", -1);
        return;
    }
    int start = sqlite3_value_int(argv[1]);

    if (start > 0) {
        start -= 1;
    }

    RuneString s_src = rstring.from_cstring(src);
    RuneString s_res = rstring.slice(s_src, start, s_src.length);
    char* res = rstring.to_cstring(s_res);
    sqlite3_result_text(context, res, -1, free);
    rstring.free(s_res);
}

// Extracts a substring of `length` characters starting at the `start` position (1-based).
// text_substring(str, start, length)
static void sqlite3_substring3(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 3);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    if (sqlite3_value_type(argv[1]) != SQLITE_INTEGER) {
        sqlite3_result_error(context, "start parameter should be integer", -1);
        return;
    }
    int start = sqlite3_value_int(argv[1]);

    if (sqlite3_value_type(argv[2]) != SQLITE_INTEGER) {
        sqlite3_result_error(context, "length parameter should be integer", -1);
        return;
    }
    int length = sqlite3_value_int(argv[2]);

    if (start > 0) {
        start -= 1;
    }

    int end = start + length;

    // slice() expects end to be greater than start,
    // so swap them if necessary
    if (end < start) {
        int tmp = start;
        start = end;
        end = tmp;
    }

    RuneString s_src = rstring.from_cstring(src);
    RuneString s_res = rstring.slice(s_src, start, end);
    char* res = rstring.to_cstring(s_res);
    sqlite3_result_text(context, res, -1, free);
    rstring.free(s_res);
}

// Extracts a substring from `start` position inclusive to `end` position non-inclusive (1-based).
// text_slice(str, start, end)
static void sqlite3_slice(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 3);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    if (sqlite3_value_type(argv[1]) != SQLITE_INTEGER) {
        sqlite3_result_error(context, "start parameter should be integer", -1);
        return;
    }
    int start = sqlite3_value_int(argv[1]);
    if (start > 0) {
        start -= 1;
    }

    if (sqlite3_value_type(argv[2]) != SQLITE_INTEGER) {
        sqlite3_result_error(context, "end parameter should be integer", -1);
        return;
    }
    int end = sqlite3_value_int(argv[2]);
    if (end > 0) {
        end -= 1;
    }

    RuneString s_src = rstring.from_cstring(src);
    RuneString s_res = rstring.slice(s_src, start, end);
    char* res = rstring.to_cstring(s_res);
    sqlite3_result_text(context, res, -1, free);
    rstring.free(s_res);
}

// Splits a string by a separator and returns the n-th part (counting from one).
// text_split(str, sep, n)
static void sqlite3_split(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 3);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    const char* sep = (const char*)sqlite3_value_text(argv[1]);
    if (sep == NULL) {
        sqlite3_result_null(context);
        return;
    }

    if (sqlite3_value_type(argv[2]) != SQLITE_INTEGER) {
        sqlite3_result_error(context, "part parameter should be integer", -1);
        return;
    }
    int part = sqlite3_value_int(argv[2]);
    if (part <= 0) {
        sqlite3_result_error(context, "part parameter should be > 0", -1);
        return;
    }

    ByteString s_src = bstring.from_cstring(src, strlen(src));
    ByteString s_sep = bstring.from_cstring(sep, strlen(sep));
    ByteString s_part = bstring.split_part(s_src, s_sep, part - 1);
    sqlite3_result_text(context, s_part.bytes, -1, SQLITE_TRANSIENT);
    bstring.free(s_part);
}

// Reverses a string.
// text_reverse(str)
static void sqlite3_reverse(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    RuneString s_src = rstring.from_cstring(src);
    RuneString s_res = rstring.reverse(s_src);
    char* res = rstring.to_cstring(s_res);

    sqlite3_result_text(context, res, -1, free);
    rstring.free(s_src);
    rstring.free(s_res);
}

// substring
// utf8 text_substring(str, start [,length])
// utf8 text_slice(str, start [,end])

// search and match
// utf8 text_index(str, other)
// utf8 text_last_index(str, other)
//      text_contains(str, other)
//      text_has_prefix(str, prefix)
//      text_has_suffix(str, suffix)
//      text_count(str, other)

// split and join
//      text_split(sep, str, ...)
//      text_join(sep, str, ...)
//      text_concat(str, ...)
//      text_repeat(str, n)

// replace
//      text_replace(str, old, new [, n])
// utf8 text_reverse(str)

// trim and pad
//      text_trim(str)
//      text_ltrim(str)
//      text_rtrim(str)
// !utf text_pad_left(str, length [,fill])
// !utf text_pad_right(str, length [,fill])

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_text_init(sqlite3* db, char** errmsg_ptr, const sqlite3_api_routines* api) {
    (void)errmsg_ptr;
    SQLITE_EXTENSION_INIT2(api);
    static const int flags = SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC;
    sqlite3_create_function(db, "text_substring", 2, flags, 0, sqlite3_substring2, 0, 0);
    sqlite3_create_function(db, "text_substring", 3, flags, 0, sqlite3_substring3, 0, 0);
    sqlite3_create_function(db, "text_slice", 2, flags, 0, sqlite3_substring2, 0, 0);
    sqlite3_create_function(db, "text_slice", 3, flags, 0, sqlite3_slice, 0, 0);
    sqlite3_create_function(db, "text_reverse", 1, flags, 0, sqlite3_reverse, 0, 0);
    sqlite3_create_function(db, "reverse", 1, flags, 0, sqlite3_reverse, 0, 0);
    sqlite3_create_function(db, "text_split", 3, flags, 0, sqlite3_split, 0, 0);
    sqlite3_create_function(db, "split_part", 3, flags, 0, sqlite3_split, 0, 0);
    return SQLITE_OK;
}
