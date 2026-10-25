// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Base85 (Ascii85) tests.

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crypto/base85.h"

typedef struct {
    const char* src;
    size_t src_len;
    const char* want;
} EncodeTest;

// clang-format off
static EncodeTest encode_tests[] = {
    {"", 0, ""},
    {"a", 1, "@/"},
    {"ab", 2, "@:B"},
    {"abc", 3, "@:E^"},
    {"abcd", 4, "@:E_W"},
    {"abcde", 5, "@:E_WAH"},
    {"hello", 5, "BOu!rDZ"},
    {"sure.", 5, "F*2M7/c"},
    // an all-zero group is encoded as a single 'z'
    {"\x00\x00\x00\x00", 4, "z"},
    {"\x00\x00\x00\x00\x00\x00\x00\x00", 8, "zz"},
    // ...but only if the group is full, otherwise the shorthand
    // would decode into more bytes than the source has
    {"\x00", 1, "!!"},
    {"\x00\x00", 2, "!!!"},
    {"\x00\x00\x00", 3, "!!!!"},
    {"\x00\x00\x00\x00\x00", 5, "z!!"},
    {"\x00\x00\x00\x00\x00\x00\x00", 7, "z!!!!"},
    // a zero group in the middle
    {"\x00\x00\x00\x00" "abcd", 8, "z@:E_W"},
    {"abcd" "\x00\x00\x00\x00" "abcd", 12, "@:E_Wz@:E_W"},
    // the largest tuple
    {"\xff\xff\xff\xff", 4, "s8W-!"},
    {"\xff\xff\xff", 3, "s8W*"},
};
// clang-format on

// encode calls base85_encode with a buffer of exactly base85_encoded_size bytes,
// so that the sanitizers catch a write past it (and with no buffer at all
// if the size is 0). Returns the result, which is never NULL; free it.
static uint8_t* encode(const uint8_t* src, size_t len, size_t* out_len) {
    size_t size = base85_encoded_size(len);
    uint8_t* buf = size > 0 ? malloc(size) : NULL;
    assert(buf != NULL || size == 0);
    *out_len = base85_encode(src, len, buf);
    assert(*out_len <= size);
    return buf != NULL ? buf : malloc(1);
}

// decode calls base85_decode with a buffer of exactly base85_decoded_size bytes,
// like encode does. Returns the result, or NULL if src is invalid.
static uint8_t* decode(const uint8_t* src, size_t len, size_t* out_len) {
    size_t size = base85_decoded_size(src, len);
    uint8_t* buf = size > 0 ? malloc(size) : NULL;
    assert(buf != NULL || size == 0);
    if (!base85_decode(src, len, buf, out_len)) {
        free(buf);
        return NULL;
    }
    assert(*out_len <= size);
    return buf != NULL ? buf : malloc(1);
}

static void test_encode(void) {
    printf("test_encode...");
    for (size_t i = 0; i < sizeof(encode_tests) / sizeof(encode_tests[0]); i++) {
        EncodeTest test = encode_tests[i];
        size_t got_len = 0;
        uint8_t* got = encode((const uint8_t*)test.src, test.src_len, &got_len);
        assert(got != NULL);
        // printf("want %s, got %s\n", test.want, got);
        assert(got_len == strlen(test.want));
        assert(memcmp(got, test.want, got_len) == 0);
        free(got);
    }
    printf("OK\n");
}

typedef struct {
    const char* src;
    const char* want;
    size_t want_len;
} DecodeTest;

// clang-format off
static DecodeTest decode_tests[] = {
    {"", "", 0},
    {"@/", "a", 1},
    {"@:B", "ab", 2},
    {"@:E^", "abc", 3},
    {"@:E_W", "abcd", 4},
    {"@:E_WAH", "abcde", 5},
    {"BOu!rDZ", "hello", 5},
    // a full group followed by a partial one
    {"F*2M7/c", "sure.", 5},
    // 'z' expands into four zero bytes
    {"z", "\x00\x00\x00\x00", 4},
    {"zz", "\x00\x00\x00\x00\x00\x00\x00\x00", 8},
    {"z!!", "\x00\x00\x00\x00\x00", 5},
    {"z@:E_Wz", "\x00\x00\x00\x00" "abcd" "\x00\x00\x00\x00", 12},
    // the same tuple spelled out in full is also valid
    {"!!!!!", "\x00\x00\x00\x00", 4},
    {"!!", "\x00", 1},
    {"!!!", "\x00\x00", 2},
    {"!!!!", "\x00\x00\x00", 3},
    // the largest tuple
    {"s8W-!", "\xff\xff\xff\xff", 4},
    {"s8W*", "\xff\xff\xff", 3},
};
// clang-format on

