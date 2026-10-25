// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Base32 (RFC 4648) tests.

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crypto/base32.h"

typedef struct {
    const char* src;
    size_t src_len;
    const char* want;
    size_t want_len;
} CodecTest;

// clang-format off
static CodecTest encode_tests[] = {
    // RFC 4648 test vectors
    {"", 0, "", 0},
    {"f", 1, "MY======", 8},
    {"fo", 2, "MZXQ====", 8},
    {"foo", 3, "MZXW6===", 8},
    {"foob", 4, "MZXW6YQ=", 8},
    {"fooba", 5, "MZXW6YTB", 8},
    {"foobar", 6, "MZXW6YTBOI======", 16},
    // the extreme bytes
    {"\x00\x00\x00\x00\x00", 5, "AAAAAAAA", 8},
    {"\xff\xff\xff\xff\xff", 5, "77777777", 8},
};

static CodecTest decode_tests[] = {
    {"", 0, "", 0},
    // RFC 4648 test vectors
    {"MY======", 8, "f", 1},
    {"MZXQ====", 8, "fo", 2},
    {"MZXW6===", 8, "foo", 3},
    {"MZXW6YQ=", 8, "foob", 4},
    {"MZXW6YTB", 8, "fooba", 5},
    {"MZXW6YTBOI======", 16, "foobar", 6},
    // the extreme bytes
    {"AAAAAAAA", 8, "\x00\x00\x00\x00\x00", 5},
    {"77777777", 8, "\xff\xff\xff\xff\xff", 5},
    // the padding is optional
    {"MY", 2, "f", 1},
    {"MZXW6", 5, "foo", 3},
    {"MY=", 3, "f", 1},
};

static const char* invalid_tests[] = {
    // characters outside the alphabet
    "M!Y=====",
    "MZ XW6===",
    "MZXW6\xff==",
    // lowercase letters (the alphabet is uppercase)
    "me======",
    "my======",
    // padding inside the input
    "MY=MY===",
    "MY======MY======",
    // leftover bits that do not make a byte
    "M",
    "MZXW6Y",
    "MZXW6YTBO",
    // non-zero leftover bits
    "MZ",
    "MZXW6YT",
    "MZ======",
};
// clang-format on

// encode calls base32_encode with a buffer of exactly base32_encoded_size bytes,
// so that the sanitizers catch a write past it (and with no buffer at all
// if the size is 0). Returns the result, which is never NULL; free it.
static uint8_t* encode(const uint8_t* src, size_t len, size_t* out_len) {
    size_t size = base32_encoded_size(len);
    uint8_t* buf = size > 0 ? malloc(size) : NULL;
    assert(buf != NULL || size == 0);
    *out_len = base32_encode(src, len, buf);
    assert(*out_len <= size);
    return buf != NULL ? buf : malloc(1);
}

// decode calls base32_decode with a buffer of exactly base32_decoded_size bytes,
// like encode does. Returns the result, or NULL if src is invalid.
static uint8_t* decode(const uint8_t* src, size_t len, size_t* out_len) {
    size_t size = base32_decoded_size(src, len);
    uint8_t* buf = size > 0 ? malloc(size) : NULL;
    assert(buf != NULL || size == 0);
    if (!base32_decode(src, len, buf, out_len)) {
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
