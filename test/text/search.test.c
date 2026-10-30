// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Search tests.

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "text/text.h"

#define LEN(a) (sizeof(a) / sizeof(a[0]))

// Str is a string literal with its size, so it may contain zero bytes.
typedef struct {
    const char* s;
    size_t n;
} Str;

#define S(x) {x, sizeof(x) - 1}

typedef struct {
    Str s, sub;
    int64_t index, last, count;
} FindTest;

static void test_find(void) {
    printf("test_find...");
    static const FindTest tests[] = {
        {S("hello"), S("l"), 3, 4, 2},
        {S("hello"), S("hello"), 1, 1, 1},
        {S("hello"), S("x"), 0, 0, 0},
        {S("hello"), S("hello!"), 0, 0, 0},
        {S("aaaa"), S("aa"), 1, 3, 2},
        {S("abab"), S("ab"), 1, 3, 2},
        {S("привет мир"), S("мир"), 8, 8, 1},
        {S("мир мир"), S("мир"), 1, 5, 2},
        // an empty sub is before each character and at the end
        {S("hello"), S(""), 1, 6, 6},
        {S("привет"), S(""), 1, 7, 7},
        {S(""), S(""), 1, 1, 1},
        {S(""), S("a"), 0, 0, 0},
        // zero bytes are characters
        {S("a\0b\0"), S("\0"), 2, 4, 2},
        {S("a\0b"), S("b"), 3, 3, 1},
        // the search compares bytes, and positions count ill-formed subparts
        {S("\xe4\xb8"
           "a\xff"
           "a"),
         S("a"), 2, 4, 2},
        {S("A\xff"
           "B"),
         S("\xff"), 2, 2, 1},
        {S("A\xff"
           "B"),
         S("\xfe"), 0, 0, 0},
        // a byte inside a character is still found
        {S("б"), S("\xb1"), 1, 1, 1},
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        const FindTest* t = &tests[i];
        int64_t index = text_index(t->s.s, t->s.n, t->sub.s, t->sub.n);
        int64_t last = text_last_index(t->s.s, t->s.n, t->sub.s, t->sub.n);
        int64_t count = text_count(t->s.s, t->s.n, t->sub.s, t->sub.n);
        if (index != t->index || last != t->last || count != t->count) {
            printf("#%zu: index %lld, last %lld, count %lld\n", i, (long long)index,
                   (long long)last, (long long)count);
        }
        assert(index == t->index);
        assert(last == t->last);
        assert(count == t->count);
        assert(text_contains(t->s.s, t->s.n, t->sub.s, t->sub.n) == (t->index > 0));
    }
    printf("OK\n");
}

typedef struct {
    Str s, affix;
    bool prefix, suffix;
} AffixTest;

static void test_affix(void) {
    printf("test_affix...");
    static const AffixTest tests[] = {
        {S("hello"), S("he"), true, false},
        {S("hello"), S("lo"), false, true},
        {S("hello"), S("hello"), true, true},
        {S("hello"), S("Hello"), false, false},
        {S("he"), S("hello"), false, false},
        {S("hello"), S(""), true, true},
        {S(""), S(""), true, true},
        {S(""), S("a"), false, false},
        {S("привет"), S("пр"), true, false},
        {S("a\0b"), S("\0b"), false, true},
        {S("\xff"
           "a"),
         S("\xff"), true, false},
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        const AffixTest* t = &tests[i];
        assert(text_has_prefix(t->s.s, t->s.n, t->affix.s, t->affix.n) == t->prefix);
        assert(text_has_suffix(t->s.s, t->s.n, t->affix.s, t->affix.n) == t->suffix);
    }
    printf("OK\n");
}

typedef struct {
    Str p, s;
    bool want;
} LikeTest;

static void test_like(void) {
    printf("test_like...");
    static const LikeTest tests[] = {
        {S("hello"), S("hello"), true},
        {S("hello"), S("HeLLo"), true},
        {S("hello"), S("hell"), false},
        {S("hell"), S("hello"), false},
        {S("h%"), S("hello"), true},
        {S("%o"), S("hello"), true},
        {S("%a"), S("a"), true},
        {S("a%"), S("a"), true},
        {S("%"), S(""), true},
        {S("%%"), S(""), true},
        {S(""), S(""), true},
        {S(""), S("a"), false},
        {S("_"), S(""), false},
        {S("_"), S("é"), true},
        {S("__"), S("é"), false},
        {S("h_llo"), S("hello"), true},
        {S("%l_o"), S("hello"), true},
        {S("%ll%"), S("hello"), true},
        {S("%lx%"), S("hello"), false},
        // backtracking to the last %
        {S("%ab"), S("aab"), true},
        {S("%a_c"), S("abaabc"), true},
        {S("a%b%c"), S("aXbYc"), true},
        {S("a%b%c"), S("aXbYb"), false},
        {S("%aaa"), S("aaaaaaaaaab"), false},
        // case-insensitive for all of Unicode
        {S("прив_т"), S("пРиВеТ"), true},
        {S("ſ"), S("S"), true},
        {S("k"), S("K"), true},  // kelvin sign
        // no escape character
        {S("a\\%"), S("a%"), false},
        {S("a\\%"), S("a\\bc"), true},
        // zero bytes and ill-formed sequences are characters
        {S("a\0_"), S("A\0b"), true},
        {S("\0"), S(""), false},
        {S("a_b"),
         S("A\xff"
           "B"),
         true},
        {S("a\xff"), S("A\xff"), true},
        {S("a\xfe"), S("A\xff"), false},
        {S("_"), S("\xe4\xb8"), true},
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        const LikeTest* t = &tests[i];
        if (text_like(t->p.s, t->p.n, t->s.s, t->s.n) != t->want) {
            printf("#%zu: want %d\n", i, t->want);
        }
        assert(text_like(t->p.s, t->p.n, t->s.s, t->s.n) == t->want);
    }
    printf("OK\n");
}

int main(void) {
    test_find();
    test_affix();
    test_like();
    return 0;
}
