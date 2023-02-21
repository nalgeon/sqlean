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
    char* res = rstring.to_cstring(s_src);

    sqlite3_result_text(context, res, -1, free);
    rstring.free(s_src);
    rstring.free(s_res);
}

// substring
// utf8 text_at(str, idx)
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
    sqlite3_create_function(db, "text_reverse", 1, flags, 0, sqlite3_reverse, 0, 0);
    sqlite3_create_function(db, "reverse", 1, flags, 0, sqlite3_reverse, 0, 0);
    sqlite3_create_function(db, "text_split", 3, flags, 0, sqlite3_split, 0, 0);
    sqlite3_create_function(db, "split_part", 3, flags, 0, sqlite3_split, 0, 0);
    return SQLITE_OK;
}
