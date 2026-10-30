// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Regular expression tests.

// The SQL tests cover the matching itself. These tests cover what they
// can't: the statuses and error messages, the snprintf-style buffer of
// regexp_replace, and the failures that raise an SQL error.

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "regexp/regexp.h"

// A string with its length, so it can contain NUL characters.
typedef struct {
    const char* s;
    size_t n;
} Str;

#define STR(s) {s, sizeof(s) - 1}

static const char* status_name(RegexpStatus status) {
    switch (status) {
        case REGEXP_OK:
            return "OK";
        case REGEXP_INVALID:
            return "INVALID";
        case REGEXP_BAD_PATTERN:
            return "BAD_PATTERN";
        case REGEXP_BAD_GROUP:
            return "BAD_GROUP";
        case REGEXP_BAD_REPLACEMENT:
            return "BAD_REPLACEMENT";
        case REGEXP_FAILED:
            return "FAILED";
        case REGEXP_NOMEM:
            return "NOMEM";
    }
    return "?";
}

// compile compiles a pattern that must be valid.
static Regexp* compile(Str pattern) {
    Regexp* re = NULL;
    char err[REGEXP_ERR_BUF_SIZE];
    RegexpStatus status = regexp_compile(pattern.s, pattern.n, &re, err);
    if (status != REGEXP_OK) {
        printf("\ncompile(%s): %s: %s\n", pattern.s, status_name(status), err);
    }
    assert(status == REGEXP_OK);
    assert(re != NULL);
    return re;
}

// check_status fails the test if the status or the error message
// (when the status has one) is not the expected one.
static void check_status(const char* name,
                         Str pattern,
                         RegexpStatus status,
                         const char* err,
                         RegexpStatus want,
                         const char* want_err) {
    bool has_err =
        want == REGEXP_BAD_PATTERN || want == REGEXP_BAD_REPLACEMENT || want == REGEXP_FAILED;
    if (status != want || (has_err && strcmp(err, want_err) != 0)) {
        printf("\n%s(%s): got %s '%s', want %s '%s'\n", name, pattern.s, status_name(status),
               status == want && has_err ? err : "", status_name(want), want_err);
    }
    assert(status == want);
    if (has_err) {
        assert(strcmp(err, want_err) == 0);
    }
}

// A pattern that backtracks exponentially, and a subject that makes it
// hit PCRE2's default match limit.
#define CATASTROPHIC "(a+)+$"
#define LIMIT_SUBJECT "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaab"

static void test_compile(void) {
    printf("test_compile...");
    const struct {
        Str pattern;
        RegexpStatus want;
        const char* err;
    } tests[] = {
        {STR(""), REGEXP_OK, ""},
        {STR("a(b)c"), REGEXP_OK, ""},
        {STR("(?<name>a)"), REGEXP_OK, ""},
        {STR("a\0b"), REGEXP_OK, ""},
        {STR("("), REGEXP_BAD_PATTERN, "missing closing parenthesis at offset 1"},
        {STR("["), REGEXP_BAD_PATTERN, "missing terminating ] for character class at offset 1"},
        {STR("a{2,1}"), REGEXP_BAD_PATTERN, "numbers out of order in {} quantifier at offset 5"},
        {STR("a**"), REGEXP_BAD_PATTERN,
         "quantifier does not follow a repeatable item at offset 2"},
        {STR("\\"), REGEXP_BAD_PATTERN, "\\ at end of pattern at offset 1"},
        {STR("(?<=a+)b"), REGEXP_BAD_PATTERN,
         "lookbehind assertion is not fixed length at offset 0"},
        // the pattern doesn't end at a NUL, so the parenthesis is unclosed
        {STR("a\0("), REGEXP_BAD_PATTERN, "missing closing parenthesis at offset 3"},
        {STR("\xff"), REGEXP_BAD_PATTERN, "UTF-8 error: illegal byte (0xfe or 0xff) at offset 0"},
    };
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        Regexp* re = NULL;
        char err[REGEXP_ERR_BUF_SIZE] = "";
        RegexpStatus status = regexp_compile(tests[i].pattern.s, tests[i].pattern.n, &re, err);
        check_status("compile", tests[i].pattern, status, err, tests[i].want, tests[i].err);
        assert((status == REGEXP_OK) == (re != NULL));
        regexp_free(re);
    }
    regexp_free(NULL);
    printf("OK\n");
}

