// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// UUID tests.

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "uuid/uuid.h"

// RFC 9562, Appendix A.3: version 4 example.
static const char* rfc_v4 = "919108f7-52d1-4320-9bac-f847db4148a8";
static const uint8_t rfc_v4_bytes[UUID_SIZE] = {0x91, 0x91, 0x08, 0xf7, 0x52, 0xd1, 0x43, 0x20,
                                                0x9b, 0xac, 0xf8, 0x47, 0xdb, 0x41, 0x48, 0xa8};

// RFC 9562, Appendix A.6: version 7 example, unix_ts_ms = 0x017f22e279b0
// (Tuesday, February 22, 2022 2:22:22.00 PM GMT-05:00).
static const char* rfc_v7 = "017f22e2-79b0-7cc3-98c4-dc0c0c07398f";
static const int64_t rfc_v7_ms = 1645557742000;
static const uint8_t rfc_v7_bytes[UUID_SIZE] = {0x01, 0x7f, 0x22, 0xe2, 0x79, 0xb0, 0x7c, 0xc3,
                                                0x98, 0xc4, 0xdc, 0x0c, 0x0c, 0x07, 0x39, 0x8f};

static const uint8_t zeros[UUID_SIZE] = {0};
static const uint8_t ones[UUID_SIZE] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

// fmt returns the canonical string form of u in a static buffer.
static const char* fmt(UUID u) {
    static char buf[UUID_STR_SIZE];
    uuid_fmt(buf, sizeof(buf), u);
    return buf;
}

// from_bytes returns a UUID made of the given bytes.
static UUID from_bytes(const uint8_t b[UUID_SIZE]) {
    UUID u;
    memcpy(u.b, b, UUID_SIZE);
    return u;
}

#pragma region Constructors

typedef struct {
    const uint8_t* rand;
    const char* want;
} V4Test;

static const V4Test v4_tests[] = {
    {rfc_v4_bytes, "919108f7-52d1-4320-9bac-f847db4148a8"},  // RFC example, bits already set
    {zeros, "00000000-0000-4000-8000-000000000000"},         // sets the version and variant
    {ones, "ffffffff-ffff-4fff-bfff-ffffffffffff"},          // clears the other bits
};

static void test_v4(void) {
    printf("test_v4...");
    for (size_t i = 0; i < sizeof(v4_tests) / sizeof(v4_tests[0]); i++) {
        V4Test test = v4_tests[i];
        UUID u = uuid_v4(test.rand);
        assert(strcmp(fmt(u), test.want) == 0);
        assert(uuid_get_version(u) == 4);
    }
    printf("OK\n");
}

typedef struct {
    int64_t ms;
    const uint8_t* rand;
    bool ok;
    const char* want;
} V7Test;

static const V7Test v7_tests[] = {
    {rfc_v7_ms, rfc_v7_bytes, true, "017f22e2-79b0-7cc3-98c4-dc0c0c07398f"},  // RFC example
    {0, zeros, true, "00000000-0000-7000-8000-000000000000"},                 // the epoch
    {0, ones, true, "00000000-0000-7fff-bfff-ffffffffffff"},                  // rand_a is random
    {UUID_V7_MAX_MS, zeros, true, "ffffffff-ffff-7000-8000-000000000000"},    // the max time
    {UUID_V7_MAX_MS + 1, zeros, false, NULL},                                 // overflows 48 bits
    {-1, zeros, false, NULL},                                                 // before the epoch
    {INT64_MAX, zeros, false, NULL},
    {INT64_MIN, zeros, false, NULL},
};

static void test_v7(void) {
    printf("test_v7...");
    for (size_t i = 0; i < sizeof(v7_tests) / sizeof(v7_tests[0]); i++) {
        V7Test test = v7_tests[i];
        UUID u;
        bool ok = uuid_v7(test.ms, test.rand, &u);
        assert(ok == test.ok);
        if (ok) {
            assert(strcmp(fmt(u), test.want) == 0);
        }
    }
    printf("OK\n");
}

typedef struct {
    int64_t sec;
    int32_t nsec;
    bool ok;
    const char* want;
} V7TimeTest;

