// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// SQLite extension for working with regular expressions.

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT3

#include "regexp/extension.h"
#include "regexp/regexp.h"

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

// result_invalid_group sets an "invalid group" error naming the group.
static void result_invalid_group(sqlite3_context* context, sqlite3_value* value) {
    char* msg = sqlite3_mprintf("invalid group: %s", sqlite3_value_text(value));
    if (msg == NULL) {
        sqlite3_result_error_nomem(context);
        return;
    }
    sqlite3_result_error(context, msg, -1);
    sqlite3_free(msg);
}

// arg_group decodes a group number argument into out, converting numeric
// text. Sets the NULL result for a blob, and the error if the value is not
// a non-negative integer. Returns false on failure.
static bool arg_group(sqlite3_context* context, sqlite3_value* value, size_t* out) {
    if (sqlite3_value_type(value) == SQLITE_BLOB) {
        sqlite3_result_null(context);
        return false;
    }
    if (sqlite3_value_numeric_type(value) != SQLITE_INTEGER) {
        result_invalid_group(context, value);
        return false;
    }
    sqlite3_int64 n = sqlite3_value_int64(value);
    if (n < 0) {
        result_invalid_group(context, value);
        return false;
    }
    // A pattern has at most 65535 groups, so clamping to UINT32_MAX keeps
    // a number that doesn't fit into a 32-bit size_t too large.
    *out = n > UINT32_MAX ? UINT32_MAX : (size_t)n;
    return true;
}

// result_error_msg sets an error made of the prefix and the message.
static void result_error_msg(sqlite3_context* context, const char* prefix, const char* err) {
    char* msg = sqlite3_mprintf("%s: %s", prefix, err);
    if (msg == NULL) {
        sqlite3_result_error_nomem(context);
        return;
    }
    sqlite3_result_error(context, msg, -1);
    sqlite3_free(msg);
}

// check_status sets the NULL result for REGEXP_INVALID, and the error for
// the other failures, using the message in err. Returns false for any of them.
static bool check_status(sqlite3_context* context, RegexpStatus status, const char* err) {
    switch (status) {
        case REGEXP_OK:
            return true;
        case REGEXP_INVALID:
            sqlite3_result_null(context);
            return false;
        case REGEXP_BAD_PATTERN:
            result_error_msg(context, "invalid pattern", err);
            return false;
        case REGEXP_BAD_GROUP:
            // The wrappers report it with the group number.
            sqlite3_result_error(context, "invalid group", -1);
            return false;
        case REGEXP_BAD_REPLACEMENT:
            result_error_msg(context, "invalid replacement", err);
            return false;
        case REGEXP_FAILED:
            result_error_msg(context, "regexp", err);
            return false;
        case REGEXP_NOMEM:
            sqlite3_result_error_nomem(context);
            return false;
    }
    return false;
}

// arg_regexp returns the compiled pattern argument argv[i]: from the
// statement's auxdata if it's there (a constant pattern compiles once),
// or compiled, with cached set accordingly. Pass the result to
// cache_regexp when done. Sets the NULL result for a blob, and the error
// for an invalid pattern or out of memory. Returns NULL on failure.
static Regexp* arg_regexp(sqlite3_context* context, sqlite3_value** argv, int i, bool* cached) {
    Regexp* re = sqlite3_get_auxdata(context, i);
    if (re != NULL) {
        *cached = true;
        return re;
    }
    const char* pattern;
    size_t n;
    if (!arg_text(context, argv[i], &pattern, &n)) {
        return NULL;
    }
    char err[REGEXP_ERR_BUF_SIZE];
    if (!check_status(context, regexp_compile(pattern, n, &re, err), err)) {
        return NULL;
    }
    *cached = false;
    return re;
}

// free_regexp is regexp_free with the signature of an auxdata destructor.
static void free_regexp(void* re) {
    regexp_free(re);
}

// cache_regexp hands a pattern compiled by arg_regexp over to SQLite to keep
// in the auxdata, so it must not be used afterwards. SQLite frees it when the
// pattern changes or the statement ends, or right away if it can't keep it
// (out of memory, or a call from the query planner).
static void cache_regexp(sqlite3_context* context, int i, Regexp* re, bool cached) {
    if (!cached) {
        sqlite3_set_auxdata(context, i, re, free_regexp);
    }
}

#pragma endregion

#pragma region Matching

// The position of the pattern argument, as the user data of fn_like.
static const int PATTERN_FIRST = 0;
static const int PATTERN_SECOND = 1;

