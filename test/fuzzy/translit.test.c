// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Transliteration and script tests.

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fuzzy/fuzzy.h"

// S makes a string argument with an explicit length, so it can contain NUL.
#define S(s) s, sizeof(s) - 1

#define LEN(a) (sizeof(a) / sizeof(a[0]))

typedef struct {
    const char* s;
    size_t n;
    const char* want;
    size_t nwant;
} TranslitTest;

typedef struct {
    const char* s;
    size_t n;
    FuzzyScript want;
} ScriptTest;

// clang-format off
static const TranslitTest translit_tests[] = {
    {S(""), S("")},
    {S("abc"), S("abc")},
    {S("crème brûlée"), S("creme brulee")},
    {S("привет"), S("privet")},
    {S("Ελλάδα"), S("Ellada")},
    {S("Straße"), S("Strasse")},
    {S("щ"), S("shch")},            // the longest expansion
    {S("ab\0c"), S("ab\0c")},       // NUL is kept
    {S("\xff"), S("?")},            // an invalid byte
    {S("a\xe2\x82"), S("a??")},    // a truncated character: two invalid bytes
};

static const ScriptTest script_tests[] = {
    {S(""), FUZZY_SCRIPT_UNKNOWN},
    {S("hello"), FUZZY_SCRIPT_LATIN},
    {S("Ærøskøbing"), FUZZY_SCRIPT_LATIN},  // Latin beyond ASCII
    {S("ʯ"), FUZZY_SCRIPT_LATIN},           // the last of IPA Extensions
    {S("привет"), FUZZY_SCRIPT_CYRILLIC},
    {S("Ελλάδα"), FUZZY_SCRIPT_GREEK},
    {S("שלום"), FUZZY_SCRIPT_HEBREW},
    {S("مرحبا"), FUZZY_SCRIPT_ARABIC},
    {S("hello привет"), FUZZY_SCRIPT_MIXED},
    {S("123"), FUZZY_SCRIPT_LATIN},         // digits count as Latin
    {S("'"), FUZZY_SCRIPT_UNKNOWN},          // punctuation counts as nothing
    {S("«»"), FUZZY_SCRIPT_UNKNOWN},
    {S("×÷"), FUZZY_SCRIPT_UNKNOWN},         // not letters, though in Latin-1
    {S("«привет»"), FUZZY_SCRIPT_CYRILLIC},
    {S("日本語"), FUZZY_SCRIPT_UNKNOWN},
    {S("\xff\xfe"), FUZZY_SCRIPT_UNKNOWN},   // invalid bytes
    {S("ab\0c"), FUZZY_SCRIPT_LATIN},       // NUL is ignored
};
// clang-format on

static void test_translit(void) {
    printf("test_translit...");
    for (size_t i = 0; i < LEN(translit_tests); i++) {
        const TranslitTest* t = &translit_tests[i];
        char* buf = malloc(FUZZY_TRANSLIT_SIZE(t->n) + 1);
        assert(buf != NULL);
        size_t len = fuzzy_translit(t->s, t->n, buf);
        assert(len == t->nwant);
        assert(memcmp(buf, t->want, len) == 0);
        free(buf);
    }
    printf("OK\n");
}

// encode_utf8 writes the UTF-8 encoding of c into buf and returns its length.
static size_t encode_utf8(uint32_t c, char* buf) {
    if (c < 0x80) {
        buf[0] = (char)c;
        return 1;
    }
    if (c < 0x800) {
        buf[0] = (char)(0xC0 | (c >> 6));
        buf[1] = (char)(0x80 | (c & 0x3F));
        return 2;
    }
    if (c < 0x10000) {
        buf[0] = (char)(0xE0 | (c >> 12));
        buf[1] = (char)(0x80 | ((c >> 6) & 0x3F));
        buf[2] = (char)(0x80 | (c & 0x3F));
        return 3;
    }
    buf[0] = (char)(0xF0 | (c >> 18));
    buf[1] = (char)(0x80 | ((c >> 12) & 0x3F));
    buf[2] = (char)(0x80 | ((c >> 6) & 0x3F));
    buf[3] = (char)(0x80 | (c & 0x3F));
    return 4;
}

static void test_translit_size(void) {
    printf("test_translit_size...");
    // every code point, including surrogates (which are invalid UTF-8)
    size_t worst = 0;
    for (uint32_t c = 0; c <= 0x10FFFF; c++) {
        char s[4];
        size_t n = encode_utf8(c, s);
        char buf[FUZZY_TRANSLIT_SIZE(4)];
        size_t len = fuzzy_translit(s, n, buf);
        assert(len <= FUZZY_TRANSLIT_SIZE(n));
        if (len == FUZZY_TRANSLIT_SIZE(n)) {
            worst++;
        }
    }
    // the size is not larger than it has to be
    assert(worst > 0);

    // the worst case in a heap buffer of the exact size
    const char* s = "щщщ";
    size_t n = strlen(s);
    char* buf = malloc(FUZZY_TRANSLIT_SIZE(n));
    assert(fuzzy_translit(s, n, buf) == FUZZY_TRANSLIT_SIZE(n));
    assert(memcmp(buf, "shchshchshch", FUZZY_TRANSLIT_SIZE(n)) == 0);
    free(buf);
    printf("OK\n");
}

static void test_script(void) {
    printf("test_script...");
    for (size_t i = 0; i < LEN(script_tests); i++) {
        const ScriptTest* t = &script_tests[i];
        assert(fuzzy_script(t->s, t->n) == t->want);
    }
    printf("OK\n");
}

int main(void) {
    test_translit();
    test_translit_size();
    test_script();
}