static void test_like(void) {
    printf("test_like...");
    const struct {
        Str pattern;
        Str s;
        RegexpStatus want;
        bool matched;
        const char* err;
    } tests[] = {
        {STR("b"), STR("abc"), REGEXP_OK, true, ""},
        {STR("^b"), STR("abc"), REGEXP_OK, false, ""},
        {STR(""), STR(""), REGEXP_OK, true, ""},
        {STR("a"), STR(""), REGEXP_OK, false, ""},
        // NUL is a character, in the subject and in the pattern
        {STR("b"), STR("a\0b"), REGEXP_OK, true, ""},
        {STR("^a\\x00b$"), STR("a\0b"), REGEXP_OK, true, ""},
        {STR("a\0z"), STR("a"), REGEXP_OK, false, ""},
        {STR("^.$"), STR("\xd0\xbf"), REGEXP_OK, true, ""},
        // invalid UTF-8, anywhere in the subject
        {STR("a"), STR("\xff"), REGEXP_INVALID, false, ""},
        {STR("a"), STR("a\xff"), REGEXP_INVALID, false, ""},
        {STR("a"), STR("\xd0"), REGEXP_INVALID, false, ""},
        {STR(CATASTROPHIC), STR(LIMIT_SUBJECT), REGEXP_FAILED, false, "match limit exceeded"},
    };
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        Regexp* re = compile(tests[i].pattern);
        char err[REGEXP_ERR_BUF_SIZE] = "";
        bool matched = !tests[i].matched;
        RegexpStatus status = regexp_like(re, tests[i].s.s, tests[i].s.n, &matched, err);
        check_status("like", tests[i].pattern, status, err, tests[i].want, tests[i].err);
        if (status == REGEXP_OK && matched != tests[i].matched) {
            printf("\nlike(%s, %s): got %d, want %d\n", tests[i].pattern.s, tests[i].s.s, matched,
                   tests[i].matched);
        }
        assert(status != REGEXP_OK || matched == tests[i].matched);
        regexp_free(re);
    }
    printf("OK\n");
}

static void test_capture(void) {
    printf("test_capture...");
    const struct {
        Str pattern;
        Str s;
        size_t group;
        RegexpStatus want;
        bool found;
        size_t start;
        size_t len;
    } tests[] = {
        {STR("b(.)d"), STR("abcdef"), 0, REGEXP_OK, true, 1, 3},
        {STR("b(.)d"), STR("abcdef"), 1, REGEXP_OK, true, 2, 1},
        {STR("((a)(b))(c)"), STR("abcd"), 1, REGEXP_OK, true, 0, 2},
        {STR("((a)(b))(c)"), STR("abcd"), 4, REGEXP_OK, true, 2, 1},
        // no match
        {STR("z"), STR("abc"), 0, REGEXP_OK, false, 0, 0},
        {STR("(z)"), STR("abc"), 1, REGEXP_OK, false, 0, 0},
        // an empty match and an empty group are found
        {STR("x*"), STR("abc"), 0, REGEXP_OK, true, 0, 0},
        {STR("$"), STR("abc"), 0, REGEXP_OK, true, 3, 0},
        {STR("a(b*)c"), STR("ac"), 1, REGEXP_OK, true, 1, 0},
        // a group that didn't participate is not found, wherever it is
        {STR("a(b)?c"), STR("ac"), 1, REGEXP_OK, false, 0, 0},
        {STR("a(b)?(c)"), STR("ac"), 1, REGEXP_OK, false, 0, 0},
        {STR("a(b)?(c)"), STR("ac"), 2, REGEXP_OK, true, 1, 1},
        {STR("(a)|(b)"), STR("b"), 1, REGEXP_OK, false, 0, 0},
        // offsets are in bytes
        {STR("(\\w+)$"), STR("\xd0\xbf\xd1\x80 \xd0\xb8"), 1, REGEXP_OK, true, 5, 2},
        // NUL is a character
        {STR("a(.*)"), STR("a\0b"), 1, REGEXP_OK, true, 1, 2},
        // a group the pattern doesn't have, with or without a match
        {STR("b"), STR("abc"), 1, REGEXP_BAD_GROUP, false, 0, 0},
        {STR("(b)"), STR("abc"), 2, REGEXP_BAD_GROUP, false, 0, 0},
        {STR("(z)"), STR("abc"), 2, REGEXP_BAD_GROUP, false, 0, 0},
        {STR("(b)"), STR("abc"), (size_t)-1, REGEXP_BAD_GROUP, false, 0, 0},
        // non-capturing groups don't count
        {STR("(?:a)(b)"), STR("ab"), 2, REGEXP_BAD_GROUP, false, 0, 0},
        {STR("a"), STR("a\xff"), 0, REGEXP_INVALID, false, 0, 0},
        // the group is checked before the subject
        {STR("a"), STR("a\xff"), 1, REGEXP_BAD_GROUP, false, 0, 0},
        {STR(CATASTROPHIC), STR(LIMIT_SUBJECT), 0, REGEXP_FAILED, false, 0, 0},
    };
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        Regexp* re = compile(tests[i].pattern);
        char err[REGEXP_ERR_BUF_SIZE] = "";
        bool found = !tests[i].found;
        size_t start = 0, len = 0;
        RegexpStatus status = regexp_capture(re, tests[i].s.s, tests[i].s.n, tests[i].group, &found,
                                             &start, &len, err);
        check_status("capture", tests[i].pattern, status, err, tests[i].want,
                     "match limit exceeded");
        if (status == REGEXP_OK) {
            bool ok = found == tests[i].found &&
                      (!found || (start == tests[i].start && len == tests[i].len));
            if (!ok) {
                printf("\ncapture(%s, %s, %zu): got %d [%zu, %zu), want %d [%zu, %zu)\n",
                       tests[i].pattern.s, tests[i].s.s, tests[i].group, found, start, len,
                       tests[i].found, tests[i].start, tests[i].len);
            }
            assert(ok);
        }
        regexp_free(re);
    }
    printf("OK\n");
}

