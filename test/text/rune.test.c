// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Rune tests.

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "text/text.h"

#define LEN(a) (sizeof(a) / sizeof(a[0]))

typedef struct {
    uint32_t c;
    uint32_t fold, lower, upper, title;
} CaseTest;

static void test_case(void) {
    printf("test_case...");
    static const CaseTest tests[] = {
        {'a', 'a', 'a', 'A', 'A'},
        {'A', 'a', 'a', 'A', 'A'},
        {'1', '1', '1', '1', '1'},
        {0, 0, 0, 0, 0},
        {0x00DF, 0x00DF, 0x00DF, 0x1E9E, 0x1E9E},  // ß uppercases to ẞ
        {0x1E9E, 0x00DF, 0x00DF, 0x1E9E, 0x1E9E},  // ẞ
        {0x017F, 's', 0x017F, 'S', 'S'},           // ſ
        {0x212A, 'k', 'k', 0x212A, 0x212A},        // kelvin sign
        {0x0130, 0x0130, 'i', 0x0130, 0x0130},     // İ has no simple folding
        {0x0131, 0x0131, 0x0131, 'I', 'I'},        // ı
        {0x0414, 0x0434, 0x0434, 0x0414, 0x0414},  // Д
        {0x0434, 0x0434, 0x0434, 0x0414, 0x0414},  // д
        {0x2C65, 0x2C65, 0x2C65, 0x023A, 0x023A},  // ⱥ (3 bytes, uppercase 2)
        {0x0250, 0x0250, 0x0250, 0x2C6F, 0x2C6F},  // ɐ (2 bytes, uppercase 3)
        // digraphs: the titlecase is neither upper nor lower
        {0x01C4, 0x01C6, 0x01C6, 0x01C4, 0x01C5},  // Ǆ
        {0x01C5, 0x01C6, 0x01C6, 0x01C4, 0x01C5},  // ǅ
        {0x01C6, 0x01C6, 0x01C6, 0x01C4, 0x01C5},  // ǆ
        {0x01CC, 0x01CC, 0x01CC, 0x01CA, 0x01CB},  // ǌ
        {0x01F3, 0x01F3, 0x01F3, 0x01F1, 0x01F2},  // ǳ
        // Georgian Mkhedruli has an uppercase, but titlecases to itself
        {0x10D0, 0x10D0, 0x10D0, 0x1C90, 0x10D0},
        {0x1C90, 0x10D0, 0x10D0, 0x1C90, 0x1C90},
        {0x01F2, 0x01F3, 0x01F3, 0x01F1, 0x01F2},  // ǲ
        // the tables only cover the BMP, so Deseret 𐐀 has no mappings
        {0x10400, 0x10400, 0x10400, 0x10400, 0x10400},
        {0x1F600, 0x1F600, 0x1F600, 0x1F600, 0x1F600},  // emoji
        {0x10FFFF, 0x10FFFF, 0x10FFFF, 0x10FFFF, 0x10FFFF},
        // ill-formed sequences are not code points and stay as they are
        {0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000},
        {0xC30000, 0xC30000, 0xC30000, 0xC30000, 0xC30000},
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        const CaseTest* t = &tests[i];
        if (rune_casefold(t->c) != t->fold || rune_tolower(t->c) != t->lower ||
            rune_toupper(t->c) != t->upper || rune_totitle(t->c) != t->title) {
            printf("U+%04X: fold %04X, lower %04X, upper %04X, title %04X\n", t->c,
                   rune_casefold(t->c), rune_tolower(t->c), rune_toupper(t->c), rune_totitle(t->c));
        }
        assert(rune_casefold(t->c) == t->fold);
        assert(rune_tolower(t->c) == t->lower);
        assert(rune_toupper(t->c) == t->upper);
        assert(rune_totitle(t->c) == t->title);
    }
    printf("OK\n");
}

typedef struct {
    uint32_t c;
    bool want;
} IsWordTest;

static void test_isword(void) {
    printf("test_isword...");
    static const IsWordTest tests[] = {
        {'a', true},       {'Z', true},   {'0', true}, {'_', true},    {' ', false},
        {'-', false},      {'\'', false}, {0, false},  {0x00E9, true},  // é
        {0x0436, true},                                                 // ж
        {0x0562, true},                                                 // Armenian բ
        {0x05D0, false},  // Hebrew א is uncased and in no listed script
        {0x4E16, true},   // 世
        {0x0661, true},   // Arabic-Indic digit one
        {0x2165, true},   // Roman numeral six (Nl)
        {0x203F, true},   // undertie (Pc)
        {0x01C5, true},   // ǅ (Lt)
        {0x00B2, false},  // superscript two (No)
        {0x2014, false},  // em dash
        {0xFF0000, false},
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        if (rune_isword(tests[i].c) != tests[i].want) {
            printf("U+%04X: want %d\n", tests[i].c, tests[i].want);
        }
        assert(rune_isword(tests[i].c) == tests[i].want);
    }
    printf("OK\n");
}

int main(void) {
    test_case();
    test_isword();
    return 0;
}
