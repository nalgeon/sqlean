// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// SQLite extension for working with text.

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT3

#include "text/extension.h"
#include "text/text.h"

#pragma region Helpers

// has_null reports whether any of the arguments is NULL.
static bool has_null(int argc, sqlite3_value** argv) {
    for (int i = 0; i < argc; i++) {
        if (sqlite3_value_type(argv[i]) == SQLITE_NULL) {
            return true;
        }
    }
    return false;
}

// arg_text decodes a text argument into s and n. Numbers are converted
// to text. Sets the NULL result for a blob, and the error if out of memory.
// Returns false on failure.
static bool arg_text(sqlite3_context* context, sqlite3_value* value, const char** s, size_t* n) {
    if (sqlite3_value_type(value) == SQLITE_BLOB) {
        sqlite3_result_null(context);
        return false;
    }
    *s = (const char*)sqlite3_value_text(value);
    if (*s == NULL) {
        sqlite3_result_error_nomem(context);
        return false;
    }
    *n = (size_t)sqlite3_value_bytes(value);
    return true;
}

// arg_texts decodes the count text arguments starting at argv into s and n,
// as arg_text does. Returns false on failure.
static bool arg_texts(sqlite3_context* context,
                      int count,
                      sqlite3_value** argv,
                      const char** s,
                      size_t* n) {
    for (int i = 0; i < count; i++) {
        if (!arg_text(context, argv[i], &s[i], &n[i])) {
            return false;
        }
    }
    return true;
}

// arg_int64 decodes an integer argument into out, converting numeric text.
// Sets the NULL result and returns false if the value is not an integer.
static bool arg_int64(sqlite3_context* context, sqlite3_value* value, int64_t* out) {
    if (sqlite3_value_numeric_type(value) != SQLITE_INTEGER) {
        sqlite3_result_null(context);
        return false;
    }
    *out = sqlite3_value_int64(value);
    return true;
}

// alloc_result allocates a buffer for a text result of size bytes.
// Sets the error and returns NULL if the size exceeds SQLITE_LIMIT_LENGTH
// or out of memory.
static char* alloc_result(sqlite3_context* context, size_t size) {
    sqlite3_int64 limit =
        sqlite3_limit(sqlite3_context_db_handle(context), SQLITE_LIMIT_LENGTH, -1);
    if ((sqlite3_uint64)size > (sqlite3_uint64)limit) {
        sqlite3_result_error_toobig(context);
        return NULL;
    }
    // sqlite3_malloc64(0) returns NULL, so allocate at least one byte.
    char* buf = sqlite3_malloc64(size > 0 ? size : 1);
    if (buf == NULL) {
        sqlite3_result_error_nomem(context);
    }
    return buf;
}

// result_text sets the size bytes of buf, allocated by alloc_result,
// as the result, and takes ownership of buf.
static void result_text(sqlite3_context* context, char* buf, size_t size) {
    sqlite3_result_text64(context, buf, size, sqlite3_free, SQLITE_UTF8);
}

// result_span sets the part of s as the result.
static void result_span(sqlite3_context* context, const char* s, TextSpan span) {
    sqlite3_result_text64(context, s + span.start, span.len, SQLITE_TRANSIENT, SQLITE_UTF8);
}

#pragma endregion

#pragma region Substrings

// text_substring(str, start[, length])
static void fn_substring(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2 || argc == 3);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* s;
    size_t n;
    int64_t start;
    int64_t length = INT64_MAX;
    if (!arg_text(context, argv[0], &s, &n) || !arg_int64(context, argv[1], &start) ||
        (argc == 3 && !arg_int64(context, argv[2], &length))) {
        return;
    }
    result_span(context, s, text_substring(s, n, start, length));
}

// text_slice(str, start[, end])
static void fn_slice(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2 || argc == 3);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* s;
    size_t n;
    int64_t start;
    int64_t end = INT64_MAX;
    if (!arg_text(context, argv[0], &s, &n) || !arg_int64(context, argv[1], &start) ||
        (argc == 3 && !arg_int64(context, argv[2], &end))) {
        return;
    }
    result_span(context, s, text_slice(s, n, start, end));
}

