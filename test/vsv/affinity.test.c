// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Affinity tests.

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "vsv/vsv.h"

#define LEN(a) (sizeof(a) / sizeof(a[0]))

static void test_affinity(void) {
    printf("test_affinity...");
    static const struct {
        const char* type;
        VsvAffinity want;
    } tests[] = {
        // the examples from https://sqlite.org/datatype3.html#affinity_name_examples
        {"INT", VSV_AFF_INTEGER},
        {"INTEGER", VSV_AFF_INTEGER},
        {"TINYINT", VSV_AFF_INTEGER},
        {"SMALLINT", VSV_AFF_INTEGER},
        {"MEDIUMINT", VSV_AFF_INTEGER},
        {"BIGINT", VSV_AFF_INTEGER},
        {"UNSIGNED BIG INT", VSV_AFF_INTEGER},
        {"INT2", VSV_AFF_INTEGER},
        {"INT8", VSV_AFF_INTEGER},
        {"CHARACTER(20)", VSV_AFF_TEXT},
        {"VARCHAR(255)", VSV_AFF_TEXT},
        {"VARYING CHARACTER(255)", VSV_AFF_TEXT},
        {"NCHAR(55)", VSV_AFF_TEXT},
        {"NATIVE CHARACTER(70)", VSV_AFF_TEXT},
        {"NVARCHAR(100)", VSV_AFF_TEXT},
        {"TEXT", VSV_AFF_TEXT},
        {"CLOB", VSV_AFF_TEXT},
        {"BLOB", VSV_AFF_BLOB},
        {"", VSV_AFF_BLOB},
        {"REAL", VSV_AFF_REAL},
        {"DOUBLE", VSV_AFF_REAL},
        {"DOUBLE PRECISION", VSV_AFF_REAL},
        {"FLOAT", VSV_AFF_REAL},
        {"NUMERIC", VSV_AFF_NUMERIC},
        {"DECIMAL(10,5)", VSV_AFF_NUMERIC},
        {"BOOLEAN", VSV_AFF_NUMERIC},
        {"DATE", VSV_AFF_NUMERIC},
        {"DATETIME", VSV_AFF_NUMERIC},
        // the first matching rule wins
        {"FLOATING POINT", VSV_AFF_INTEGER},
        {"CHARINT", VSV_AFF_INTEGER},
        {"TEXT BLOB", VSV_AFF_TEXT},
        {"BLOB TEXT", VSV_AFF_TEXT},
        {"REAL BLOB", VSV_AFF_BLOB},
        {"BLOB REAL", VSV_AFF_BLOB},
        {"TEXT REAL", VSV_AFF_TEXT},
        {"REAL TEXT", VSV_AFF_TEXT},
        // case-insensitive, anything else is numeric
        {"int", VSV_AFF_INTEGER},
        {"Text", VSV_AFF_TEXT},
        {"STRING", VSV_AFF_NUMERIC},
        {"ANY", VSV_AFF_NUMERIC},
        {" ", VSV_AFF_NUMERIC},
        {"IN T", VSV_AFF_NUMERIC},
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        VsvAffinity got = vsv_affinity(tests[i].type);
        if (got != tests[i].want) {
            printf("%s: got %d, want %d\n", tests[i].type, got, tests[i].want);
        }
        assert(got == tests[i].want);
    }
    printf("OK\n");
}

static void test_real_to_int(void) {
    printf("test_real_to_int...");
    static const struct {
        double r;
        bool ok;
        int64_t want;
    } tests[] = {
        {0.0, true, 0},
        {-0.0, true, 0},
        {1.0, true, 1},
        {-1.0, true, -1},
        {1.5, false, 0},
        {-0.5, false, 0},
        {1e17, true, 100000000000000000},
        {9007199254740993.0, true, 9007199254740992},  // rounded to a double first
        // the largest doubles below 2^63
        {9223372036854774784.0, true, 9223372036854774784},
        {-9223372036854774784.0, true, -9223372036854774784},
        // 2^63 and -2^63 saturate in SQLite, so they aren't integers
        {9223372036854775808.0, false, 0},
        {-9223372036854775808.0, false, 0},
        {1e20, false, 0},
        {INFINITY, false, 0},
        {-INFINITY, false, 0},
        {NAN, false, 0},
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        int64_t got = 0;
        bool ok = vsv_real_to_int(tests[i].r, &got);
        if (ok != tests[i].ok || (ok && got != tests[i].want)) {
            printf("%g: got %d %lld, want %d %lld\n", tests[i].r, ok, (long long)got, tests[i].ok,
                   (long long)tests[i].want);
        }
        assert(ok == tests[i].ok);
        assert(!ok || got == tests[i].want);
    }
    printf("OK\n");
}

int main(void) {
    test_affinity();
    test_real_to_int();
    return 0;
}
