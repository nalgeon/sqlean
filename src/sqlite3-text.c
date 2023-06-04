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

#pragma region Substrings

// Extracts a substring starting at the `start` position (1-based).
// text_substring(str, start)
// [pg-compatible] substr(string, start)
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

    // convert to 0-based index
    // postgres-compatible: treat negative index as zero
    start = start > 0 ? start - 1 : 0;

    RuneString s_src = rstring.from_cstring(src);
    RuneString s_res = rstring.slice(s_src, start, s_src.length);
    char* res = rstring.to_cstring(s_res);
    sqlite3_result_text(context, res, -1, free);
    rstring.free(s_src);
    rstring.free(s_res);
}

// Extracts a substring of `length` characters starting at the `start` position (1-based).
// text_substring(str, start, length)
// [pg-compatible] substr(string, start, count)
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
    if (length < 0) {
        sqlite3_result_error(context, "length parameter should >= 0", -1);
        return;
    }

    // convert to 0-based index
    start -= 1;
    // postgres-compatible: treat negative start as 0, but shorten the length accordingly
    if (start < 0) {
        length += start;
        start = 0;
    }

    // zero-length substring
    if (length <= 0) {
        sqlite3_result_text(context, "", -1, SQLITE_TRANSIENT);
        return;
    }

    RuneString s_src = rstring.from_cstring(src);

    // postgres-compatible: the substring cannot be longer the the original string
    if (length > s_src.length) {
        length = s_src.length;
    }

    RuneString s_res = rstring.substring(s_src, start, length);
    char* res = rstring.to_cstring(s_res);
    sqlite3_result_text(context, res, -1, free);
    rstring.free(s_src);
    rstring.free(s_res);
}

// Extracts a substring starting at the `start` position (1-based).
// text_slice(str, start)
static void sqlite3_slice2(sqlite3_context* context, int argc, sqlite3_value** argv) {
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

    // convert to 0-based index
    start = start > 0 ? start - 1 : start;

    RuneString s_src = rstring.from_cstring(src);

    // python-compatible: treat negative index larger than the length of the string as zero
    // and return the original string
    if (start < -(int)s_src.length) {
        sqlite3_result_text(context, src, -1, SQLITE_TRANSIENT);
        rstring.free(s_src);
        return;
    }

    RuneString s_res = rstring.slice(s_src, start, s_src.length);
    char* res = rstring.to_cstring(s_res);
    sqlite3_result_text(context, res, -1, free);
    rstring.free(s_src);
    rstring.free(s_res);
}

// Extracts a substring from `start` position inclusive to `end` position non-inclusive (1-based).
// text_slice(str, start, end)
static void sqlite3_slice3(sqlite3_context* context, int argc, sqlite3_value** argv) {
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
    // convert to 0-based index
    start = start > 0 ? start - 1 : start;

    if (sqlite3_value_type(argv[2]) != SQLITE_INTEGER) {
        sqlite3_result_error(context, "end parameter should be integer", -1);
        return;
    }
    int end = sqlite3_value_int(argv[2]);
    // convert to 0-based index
    end = end > 0 ? end - 1 : end;

    RuneString s_src = rstring.from_cstring(src);
    RuneString s_res = rstring.slice(s_src, start, end);
    char* res = rstring.to_cstring(s_res);
    sqlite3_result_text(context, res, -1, free);
    rstring.free(s_src);
    rstring.free(s_res);
}

// Extracts a substring of `length` characters from the beginning of the string.
// For `length < 0`, extracts all but the last `|length|` characters.
// text_left(str, length)
// [pg-compatible] left(string, n)
static void sqlite3_left(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    if (sqlite3_value_type(argv[1]) != SQLITE_INTEGER) {
        sqlite3_result_error(context, "length parameter should be integer", -1);
        return;
    }
    int length = sqlite3_value_int(argv[1]);

    RuneString s_src = rstring.from_cstring(src);
    if (length < 0) {
        length = s_src.length + length;
        length = length >= 0 ? length : 0;
    }
    RuneString s_res = rstring.substring(s_src, 0, length);
    char* res = rstring.to_cstring(s_res);
    sqlite3_result_text(context, res, -1, free);
    rstring.free(s_src);
    rstring.free(s_res);
}

// Extracts a substring of `length` characters from the end of the string.
// For `length < 0`, extracts all but the first `|length|` characters.
// text_right(str, length)
// [pg-compatible] right(string, n)
static void sqlite3_right(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    if (sqlite3_value_type(argv[1]) != SQLITE_INTEGER) {
        sqlite3_result_error(context, "length parameter should be integer", -1);
        return;
    }
    int length = sqlite3_value_int(argv[1]);

    RuneString s_src = rstring.from_cstring(src);

    length = (length < 0) ? (int)s_src.length + length : length;
    int start = (int)s_src.length - length;
    start = start < 0 ? 0 : start;

    RuneString s_res = rstring.substring(s_src, start, length);
    char* res = rstring.to_cstring(s_res);
    sqlite3_result_text(context, res, -1, free);
    rstring.free(s_src);
    rstring.free(s_res);
}

