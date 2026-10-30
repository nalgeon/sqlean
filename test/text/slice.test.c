// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Substring, trim and pad tests.

#include <assert.h>
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

// span_is reports whether span is the part of s equal to want.
static int span_is(Str s, TextSpan span, Str want) {
    return span.start + span.len <= s.n && span.len == want.n &&
           memcmp(s.s + span.start, want.s, want.n) == 0;
}

typedef struct {
    Str s;
    int64_t a, b;
    Str want;
} RangeTest;

static void test_substring(void) {
    printf("test_substring...");
    static const RangeTest tests[] = {
        {S("hello"), 1, 5, S("hello")},
        {S("hello"), 2, 3, S("ell")},
        {S("hello"), 5, 1, S("o")},
        {S("hello"), 6, 1, S("")},
        {S("hello"), 1, 0, S("")},
        {S("hello"), 1, -1, S("")},
        {S("hello"), 0, 2, S("h")},
        {S("hello"), -1, 3, S("h")},
        {S("hello"), -2, 3, S("")},
        {S("hello"), INT64_MIN, 5, S("")},
        {S("hello"), INT64_MIN, INT64_MAX, S("")},
        {S("hello"), -1, INT64_MAX, S("hello")},
        {S("hello"), INT64_MAX, INT64_MAX, S("")},
        {S("hello"), 2, INT64_MAX, S("ello")},
        {S("hello"), 1, INT64_MIN, S("")},
        {S("привет"), 2, 3, S("рив")},
        {S("a\0b"), 2, 2, S("\0b")},
        {S("\xe4\xb8"
           "a"),
         2, 1, S("a")},
        {S(""), 1, 1, S("")},
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        const RangeTest* t = &tests[i];
        assert(span_is(t->s, text_substring(t->s.s, t->s.n, t->a, t->b), t->want));
    }
    printf("OK\n");
}

static void test_slice(void) {
    printf("test_slice...");
    static const RangeTest tests[] = {
        {S("hello"), 1, 6, S("hello")},
        {S("hello"), 2, 4, S("el")},
        {S("hello"), 0, 3, S("he")},
        {S("hello"), 1, 0, S("")},
        {S("hello"), 0, 0, S("")},
        {S("hello"), -2, 6, S("lo")},
        {S("hello"), 1, -1, S("hell")},
        {S("hello"), -3, -1, S("ll")},
        {S("hello"), -1, -3, S("")},
        {S("hello"), 4, 2, S("")},
        {S("hello"), 10, 20, S("")},
        {S("hello"), INT64_MIN, INT64_MAX, S("hello")},
        {S("hello"), INT64_MAX, INT64_MIN, S("")},
        {S("hello"), 1, INT64_MIN, S("")},
        {S("привет"), -3, INT64_MAX, S("вет")},
        {S("a\0b"), -2, INT64_MAX, S("\0b")},
        {S(""), 1, 2, S("")},
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        const RangeTest* t = &tests[i];
        assert(span_is(t->s, text_slice(t->s.s, t->s.n, t->a, t->b), t->want));
    }
    printf("OK\n");
}

typedef struct {
    Str s;
    int64_t count;
    Str left, right;
} PartTest;

static void test_left_right(void) {
    printf("test_left_right...");
    static const PartTest tests[] = {
        {S("hello"), 0, S(""), S("")},
        {S("hello"), 2, S("he"), S("lo")},
        {S("hello"), 5, S("hello"), S("hello")},
        {S("hello"), 9, S("hello"), S("hello")},
        {S("hello"), -1, S("hell"), S("ello")},
        {S("hello"), -5, S(""), S("")},
        {S("hello"), -9, S(""), S("")},
        {S("hello"), INT64_MAX, S("hello"), S("hello")},
        {S("hello"), INT64_MIN, S(""), S("")},
        {S("привет"), 2, S("пр"), S("ет")},
        {S("\xff"
           "ab\xc3"),
         2,
         S("\xff"
           "a"),
         S("b\xc3")},
        {S(""), 1, S(""), S("")},
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        const PartTest* t = &tests[i];
        assert(span_is(t->s, text_left(t->s.s, t->s.n, t->count), t->left));
        assert(span_is(t->s, text_right(t->s.s, t->s.n, t->count), t->right));
    }
    printf("OK\n");
}

typedef struct {
    Str s, chars;
    Str left, right, both;
} TrimTest;