// regexp_like(source, pattern)
// regexp(pattern, source), called by source REGEXP pattern
static void fn_like(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const int ipattern = *(const int*)sqlite3_user_data(context);
    const char* s;
    size_t n;
    if (!arg_text(context, argv[1 - ipattern], &s, &n)) {
        return;
    }
    bool cached;
    Regexp* re = arg_regexp(context, argv, ipattern, &cached);
    if (re == NULL) {
        return;
    }
    char err[REGEXP_ERR_BUF_SIZE];
    bool matched;
    if (check_status(context, regexp_like(re, s, n, &matched, err), err)) {
        sqlite3_result_int(context, matched);
    }
    cache_regexp(context, ipattern, re, cached);
}

// regexp_substr(source, pattern)
// regexp_capture(source, pattern[, n])
static void fn_capture(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2 || argc == 3);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* s;
    size_t n;
    if (!arg_text(context, argv[0], &s, &n)) {
        return;
    }
    size_t group = 0;
    if (argc == 3 && !arg_group(context, argv[2], &group)) {
        return;
    }
    bool cached;
    Regexp* re = arg_regexp(context, argv, 1, &cached);
    if (re == NULL) {
        return;
    }
    char err[REGEXP_ERR_BUF_SIZE];
    bool found;
    size_t start, len;
    RegexpStatus status = regexp_capture(re, s, n, group, &found, &start, &len, err);
    if (status == REGEXP_BAD_GROUP) {
        // Only reachable with an explicit group, since group 0 always exists.
        result_invalid_group(context, argv[2]);
    } else if (check_status(context, status, err)) {
        if (found) {
            sqlite3_result_text64(context, s + start, len, SQLITE_TRANSIENT, SQLITE_UTF8);
        } else {
            sqlite3_result_null(context);
        }
    }
    cache_regexp(context, 1, re, cached);
}

#pragma endregion

#pragma region Replacing

// result_replace sets the result of regexp_replace(s, re, repl).
static void result_replace(sqlite3_context* context,
                           Regexp* re,
                           const char* s,
                           size_t n,
                           const char* repl,
                           size_t nrepl) {
    sqlite3_uint64 limit =
        (sqlite3_uint64)sqlite3_limit(sqlite3_context_db_handle(context), SQLITE_LIMIT_LENGTH, -1);
    // Most replacements keep the length about the same, so start with some
    // room to grow. A result that doesn't fit costs a second match with the
    // exact size, so the loop runs at most twice.
    sqlite3_uint64 size = (sqlite3_uint64)n + n / 4 + 16;
    if (size > limit + 1) {
        size = limit + 1;
    }
    for (;;) {
        char* buf = sqlite3_malloc64(size);
        if (buf == NULL) {
            sqlite3_result_error_nomem(context);
            return;
        }
        char err[REGEXP_ERR_BUF_SIZE];
        size_t len;
        RegexpStatus status = regexp_replace(re, s, n, repl, nrepl, buf, size, &len, err);
        if (!check_status(context, status, err)) {
            sqlite3_free(buf);
            return;
        }
        if (len < size) {
            sqlite3_result_text64(context, buf, len, sqlite3_free, SQLITE_UTF8);
            return;
        }
        sqlite3_free(buf);
        if (len > limit) {
            sqlite3_result_error_toobig(context);
            return;
        }
        size = (sqlite3_uint64)len + 1;
    }
}

// regexp_replace(source, pattern, replacement)
static void fn_replace(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 3);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* s;
    size_t n;
    if (!arg_text(context, argv[0], &s, &n)) {
        return;
    }
    const char* repl;
    size_t nrepl;
    if (!arg_text(context, argv[2], &repl, &nrepl)) {
        return;
    }
    bool cached;
    Regexp* re = arg_regexp(context, argv, 1, &cached);
    if (re == NULL) {
        return;
    }
    result_replace(context, re, s, n, repl, nrepl);
    cache_regexp(context, 1, re, cached);
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
    // matching
    {"regexp_like", 2, FLAGS, (void*)&PATTERN_SECOND, fn_like},
    {"regexp_substr", 2, FLAGS, 0, fn_capture},
    {"regexp_capture", 2, FLAGS, 0, fn_capture},
    {"regexp_capture", 3, FLAGS, 0, fn_capture},

    // replacing
    {"regexp_replace", 3, FLAGS, 0, fn_replace},

    // the REGEXP operator
    {"regexp", 2, FLAGS, (void*)&PATTERN_FIRST, fn_like},
};

int regexp_init(sqlite3* db) {
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
