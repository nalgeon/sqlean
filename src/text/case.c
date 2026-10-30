// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Case conversion and case-insensitive comparison.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "text/common.h"
#include "text/text.h"

#pragma region Case

// CaseMode is the conversion change_case does.
typedef enum { CASE_UPPER, CASE_LOWER, CASE_TITLE, CASE_FOLD } CaseMode;

// change_case writes s converted by mode to out, and returns the size of
// the result. Ill-formed sequences are kept as they are. A conversion may
// change the size of a character (ɐ is 2 bytes, its uppercase Ɐ is 3),
// so the result may be larger or smaller than s.
static size_t change_case(const char* s, size_t n, char* out, CaseMode mode) {
    size_t pos = 0;
    // For CASE_TITLE: whether the previous character is part of a word.
    bool in_word = false;
    for (size_t i = 0; i < n;) {
        size_t start = i;
        uint32_t c = utf8_next(s, n, &i);
        if (!utf8_valid(c)) {
            pos = put(out, pos, s + start, i - start);
            in_word = false;
            continue;
        }
        uint32_t res = c;
        switch (mode) {
            case CASE_UPPER:
                res = rune_toupper(c);
                break;
            case CASE_LOWER:
                res = rune_tolower(c);
                break;
            case CASE_TITLE:
                res = in_word ? rune_tolower(c) : rune_totitle(c);
                in_word = rune_isword(c);
                break;
            case CASE_FOLD:
                res = rune_casefold(c);
                break;
        }
        char buf[4];
        pos = put(out, pos, buf, utf8_encode(buf, res));
    }
    return pos;
}

// text_upper writes s in uppercase to out, and returns the size of the result.
// It uses the simple case mappings, one character to one, so ß becomes ẞ
// and not SS.
size_t text_upper(const char* s, size_t n, char* out) {
    return change_case(s, n, out, CASE_UPPER);
}

// text_lower writes s in lowercase to out, and returns the size of the result.
// It uses the simple case mappings, one character to one.
size_t text_lower(const char* s, size_t n, char* out) {
    return change_case(s, n, out, CASE_LOWER);
}

// text_title writes s in title case to out, and returns the size of the result.
// The first character of each word is in titlecase and the rest are in
// lowercase. A word is a run of letters, digits and connector punctuation
// (see rune_isword), so an apostrophe starts a new word: don't → Don'T,
// as in Postgres' initcap.
size_t text_title(const char* s, size_t n, char* out) {
    return change_case(s, n, out, CASE_TITLE);
}

// text_casefold writes s case-folded to out, and returns the size of the result.
// It uses the simple case folding, one character to one.
size_t text_casefold(const char* s, size_t n, char* out) {
    return change_case(s, n, out, CASE_FOLD);
}

// text_casecmp compares s1 and s2 case-insensitively, and returns a negative
// number, zero or a positive number if s1 is less than, equal to or greater
// than s2. It compares the case-folded code points one by one, so the order
// is by code point and not linguistic. An ill-formed sequence only equals
// the same bytes, and sorts after every valid character.
int text_casecmp(const char* s1, size_t n1, const char* s2, size_t n2) {
    size_t i1 = 0, i2 = 0;
    while (i1 < n1 && i2 < n2) {
        uint32_t c1 = rune_casefold(utf8_next(s1, n1, &i1));
        uint32_t c2 = rune_casefold(utf8_next(s2, n2, &i2));
        if (c1 != c2) {
            return c1 < c2 ? -1 : 1;
        }
    }
    // The string that has characters left is the greater one. Byte lengths
    // can't tell, since a character and its folding may differ in size.
    return (i1 < n1) - (i2 < n2);
}

#pragma endregion