// replace calls regexp_replace with a buffer of the given size, and checks
// the status, the error message, the length and, if it fits, the result.
static void replace(Str pattern,
                    Str s,
                    Str repl,
                    size_t size,
                    RegexpStatus want,
                    const char* want_err,
                    Str want_res) {
    char buf[256];
    assert(size <= sizeof(buf));
    // Fill the buffer to catch a missing NUL or a write past size.
    memset(buf, '#', sizeof(buf));
    Regexp* re = compile(pattern);
    char err[REGEXP_ERR_BUF_SIZE] = "";
    size_t len = 0;
    RegexpStatus status = regexp_replace(re, s.s, s.n, repl.s, repl.n, buf, size, &len, err);
    check_status("replace", pattern, status, err, want, want_err);
    if (status == REGEXP_OK) {
        bool ok = len == want_res.n;
        if (ok && len < size) {
            ok = memcmp(buf, want_res.s, len) == 0 && buf[len] == '\0';
        }
        if (!ok) {
            printf("\nreplace(%s, %s, %s, %zu): got %zu '%.*s', want %zu '%s'\n", pattern.s, s.s,
                   repl.s, size, len, len < size ? (int)len : 0, buf, want_res.n, want_res.s);
        }
        assert(ok);
    }
    for (size_t i = size; i < sizeof(buf); i++) {
        assert(buf[i] == '#');
    }
    regexp_free(re);
}

static void test_replace(void) {
    printf("test_replace...");
    const struct {
        Str pattern;
        Str s;
        Str repl;
        Str want;
    } tests[] = {
        {STR("[0-9]+"), STR("the year is 2021"), STR("2050"), STR("the year is 2050")},
        {STR("z"), STR("abc"), STR("x"), STR("abc")},
        {STR("a"), STR("aaa"), STR("b"), STR("bbb")},
        {STR(""), STR("abc"), STR("-"), STR("-a-b-c-")},
        {STR(""), STR(""), STR("x"), STR("x")},
        {STR("a"), STR(""), STR("x"), STR("")},
        {STR("a+"), STR("aaaa"), STR(""), STR("")},
        // the replacement syntax
        {STR("(b)"), STR("abc"), STR("[$1]"), STR("a[b]c")},
        {STR("b"), STR("abc"), STR("[$0]"), STR("a[b]c")},
        {STR("(b)"), STR("abc"), STR("[${1}]"), STR("a[b]c")},
        {STR("(?<x>b)"), STR("abc"), STR("[${x}]"), STR("a[b]c")},
        {STR("b"), STR("abc"), STR("$$"), STR("a$c")},
        {STR("(a)(b)"), STR("ab"), STR("$2$1"), STR("ba")},
        {STR("(a)(b)(c)(d)(e)(f)(g)(h)(i)(j)"), STR("abcdefghijk"), STR("$10"), STR("jk")},
        {STR("(b)"), STR("abc"), STR("\\U$1"), STR("aBc")},
        {STR("(B)"), STR("ABC"), STR("\\L$1"), STR("AbC")},
        {STR("(a)(\\w+)"), STR("abc abc"), STR("\\u$1\\E$2"), STR("Abc Abc")},
        {STR("b"), STR("abc"), STR("\\$"), STR("a$c")},
        {STR("b"), STR("abc"), STR("\\t"), STR("a\tc")},
        {STR("(b)"), STR("abc"), STR("${1:+x:y}"), STR("axc")},
        {STR("(z)?b"), STR("abc"), STR("${1:+x:y}"), STR("ayc")},
        // an unset group inserts an empty string
        {STR("a(b)?c"), STR("ac"), STR("[$1]"), STR("[]")},
        {STR("(a)|(b)"), STR("ab"), STR("[$1$2]"), STR("[a][b]")},
        // NUL is a character, in the subject, the pattern and the replacement
        {STR("a"), STR("a\0b"), STR("x"), STR("x\0b")},
        {STR("\\x00"), STR("a\0b"), STR("-"), STR("a-b")},
        {STR("b"), STR("abc"), STR("\0"), STR("a\0c")},
        {STR("^"), STR("\xd0\xbf"), STR("\xd1\x80"), STR("\xd1\x80\xd0\xbf")},
    };
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        Str want = tests[i].want;
        // A buffer that is big enough, exactly big enough,
        // one byte short, and of one byte.
        replace(tests[i].pattern, tests[i].s, tests[i].repl, 256, REGEXP_OK, "", want);
        replace(tests[i].pattern, tests[i].s, tests[i].repl, want.n + 1, REGEXP_OK, "", want);
        if (want.n > 0) {
            replace(tests[i].pattern, tests[i].s, tests[i].repl, want.n, REGEXP_OK, "", want);
        }
        replace(tests[i].pattern, tests[i].s, tests[i].repl, 1, REGEXP_OK, "", want);
    }
    printf("OK\n");
}

