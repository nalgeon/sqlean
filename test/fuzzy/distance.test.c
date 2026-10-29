// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// String distance tests.

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fuzzy/fuzzy.h"

// S makes a string argument with an explicit length, so it can contain NUL.
#define S(s) s, sizeof(s) - 1

typedef struct {
    const char* a;
    size_t na;
    const char* b;
    size_t nb;
    size_t want;
} DistanceTest;

typedef struct {
    const char* a;
    size_t na;
    const char* b;
    size_t nb;
    double want;
} SimilarityTest;

typedef struct {
    const char* a;
    size_t na;
    const char* b;
    size_t nb;
    int64_t want;
} EditdistTest;

// check_symmetric checks that distance(a, b) is want, and so is distance(b, a).
static void check_symmetric(
    FuzzyStatus (*distance)(const char*, size_t, const char*, size_t, size_t*),
    const DistanceTest* t) {
    size_t dist = SIZE_MAX;
    assert(distance(t->a, t->na, t->b, t->nb, &dist) == FUZZY_OK);
    assert(dist == t->want);
    dist = SIZE_MAX;
    assert(distance(t->b, t->nb, t->a, t->na, &dist) == FUZZY_OK);
    assert(dist == t->want);
}

// repeat returns a string of n bytes repeating pattern. Free it.
static char* repeat(const char* pattern, size_t n) {
    char* s = malloc(n + 1);
    assert(s != NULL);
    size_t plen = strlen(pattern);
    for (size_t i = 0; i < n; i++) {
        s[i] = pattern[i % plen];
    }
    s[n] = 0;
    return s;
}

// clang-format off
static const DistanceTest damlev_tests[] = {
    {S(""), S(""), 0},
    {S("abc"), S(""), 3},
    {S("abc"), S("abc"), 0},
    {S("abc"), S("ab"), 1},
    {S("kitten"), S("sitting"), 3},
    {S("awesome"), S("aewsme"), 2},
    {S("ab"), S("ba"), 1},          // a transposition
    {S("abc"), S("ca"), 2},         // a substring edited twice
    {S("abc"), S("ABC"), 3},        // case-sensitive
    {S("привет"), S("пирвет"), 1},  // characters, not bytes
    {S("日本語"), S("日本"), 1},
    {S("\xff"), S("\xfe"), 0},      // both are U+FFFD
    {S("ab\0c"), S("ab"), 2},       // NUL is a character
};

static const DistanceTest leven_tests[] = {
    {S(""), S(""), 0},
    {S("abc"), S(""), 3},
    {S("abc"), S("abc"), 0},
    {S("abc"), S("ab"), 1},
    {S("kitten"), S("sitting"), 3},
    {S("flaw"), S("lawn"), 2},
    {S("ab"), S("ba"), 2},          // no transpositions
    {S("abc"), S("ca"), 3},
    {S("café"), S("cafe"), 1},      // characters, not bytes
    {S("😀"), S("😃"), 1},
    {S("\xff"), S("a"), 1},         // U+FFFD vs 'a'
    {S("ab\0c"), S("ab"), 2},       // NUL is a character
};

static const DistanceTest osadist_tests[] = {
    {S(""), S(""), 0},
    {S("abc"), S(""), 3},
    {S("abc"), S("abc"), 0},
    {S("abc"), S("ab"), 1},
    {S("kitten"), S("sitting"), 3},
    {S("ab"), S("ba"), 1},          // a transposition
    {S("martha"), S("marhta"), 1},
    {S("abc"), S("ca"), 3},         // a substring is edited at most once
    {S("awesome"), S("aewsme"), 2},
    {S("привет"), S("пирвет"), 1},  // characters, not bytes
    {S("ab\0c"), S("ab"), 2},       // NUL is a character
};

static const DistanceTest hamming_tests[] = {
    {S(""), S(""), 0},
    {S("abc"), S("abc"), 0},
    {S("hello"), S("hellp"), 1},
    {S("karolin"), S("kathrin"), 3},
    {S("abc"), S("ABC"), 3},        // case-sensitive
    {S("café"), S("cafe"), 1},      // characters, not bytes
    {S("\xff\xff"), S("ab"), 2},    // each invalid byte is a character
    {S("ab\0"), S("abc"), 1},       // NUL is a character
};

static const DistanceTest hamming_invalid[] = {
    {S("abc"), S(""), 0},
    {S("abc"), S("abcd"), 0},
    {S("café"), S("caf"), 0},       // 4 characters vs 3
    {S("ab\0c"), S("ab"), 0},
};

static const SimilarityTest jarowin_tests[] = {
    {S(""), S(""), 1.0},            // equal strings (go-edlib gives 0)
    {S("abc"), S(""), 0.0},
    {S("abc"), S("abc"), 1.0},
    {S("abc"), S("xyz"), 0.0},
    {S("martha"), S("marhta"), 0.961111},
    {S("dwayne"), S("duane"), 0.84},
    {S("dixon"), S("dicksonx"), 0.813333},
    {S("ab"), S("b"), 0.0},         // 'b' is outside the match window
    {S("abcdefgh"), S("abcdefhg"), 0.975},  // the prefix boost stops at 4
    {S("café"), S("cafe"), 0.883333},       // characters, not bytes
};

