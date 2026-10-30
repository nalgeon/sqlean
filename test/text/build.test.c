// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Split, join and modify tests.

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "text/text.h"

#define LEN(a) (sizeof(a) / sizeof(a[0]))

// Str is a string literal with its size, so it may contain zero bytes.
typedef struct {
    const char* s;
    size_t n;
} Str;

#define S(x) {x, sizeof(x) - 1}

// check_result checks that buf holds size bytes equal to want, and frees buf.
static void check_result(char* buf, size_t size, Str want) {
    assert(size == want.n);
    assert(memcmp(buf, want.s, size) == 0);
    free(buf);
}

// alloc allocates a buffer of size bytes, at least one.
static char* alloc(size_t size) {
    char* buf = malloc(size > 0 ? size : 1);
    assert(buf != NULL);
    return buf;
}

typedef struct {
    Str s, sep;
    int64_t part;
    Str want;
} SplitTest;

static void test_split(void) {
    printf("test_split...");
    static const SplitTest tests[] = {
        {S("a|b|c"), S("|"), 1, S("a")},
        {S("a|b|c"), S("|"), 3, S("c")},
        {S("a|b|c"), S("|"), 4, S("")},
        {S("a|b|c"), S("|"), -1, S("c")},
        {S("a|b|c"), S("|"), -3, S("a")},
        {S("a|b|c"), S("|"), -4, S("")},
        {S("a|b|c"), S("|"), INT64_MAX, S("")},
        {S("a|b|c"), S("|"), INT64_MIN, S("")},
        {S("|a|"), S("|"), 1, S("")},
        {S("|a|"), S("|"), 2, S("a")},
        {S("|a|"), S("|"), 3, S("")},
        {S("a||b"), S("|"), 2, S("")},
        {S("a"), S("abc"), 1, S("a")},
        {S("abc"), S("abc"), 1, S("")},
        {S("abc"), S("abc"), 2, S("")},
        {S("один—два"), S("—"), 2, S("два")},
        {S("a/\\b"), S("/\\"), 2, S("b")},
        {S("aaa"), S("aa"), 2, S("a")},
        {S("a\0b"), S("\0"), 2, S("b")},
        {S("a\xff"
           "b"),
         S("\xff"), -1, S("b")},
        // an empty separator doesn't split
        {S("a|b"), S(""), 1, S("a|b")},
        {S("a|b"), S(""), -1, S("a|b")},
        {S("a|b"), S(""), 2, S("")},
        {S(""), S("|"), 1, S("")},
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        const SplitTest* t = &tests[i];
        TextSpan span;
        assert(text_split(t->s.s, t->s.n, t->sep.s, t->sep.n, t->part, &span));
        assert(span.len == t->want.n);
        assert(memcmp(t->s.s + span.start, t->want.s, span.len) == 0);
    }
    // there is no part 0
    TextSpan span;
    assert(!text_split("a|b", 3, "|", 1, 0, &span));
    printf("OK\n");
}

static void test_join(void) {
    printf("test_join...");
    {
        const char* parts[] = {"one", "", "t\0o"};
        const size_t sizes[] = {3, 0, 3};
        size_t size = text_join("--", 2, parts, sizes, 3, NULL);
        char* buf = alloc(size);
        assert(text_join("--", 2, parts, sizes, 3, buf) == size);
        check_result(buf, size, (Str)S("one----t\0o"));
    }
    {
        const char* parts[] = {"one"};
        const size_t sizes[] = {3};
        char* buf = alloc(3);
        check_result(buf, text_join("|", 1, parts, sizes, 1, buf), (Str)S("one"));
    }
    assert(text_join("|", 1, NULL, NULL, 0, NULL) == 0);
    printf("OK\n");
}

typedef struct {
    Str s;
    int64_t count;
    Str want;
} RepeatTest;

static void test_repeat(void) {
    printf("test_repeat...");
    static const RepeatTest tests[] = {
        {S("ab"), 3, S("ababab")},  {S("ab"), 1, S("ab")},       {S("ab"), 0, S("")},
        {S("ab"), -1, S("")},       {S("ab"), INT64_MIN, S("")}, {S(""), INT64_MAX, S("")},
        {S("ж\0"), 2, S("ж\0ж\0")},
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        const RepeatTest* t = &tests[i];
        size_t size = text_repeat(t->s.s, t->s.n, t->count, NULL);
        char* buf = alloc(size);
        assert(text_repeat(t->s.s, t->s.n, t->count, buf) == size);
        check_result(buf, size, t->want);
    }
    // measuring a huge result is arithmetic, so it's fast
    assert(text_repeat("ab", 2, INT64_MAX, NULL) == (size_t)INT64_MAX * 2);
    assert(text_repeat("abc", 3, INT64_MAX, NULL) == SIZE_MAX);
    printf("OK\n");
}