// Part is the signature of the functions fn_part calls.
typedef TextSpan (*Part)(const char* s, size_t n, int64_t count);

// text_left(str, length)
// text_right(str, length)
static void fn_part(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* s;
    size_t n;
    int64_t count;
    if (!arg_text(context, argv[0], &s, &n) || !arg_int64(context, argv[1], &count)) {
        return;
    }
    Part part = (Part)sqlite3_user_data(context);
    result_span(context, s, part(s, n, count));
}

#pragma endregion

#pragma region Search

// Locate is the signature of the functions fn_locate calls.
typedef int64_t (*Locate)(const char* s, size_t n, const char* sub, size_t nsub);

// text_index(str, other)
// text_last_index(str, other)
// text_count(str, other)
static void fn_locate(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* s[2];
    size_t n[2];
    if (!arg_texts(context, argc, argv, s, n)) {
        return;
    }
    Locate locate = (Locate)sqlite3_user_data(context);
    sqlite3_result_int64(context, locate(s[0], n[0], s[1], n[1]));
}

// Match is the signature of the functions fn_match calls.
typedef bool (*Match)(const char* s, size_t n, const char* sub, size_t nsub);

// text_contains(str, other)
// text_has_prefix(str, other)
// text_has_suffix(str, other)
// text_like(pattern, str)
static void fn_match(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* s[2];
    size_t n[2];
    if (!arg_texts(context, argc, argv, s, n)) {
        return;
    }
    Match match = (Match)sqlite3_user_data(context);
    sqlite3_result_int(context, match(s[0], n[0], s[1], n[1]));
}

#pragma endregion

#pragma region Split and join

// text_split(str, sep, part)
static void fn_split(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 3);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* s[2];
    size_t n[2];
    int64_t part;
    if (!arg_texts(context, 2, argv, s, n) || !arg_int64(context, argv[2], &part)) {
        return;
    }
    TextSpan span;
    if (!text_split(s[0], n[0], s[1], n[1], part, &span)) {
        sqlite3_result_null(context);
        return;
    }
    result_span(context, s[0], span);
}

// result_join sets the result of joining the non-NULL values in argv with sep.
static void result_join(sqlite3_context* context,
                        const char* sep,
                        size_t nsep,
                        int argc,
                        sqlite3_value** argv) {
    const char** parts = sqlite3_malloc64((sqlite3_uint64)argc * sizeof(*parts));
    size_t* sizes = sqlite3_malloc64((sqlite3_uint64)argc * sizeof(*sizes));
    if (parts == NULL || sizes == NULL) {
        sqlite3_free(parts);
        sqlite3_free(sizes);
        sqlite3_result_error_nomem(context);
        return;
    }
    size_t count = 0;
    for (int i = 0; i < argc; i++) {
        if (sqlite3_value_type(argv[i]) == SQLITE_NULL) {
            continue;
        }
        if (!arg_text(context, argv[i], &parts[count], &sizes[count])) {
            goto done;
        }
        count++;
    }
    size_t size = text_join(sep, nsep, parts, sizes, count, NULL);
    char* buf = alloc_result(context, size);
    if (buf != NULL) {
        text_join(sep, nsep, parts, sizes, count, buf);
        result_text(context, buf, size);
    }
done:
    sqlite3_free(parts);
    sqlite3_free(sizes);
}

// text_join(sep, str, ...)
// Registered with any number of arguments, so it checks the count itself.
static void fn_join(sqlite3_context* context, int argc, sqlite3_value** argv) {
    if (argc < 2) {
        sqlite3_result_error(context, "wrong number of arguments to function text_join()", -1);
        return;
    }
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        sqlite3_result_null(context);
        return;
    }
    const char* sep;
    size_t nsep;
    if (!arg_text(context, argv[0], &sep, &nsep)) {
        return;
    }
    result_join(context, sep, nsep, argc - 1, argv + 1);
}