static const EditdistTest editdist_tests[] = {
    {S(""), S(""), 0},
    {S("abc"), S("abc"), 0},
    {S("abc"), S(""), 300},
    {S(""), S("abc"), 75},          // inserting at the end is cheap
    {S("abc"), S("ABC"), 0},        // case is ignored
    {S("bat"), S("bad"), 40},       // the same class
    {S("bat"), S("bap"), 75},       // consonants of different classes
    {S("bat"), S("ba1"), 100},
    {S("kitten"), S("sitting"), 105},
    {S("ab*"), S("abxyz"), 0},      // a prefix
    {S("ax*"), S("abxyz"), 75},
    {S("ab\0c"), S("ab"), 200},     // NUL is a character
};

static const EditdistTest editdist_invalid[] = {
    {S("é"), S("e"), 0},
    {S("e"), S("é"), 0},
    {S("\x80"), S(""), 0},
};
// clang-format on

#define LEN(a) (sizeof(a) / sizeof(a[0]))

static void test_damlev(void) {
    printf("test_damlev...");
    for (size_t i = 0; i < LEN(damlev_tests); i++) {
        check_symmetric(fuzzy_damlev, &damlev_tests[i]);
    }
    printf("OK\n");
}

static void test_editdist(void) {
    printf("test_editdist...");
    for (size_t i = 0; i < LEN(editdist_tests); i++) {
        const EditdistTest* t = &editdist_tests[i];
        int64_t dist = -1;
        assert(fuzzy_editdist(t->a, t->na, t->b, t->nb, &dist) == FUZZY_OK);
        assert(dist == t->want);
    }
    for (size_t i = 0; i < LEN(editdist_invalid); i++) {
        const EditdistTest* t = &editdist_invalid[i];
        int64_t dist;
        assert(fuzzy_editdist(t->a, t->na, t->b, t->nb, &dist) == FUZZY_INVALID);
    }
    printf("OK\n");
}

static void test_hamming(void) {
    printf("test_hamming...");
    for (size_t i = 0; i < LEN(hamming_tests); i++) {
        check_symmetric(fuzzy_hamming, &hamming_tests[i]);
    }
    for (size_t i = 0; i < LEN(hamming_invalid); i++) {
        const DistanceTest* t = &hamming_invalid[i];
        size_t dist;
        assert(fuzzy_hamming(t->a, t->na, t->b, t->nb, &dist) == FUZZY_INVALID);
        assert(fuzzy_hamming(t->b, t->nb, t->a, t->na, &dist) == FUZZY_INVALID);
    }
    printf("OK\n");
}

static void test_jarowin(void) {
    printf("test_jarowin...");
    for (size_t i = 0; i < LEN(jarowin_tests); i++) {
        const SimilarityTest* t = &jarowin_tests[i];
        double sim = -1;
        assert(fuzzy_jarowin(t->a, t->na, t->b, t->nb, &sim) == FUZZY_OK);
        assert(fabs(sim - t->want) < 1e-6);
        // symmetric
        sim = -1;
        assert(fuzzy_jarowin(t->b, t->nb, t->a, t->na, &sim) == FUZZY_OK);
        assert(fabs(sim - t->want) < 1e-6);
    }
    printf("OK\n");
}

static void test_leven(void) {
    printf("test_leven...");
    for (size_t i = 0; i < LEN(leven_tests); i++) {
        check_symmetric(fuzzy_leven, &leven_tests[i]);
    }
    printf("OK\n");
}

static void test_osadist(void) {
    printf("test_osadist...");
    for (size_t i = 0; i < LEN(osadist_tests); i++) {
        check_symmetric(fuzzy_osadist, &osadist_tests[i]);
    }
    printf("OK\n");
}

// test_long checks the functions on long strings, which have
// the largest matrices and distances.
static void test_long(void) {
    printf("test_long...");
    const size_t n = 2000;
    char* a = repeat("a", n);
    char* b = repeat("b", n);
    char* ab = repeat("ab", n);
    char* ba = repeat("ba", n);

    DistanceTest diff = {a, n, b, n, n};
    check_symmetric(fuzzy_damlev, &diff);
    check_symmetric(fuzzy_hamming, &diff);
    check_symmetric(fuzzy_leven, &diff);
    check_symmetric(fuzzy_osadist, &diff);

    // a shift by one character: a deletion and an insertion
    DistanceTest shift = {ab, n, ba, n, 2};
    check_symmetric(fuzzy_damlev, &shift);
    check_symmetric(fuzzy_leven, &shift);
    check_symmetric(fuzzy_osadist, &shift);
    DistanceTest shift_hamming = {ab, n, ba, n, n};
    check_symmetric(fuzzy_hamming, &shift_hamming);

    DistanceTest empty = {a, n, "", 0, n};
    check_symmetric(fuzzy_damlev, &empty);
    check_symmetric(fuzzy_leven, &empty);
    check_symmetric(fuzzy_osadist, &empty);

    double sim;
    assert(fuzzy_jarowin(a, n, b, n, &sim) == FUZZY_OK && sim == 0.0);
    assert(fuzzy_jarowin(a, n, a, n, &sim) == FUZZY_OK && sim == 1.0);

    int64_t dist;
    assert(fuzzy_editdist(a, n, b, n, &dist) == FUZZY_OK && dist > 0);
    assert(fuzzy_editdist(a, n, "", 0, &dist) == FUZZY_OK && dist > 0);

    free(a);
    free(b);
    free(ab);
    free(ba);
    printf("OK\n");
}

int main(void) {
    test_damlev();
    test_editdist();
    test_hamming();
    test_jarowin();
    test_leven();
    test_osadist();
    test_long();
}
