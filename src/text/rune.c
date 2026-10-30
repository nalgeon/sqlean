/* MIT License
 *
 * Copyright (c) 2023 Tyge Løvset
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// Based on the STC library (utf8code), by Tyge Løvset, MIT License
// https://github.com/stclib/STC

// Case mapping and classes of runes (Unicode code points).

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "text/tables.h"
#include "text/text.h"

#pragma region Runes

static const URangeTable Lt = RANGE_TABLE(Lt);
static const URangeTable Nd = RANGE_TABLE(Nd);
static const URangeTable Nl = RANGE_TABLE(Nl);
static const URangeTable Pc = RANGE_TABLE(Pc);

// The scripts whose uncased letters (such as Han) count as letters.
static const URangeTable scripts[] = {
    RANGE_TABLE(Latin), RANGE_TABLE(Greek),  RANGE_TABLE(Cyrillic),
    RANGE_TABLE(Han),   RANGE_TABLE(Arabic), RANGE_TABLE(Devanagari),
};

// in_table reports whether c is in one of the ranges of the table.
static bool in_table(const URangeTable* table, uint32_t c) {
    for (size_t i = 0; i < table->len; i++) {
        if (c < table->ranges[i].lo) {
            return false;
        }
        if (c <= table->ranges[i].hi) {
            return true;
        }
    }
    return false;
}

// lookup_lower maps c using the mapping entry if c is in its uppercase range.
// Sets *done if c is below the end of the range, since the entries are sorted
// and later ones can't contain c either.
static uint32_t lookup_lower(const CaseMapping* entry, uint32_t c, bool* done) {
    if (c > entry->c2) {
        *done = false;
        return c;
    }
    *done = true;
    if (c < entry->c1) {
        return c;
    }
    int d = entry->m2 - entry->c2;
    if (d == 1) {
        // Alternating upper and lower case: only the uppercase ones map.
        return c + ((entry->c2 & 1) == (c & 1));
    }
    return (uint32_t)((int)c + d);
}

// rune_casefold returns the simple case folding of the code point c,
// or c if it has none.
uint32_t rune_casefold(uint32_t c) {
    for (size_t i = 0; i < casefold_len; i++) {
        bool done;
        uint32_t res = lookup_lower(&casemappings[i], c, &done);
        if (done) {
            return res;
        }
    }
    return c;
}

// is_digraph_title reports whether c is one of the titlecase digraphs
// ǅ ǈ ǋ ǲ, which the tables don't map. Each sits between its uppercase
// (c - 1) and its lowercase (c + 1).
static bool is_digraph_title(uint32_t c) {
    return c == 0x01C5 || c == 0x01C8 || c == 0x01CB || c == 0x01F2;
}

// rune_tolower returns the lowercase of the code point c, or c if it has none.
uint32_t rune_tolower(uint32_t c) {
    if (is_digraph_title(c)) {
        return c + 1;
    }
    for (size_t i = 0; i < sizeof(upcase_ind) / sizeof(upcase_ind[0]); i++) {
        bool done;
        uint32_t res = lookup_lower(&casemappings[upcase_ind[i]], c, &done);
        if (done) {
            return res;
        }
    }
    return c;
}

// rune_toupper returns the uppercase of the code point c, or c if it has none.
uint32_t rune_toupper(uint32_t c) {
    if (is_digraph_title(c)) {
        return c - 1;
    }
    for (size_t i = 0; i < sizeof(lowcase_ind) / sizeof(lowcase_ind[0]); i++) {
        const CaseMapping* entry = &casemappings[lowcase_ind[i]];
        if (c <= entry->m2) {
            int d = entry->m2 - entry->c2;
            if (c < (uint32_t)(entry->c1 + d)) {
                return c;
            }
            if (d == 1) {
                // Alternating upper and lower case: only the lowercase ones map.
                return c - ((entry->m2 & 1) == (c & 1));
            }
            return (uint32_t)((int)c - d);
        }
    }
    return c;
}

// rune_totitle returns the titlecase of the code point c, or c if it has none.
// It is the uppercase, except for the digraphs that have a titlecase form
// (ǆ → ǅ, not Ǆ) and the Georgian letters, which have an uppercase
// (Mtavruli) but are their own titlecase. The same exceptions as in
// Go's unicode.ToTitle.
uint32_t rune_totitle(uint32_t c) {
    if (c >= 0x01C4 && c <= 0x01CC) {
        // Ǆ ǅ ǆ, Ǉ ǈ ǉ, Ǌ ǋ ǌ: the titlecase is in the middle of each triple.
        return 0x01C5 + (c - 0x01C4) / 3 * 3;
    }
    if (c >= 0x01F1 && c <= 0x01F3) {
        // Ǳ ǲ ǳ
        return 0x01F2;
    }
    if (c >= 0x10D0 && c <= 0x10FF) {
        // Georgian Mkhedruli
        return c;
    }
    return rune_toupper(c);
}

// rune_isword reports whether the code point c is a letter, a digit
// or a connector punctuation (such as an underscore). A letter is a cased
// character, a titlecase letter, a letter number (such as Ⅻ), or a letter
// of one of the common uncased scripts.
bool rune_isword(uint32_t c) {
    if (c < 0x80) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
               c == '_';
    }
    if (rune_tolower(c) != c || rune_toupper(c) != c) {
        return true;
    }
    if (in_table(&Lt, c) || in_table(&Nd, c) || in_table(&Nl, c) || in_table(&Pc, c)) {
        return true;
    }
    for (size_t i = 0; i < sizeof(scripts) / sizeof(scripts[0]); i++) {
        if (in_table(&scripts[i], c)) {
            return true;
        }
    }
    return false;
}

#pragma endregion
