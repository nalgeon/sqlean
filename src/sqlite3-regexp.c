// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

/*
 * SQLite extension for working with regular expressions.
 *
 * regexp_like(source, pattern)
 *   - checks if the source string matches the pattern
 * regexp_substr(source, pattern)
 *   - returns a substring of the source string that matches the pattern
 * regexp_replace(source, pattern, replacement)
 *   - replaces all matching substrings with the replacement string
 *
 * Supports PCRE syntax, see docs/regexp.md
 *
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "regexp/pcre2/pcre2.h"
#include "regexp/regexp.h"
#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

/*
 * Checks if the source string matches the pattern.
 * regexp_statement(pattern, source)
 * E.g.:
 * select true where 'abc' regexp 'a.c';
 */
static void regexp_statement(sqlite3_context* context, int argc, sqlite3_value** argv) {
    const char* source;
    const char* pattern;
    int is_match = 0;

    assert(argc == 2);

    source = (const char*)sqlite3_value_text(argv[1]);
#ifdef DEBUG
    fprintf(stderr, "source = %s\n", source);
#endif
    if (!source) {
        sqlite3_result_int(context, is_match);
        return;
    }

    pattern = (const char*)sqlite3_value_text(argv[0]);
#ifdef DEBUG
    fprintf(stderr, "pattern = %s\n", pattern);
#endif
    if (!pattern) {
        sqlite3_result_error(context, "missing regexp pattern", -1);
        return;
    }

    bool is_new_re = false;
    pcre2_code* re = sqlite3_get_auxdata(context, 0);
    if (re == NULL) {
        re = regexp.compile(pattern);
        if (re == NULL) {
            sqlite3_result_error_nomem(context);
            return;
        }
        is_new_re = true;
    }

    int rc = regexp.like(re, source);
    if (rc == -1) {
        if (is_new_re) {
            regexp.free(re);
        }
        sqlite3_result_error(context, "invalid regexp pattern", -1);
        return;
    }

    is_match = rc;
    sqlite3_result_int(context, is_match);

    if (is_new_re) {
        sqlite3_set_auxdata(context, 0, re, (void (*)(void*))regexp.free);
    }
}

/*
 * Checks if the source string matches the pattern.
 * regexp_like(source, pattern)
 * E.g.:
 * select regexp_like('abc', 'a.c');
 */
static void regexp_like(sqlite3_context* context, int argc, sqlite3_value** argv) {
    const char* source;
    const char* pattern;
    int is_match = 0;

    assert(argc == 2);

    source = (const char*)sqlite3_value_text(argv[0]);
#ifdef DEBUG
    fprintf(stderr, "source = %s\n", source);
#endif
    if (!source) {
        sqlite3_result_int(context, is_match);
        return;
    }

    pattern = (const char*)sqlite3_value_text(argv[1]);
#ifdef DEBUG
    fprintf(stderr, "pattern = %s\n", pattern);
#endif
    if (!pattern) {
        sqlite3_result_error(context, "missing regexp pattern", -1);
        return;
    }

    bool is_new_re = false;
    pcre2_code* re = sqlite3_get_auxdata(context, 1);
    if (re == NULL) {
        re = regexp.compile(pattern);
        if (re == NULL) {
            sqlite3_result_error_nomem(context);
            return;
        }
        is_new_re = true;
    }

    int rc = regexp.like(re, source);
    if (rc == -1) {
        if (is_new_re) {
            regexp.free(re);
        }
        sqlite3_result_error(context, "invalid regexp pattern", -1);
        return;
    }

    is_match = rc;
    sqlite3_result_int(context, is_match);

    if (is_new_re) {
        sqlite3_set_auxdata(context, 1, re, (void (*)(void*))regexp.free);
    }
}

/*
 * Returns a substring of the source string that matches the pattern.
 * regexp_substr(source, pattern)
 * E.g.: select regexp_substr('abcdef', 'b.d') = 'bcd';
 */
static void regexp_substr(sqlite3_context* context, int argc, sqlite3_value** argv) {
    const char* source;
    const char* pattern;

    assert(argc == 2);

    source = (const char*)sqlite3_value_text(argv[0]);
    if (!source) {
        return;
    }

    pattern = (const char*)sqlite3_value_text(argv[1]);
    if (!pattern) {
        sqlite3_result_error(context, "missing regexp pattern", -1);
        return;
    }

    bool is_new_re = false;
    pcre2_code* re = sqlite3_get_auxdata(context, 1);
    if (re == NULL) {
        re = regexp.compile(pattern);
        if (re == NULL) {
            sqlite3_result_error_nomem(context);
            return;
        }
        is_new_re = true;
    }

    char* matched_str;
    int rc = regexp.extract(re, source, 0, &matched_str);
    if (rc == -1) {
        if (is_new_re) {
            regexp.free(re);
        }
        sqlite3_result_error(context, "invalid regexp pattern", -1);
        return;
    }

    if (rc == 0) {
        if (is_new_re) {
            regexp.free(re);
        }
        return;
    }

#ifdef DEBUG
    fprintf(stderr, "matched_str = '%s'\n", matched_str);
#endif

    sqlite3_result_text(context, matched_str, -1, SQLITE_TRANSIENT);
    free(matched_str);

    if (is_new_re) {
        sqlite3_set_auxdata(context, 1, re, (void (*)(void*))regexp.free);
    }
}