// 1645557742 sec = 0x017f22e279b0 ms. rand_a is the sub-millisecond
// fraction scaled to 4096 steps: nsec % 1000000 * 4096 / 1000000.
static const V7TimeTest v7_time_tests[] = {
    {1645557742, 0, true, "017f22e2-79b0-7000-8000-000000000000"},          // no fraction
    {1645557742, 244, true, "017f22e2-79b0-7000-8000-000000000000"},        // less than a step
    {1645557742, 245, true, "017f22e2-79b0-7001-8000-000000000000"},        // one step
    {1645557742, 500000, true, "017f22e2-79b0-7800-8000-000000000000"},     // half a millisecond
    {1645557742, 999999, true, "017f22e2-79b0-7fff-8000-000000000000"},     // the last step
    {1645557742, 1000000, true, "017f22e2-79b1-7000-8000-000000000000"},    // the next millisecond
    {1645557742, 999999999, true, "017f22e2-7d97-7fff-8000-000000000000"},  // the last nanosecond
    {0, 0, true, "00000000-0000-7000-8000-000000000000"},                   // the epoch
    {281474976710, 655999999, true, "ffffffff-ffff-7fff-8000-000000000000"},  // the max time
    {281474976710, 656000000, false, NULL},                                   // overflows 48 bits
    {281474976711, 0, false, NULL},                                           // overflows 48 bits
    {INT64_MAX, 0, false, NULL},   // sec * 1000 would overflow int64
    {-1, 999999999, false, NULL},  // before the epoch
    {INT64_MIN, 0, false, NULL},
    {0, -1, false, NULL},          // invalid nsec
    {0, 1000000000, false, NULL},  // invalid nsec
};

static void test_v7_time(void) {
    printf("test_v7_time...");
    for (size_t i = 0; i < sizeof(v7_time_tests) / sizeof(v7_time_tests[0]); i++) {
        V7TimeTest test = v7_time_tests[i];
        UUID u;
        bool ok = uuid_v7_time(test.sec, test.nsec, zeros, &u);
        assert(ok == test.ok);
        if (ok) {
            assert(strcmp(fmt(u), test.want) == 0);
        }
    }
    printf("OK\n");
}

// UUIDs made at least a step (244.14 ns) apart sort by time,
// across millisecond and second boundaries, whatever the random bits.
static void test_v7_time_order(void) {
    printf("test_v7_time_order...");
    UUID prev;
    assert(uuid_v7_time(1645557742, 0, ones, &prev));
    for (int64_t ns = 245; ns < 3 * 1000000000LL; ns += 245) {
        UUID u;
        const uint8_t* rand = (ns / 245) % 2 ? zeros : ones;
        assert(uuid_v7_time(1645557742 + ns / 1000000000, (int32_t)(ns % 1000000000), rand, &u));
        assert(memcmp(prev.b, u.b, UUID_SIZE) < 0);
        prev = u;
    }
    printf("OK\n");
}

static void test_v7_now(void) {
    printf("test_v7_now...");
    int64_t before = (int64_t)time(NULL) * 1000;
    UUID u;
    assert(uuid_v7_now(zeros, &u));
    int64_t after = (int64_t)time(NULL) * 1000 + 1000;
    int64_t ms;
    assert(uuid_get_unix_ms(u, &ms));
    assert(ms >= before && ms < after);
    assert(uuid_get_version(u) == 7);
    assert((u.b[8] & 0xc0) == 0x80);
    printf("OK\n");
}

#pragma endregion

#pragma region Fields

typedef struct {
    const char* s;
    int version;
    bool ok;
    int64_t ms;
} FieldTest;

static const FieldTest field_tests[] = {
    {"017f22e2-79b0-7cc3-98c4-dc0c0c07398f", 7, true, 1645557742000},   // RFC v7 example
    {"00000000-0000-7000-8000-000000000000", 7, true, 0},               // the epoch
    {"ffffffff-ffff-7fff-bfff-ffffffffffff", 7, true, UUID_V7_MAX_MS},  // the max time
    {"017f22e2-79b0-7cc3-08c4-dc0c0c07398f", 7, true, 1645557742000},   // the variant is ignored
    {"919108f7-52d1-4320-9bac-f847db4148a8", 4, false, 0},              // RFC v4 example
    {"c232ab00-9414-11ec-b3c8-9f6bdeced846", 1, false, 0},              // RFC v1 example
    {"017f22e2-79b0-8cc3-98c4-dc0c0c07398f", 8, false, 0},              // version 8
    {"00000000-0000-0000-0000-000000000000", 0, false, 0},              // nil UUID
    {"ffffffff-ffff-ffff-ffff-ffffffffffff", 15, false, 0},             // max UUID
};

