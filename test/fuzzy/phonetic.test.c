// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Phonetic code tests.

// Each function gets a heap buffer of exactly the advertised size,
// so a sanitizer catches an overflow.

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fuzzy/fuzzy.h"

// S makes a string argument with an explicit length, so it can contain NUL.
#define S(s) s, sizeof(s) - 1

#define LEN(a) (sizeof(a) / sizeof(a[0]))

typedef FuzzyStatus (*Encoder)(const char* s, size_t n, char* buf);

typedef struct {
    const char* s;
    size_t n;
    const char* want;  // NULL for FUZZY_INVALID
} CodeTest;

// check_code encodes t->s into a buffer of size bytes and checks the result.
static void check_code(Encoder encode, const CodeTest* t, size_t size) {
    char* buf = malloc(size);
    assert(buf != NULL);
    FuzzyStatus status = encode(t->s, t->n, buf);
    if (t->want == NULL) {
        assert(status == FUZZY_INVALID);
    } else {
        assert(status == FUZZY_OK);
        assert(strcmp(buf, t->want) == 0);
    }
    free(buf);
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
static const CodeTest caver_tests[] = {
    {S(""), ""},
    {S("awesome"), "AWSM111111"},
    {S("AWESOME"), "AWSM111111"},   // case is ignored
    {S("Gnome"), "NM11111111"},
    {S("e"), "1111111111"},         // all letters are removed
    {S("123"), ""},                 // no letters
    {S("'- !"), ""},
    {S("ab\0c"), "APK1111111"},     // NUL is not a letter
    {S("abcdefghijklmnopqrstuvwxyz"), "APKTFKMNPK"},  // cut to 10
    {S("café"), NULL},              // non-ASCII
    {S("\xff"), NULL},
};

static const CodeTest phonetic_tests[] = {
    {S(""), ""},
    {S("phonetics"), "BAMADAC"},
    {S("PHONETICS"), "BAMADAC"},    // case is ignored
    {S("knight"), "MACD"},
    {S("KNIGHT"), "MACD"},
    {S("Gnome"), "MAMA"},
    {S("WRIGHT"), "RCD"},
    {S("MATCH"), "MAC"},
    {S("EDGE"), "ACA"},
    {S("a1b2"), "A9B9"},            // digits are kept
    {S("123"), "9"},
    {S("café"), NULL},              // non-ASCII
    {S("\xff"), NULL},
};

static const CodeTest soundex_tests[] = {
    {S(""), ""},
    {S("Robert"), "R163"},
    {S("ROBERT"), "R163"},          // case is ignored
    {S("Ashcraft"), "A261"},
    {S("ASHCRAFT"), "A261"},
    {S("a"), "A000"},
    {S("1abc"), "A120"},            // the code starts at the first letter
    {S("123"), ""},                 // no letters
    {S("'- !"), ""},
    {S("ab\0c"), "A120"},           // NUL is not a letter
    {S("café"), NULL},              // non-ASCII
    {S("\xff"), NULL},
};

static const CodeTest rsoundex_tests[] = {
    {S(""), ""},
    {S("robert"), "R901096"},
    {S("ROBERT"), "R901096"},       // case is ignored
    {S("Lloyd"), "L706"},
    {S("a"), "A0"},
    {S("123"), ""},                 // no letters
    {S("'- !"), ""},
    {S("café"), NULL},              // non-ASCII
    {S("\xff"), NULL},
};
// clang-format on

static void test_caver(void) {
    printf("test_caver...");
    for (size_t i = 0; i < LEN(caver_tests); i++) {
        check_code(fuzzy_caver, &caver_tests[i], FUZZY_CAVER_SIZE);
    }
    printf("OK\n");
}

static void test_phonetic(void) {
    printf("test_phonetic...");
    for (size_t i = 0; i < LEN(phonetic_tests); i++) {
        const CodeTest* t = &phonetic_tests[i];
        check_code(fuzzy_phonetic, t, FUZZY_PHONETIC_SIZE(t->n));
    }
    // vowels and consonants alternate, so each character has a code
    for (size_t n = 0; n < 64; n++) {
        char* s = repeat("ba", n);
        char* buf = malloc(FUZZY_PHONETIC_SIZE(n));
        assert(fuzzy_phonetic(s, n, buf) == FUZZY_OK);
        assert(strlen(buf) == n);
        free(buf);
        free(s);
    }
    printf("OK\n");
}

static void test_soundex(void) {
    printf("test_soundex...");
    for (size_t i = 0; i < LEN(soundex_tests); i++) {
        check_code(fuzzy_soundex, &soundex_tests[i], FUZZY_SOUNDEX_SIZE);
    }
    // the code is never longer than FUZZY_SOUNDEX_SIZE - 1
    for (size_t n = 1; n < 64; n++) {
        char* s = repeat("bdl", n);
        char* buf = malloc(FUZZY_SOUNDEX_SIZE);
        assert(fuzzy_soundex(s, n, buf) == FUZZY_OK);
        assert(strlen(buf) == FUZZY_SOUNDEX_SIZE - 1);
        free(buf);
        free(s);
    }
    printf("OK\n");
}

static void test_rsoundex(void) {
    printf("test_rsoundex...");
    for (size_t i = 0; i < LEN(rsoundex_tests); i++) {
        const CodeTest* t = &rsoundex_tests[i];
        check_code(fuzzy_rsoundex, t, FUZZY_RSOUNDEX_SIZE(t->n));
    }
    // the codes alternate, so each character has a digit after the first letter
    for (size_t n = 0; n < 64; n++) {
        char* s = repeat("ab", n);
        char* buf = malloc(FUZZY_RSOUNDEX_SIZE(n));
        assert(fuzzy_rsoundex(s, n, buf) == FUZZY_OK);
        assert(strlen(buf) == (n == 0 ? 0 : n + 1));
        free(buf);
        free(s);
    }
    printf("OK\n");
}

int main(void) {
    test_caver();
    test_phonetic();
    test_soundex();
    test_rsoundex();
}