/*
 * Finds a substring of the source string that matches the pattern
 * and returns the nth matching group within that substring.
 * regexp_capture(source, pattern[, n])
 * E.g.: select regexp_capture('abcdef', 'b(.)d', 1) = 'c';
 */
static void regexp_capture(sqlite3_context* context, int argc, sqlite3_value** argv) {
    const char* source;
    const char* pattern;

    assert(argc == 2 || argc == 3);

    source = (const char*)sqlite3_value_text(argv[0]);
    if (!source) {
        return;
    }

    pattern = (const char*)sqlite3_value_text(argv[1]);
    if (!pattern) {
        sqlite3_result_error(context, "missing regexp pattern", -1);
        return;
    }

    size_t group_idx = 0;
    if (argc == 3) {
        if (sqlite3_value_type(argv[2]) != SQLITE_INTEGER) {
            sqlite3_result_error(context, "group number should be integer", -1);
            return;
        }
        group_idx = sqlite3_value_int64(argv[2]);
    }

    bool is_new_re = false;
    pcre2_code* re = sqlite3_get_auxdata(context, 1);
    if (re == NULL) {
        re = regexp.compile(pattern);
        if (re == NULL) {
            sqlite3_result_error_nomem(context);
            return;
        }
        is_new_re = true;
    }

    char* matched_str;
    int rc = regexp.extract(re, source, group_idx, &matched_str);
    if (rc == -1) {
        if (is_new_re) {
            regexp.free(re);
        }
        sqlite3_result_error(context, "invalid regexp pattern", -1);
        return;
    }

    if (rc == 0) {
        if (is_new_re) {
            regexp.free(re);
        }
        return;
    }

    sqlite3_result_text(context, matched_str, -1, SQLITE_TRANSIENT);
    free(matched_str);

    if (is_new_re) {
        sqlite3_set_auxdata(context, 1, re, (void (*)(void*))regexp.free);
    }
}

/*
 * Replaces all matching substrings with the replacement string.
 * regexp_replace(source, pattern, replacement)
 * E.g.: select regexp_replace('abcdef', 'b.d', '...') = 'a...ef';
 */
static void regexp_replace(sqlite3_context* context, int argc, sqlite3_value** argv) {
    const char* source;
    const char* pattern;
    const char* replacement;
    char* result;

    assert(argc == 3);

    source = (char*)sqlite3_value_text(argv[0]);
    if (!source) {
        return;
    }

    pattern = (char*)sqlite3_value_text(argv[1]);
    if (!pattern) {
        sqlite3_result_error(context, "missing regexp pattern", -1);
        return;
    }

    replacement = (char*)sqlite3_value_text(argv[2]);
    if (!replacement) {
        sqlite3_result_value(context, argv[0]);
        return;
    }

    bool is_new_re = false;
    pcre2_code* re = sqlite3_get_auxdata(context, 1);
    if (re == NULL) {
        re = regexp.compile(pattern);
        if (re == NULL) {
            sqlite3_result_error_nomem(context);
            return;
        }
        is_new_re = true;
    }

    int rc = regexp.replace(re, source, replacement, &result);
    if (rc == -1) {
        if (is_new_re) {
            regexp.free(re);
        }
        sqlite3_result_error(context, "invalid regexp pattern", -1);
        return;
    }

    if (rc == 0) {
        if (is_new_re) {
            regexp.free(re);
        }
        sqlite3_result_value(context, argv[0]);
        return;
    }

#ifdef DEBUG
    fprintf(stderr, "result = '%s'\n", result);
#endif

    sqlite3_result_text(context, result, -1, SQLITE_TRANSIENT);
    free(result);

    if (is_new_re) {
        sqlite3_set_auxdata(context, 1, re, (void (*)(void*))regexp.free);
    }
}

/*
 * Registers the extension.
 */
#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_regexp_init(sqlite3* db, char** pzErrMsg, const sqlite3_api_routines* pApi) {
    SQLITE_EXTENSION_INIT2(pApi);
    static const int flags = SQLITE_UTF8 | SQLITE_DETERMINISTIC;
    sqlite3_create_function(db, "regexp", 2, flags, 0, regexp_statement, 0, 0);
    sqlite3_create_function(db, "regexp_like", 2, flags, 0, regexp_like, 0, 0);
    sqlite3_create_function(db, "regexp_substr", 2, flags, 0, regexp_substr, 0, 0);
    sqlite3_create_function(db, "regexp_capture", 2, flags, 0, regexp_capture, 0, 0);
    sqlite3_create_function(db, "regexp_capture", 3, flags, 0, regexp_capture, 0, 0);
    sqlite3_create_function(db, "regexp_replace", 3, flags, 0, regexp_replace, 0, 0);
    return SQLITE_OK;
}