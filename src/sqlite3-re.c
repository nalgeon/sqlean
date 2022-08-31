// Copyright (c) 2021 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

/*
 * SQLite extension for working with regular expressions.
 *
 * regexp_like(source, pattern)
 *   - checks if source string matches pattern
 * regexp_substr(source, pattern)
 *   - returns source substring matching pattern
 * regexp_replace(source, pattern, replacement)
 *   - replaces matching substring with replacement string
 *
 *  The following regular expression syntax is supported:
 *     X*      zero or more occurrences of X
 *     X+      one or more occurrences of X
 *     X?      zero or one occurrences of X
 *     (X)     match X
 *     X|Y     X or Y
 *     ^X      X occurring at the beginning of the string
 *     X$      X occurring at the end of the string
 *     .       Match any single character
 *     \c      Character c where c is one of \{}()[]|*+?.
 *     \c      C-language escapes for c in afnrtv.  ex: \t or \n
 *     [abc]   Any single character from the set abc
 *     [^abc]  Any single character not in the set abc
 *     [a-z]   Any single character in the range a-z
 *     [^a-z]  Any single character not in the range a-z
 **
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "re.h"
#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

/*
 * Checks if source string matches pattern.
 * regexp_statement(pattern, source)
 * E.g.:
 * select true where 'abc' regexp 'a.c';
 */
static void regexp_statement(sqlite3_context* context, int argc, sqlite3_value** argv) {
    regexp* r;
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

    r = re_compile(pattern);
    if (r == NULL) {
        sqlite3_result_error(context, "invalid regexp pattern", -1);
        return;
    }

    is_match = re_execute(r, source);
    sqlite3_result_int(context, is_match);
    free((char*)r);
}

/*
 * Checks if source string matches pattern.
 * regexp_like(source, pattern)
 * E.g.:
 * select regexp_like('abc', 'a.c');
 */
static void regexp_like(sqlite3_context* context, int argc, sqlite3_value** argv) {
    regexp* r;
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

    r = re_compile(pattern);
    if (r == NULL) {
        sqlite3_result_error(context, "invalid regexp pattern", -1);
        return;
    }

    is_match = re_execute(r, source);
    sqlite3_result_int(context, is_match);
    free((char*)r);
}

/*
 * Returns source substring matching pattern.
 * regexp_substr(source, pattern)
 * E.g.: select regexp_substr('abcdef', 'b.d') = 'bcd';
 */
static void regexp_substr(sqlite3_context* context, int argc, sqlite3_value** argv) {
    regexp* r;
    const char* source;
    const char* pattern;
    int is_match = 0;

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

    r = re_compile(pattern);
    if (r == NULL) {
        sqlite3_result_error(context, "invalid regexp pattern", -1);
        return;
    }

    is_match = re_execute(r, source);
    if (!is_match) {
        return;
    }

    int len = r->endp[0] - r->startp[0];
    char* matched_str = sqlite3_malloc(len + 1);
    (void)strncpy(matched_str, r->startp[0], len);
    matched_str[len] = '\0';
#ifdef DEBUG
    fprintf(stderr, "matched_str = '%s'\n", matched_str);
#endif

    sqlite3_result_text(context, (char*)matched_str, -1, sqlite3_free);
    free((char*)r);
}

/*
 * Returns source substring matching pattern.
 * regexp_replace(source, pattern, replacement)
 * E.g.: select regexp_replace('abcdef', 'b.d', '...') = 'a...ef';
 */
static void regexp_replace(sqlite3_context* context, int argc, sqlite3_value** argv) {
    regexp* r;
    char* source;
    char* pattern;
    char* replacement;
    char* result;

    int is_match = 0;

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

    r = re_compile(pattern);
    if (r == NULL) {
        sqlite3_result_error(context, "invalid regexp pattern", -1);
        return;
    }

    replacement = (char*)sqlite3_value_text(argv[2]);
    if (!replacement) {
        sqlite3_result_value(context, argv[0]);
        return;
    }

    is_match = re_execute(r, source);
    if (!is_match) {
        sqlite3_result_value(context, argv[0]);
        return;
    }

    char replacement_str[BUFSIZ];
    int err = re_substitute(r, replacement, replacement_str);
    if (err) {
        sqlite3_result_error(context, "invalid replacement pattern", -1);
        return;
    }

    int head_len = r->startp[0] - source;
    int tail_len = source + strlen(source) - r->endp[0];
    int replacement_len = strlen(replacement_str);

    int result_len = head_len + replacement_len + tail_len;
    result = sqlite3_malloc(result_len + 1);
    char* at = result;
    memcpy(at, source, head_len);
    at += head_len;
    memcpy(at, replacement_str, replacement_len);
    at += replacement_len;
    memcpy(at, r->endp[0], tail_len);
    result[result_len] = '\0';

#ifdef DEBUG
    fprintf(stderr, "head / tail = %d / %d\n", head_len, tail_len);
    fprintf(stderr, "repl string (%d) = '%s'\n", replacement_len, replacement_str);
    fprintf(stderr, "result string (%d) = '%s'\n", result_len, result);
    fprintf(stderr, "replace('%s', '%s', '%s') = '%s'\n", source, pattern, replacement, result);
#endif

    sqlite3_result_text(context, (char*)result, -1, sqlite3_free);
    free((char*)r);
}

/*
 * Registers the extension.
 */
#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_re_init(sqlite3* db, char** pzErrMsg, const sqlite3_api_routines* pApi) {
    SQLITE_EXTENSION_INIT2(pApi);
    sqlite3_create_function(db, "regexp", 2, SQLITE_UTF8, 0, regexp_statement, 0, 0);
    sqlite3_create_function(db, "regexp_like", 2, SQLITE_UTF8, 0, regexp_like, 0, 0);
    sqlite3_create_function(db, "regexp_substr", 2, SQLITE_UTF8, 0, regexp_substr, 0, 0);
    sqlite3_create_function(db, "regexp_replace", 3, SQLITE_UTF8, 0, regexp_replace, 0, 0);
    return SQLITE_OK;
}