// text_concat(str, ...)
// Registered with any number of arguments, so it checks the count itself.
static void fn_concat(sqlite3_context* context, int argc, sqlite3_value** argv) {
    if (argc < 1) {
        sqlite3_result_error(context, "wrong number of arguments to function text_concat()", -1);
        return;
    }
    result_join(context, "", 0, argc, argv);
}

// text_repeat(str, count)
static void fn_repeat(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* s;
    size_t n;
    int64_t count;
    if (!arg_text(context, argv[0], &s, &n) || !arg_int64(context, argv[1], &count)) {
        return;
    }
    size_t size = text_repeat(s, n, count, NULL);
    char* buf = alloc_result(context, size);
    if (buf == NULL) {
        return;
    }
    text_repeat(s, n, count, buf);
    result_text(context, buf, size);
}

#pragma endregion

#pragma region Trim and pad

// Trim is the signature of the functions fn_trim calls.
typedef TextSpan (*Trim)(const char* s, size_t n, const char* chars, size_t nchars);

// text_ltrim(str[, chars])
// text_rtrim(str[, chars])
// text_trim(str[, chars])
static void fn_trim(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1 || argc == 2);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* s[2] = {NULL, " "};
    size_t n[2] = {0, 1};
    if (!arg_texts(context, argc, argv, s, n)) {
        return;
    }
    Trim trim = (Trim)sqlite3_user_data(context);
    result_span(context, s[0], trim(s[0], n[0], s[1], n[1]));
}

// Pad is the signature of the functions fn_pad calls.
typedef size_t (
    *Pad)(const char* s, size_t n, int64_t length, const char* fill, size_t nfill, char* out);

// text_lpad(str, length[, fill])
// text_rpad(str, length[, fill])
static void fn_pad(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2 || argc == 3);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* s;
    size_t n;
    int64_t length;
    const char* fill = " ";
    size_t nfill = 1;
    if (!arg_text(context, argv[0], &s, &n) || !arg_int64(context, argv[1], &length) ||
        (argc == 3 && !arg_text(context, argv[2], &fill, &nfill))) {
        return;
    }
    Pad pad = (Pad)sqlite3_user_data(context);
    size_t size = pad(s, n, length, fill, nfill, NULL);
    char* buf = alloc_result(context, size);
    if (buf == NULL) {
        return;
    }
    pad(s, n, length, fill, nfill, buf);
    result_text(context, buf, size);
}

#pragma endregion

#pragma region Case

// ChangeCase is the signature of the functions fn_case calls.
typedef size_t (*ChangeCase)(const char* s, size_t n, char* out);

// text_upper(str)
// text_lower(str)
// text_title(str)
// text_casefold(str)
static void fn_case(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* s;
    size_t n;
    if (!arg_text(context, argv[0], &s, &n)) {
        return;
    }
    ChangeCase change = (ChangeCase)sqlite3_user_data(context);
    size_t size = change(s, n, NULL);
    char* buf = alloc_result(context, size);
    if (buf == NULL) {
        return;
    }
    change(s, n, buf);
    result_text(context, buf, size);
}

#pragma endregion

#pragma region Modify

// text_replace(str, old, new[, count])
static void fn_replace(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 3 || argc == 4);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* s[3];
    size_t n[3];
    int64_t count = INT64_MAX;
    if (!arg_texts(context, 3, argv, s, n) || (argc == 4 && !arg_int64(context, argv[3], &count))) {
        return;
    }
    size_t size = text_replace(s[0], n[0], s[1], n[1], s[2], n[2], count, NULL);
    char* buf = alloc_result(context, size);
    if (buf == NULL) {
        return;
    }
    text_replace(s[0], n[0], s[1], n[1], s[2], n[2], count, buf);
    result_text(context, buf, size);
}

