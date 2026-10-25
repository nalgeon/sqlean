// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// URL (percent) encoding tests.

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crypto/url.h"

typedef struct {
    const char* src;
    size_t src_len;
    const char* want;
    size_t want_len;
} CodecTest;

// clang-format off
static CodecTest encode_tests[] = {
    {"", 0, "", 0},
    {"abc", 3, "abc", 3},
    // unreserved characters are kept as is
    {"AZaz09-_.~", 10, "AZaz09-_.~", 10},
    // everything else is percent-encoded with uppercase digits
    {"a b", 3, "a%20b", 5},
    {"a+b", 3, "a%2Bb", 5},
    {"/?#&=%", 6, "%2F%3F%23%26%3D%25", 18},
    {"\x01\x7f\x80\xff", 4, "%01%7F%80%FF", 12},
    {"\xc3\xa9", 2, "%C3%A9", 6},
    // current behavior, to be fixed: the NUL byte is kept as is
    {"\x00", 1, "\x00", 1},
    {"a\x00" "b", 3, "a\x00" "b", 3},
};

static CodecTest decode_tests[] = {
    {"abc", 3, "abc", 3},
    {"AZaz09-_.~", 10, "AZaz09-_.~", 10},
    {"a%20b", 5, "a b", 3},
    {"%2F%3F%23%26%3D%25", 18, "/?#&=%", 6},
    {"%01%7F%80%FF", 12, "\x01\x7f\x80\xff", 4},
    {"%C3%A9", 6, "\xc3\xa9", 2},
    // lowercase digits are accepted
    {"%2f%c3%a9", 9, "/\xc3\xa9", 3},
    // the decoder is lenient with characters that should have been encoded
    {"a b/?#&=", 8, "a b/?#&=", 8},
    {"\xc3\xa9", 2, "\xc3\xa9", 2},
    // a '+' is decoded as a space, as in HTML forms
    {"a+b", 3, "a b", 3},
    {"a%2Bb", 5, "a+b", 3},
};

static const char* invalid_tests[] = {
    // a truncated escape
    "%",
    "%4",
    "abc%",
    "abc%4",
    "%%41",
    // not hex digits
    "%zz",
    "%4g",
    "%g4",
    "% 41",
    "%-1",
};
// clang-format on

static void test_encode(void) {
    printf("test_encode...");
    for (size_t i = 0; i < sizeof(encode_tests) / sizeof(encode_tests[0]); i++) {
        CodecTest test = encode_tests[i];
        size_t got_len = 0;
        uint8_t* got = url_encode((const uint8_t*)test.src, test.src_len, &got_len);
        assert(got != NULL);
        // printf("want %s, got %s\n", test.want, got);
        assert(got_len == test.want_len);
        assert(memcmp(got, test.want, got_len) == 0);
        assert(got[got_len] == '\0');
        free(got);
    }
    printf("OK\n");
}

static void test_decode(void) {
    printf("test_decode...");
    for (size_t i = 0; i < sizeof(decode_tests) / sizeof(decode_tests[0]); i++) {
        CodecTest test = decode_tests[i];
        size_t got_len = 0;
        uint8_t* got = url_decode((const uint8_t*)test.src, test.src_len, &got_len);
        // printf("%s: want %zu bytes, got %zu\n", test.src, test.want_len, got_len);
        assert(got != NULL);
        assert(got_len == test.want_len);
        assert(memcmp(got, test.want, got_len) == 0);
        free(got);
    }
    printf("OK\n");
}

static void test_decode_invalid(void) {
    printf("test_decode_invalid...");
    for (size_t i = 0; i < sizeof(invalid_tests) / sizeof(invalid_tests[0]); i++) {
        const char* src = invalid_tests[i];
        size_t got_len = 0;
        uint8_t* got = url_decode((const uint8_t*)src, strlen(src), &got_len);
        // printf("%s: want NULL, got %p\n", src, (void*)got);
        assert(got == NULL);
        // got_len is not checked: on failure, the decoder leaves it unset or garbage
    }
    printf("OK\n");
}

// Encodes src and decodes it back, expecting to get src.
static void assert_roundtrip(const uint8_t* src, size_t len) {
    size_t enc_len = 0;
    uint8_t* enc = url_encode(src, len, &enc_len);
    assert(enc != NULL);
    assert(enc[enc_len] == '\0');

    size_t dec_len = 0;
    uint8_t* dec = url_decode(enc, enc_len, &dec_len);
    // printf("len %zu: encoded as %s\n", len, enc);
    assert(dec != NULL);
    assert(dec_len == len);
    assert(memcmp(dec, src, len) == 0);

    free(enc);
    free(dec);
}

static void test_roundtrip(void) {
    printf("test_roundtrip...");
    uint8_t buf[256];

    // Lengths start at 1: the binding layer handles empty input itself,
    // and some decoders do not support it (base64_decode reads src[-1]).

    // all-zero and all-0xff data of every length
    memset(buf, 0, sizeof(buf));
    for (size_t len = 1; len <= 64; len++) {
        assert_roundtrip(buf, len);
    }
    memset(buf, 0xff, sizeof(buf));
    for (size_t len = 1; len <= 64; len++) {
        assert_roundtrip(buf, len);
    }

    // pseudo-random data of every length
    uint32_t state = 1;
    for (size_t i = 0; i < sizeof(buf); i++) {
        state = state * 1103515245 + 12345;
        buf[i] = (state >> 16) & 0xff;
    }
    for (size_t len = 1; len <= 64; len++) {
        assert_roundtrip(buf, len);
    }

    // every byte value
    for (size_t i = 0; i < sizeof(buf); i++) {
        buf[i] = (uint8_t)i;
    }
    assert_roundtrip(buf, sizeof(buf));

    printf("OK\n");
}

int main(void) {
    test_encode();
    test_decode();
    test_decode_invalid();
    test_roundtrip();
}
