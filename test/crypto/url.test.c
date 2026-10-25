// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// URL (percent) encoding tests.

#include <assert.h>
#include <stdbool.h>
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
    // the NUL byte is escaped like any other
    {"\x00", 1, "%00", 3},
    {"a\x00" "b", 3, "a%00b", 5},
};

static CodecTest decode_tests[] = {
    {"", 0, "", 0},
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

// encode calls url_encode with a buffer of exactly url_encoded_size bytes,
// so that the sanitizers catch a write past it (and with no buffer at all
// if the size is 0). Returns the result, which is never NULL; free it.
static uint8_t* encode(const uint8_t* src, size_t len, size_t* out_len) {
    size_t size = url_encoded_size(len);
    uint8_t* buf = size > 0 ? malloc(size) : NULL;
    assert(buf != NULL || size == 0);
    *out_len = url_encode(src, len, buf);
    assert(*out_len <= size);
    return buf != NULL ? buf : malloc(1);
}

// decode calls url_decode with a buffer of exactly url_decoded_size bytes,
// like encode does. Returns the result, or NULL if src is invalid.
static uint8_t* decode(const uint8_t* src, size_t len, size_t* out_len) {
    size_t size = url_decoded_size(src, len);
    uint8_t* buf = size > 0 ? malloc(size) : NULL;
    assert(buf != NULL || size == 0);
    if (!url_decode(src, len, buf, out_len)) {
        free(buf);
        return NULL;
    }
    assert(*out_len <= size);
    return buf != NULL ? buf : malloc(1);
}

static void test_encode(void) {
    printf("test_encode...");
    for (size_t i = 0; i < sizeof(encode_tests) / sizeof(encode_tests[0]); i++) {
        CodecTest test = encode_tests[i];
        size_t got_len = 0;
        uint8_t* got = encode((const uint8_t*)test.src, test.src_len, &got_len);
        assert(got != NULL);
        // printf("want %s, got %s\n", test.want, got);
        assert(got_len == test.want_len);
        assert(memcmp(got, test.want, got_len) == 0);
        free(got);
    }
    printf("OK\n");
}

static void test_decode(void) {
    printf("test_decode...");
    for (size_t i = 0; i < sizeof(decode_tests) / sizeof(decode_tests[0]); i++) {
        CodecTest test = decode_tests[i];
        size_t got_len = 0;
        uint8_t* got = decode((const uint8_t*)test.src, test.src_len, &got_len);
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
        uint8_t* got = decode((const uint8_t*)src, strlen(src), &got_len);
        // printf("%s: want NULL, got %p\n", src, (void*)got);
        assert(got == NULL);
    }
    printf("OK\n");
}

// Encodes src and decodes it back, expecting to get src.
static void assert_roundtrip(const uint8_t* src, size_t len) {
    size_t enc_len = 0;
    uint8_t* enc = encode(src, len, &enc_len);
    assert(enc != NULL);

    size_t dec_len = 0;
    uint8_t* dec = decode(enc, enc_len, &dec_len);
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

    // all-zero and all-0xff data of every length
    memset(buf, 0, sizeof(buf));
    for (size_t len = 0; len <= 64; len++) {
        assert_roundtrip(buf, len);
    }
    memset(buf, 0xff, sizeof(buf));
    for (size_t len = 0; len <= 64; len++) {
        assert_roundtrip(buf, len);
    }

    // pseudo-random data of every length
    uint32_t state = 1;
    for (size_t i = 0; i < sizeof(buf); i++) {
        state = state * 1103515245 + 12345;
        buf[i] = (state >> 16) & 0xff;
    }
    for (size_t len = 0; len <= 64; len++) {
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