typedef struct {
    Str s, old, new;
    int64_t count;
    Str want;
} ReplaceTest;

static void test_replace(void) {
    printf("test_replace...");
    static const ReplaceTest tests[] = {
        {S("hello"), S("l"), S("L"), INT64_MAX, S("heLLo")},
        {S("hello"), S("l"), S("xyz"), INT64_MAX, S("hexyzxyzo")},
        {S("hello"), S("l"), S(""), INT64_MAX, S("heo")},
        {S("hello"), S("hello"), S("-"), INT64_MAX, S("-")},
        {S("aaaa"), S("aa"), S("b"), INT64_MAX, S("bb")},
        {S("aaa"), S("aa"), S("b"), INT64_MAX, S("ba")},
        {S("hello"), S("l"), S("L"), 1, S("heLlo")},
        {S("hello"), S("l"), S("L"), 0, S("hello")},
        {S("hello"), S("l"), S("L"), -1, S("hello")},
        {S("hello"), S("l"), S("L"), INT64_MIN, S("hello")},
        {S("hello"), S("x"), S("L"), INT64_MAX, S("hello")},
        // an empty old matches nothing
        {S("hello"), S(""), S("*"), INT64_MAX, S("hello")},
        {S(""), S("a"), S("b"), INT64_MAX, S("")},
        {S("нетто"), S("т"), S("три"), INT64_MAX, S("нетритрио")},
        {S("a\0a"), S("a"), S("\0"), INT64_MAX, S("\0\0\0")},
        {S("a\xff"
           "b"),
         S("\xff"), S("-"), INT64_MAX, S("a-b")},
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        const ReplaceTest* t = &tests[i];
        size_t size =
            text_replace(t->s.s, t->s.n, t->old.s, t->old.n, t->new.s, t->new.n, t->count, NULL);
        char* buf = alloc(size);
        assert(text_replace(t->s.s, t->s.n, t->old.s, t->old.n, t->new.s, t->new.n, t->count,
                            buf) == size);
        check_result(buf, size, t->want);
    }
    printf("OK\n");
}

typedef struct {
    Str s, from, to;
    Str want;
} TranslateTest;

static void test_translate(void) {
    printf("test_translate...");
    static const TranslateTest tests[] = {
        {S("hello"), S("l"), S("1"), S("he11o")},
        {S("hello"), S("ole"), S("013"), S("h3110")},
        // characters in from without a counterpart in to are deleted
        {S("12345"), S("143"), S("ax"), S("a2x5")},
        {S("hello"), S("l"), S(""), S("heo")},
        // extra characters in to are ignored, and the first match in from counts
        {S("abc"), S("a"), S("xyz"), S("xbc")},
        {S("abc"), S("aa"), S("xy"), S("xbc")},
        {S("hello"), S(""), S("x"), S("hello")},
        {S(""), S("a"), S("b"), S("")},
        // characters of different sizes
        {S("нетто"), S("от"), S("03"), S("не330")},
        {S("abc"), S("b"), S("ж"), S("aжc")},
        {S("a\0b"), S("\0"), S("-"), S("a-b")},
        {S("a\xff"
           "b"),
         S("\xff"), S("\xfe"),
         S("a\xfe"
           "b")},
        // б (D0 B1) is not the ill-formed D0
        {S("б"), S("\xd0"), S("x"), S("б")},
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        const TranslateTest* t = &tests[i];
        size_t size = text_translate(t->s.s, t->s.n, t->from.s, t->from.n, t->to.s, t->to.n, NULL);
        char* buf = alloc(size);
        assert(text_translate(t->s.s, t->s.n, t->from.s, t->from.n, t->to.s, t->to.n, buf) == size);
        check_result(buf, size, t->want);
    }
    printf("OK\n");
}

typedef struct {
    Str s;
    Str want;
} ReverseTest;

static void test_reverse(void) {
    printf("test_reverse...");
    static const ReverseTest tests[] = {
        {S("hello"), S("olleh")},
        {S(""), S("")},
        {S("привет"), S("тевирп")},
        {S("𐌀𐌁𐌂"), S("𐌂𐌁𐌀")},
        {S("a\0b"), S("b\0a")},
        // ill-formed sequences move whole
        {S("A\xe4\xb8"
           "B"),
         S("B\xe4\xb8"
           "A")},
        {S("\xc3"
           "A"),
         S("A\xc3")},
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        const ReverseTest* t = &tests[i];
        char* buf = alloc(t->s.n);
        text_reverse(t->s.s, t->s.n, buf);
        check_result(buf, t->s.n, t->want);
    }
    printf("OK\n");
}

int main(void) {
    test_split();
    test_join();
    test_repeat();
    test_replace();
    test_translate();
    test_reverse();
    return 0;
}
