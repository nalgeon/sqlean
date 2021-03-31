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
 * Replaces `rep` substring of the `orig` string with `with` substring.
 */
static char *str_replace(char *orig, char *rep, char *with) {
    char *result;   // the return string
    char *ins;      // the next insert point
    char *tmp;      // varies
    int len_rep;    // length of rep (the string to remove)
    int len_with;   // length of with (the string to replace rep with)
    int len_front;  // distance between rep and end of last rep
    int count;      // number of replacements

    // sanity checks and initialization
    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL;  // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = sqlite3_malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep;  // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}

/*
 * Checks if source string matches pattern.
 * regexp_statement(pattern, source)
 * E.g.:
 * select true where 'abc' regexp 'a.c';
 */
static void regexp_statement(
    sqlite3_context *context,
    int argc,
    sqlite3_value **argv) {
    regexp *r;
    const char *source;
    const char *pattern;
    int is_match = 0;

    assert(argc == 2);

    source = (const char *)sqlite3_value_text(argv[1]);
#ifdef DEBUG
    fprintf(stderr, "source = %s\n", source);
#endif
    if (!source) {
        sqlite3_result_int(context, is_match);
        return;
    }

    pattern = (const char *)sqlite3_value_text(argv[0]);
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
    free((char *)r);
}

/*
 * Checks if source string matches pattern.
 * regexp_like(source, pattern)
 * E.g.:
 * select regexp_like('abc', 'a.c');
 */
static void regexp_like(
    sqlite3_context *context,
    int argc,
    sqlite3_value **argv) {
    regexp *r;
    const char *source;
    const char *pattern;
    int is_match = 0;

    assert(argc == 2);

    source = (const char *)sqlite3_value_text(argv[0]);
#ifdef DEBUG
    fprintf(stderr, "source = %s\n", source);
#endif
    if (!source) {
        sqlite3_result_int(context, is_match);
        return;
    }

    pattern = (const char *)sqlite3_value_text(argv[1]);
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
    free((char *)r);
}

/*
 * Returns source substring matching pattern.
 * regexp_substr(source, pattern)
 * E.g.: select regexp_substr('abcdef', 'b.d') = 'bcd';
 */
static void regexp_substr(
    sqlite3_context *context,
    int argc,
    sqlite3_value **argv) {
    regexp *r;
    const char *source;
    const char *pattern;
    int is_match = 0;

    assert(argc == 2);

    source = (const char *)sqlite3_value_text(argv[0]);
    if (!source) {
        return;
    }

    pattern = (const char *)sqlite3_value_text(argv[1]);
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
    char *matched_str = sqlite3_malloc(len + 1);
    (void)strncpy(matched_str, r->startp[0], len);
    matched_str[len] = '\0';
#ifdef DEBUG
    fprintf(stderr, "matched_str = '%s'\n", matched_str);
#endif

    sqlite3_result_text(context, (char *)matched_str, -1, sqlite3_free);
    free((char *)r);
}

/*
 * Returns source substring matching pattern.
 * regexp_replace(source, pattern, replacement)
 * E.g.: select regexp_replace('abcdef', 'b.d', '...') = 'a...ef';
 */
static void regexp_replace(
    sqlite3_context *context,
    int argc,
    sqlite3_value **argv) {
    regexp *r;
    char *source;
    char *pattern;
    char *replacement;
    char *result;

    int is_match = 0;

    assert(argc == 3);

    source = (char *)sqlite3_value_text(argv[0]);
    if (!source) {
        return;
    }

    pattern = (char *)sqlite3_value_text(argv[1]);
    if (!pattern) {
        sqlite3_result_error(context, "missing regexp pattern", -1);
        return;
    }

    r = re_compile(pattern);
    if (r == NULL) {
        sqlite3_result_error(context, "invalid regexp pattern", -1);
        return;
    }

    replacement = (char *)sqlite3_value_text(argv[2]);
    if (!replacement) {
        sqlite3_result_value(context, argv[0]);
        return;
    }

    is_match = re_execute(r, source);
    if (!is_match) {
        sqlite3_result_value(context, argv[0]);
        return;
    }

    int len = r->endp[0] - r->startp[0];
    char *matched_str = sqlite3_malloc(len + 1);
    (void)strncpy(matched_str, r->startp[0], len);
    matched_str[len] = '\0';

#ifdef DEBUG
    fprintf(stderr, "len = %d\n", len);
    fprintf(stderr, "matched_str = '%s'\n", matched_str);
#endif

    char replacement_str[BUFSIZ];
    int err = re_substitute(r, replacement, replacement_str);
#ifdef DEBUG
    fprintf(stderr, "replacement_str = '%s'\n", replacement_str);
#endif
    if (err) {
        sqlite3_result_error(context, "invalid replacement pattern", -1);
        return;
    }

    result = str_replace(source, matched_str, replacement_str);
#ifdef DEBUG
    fprintf(stderr, "replace('%s', '%s', '%s') = '%s'\n", source, matched_str, replacement_str, result);
#endif
    sqlite3_result_text(context, (char *)result, -1, sqlite3_free);
    free((char *)r);
}

/*
 * Registers the extension.
 */
#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_sqlitere_init(
        sqlite3 *db,
        char **pzErrMsg,
        const sqlite3_api_routines *pApi) {
    SQLITE_EXTENSION_INIT2(pApi);
    sqlite3_create_function(db, "regexp", 2, SQLITE_UTF8, 0, regexp_statement, 0, 0);
    sqlite3_create_function(db, "regexp_like", 2, SQLITE_UTF8, 0, regexp_like, 0, 0);
    sqlite3_create_function(db, "regexp_substr", 2, SQLITE_UTF8, 0, regexp_substr, 0, 0);
    sqlite3_create_function(db, "regexp_replace", 3, SQLITE_UTF8, 0, regexp_replace, 0, 0);
    return SQLITE_OK;
}