#pragma endregion

#pragma region Search and match

// Returns the first index of the substring in the original string.
// text_index(str, other)
// [pg-compatible] strpos(string, substring)
static void sqlite3_index(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    const char* other = (char*)sqlite3_value_text(argv[1]);
    if (other == NULL) {
        sqlite3_result_null(context);
        return;
    }

    RuneString s_src = rstring.from_cstring(src);
    RuneString s_other = rstring.from_cstring(other);
    int idx = rstring.index(s_src, s_other);
    sqlite3_result_int64(context, idx + 1);
    rstring.free(s_src);
    rstring.free(s_other);
}

// Returns the last index of the substring in the original string.
// text_last_index(str, other)
static void sqlite3_last_index(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    const char* other = (char*)sqlite3_value_text(argv[1]);
    if (other == NULL) {
        sqlite3_result_null(context);
        return;
    }

    RuneString s_src = rstring.from_cstring(src);
    RuneString s_other = rstring.from_cstring(other);
    int idx = rstring.last_index(s_src, s_other);
    sqlite3_result_int64(context, idx + 1);
    rstring.free(s_src);
    rstring.free(s_other);
}

// Checks if the string contains the substring.
// text_contains(str, other)
static void sqlite3_contains(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    const char* other = (char*)sqlite3_value_text(argv[1]);
    if (other == NULL) {
        sqlite3_result_null(context);
        return;
    }

    ByteString s_src = bstring.from_cstring(src, sqlite3_value_bytes(argv[0]));
    ByteString s_other = bstring.from_cstring(other, sqlite3_value_bytes(argv[1]));
    bool found = bstring.contains(s_src, s_other);
    sqlite3_result_int(context, found);
    bstring.free(s_src);
    bstring.free(s_other);
}

// Checks if the string starts with the substring.
// text_has_prefix(str, other)
// [pg-compatible] starts_with(string, prefix)
static void sqlite3_has_prefix(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    const char* other = (char*)sqlite3_value_text(argv[1]);
    if (other == NULL) {
        sqlite3_result_null(context);
        return;
    }

    ByteString s_src = bstring.from_cstring(src, sqlite3_value_bytes(argv[0]));
    ByteString s_other = bstring.from_cstring(other, sqlite3_value_bytes(argv[1]));
    bool found = bstring.has_prefix(s_src, s_other);
    sqlite3_result_int(context, found);
    bstring.free(s_src);
    bstring.free(s_other);
}

// Checks if the string ends with the substring.
// text_has_suffix(str, other)
static void sqlite3_has_suffix(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    const char* other = (char*)sqlite3_value_text(argv[1]);
    if (other == NULL) {
        sqlite3_result_null(context);
        return;
    }

    ByteString s_src = bstring.from_cstring(src, sqlite3_value_bytes(argv[0]));
    ByteString s_other = bstring.from_cstring(other, sqlite3_value_bytes(argv[1]));
    bool found = bstring.has_suffix(s_src, s_other);
    sqlite3_result_int(context, found);
    bstring.free(s_src);
    bstring.free(s_other);
}

// Counts how many times the substring is contained in the original string.
// text_count(str, other)
static void sqlite3_count(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    const char* src = (char*)sqlite3_value_text(argv[0]);
    if (src == NULL) {
        sqlite3_result_null(context);
        return;
    }

    const char* other = (char*)sqlite3_value_text(argv[1]);
    if (other == NULL) {
        sqlite3_result_null(context);
        return;
    }

    ByteString s_src = bstring.from_cstring(src, sqlite3_value_bytes(argv[0]));
    ByteString s_other = bstring.from_cstring(other, sqlite3_value_bytes(argv[1]));
    size_t count = bstring.count(s_src, s_other);
    sqlite3_result_int(context, count);
    bstring.free(s_src);
    bstring.free(s_other);
}

#pragma endregion

#pragma region Split and join

// Splits a string by a separator and returns the n-th part (counting from one).
// When n is negative, returns the |n|'th-from-last part.
// text_split(str, sep, n)
// [pg-compatible] split_part(string, delimiter, n)
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
    // pg-compatible
    if (part == 0) {
        sqlite3_result_error(context, "part parameter should not be 0", -1);
        return;
    }
    // convert to 0-based index
    part = part > 0 ? part - 1 : part;

    ByteString s_src = bstring.from_cstring(src, strlen(src));
    ByteString s_sep = bstring.from_cstring(sep, strlen(sep));

    // count from the last part backwards
    if (part < 0) {
        int n_parts = bstring.count(s_src, s_sep) + 1;
        part = n_parts + part;
    }

    ByteString s_part = bstring.split_part(s_src, s_sep, part);
    sqlite3_result_text(context, s_part.bytes, -1, SQLITE_TRANSIENT);
    bstring.free(s_src);
    bstring.free(s_sep);
    bstring.free(s_part);
}

