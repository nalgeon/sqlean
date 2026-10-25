// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Base64 (RFC 4648) tests.

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crypto/base64.h"

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
    {"f", 1, "Zg==", 4},
    {"fo", 2, "Zm8=", 4},
    {"foo", 3, "Zm9v", 4},
    {"foob", 4, "Zm9vYg==", 8},
    {"fooba", 5, "Zm9vYmE=", 8},
    {"foobar", 6, "Zm9vYmFy", 8},
    // the extreme bytes
    {"\x00\x00\x00", 3, "AAAA", 4},
    {"\xff\xff\xff", 3, "////", 4},
    // the characters past letters and digits
    {"\xfb\xef\xbe", 3, "++++", 4},
    {"\xfb\xff", 2, "+/8=", 4},
};

static CodecTest decode_tests[] = {
    // RFC 4648 test vectors
    {"Zg==", 4, "f", 1},
    {"Zm8=", 4, "fo", 2},
    {"Zm9v", 4, "foo", 3},
    {"Zm9vYg==", 8, "foob", 4},
    {"Zm9vYmE=", 8, "fooba", 5},
    {"Zm9vYmFy", 8, "foobar", 6},
    // the extreme bytes
    {"AAAA", 4, "\x00\x00\x00", 3},
    {"////", 4, "\xff\xff\xff", 3},
    {"++++", 4, "\xfb\xef\xbe", 3},
    // non-zero padding bits are ignored
    {"Zh==", 4, "f", 1},
    {"Zm9=", 4, "fo", 2},
    // current behavior, to be fixed: '=' is accepted inside the input
    {"YQ=A", 4, "a\x00", 2},
    {"A=AA", 4, "\x00\x00\x00", 3},
    {"====", 4, "\x00", 1},
    {"Y===", 4, "`", 1},
    {"YQ==YQ==", 8, "a\x00\x00" "a", 4},
};

static const char* invalid_tests[] = {
    // the length is not a multiple of 4
    "Z",
    "Zg",
    "Zg=",
    "Zm9vY",
    "Zg===",
    // characters outside the alphabet
    "Zg!=",
    "Z-9v",
    "Zm9_",
    "Zm9v Zg==",
    "\n\n\n\n",
    // base64url is not accepted
    "Zm-_",
};
// clang-format on

static void test_encode(void) {
    printf("test_encode...");
    for (size_t i = 0; i < sizeof(encode_tests) / sizeof(encode_tests[0]); i++) {
        CodecTest test = encode_tests[i];
        size_t got_len = 0;
        uint8_t* got = base64_encode((const uint8_t*)test.src, test.src_len, &got_len);
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
        uint8_t* got = base64_decode((const uint8_t*)test.src, test.src_len, &got_len);
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
        uint8_t* got = base64_decode((const uint8_t*)src, strlen(src), &got_len);
        // printf("%s: want NULL, got %p\n", src, (void*)got);
        assert(got == NULL);
        // got_len is not checked: on failure, the decoder leaves it unset or garbage
    }
    printf("OK\n");
}

// Encodes src and decodes it back, expecting to get src.
static void assert_roundtrip(const uint8_t* src, size_t len) {
    size_t enc_len = 0;
    uint8_t* enc = base64_encode(src, len, &enc_len);
    assert(enc != NULL);
    assert(enc[enc_len] == '\0');

    size_t dec_len = 0;
    uint8_t* dec = base64_decode(enc, enc_len, &dec_len);
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