static void test_get_version(void) {
    printf("test_get_version...");
    for (size_t i = 0; i < sizeof(field_tests) / sizeof(field_tests[0]); i++) {
        FieldTest test = field_tests[i];
        UUID u;
        assert(uuid_parse(test.s, strlen(test.s), &u));
        assert(uuid_get_version(u) == test.version);
    }
    printf("OK\n");
}

static void test_get_unix_ms(void) {
    printf("test_get_unix_ms...");
    for (size_t i = 0; i < sizeof(field_tests) / sizeof(field_tests[0]); i++) {
        FieldTest test = field_tests[i];
        UUID u;
        assert(uuid_parse(test.s, strlen(test.s), &u));
        int64_t ms = -1;
        assert(uuid_get_unix_ms(u, &ms) == test.ok);
        if (test.ok) {
            assert(ms == test.ms);
        } else {
            assert(ms == -1);  // out is untouched
        }
    }
    printf("OK\n");
}

#pragma endregion

#pragma region Formatting

static void test_fmt(void) {
    printf("test_fmt...");
    char buf[UUID_STR_SIZE];
    assert(uuid_fmt(buf, sizeof(buf), from_bytes(rfc_v4_bytes)) == 36);
    assert(strcmp(buf, rfc_v4) == 0);
    assert(uuid_fmt(buf, sizeof(buf), from_bytes(rfc_v7_bytes)) == 36);
    assert(strcmp(buf, rfc_v7) == 0);
    assert(uuid_fmt(buf, sizeof(buf), from_bytes(zeros)) == 36);
    assert(strcmp(buf, "00000000-0000-0000-0000-000000000000") == 0);
    assert(uuid_fmt(buf, sizeof(buf), from_bytes(ones)) == 36);
    assert(strcmp(buf, "ffffffff-ffff-ffff-ffff-ffffffffffff") == 0);

    // A heap buffer of exactly UUID_STR_SIZE, so the sanitizer catches an overflow.
    char* exact = malloc(UUID_STR_SIZE);
    assert(exact != NULL);
    assert(uuid_fmt(exact, UUID_STR_SIZE, from_bytes(ones)) == 36);
    assert(strcmp(exact, "ffffffff-ffff-ffff-ffff-ffffffffffff") == 0);
    free(exact);

    // A smaller buffer gets a truncated string, a zero-sized one nothing.
    char small[9];
    assert(uuid_fmt(small, sizeof(small), from_bytes(rfc_v4_bytes)) == 36);
    assert(strcmp(small, "919108f7") == 0);
    assert(uuid_fmt(NULL, 0, from_bytes(rfc_v4_bytes)) == 36);
    printf("OK\n");
}

typedef struct {
    const char* s;
    size_t n;  // 0 means strlen(s)
    bool ok;
} ParseTest;