// text_translate(str, from, to)
static void fn_translate(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 3);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* s[3];
    size_t n[3];
    if (!arg_texts(context, argc, argv, s, n)) {
        return;
    }
    size_t size = text_translate(s[0], n[0], s[1], n[1], s[2], n[2], NULL);
    char* buf = alloc_result(context, size);
    if (buf == NULL) {
        return;
    }
    text_translate(s[0], n[0], s[1], n[1], s[2], n[2], buf);
    result_text(context, buf, size);
}

// text_reverse(str)
static void fn_reverse(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* s;
    size_t n;
    if (!arg_text(context, argv[0], &s, &n)) {
        return;
    }
    char* buf = alloc_result(context, n);
    if (buf == NULL) {
        return;
    }
    text_reverse(s, n, buf);
    result_text(context, buf, n);
}

#pragma endregion

#pragma region Properties

// The unit fn_size counts in, as the user data.
static const int64_t BYTES = 1;
static const int64_t BITS = 8;

// text_length(str)
static void fn_length(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* s;
    size_t n;
    if (!arg_text(context, argv[0], &s, &n)) {
        return;
    }
    sqlite3_result_int64(context, text_length(s, n));
}

// text_size(str)
// text_bitsize(str)
static void fn_size(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* s;
    size_t n;
    if (!arg_text(context, argv[0], &s, &n)) {
        return;
    }
    // sqlite3_value_bytes is an int, so the product fits.
    const int64_t unit = *(const int64_t*)sqlite3_user_data(context);
    sqlite3_result_int64(context, (int64_t)n * unit);
}

#pragma endregion

#pragma region Collation

// text_nocase
static int collate_nocase(void* unused, int n1, const void* s1, int n2, const void* s2) {
    (void)unused;
    return text_casecmp(s1, (size_t)n1, s2, (size_t)n2);
}

#pragma endregion

#define FLAGS (SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC)

// Function describes an SQL function to register.
typedef struct {
    const char* name;
    int nargs;
    int flags;
    void* user_data;
    void (*fn)(sqlite3_context* context, int argc, sqlite3_value** argv);
} Function;