static void test_decode(void) {
    printf("test_decode...");
    for (size_t i = 0; i < sizeof(decode_tests) / sizeof(decode_tests[0]); i++) {
        DecodeTest test = decode_tests[i];
        size_t got_len = 0;
        uint8_t* got = decode((const uint8_t*)test.src, strlen(test.src), &got_len);
        assert(got != NULL);
        // printf("%s: want %zu bytes, got %zu\n", test.src, test.want_len, got_len);
        assert(got_len == test.want_len);
        assert(memcmp(got, test.want, got_len) == 0);
        free(got);
    }
    printf("OK\n");
}

// clang-format off
static const char* invalid_tests[] = {
    // characters outside the '!'..'u' range
    "v", "abcw", "ab de", "@:E_~", "\n", "\x01\x02",
    // 'z' is only valid at a group boundary
    "@:zE_W", "@:E_Wz!z",
    // a final group of a single character encodes no bytes
    "@:E_W!", "!", "z!",
    // the tuple value does not fit into 32 bits
    "uuuuu", "s8W-\"", "uu", "uuu", "uuuu",
};
// clang-format on

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
    uint8_t buf[64];

    // all-zero data of every length (exercises the 'z' shorthand
    // next to partial groups)
    memset(buf, 0, sizeof(buf));
    for (size_t len = 0; len <= sizeof(buf); len++) {
        assert_roundtrip(buf, len);
    }

    // all-0xff data of every length (exercises the largest tuple)
    memset(buf, 0xff, sizeof(buf));
    for (size_t len = 0; len <= sizeof(buf); len++) {
        assert_roundtrip(buf, len);
    }

    // pseudo-random data of every length
    uint32_t state = 1;
    for (size_t i = 0; i < sizeof(buf); i++) {
        state = state * 1103515245 + 12345;
        buf[i] = (state >> 16) & 0xff;
    }
    for (size_t len = 0; len <= sizeof(buf); len++) {
        assert_roundtrip(buf, len);
    }

    printf("OK\n");
}

// Returns the number of bytes src decodes into,
// assuming it is a valid base85 string.
static size_t decoded_size(const uint8_t* src, size_t len) {
    size_t n_shorthand = 0;
    for (size_t i = 0; i < len; i++) {
        if (src[i] == 'z') {
            n_shorthand++;
        }
    }
    return (len - n_shorthand) * 4 / 5 + n_shorthand * 4;
}

// Decodes every string over a representative alphabet, checking that the
// decoder either rejects the input or reports exactly as many bytes as the
// allocation formula predicts. Combined with a sanitizer build, this catches
// both under-allocation (a heap overflow) and drift between the two.
static void test_decode_exhaustive(void) {
    printf("test_decode_exhaustive...");
    // the smallest digit, a middle one, the largest digit,
    // the shorthand and an out-of-range character
    static const char alphabet[] = "!5uzv";
    size_t n_chars = sizeof(alphabet) - 1;

    uint8_t src[6];
    size_t n_valid = 0;
    for (size_t len = 0; len <= sizeof(src); len++) {
        size_t n_words = 1;
        for (size_t i = 0; i < len; i++) {
            n_words *= n_chars;
        }
        for (size_t word = 0; word < n_words; word++) {
            size_t rest = word;
            for (size_t i = 0; i < len; i++) {
                src[i] = alphabet[rest % n_chars];
                rest /= n_chars;
            }

            size_t got_len = 0;
            uint8_t* got = decode(src, len, &got_len);
            if (got == NULL) {
                continue;
            }
            assert(got_len == decoded_size(src, len));
            // valid input survives a round trip through the canonical encoding
            assert_roundtrip(got, got_len);
            free(got);
            n_valid++;
        }
    }
    // printf("%zu valid strings\n", n_valid);
    assert(n_valid > 0);
    printf("OK\n");
}

int main(void) {
    test_encode();
    test_decode();
    test_decode_invalid();
    test_roundtrip();
    test_decode_exhaustive();
}