// Joins strings using the separator and returns the resulting string. Ignores nulls.
// text_join(sep, str, ...)
// [pg-compatible] concat_ws(sep, val1[, val2 [, ...]])
static void sqlite3_join(sqlite3_context* context, int argc, sqlite3_value** argv) {
    if (argc < 2) {
        sqlite3_result_error(context, "expected at least 2 parameters", -1);
        return;
    }

    // separator
    const char* sep = (char*)sqlite3_value_text(argv[0]);
    if (sep == NULL) {
        sqlite3_result_null(context);
        return;
    }
    ByteString s_sep = bstring.from_cstring(sep, sqlite3_value_bytes(argv[0]));

    // parts
    size_t n_parts = argc - 1;
    ByteString* s_parts = malloc(n_parts * sizeof(ByteString));
    if (s_parts == NULL) {
        sqlite3_result_null(context);
        return;
    }
    for (size_t i = 1, part_idx = 0; i < argc; i++) {
        if (sqlite3_value_type(argv[i]) == SQLITE_NULL) {
            // ignore nulls
            n_parts--;
            continue;
        }
        const char* part = (char*)sqlite3_value_text(argv[i]);
        int part_len = sqlite3_value_bytes(argv[i]);
        s_parts[part_idx] = bstring.from_cstring(part, part_len);
        part_idx++;
    }

    // join parts with separator
    ByteString s_res = bstring.join(s_parts, n_parts, s_sep);
    const char* res = bstring.to_cstring(s_res);
    sqlite3_result_text(context, res, -1, SQLITE_TRANSIENT);
    bstring.free(s_sep);
    bstring.free(s_res);
    free(s_parts);
}

#pragma endregion

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
// utf8 text_slice(str, start [,end])
// utf8 text_substring(str, start [,length])
// utf8 text_left(str, length)
// utf8 text_right(str, length)

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
// utf8 text_lpad(str, length [,fill])
// utf8 text_rpad(str, length [,fill])

// length and size
// utf8 text_length(str)
//      text_size(str)

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_text_init(sqlite3* db, char** errmsg_ptr, const sqlite3_api_routines* api) {
    (void)errmsg_ptr;
    SQLITE_EXTENSION_INIT2(api);
    static const int flags = SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC;

    // substrings
    sqlite3_create_function(db, "text_substring", 2, flags, 0, sqlite3_substring2, 0, 0);
    sqlite3_create_function(db, "text_substring", 3, flags, 0, sqlite3_substring3, 0, 0);
    sqlite3_create_function(db, "text_slice", 2, flags, 0, sqlite3_slice2, 0, 0);
    sqlite3_create_function(db, "text_slice", 3, flags, 0, sqlite3_slice3, 0, 0);
    sqlite3_create_function(db, "text_left", 2, flags, 0, sqlite3_left, 0, 0);
    sqlite3_create_function(db, "left", 2, flags, 0, sqlite3_left, 0, 0);
    sqlite3_create_function(db, "text_right", 2, flags, 0, sqlite3_right, 0, 0);
    sqlite3_create_function(db, "right", 2, flags, 0, sqlite3_right, 0, 0);

    // search and match
    sqlite3_create_function(db, "text_index", 2, flags, 0, sqlite3_index, 0, 0);
    sqlite3_create_function(db, "strpos", 2, flags, 0, sqlite3_index, 0, 0);
    sqlite3_create_function(db, "text_last_index", 2, flags, 0, sqlite3_last_index, 0, 0);
    sqlite3_create_function(db, "text_contains", 2, flags, 0, sqlite3_contains, 0, 0);
    sqlite3_create_function(db, "text_has_prefix", 2, flags, 0, sqlite3_has_prefix, 0, 0);
    sqlite3_create_function(db, "starts_with", 2, flags, 0, sqlite3_has_prefix, 0, 0);
    sqlite3_create_function(db, "text_has_suffix", 2, flags, 0, sqlite3_has_suffix, 0, 0);
    sqlite3_create_function(db, "text_count", 2, flags, 0, sqlite3_count, 0, 0);

    // split and join
    sqlite3_create_function(db, "text_split", 3, flags, 0, sqlite3_split, 0, 0);
    sqlite3_create_function(db, "split_part", 3, flags, 0, sqlite3_split, 0, 0);
    sqlite3_create_function(db, "text_join", -1, flags, 0, sqlite3_join, 0, 0);
    sqlite3_create_function(db, "concat_ws", -1, flags, 0, sqlite3_join, 0, 0);

    sqlite3_create_function(db, "text_reverse", 1, flags, 0, sqlite3_reverse, 0, 0);
    sqlite3_create_function(db, "reverse", 1, flags, 0, sqlite3_reverse, 0, 0);
    return SQLITE_OK;
}