static const Function functions[] = {
    // substrings
    {"text_substring", 2, FLAGS, 0, fn_substring},
    {"text_substring", 3, FLAGS, 0, fn_substring},
    {"text_slice", 2, FLAGS, 0, fn_slice},
    {"text_slice", 3, FLAGS, 0, fn_slice},
    {"text_left", 2, FLAGS, (void*)text_left, fn_part},
    {"text_right", 2, FLAGS, (void*)text_right, fn_part},

    // search
    {"text_index", 2, FLAGS, (void*)text_index, fn_locate},
    {"text_last_index", 2, FLAGS, (void*)text_last_index, fn_locate},
    {"text_contains", 2, FLAGS, (void*)text_contains, fn_match},
    {"text_has_prefix", 2, FLAGS, (void*)text_has_prefix, fn_match},
    {"text_has_suffix", 2, FLAGS, (void*)text_has_suffix, fn_match},
    {"text_count", 2, FLAGS, (void*)text_count, fn_locate},
    {"text_like", 2, FLAGS, (void*)text_like, fn_match},

    // split and join
    {"text_split", 3, FLAGS, 0, fn_split},
    // Variadic: the only functions registered with -1 arguments.
    {"text_join", -1, FLAGS, 0, fn_join},
    {"text_concat", -1, FLAGS, 0, fn_concat},
    {"text_repeat", 2, FLAGS, 0, fn_repeat},

    // trim and pad
    {"text_ltrim", 1, FLAGS, (void*)text_ltrim, fn_trim},
    {"text_ltrim", 2, FLAGS, (void*)text_ltrim, fn_trim},
    {"text_rtrim", 1, FLAGS, (void*)text_rtrim, fn_trim},
    {"text_rtrim", 2, FLAGS, (void*)text_rtrim, fn_trim},
    {"text_trim", 1, FLAGS, (void*)text_trim, fn_trim},
    {"text_trim", 2, FLAGS, (void*)text_trim, fn_trim},
    {"text_lpad", 2, FLAGS, (void*)text_lpad, fn_pad},
    {"text_lpad", 3, FLAGS, (void*)text_lpad, fn_pad},
    {"text_rpad", 2, FLAGS, (void*)text_rpad, fn_pad},
    {"text_rpad", 3, FLAGS, (void*)text_rpad, fn_pad},

    // case
    {"text_upper", 1, FLAGS, (void*)text_upper, fn_case},
    {"text_lower", 1, FLAGS, (void*)text_lower, fn_case},
    {"text_title", 1, FLAGS, (void*)text_title, fn_case},
    {"text_casefold", 1, FLAGS, (void*)text_casefold, fn_case},

    // modify
    {"text_replace", 3, FLAGS, 0, fn_replace},
    {"text_replace", 4, FLAGS, 0, fn_replace},
    {"text_translate", 3, FLAGS, 0, fn_translate},
    {"text_reverse", 1, FLAGS, 0, fn_reverse},

    // properties
    {"text_length", 1, FLAGS, 0, fn_length},
    {"text_size", 1, FLAGS, (void*)&BYTES, fn_size},
    {"text_bitsize", 1, FLAGS, (void*)&BITS, fn_size},

    // Postgres aliases
    {"left", 2, FLAGS, (void*)text_left, fn_part},
    {"right", 2, FLAGS, (void*)text_right, fn_part},
    {"strpos", 2, FLAGS, (void*)text_index, fn_locate},
    {"starts_with", 2, FLAGS, (void*)text_has_prefix, fn_match},
    {"split_part", 3, FLAGS, 0, fn_split},
    {"repeat", 2, FLAGS, 0, fn_repeat},
    {"btrim", 1, FLAGS, (void*)text_trim, fn_trim},
    {"btrim", 2, FLAGS, (void*)text_trim, fn_trim},
    {"lpad", 2, FLAGS, (void*)text_lpad, fn_pad},
    {"lpad", 3, FLAGS, (void*)text_lpad, fn_pad},
    {"rpad", 2, FLAGS, (void*)text_rpad, fn_pad},
    {"rpad", 3, FLAGS, (void*)text_rpad, fn_pad},
    {"translate", 3, FLAGS, 0, fn_translate},
    {"reverse", 1, FLAGS, 0, fn_reverse},
    {"char_length", 1, FLAGS, 0, fn_length},
    {"character_length", 1, FLAGS, 0, fn_length},
    {"bit_length", 1, FLAGS, (void*)&BITS, fn_size},
};

// Builtin is a Postgres alias that SQLite has as a built-in function since
// the version. It is registered only for an older SQLite, so the extension
// never replaces a built-in.
typedef struct {
    Function f;
    int version;
} Builtin;

static const Builtin builtins[] = {
    {{"concat_ws", -1, FLAGS, 0, fn_join}, 3044000},
    {{"concat", -1, FLAGS, 0, fn_concat}, 3044000},
    {{"octet_length", 1, FLAGS, (void*)&BYTES, fn_size}, 3043000},
};

// create_function registers the function f.
static int create_function(sqlite3* db, const Function* f) {
    return sqlite3_create_function(db, f->name, f->nargs, f->flags, f->user_data, f->fn, 0, 0);
}

int text_init(sqlite3* db) {
    for (size_t i = 0; i < sizeof(functions) / sizeof(functions[0]); i++) {
        int rc = create_function(db, &functions[i]);
        if (rc != SQLITE_OK) {
            return rc;
        }
    }
    for (size_t i = 0; i < sizeof(builtins) / sizeof(builtins[0]); i++) {
        if (sqlite3_libversion_number() >= builtins[i].version) {
            continue;
        }
        int rc = create_function(db, &builtins[i].f);
        if (rc != SQLITE_OK) {
            return rc;
        }
    }
    return sqlite3_create_collation(db, "text_nocase", SQLITE_UTF8, NULL, collate_nocase);
}
