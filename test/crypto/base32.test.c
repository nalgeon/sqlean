// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Base32 (RFC 4648) tests.

#include <assert.h>
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
    // current behavior, to be fixed: characters outside the alphabet
    // and lowercase letters are skipped instead of rejected
    {"me======", 8, "", 0},
    {"M!Y======", 9, "f", 1},
    {"MZ XW6===", 9, "foo", 3},
    {"my======", 8, "", 0},
};

static const char* invalid_tests[] = {
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

static void test_encode(void) {
    printf("test_encode...");
    for (size_t i = 0; i < sizeof(encode_tests) / sizeof(encode_tests[0]); i++) {
        CodecTest test = encode_tests[i];
        size_t got_len = 0;
        uint8_t* got = base32_encode((const uint8_t*)test.src, test.src_len, &got_len);
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
        uint8_t* got = base32_decode((const uint8_t*)test.src, test.src_len, &got_len);
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
        uint8_t* got = base32_decode((const uint8_t*)src, strlen(src), &got_len);
        // printf("%s: want NULL, got %p\n", src, (void*)got);
        assert(got == NULL);
        // got_len is not checked: on failure, the decoder leaves it unset or garbage
    }
    printf("OK\n");
}

// Encodes src and decodes it back, expecting to get src.
static void assert_roundtrip(const uint8_t* src, size_t len) {
    size_t enc_len = 0;
    uint8_t* enc = base32_encode(src, len, &enc_len);
    assert(enc != NULL);
    assert(enc[enc_len] == '\0');

    size_t dec_len = 0;
    uint8_t* dec = base32_decode(enc, enc_len, &dec_len);
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