static void test_trim(void) {
    printf("test_trim...");
    static const TrimTest tests[] = {
        {S("  ab  "), S(" "), S("ab  "), S("  ab"), S("ab")},
        {S("ab"), S(" "), S("ab"), S("ab"), S("ab")},
        {S("   "), S(" "), S(""), S(""), S("")},
        {S(""), S(" "), S(""), S(""), S("")},
        {S(" ab "), S(""), S(" ab "), S(" ab "), S(" ab ")},
        {S("xyaxby"), S("yx"), S("axby"), S("xyaxb"), S("axb")},
        {S("хоприветох"), S("ох"), S("приветох"), S("хопривет"), S("привет")},
        // characters match whole, not by bytes: б (D0 B1) is not in "в" (D0 B2)
        {S("бв"), S("в"), S("бв"), S("б"), S("б")},
        {S("\0a\0"), S("\0"), S("a\0"), S("\0a"), S("a")},
        {S("\xff"
           "a\xff"),
         S("\xff"), S("a\xff"),
         S("\xff"
           "a"),
         S("a")},
        // ill-formed sequences only match the same bytes
        {S("\xe4\xb8"
           "a"),
         S("\xe4"),
         S("\xe4\xb8"
           "a"),
         S("\xe4\xb8"
           "a"),
         S("\xe4\xb8"
           "a")},
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        const TrimTest* t = &tests[i];
        assert(span_is(t->s, text_ltrim(t->s.s, t->s.n, t->chars.s, t->chars.n), t->left));
        assert(span_is(t->s, text_rtrim(t->s.s, t->s.n, t->chars.s, t->chars.n), t->right));
        assert(span_is(t->s, text_trim(t->s.s, t->s.n, t->chars.s, t->chars.n), t->both));
    }
    printf("OK\n");
}

typedef struct {
    Str s;
    int64_t length;
    Str fill;
    Str left, right;
} PadTest;

// check_pad measures and writes the result of pad, and checks it against want.
static void check_pad(size_t (*pad)(const char*, size_t, int64_t, const char*, size_t, char*),
                      const PadTest* t,
                      Str want) {
    size_t size = pad(t->s.s, t->s.n, t->length, t->fill.s, t->fill.n, NULL);
    assert(size == want.n);
    char* buf = malloc(size + 1);
    assert(buf != NULL);
    assert(pad(t->s.s, t->s.n, t->length, t->fill.s, t->fill.n, buf) == size);
    assert(memcmp(buf, want.s, size) == 0);
    free(buf);
}

static void test_pad(void) {
    printf("test_pad...");
    static const PadTest tests[] = {
        {S("ab"), 5, S(" "), S("   ab"), S("ab   ")},
        {S("ab"), 5, S("xy"), S("xyxab"), S("abxyx")},
        {S("ab"), 2, S("*"), S("ab"), S("ab")},
        {S("abc"), 2, S("*"), S("ab"), S("ab")},
        {S("abc"), 0, S("*"), S(""), S("")},
        {S("abc"), -1, S("*"), S(""), S("")},
        {S("abc"), INT64_MIN, S("*"), S(""), S("")},
        {S("ab"), 5, S(""), S("ab"), S("ab")},
        {S("ab"), INT64_MAX, S(""), S("ab"), S("ab")},
        {S(""), 3, S("*"), S("***"), S("***")},
        {S("мир"), 6, S("хо"), S("хохмир"), S("мирхох")},
        {S("\xff"), 2, S("\0"), S("\0\xff"), S("\xff\0")},
        {S("a"), 3, S("\xe4\xb8"),
         S("\xe4\xb8\xe4\xb8"
           "a"),
         S("a\xe4\xb8\xe4\xb8")},
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        check_pad(text_lpad, &tests[i], tests[i].left);
        check_pad(text_rpad, &tests[i], tests[i].right);
    }
    // measuring a huge result is arithmetic, so it's fast
    assert(text_lpad("", 0, INT64_MAX, "ab", 2, NULL) == (size_t)INT64_MAX);
    assert(text_rpad("a", 1, INT64_MAX, "abc", 3, NULL) == (size_t)INT64_MAX);
    assert(text_lpad("", 0, INT64_MAX, "日本", 6, NULL) == SIZE_MAX);
    printf("OK\n");
}

typedef struct {
    Str s;
    int64_t want;
} LengthTest;

static void test_length(void) {
    printf("test_length...");
    static const LengthTest tests[] = {
        {S(""), 0},
        {S("hello"), 5},
        {S("привет"), 6},
        {S("𐌀𐌁"), 2},
        {S("a\0b"), 3},
        // each maximal ill-formed subpart is one character
        {S("\x80"), 1},
        {S("\xc3"), 1},
        {S("\xe4\xb8"), 1},
        {S("\xf0\x9f\x98"), 1},
        {S("\xc3"
           "A"),
         2},
        {S("\xe0\x80"), 2},          // E0 needs A0..BF next
        {S("\xed\xa0\x80"), 3},      // a surrogate
        {S("\xf4\x90\x80\x80"), 4},  // above U+10FFFF
        {S("\xf5"), 1},
        {S("\xc0\xaf"), 2},  // overlong
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        assert(text_length(tests[i].s.s, tests[i].s.n) == tests[i].want);
    }
    printf("OK\n");
}

int main(void) {
    test_substring();
    test_slice();
    test_left_right();
    test_trim();
    test_pad();
    test_length();
    return 0;
}