static const ParseTest parse_tests[] = {
    // The accepted forms.
    {"919108f7-52d1-4320-9bac-f847db4148a8", 0, true},           // canonical
    {"919108F7-52D1-4320-9BAC-F847DB4148A8", 0, true},           // uppercase
    {"919108f7-52D1-4320-9bAc-f847dB4148a8", 0, true},           // mixed case
    {"{919108f7-52d1-4320-9bac-f847db4148a8}", 0, true},         // braces
    {"urn:uuid:919108f7-52d1-4320-9bac-f847db4148a8", 0, true},  // URN
    {"URN:UUID:919108f7-52d1-4320-9bac-f847db4148a8", 0, true},  // URN prefix in any case
    {"Urn:UUID:919108f7-52d1-4320-9bac-f847db4148a8", 0, true},
    {"919108f752d143209bacf847db4148a8", 0, true},  // hex only

    // Wrong length.
    {"", 0, false},
    {"919108f7-52d1-4320-9bac-f847db4148a", 0, false},
    {"919108f7-52d1-4320-9bac-f847db4148a88", 0, false},
    {"919108f752d143209bacf847db4148a", 0, false},
    {"919108f752d143209bacf847db4148a88", 0, false},
    {"{919108f7-52d1-4320-9bac-f847db4148a8", 0, false},  // unbalanced braces
    {"919108f7-52d1-4320-9bac-f847db4148a8}", 0, false},
    {"{{919108f7-52d1-4320-9bac-f847db4148a8}}", 0, false},
    {" 919108f7-52d1-4320-9bac-f847db4148a8", 0, false},  // whitespace
    {"919108f7-52d1-4320-9bac-f847db4148a8 ", 0, false},
    {"urn:uuid:919108f752d143209bacf847db4148a8", 0, false},  // URN without hyphens
    {"{919108f752d143209bacf847db4148a8}", 0, false},         // braces without hyphens

    // Right length, wrong content.
    {"x919108f7-52d1-4320-9bac-f847db4148a8y", 0, false},  // Go accepts any first and last byte
    {"(919108f7-52d1-4320-9bac-f847db4148a8)", 0, false},
    {"}919108f7-52d1-4320-9bac-f847db4148a8{", 0, false},
    {"urn:uuix:919108f7-52d1-4320-9bac-f847db4148a8", 0, false},
    {"urn-uuid-919108f7-52d1-4320-9bac-f847db4148a8", 0, false},
    {"919108f7-52d1-4320-9bac-f847db4148ag", 0, false},  // non-hex digit
    {"g19108f7-52d1-4320-9bac-f847db4148a8", 0, false},
    {"919108f752d143209bacf847db4148ag", 0, false},
    {"919108f7_52d1-4320-9bac-f847db4148a8", 0, false},  // wrong separator
    {"919108f752-d1-4320-9bac-f847db4148a8", 0, false},  // hyphen in the wrong place
    {"919108f7-52d1-4320-9bac-f847-b4148a8", 0, false},
    {"a0ee-bc99-9c0b-4ef8-bb6d-6bb9-bd38-0a11", 0, false},  // PostgreSQL forms
    {"{a0eebc99-9c0b4ef8-bb6d6bb9-bd380a11}", 0, false},
    {"919108f7-52d1-4320-9bac-f847db4148\xe9\xe9", 0, false},  // high bytes
    {"919108f7-52d1-4320-9bac-f847db4148\xa8", 0, false},

    // Embedded NUL.
    {"919108f7-52d1-4320-9bac-f847db41\0"
     "8a8",
     36, false},
    {"919108f7-52d1-4320-9bac-f847db4148a8\0", 37, false},
    {"919108f752d143209bacf847db41\0"
     "8a8",
     32, false},
    {"\0"
     "919108f7-52d1-4320-9bac-f847db4148a8",
     37, false},
};

static void test_parse(void) {
    printf("test_parse...");
    for (size_t i = 0; i < sizeof(parse_tests) / sizeof(parse_tests[0]); i++) {
        ParseTest test = parse_tests[i];
        size_t n = test.n ? test.n : strlen(test.s);
        // Copy to an exact-size heap buffer, so the sanitizer catches reads past n.
        char* s = malloc(n ? n : 1);
        assert(s != NULL);
        memcpy(s, test.s, n);
        UUID u;
        memset(u.b, 0xaa, UUID_SIZE);
        bool ok = uuid_parse(s, n, &u);
        assert(ok == test.ok);
        if (ok) {
            assert(memcmp(u.b, rfc_v4_bytes, UUID_SIZE) == 0);
        } else {
            for (int j = 0; j < UUID_SIZE; j++) {
                assert(u.b[j] == 0xaa);  // out is untouched
            }
        }
        free(s);
    }
    printf("OK\n");
}

static void test_parse_null(void) {
    printf("test_parse_null...");
    UUID u;
    assert(!uuid_parse(NULL, 0, &u));
    printf("OK\n");
}

// Every byte value round-trips through format and parse.
static void test_fmt_parse_roundtrip(void) {
    printf("test_fmt_parse_roundtrip...");
    for (int x = 0; x < 256; x++) {
        UUID u;
        for (int i = 0; i < UUID_SIZE; i++) {
            u.b[i] = (uint8_t)(x + i * 17);
        }
        char buf[UUID_STR_SIZE];
        assert(uuid_fmt(buf, sizeof(buf), u) == 36);
        UUID got;
        assert(uuid_parse(buf, 36, &got));
        assert(memcmp(got.b, u.b, UUID_SIZE) == 0);
    }
    printf("OK\n");
}

#pragma endregion

int main(void) {
    // Constructors.
    test_v4();
    test_v7();
    test_v7_time();
    test_v7_time_order();
    test_v7_now();

    // Fields.
    test_get_version();
    test_get_unix_ms();

    // Formatting.
    test_fmt();
    test_parse();
    test_parse_null();
    test_fmt_parse_roundtrip();
}