static void test_replace_errors(void) {
    printf("test_replace_errors...");
    const struct {
        Str pattern;
        Str s;
        Str repl;
        RegexpStatus want;
        const char* err;
    } tests[] = {
        // a group the pattern doesn't have
        {STR("b"), STR("abc"), STR("$1"), REGEXP_BAD_REPLACEMENT, "unknown substring at offset 2"},
        {STR("(b)"), STR("abc"), STR("${2}"), REGEXP_BAD_REPLACEMENT,
         "unknown substring at offset 4"},
        {STR("(b)"), STR("abc"), STR("${x}"), REGEXP_BAD_REPLACEMENT,
         "unknown substring at offset 4"},
        // malformed
        {STR("b"), STR("abc"), STR("${x"), REGEXP_BAD_REPLACEMENT,
         "expected closing curly bracket in replacement string at offset 3"},
        {STR("b"), STR("abc"), STR("$"), REGEXP_BAD_REPLACEMENT,
         "invalid replacement string at offset 1"},
        {STR("b"), STR("abc"), STR("x\\"), REGEXP_BAD_REPLACEMENT,
         "bad escape sequence in replacement string at offset 2"},
        // the replacement is checked only on a match
        {STR("z"), STR("abc"), STR("$1"), REGEXP_OK, ""},
        // invalid UTF-8 in the subject or in the replacement
        {STR("b"), STR("a\xff"), STR("x"), REGEXP_INVALID, ""},
        {STR("b"), STR("abc"), STR("\xff"), REGEXP_INVALID, ""},
        {STR(CATASTROPHIC), STR(LIMIT_SUBJECT), STR("x"), REGEXP_FAILED, "match limit exceeded"},
    };
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        Regexp* re = compile(tests[i].pattern);
        char buf[64];
        char err[REGEXP_ERR_BUF_SIZE] = "";
        size_t len = 0;
        RegexpStatus status = regexp_replace(re, tests[i].s.s, tests[i].s.n, tests[i].repl.s,
                                             tests[i].repl.n, buf, sizeof(buf), &len, err);
        check_status("replace", tests[i].pattern, status, err, tests[i].want, tests[i].err);
        regexp_free(re);
    }
    printf("OK\n");
}

// A compiled pattern is reused across calls, which share its match data.
static void test_reuse(void) {
    printf("test_reuse...");
    Regexp* re = compile((Str)STR("a(b)?(c)"));
    char err[REGEXP_ERR_BUF_SIZE];
    bool found;
    size_t start, len;

    RegexpStatus status = regexp_capture(re, "abc", 3, 1, &found, &start, &len, err);
    assert(status == REGEXP_OK && found && start == 1 && len == 1);
    // the group from the previous match doesn't leak into this one
    status = regexp_capture(re, "ac", 2, 1, &found, &start, &len, err);
    assert(status == REGEXP_OK && !found);
    // nor does a failed match
    status = regexp_capture(re, "\xff", 1, 0, &found, &start, &len, err);
    assert(status == REGEXP_INVALID);
    status = regexp_capture(re, "xyz", 3, 0, &found, &start, &len, err);
    assert(status == REGEXP_OK && !found);

    char buf[16];
    status = regexp_replace(re, "abcac", 5, "[$1]", 4, buf, sizeof(buf), &len, err);
    assert(status == REGEXP_OK && len == 5 && strcmp(buf, "[b][]") == 0);
    bool matched;
    status = regexp_like(re, "ac", 2, &matched, err);
    assert(status == REGEXP_OK && matched);
    regexp_free(re);
    printf("OK\n");
}

int main(void) {
    test_compile();
    test_like();
    test_capture();
    test_replace();
    test_replace_errors();
    test_reuse();
    return 0;
}
