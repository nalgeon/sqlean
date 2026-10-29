// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Fuzzy matching tests.

// These tests check the buffer sizes of the domain layer against
// the worst cases.

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fuzzy/fuzzy.h"

#pragma region Buffer sizes

// The phonetic functions get a heap buffer of exactly the advertised size,
// so a sanitizer catches an overflow.

// repeat returns a string of n bytes repeating pattern.
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

static void test_caver_size(void) {
    printf("test_caver_size...");
    const char* inputs[] = {"", "a", "awesome", "abcdefghijklmnopqrstuvwxyz"};
    for (size_t i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++) {
        char* buf = malloc(FUZZY_CAVER_SIZE);
        assert(fuzzy_caver(inputs[i], strlen(inputs[i]), buf) == FUZZY_OK);
        assert(strlen(buf) == (i == 0 ? 0 : FUZZY_CAVER_SIZE - 1));
        free(buf);
    }
    printf("OK\n");
}

static void test_phonetic_size(void) {
    printf("test_phonetic_size...");
    // vowels and consonants alternate, so each character has a code
    for (size_t n = 0; n < 64; n++) {
        char* s = repeat("ba", n);
        char* buf = malloc(FUZZY_PHONETIC_SIZE(n));
        assert(fuzzy_phonetic(s, n, buf));
        assert(strlen(buf) == n);
        free(buf);
        free(s);
    }
    printf("OK\n");
}

static void test_soundex_size(void) {
    printf("test_soundex_size...");
    for (size_t n = 1; n < 64; n++) {
        char* s = repeat("bdl", n);
        char* buf = malloc(FUZZY_SOUNDEX_SIZE);
        assert(fuzzy_soundex(s, n, buf));
        assert(strlen(buf) == FUZZY_SOUNDEX_SIZE - 1);
        free(buf);
        free(s);
    }
    printf("OK\n");
}

static void test_rsoundex_size(void) {
    printf("test_rsoundex_size...");
    // the codes alternate, so each character has a digit after the first letter
    for (size_t n = 0; n < 64; n++) {
        char* s = repeat("ab", n);
        char* buf = malloc(FUZZY_RSOUNDEX_SIZE(n));
        assert(fuzzy_rsoundex(s, n, buf));
        assert(strlen(buf) == (n == 0 ? 0 : n + 1));
        free(buf);
        free(s);
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

#pragma endregion

int main(void) {
    test_caver_size();
    test_phonetic_size();
    test_soundex_size();
    test_rsoundex_size();
    test_translit_size();
}
