// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Case tests.

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

typedef struct {
    Str s;
    Str upper, lower, title, fold;
} CaseTest;

// check_case measures and writes the result of change, and checks it against want.
static void check_case(size_t (*change)(const char*, size_t, char*), Str s, Str want) {
    size_t size = change(s.s, s.n, NULL);
    if (size != want.n) {
        printf("%.*s: size %zu, want %zu\n", (int)s.n, s.s, size, want.n);
    }
    assert(size == want.n);
    char* buf = malloc(size > 0 ? size : 1);
    assert(buf != NULL);
    assert(change(s.s, s.n, buf) == size);
    if (memcmp(buf, want.s, size) != 0) {
        printf("%.*s: got %.*s, want %.*s\n", (int)s.n, s.s, (int)size, buf, (int)want.n, want.s);
    }
    assert(memcmp(buf, want.s, size) == 0);
    free(buf);
}

static void test_case(void) {
    printf("test_case...");
    static const CaseTest tests[] = {
        {S("hello World"), S("HELLO WORLD"), S("hello world"), S("Hello World"), S("hello world")},
        {S("пРиВеТ мир"), S("ПРИВЕТ МИР"), S("привет мир"), S("Привет Мир"), S("привет мир")},
        {S(""), S(""), S(""), S(""), S("")},
        // the size changes: ß (2 bytes) → ẞ (3), İ (2) → i (1), ⱥ (3) → Ⱥ (2)
        {S("ßx"), S("ẞX"), S("ßx"), S("ẞx"), S("ßx")},
        {S("İx"), S("İX"), S("ix"), S("İx"), S("İx")},
        {S("ⱥb"), S("ȺB"), S("ⱥb"), S("Ⱥb"), S("ⱥb")},
        {S("aK"), S("AK"), S("ak"), S("Ak"), S("ak")},  // kelvin sign
        // digraphs have a titlecase form
        {S("ǆemal ǳ"), S("ǄEMAL Ǳ"), S("ǆemal ǳ"), S("ǅemal ǲ"), S("ǆemal ǳ")},
        // a word is a run of letters, digits and underscores
        {S("don't stop-me now_x 1st"), S("DON'T STOP-ME NOW_X 1ST"), S("don't stop-me now_x 1st"),
         S("Don'T Stop-Me Now_x 1st"), S("don't stop-me now_x 1st")},
        {S("x²y"), S("X²Y"), S("x²y"), S("X²Y"), S("x²y")},
        // zero bytes and ill-formed sequences are kept, and end a word
        {S("a\0b"), S("A\0B"), S("a\0b"), S("A\0B"), S("a\0b")},
        {S("a\xff"
           "b"),
         S("A\xff"
           "B"),
         S("a\xff"
           "b"),
         S("A\xff"
           "B"),
         S("a\xff"
           "b")},
        {S("\xe4\xb8"
           "Ab"),
         S("\xe4\xb8"
           "AB"),
         S("\xe4\xb8"
           "ab"),
         S("\xe4\xb8"
           "Ab"),
         S("\xe4\xb8"
           "ab")},
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        const CaseTest* t = &tests[i];
        check_case(text_upper, t->s, t->upper);
        check_case(text_lower, t->s, t->lower);
        check_case(text_title, t->s, t->title);
        check_case(text_casefold, t->s, t->fold);
    }
    printf("OK\n");
}

typedef struct {
    Str a, b;
    int want;
} CompareTest;

static void test_casecmp(void) {
    printf("test_casecmp...");
    static const CompareTest tests[] = {
        {S("hello"), S("HeLLo"), 0},
        {S("привет"), S("ПРИВЕТ"), 0},
        {S("ſ"), S("S"), 0},
        {S("a"), S("b"), -1},
        {S("B"), S("a"), 1},
        {S(""), S(""), 0},
        {S(""), S("a"), -1},
        {S("ab"), S("a"), 1},
        // the string that runs out first is smaller, whatever the byte sizes
        {S("ſx"), S("sxx"), -1},
        {S("s"), S("ſx"), -1},
        // zero bytes are compared too
        {S("a\0b"), S("A\0c"), -1},
        {S("a\0B"), S("A\0b"), 0},
        {S("a\0"), S("a"), 1},
        // ill-formed sequences only equal the same bytes, and sort after characters
        {S("\xff"), S("\xff"), 0},
        {S("\xe4\xb8"), S("\xff"), -1},
        {S("\xff"), S("\U0010FFFF"), 1},
        {S("a\xff"
           "B"),
         S("A\xff"
           "b"),
         0},
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        const CompareTest* t = &tests[i];
        int got = text_casecmp(t->a.s, t->a.n, t->b.s, t->b.n);
        int rev = text_casecmp(t->b.s, t->b.n, t->a.s, t->a.n);
        if ((got > 0) - (got < 0) != t->want) {
            printf("#%zu: got %d, want %d\n", i, got, t->want);
        }
        assert((got > 0) - (got < 0) == t->want);
        assert((rev > 0) - (rev < 0) == -t->want);
    }
    printf("OK\n");
}

int main(void) {
    test_case();
    test_casecmp();
    return 0;
}
