// Copyright (c) 2014 Ross Bayer, MIT License
// https://github.com/Rostepher/libstrcmp
// Modified by Anton Zhiyanov, https://github.com/nalgeon/sqlean/, MIT License

// Soundex and Refined Soundex phonetic codes.

#include <stddef.h>

#include "fuzzy/common.h"
#include "fuzzy/fuzzy.h"

#pragma region Helpers

// skip_non_letters returns the index of the first letter in s,
// or n if there are none.
static size_t skip_non_letters(const char* s, size_t n) {
    size_t i = 0;
    while (i < n && !is_alpha(s[i])) {
        i++;
    }
    return i;
}

#pragma endregion

#pragma region Soundex

// soundex_encode returns the Soundex digit for c,
// or '0' for vowels and non-letters.
static char soundex_encode(char c) {
    switch (to_lower(c)) {
        case 'b':
        case 'f':
        case 'p':
        case 'v':
            return '1';
        case 'c':
        case 'g':
        case 'j':
        case 'k':
        case 'q':
        case 's':
        case 'x':
        case 'z':
            return '2';
        case 'd':
        case 't':
            return '3';
        case 'l':
            return '4';
        case 'm':
        case 'n':
            return '5';
        case 'r':
            return '6';
        default:
            return '0';
    }
}

// fuzzy_soundex writes the Soundex code of s into buf, which must hold
// at least FUZZY_SOUNDEX_SIZE bytes. The code is the first letter of s
// followed by three digits. Leading non-letters are skipped,
// and a string without letters has an empty code.
// See https://en.wikipedia.org/wiki/Soundex
// Returns FUZZY_INVALID if s is not ASCII.
FuzzyStatus fuzzy_soundex(const char* s, size_t n, char* buf) {
    if (!is_ascii(s, n)) {
        return FUZZY_INVALID;
    }
    size_t start = skip_non_letters(s, n);
    if (start == n) {
        buf[0] = 0;
        return FUZZY_OK;
    }

    buf[0] = to_upper(s[start]);
    size_t d = 1;
    for (size_t i = start + 1; i < n && d < 4; i++) {
        char code = soundex_encode(s[i]);
        if (code == '0' || code == soundex_encode(s[i - 1])) {
            continue;
        }
        // digits separated by an 'h' or 'w' are coded once
        char sep = to_lower(s[i - 1]);
        if (i > start + 1 && code == soundex_encode(s[i - 2]) && (sep == 'h' || sep == 'w')) {
            continue;
        }
        buf[d++] = code;
    }
    // pad with zeros if too short
    while (d < 4) {
        buf[d++] = '0';
    }
    buf[d] = 0;
    return FUZZY_OK;
}

#pragma endregion

#pragma region Refined Soundex

// rsoundex_encode returns the Refined Soundex digit for c,
// or '0' for vowels and non-letters.
static char rsoundex_encode(char c) {
    switch (to_lower(c)) {
        case 'b':
        case 'p':
            return '1';
        case 'f':
        case 'v':
            return '2';
        case 'c':
        case 'k':
        case 's':
            return '3';
        case 'g':
        case 'j':
            return '4';
        case 'q':
        case 'x':
        case 'z':
            return '5';
        case 'd':
        case 't':
            return '6';
        case 'l':
            return '7';
        case 'm':
        case 'n':
            return '8';
        case 'r':
            return '9';
        default:
            return '0';
    }
}

// fuzzy_rsoundex writes the Refined Soundex code of s into buf, which must
// hold at least FUZZY_RSOUNDEX_SIZE(n) bytes. The code is the first letter
// of s followed by the digits of every character, with repeated digits
// coded once. Leading non-letters are skipped, and a string without letters
// has an empty code.
// See http://ntz-develop.blogspot.com/2011/03/phonetic-algorithms.html
// Returns FUZZY_INVALID if s is not ASCII.
FuzzyStatus fuzzy_rsoundex(const char* s, size_t n, char* buf) {
    if (!is_ascii(s, n)) {
        return FUZZY_INVALID;
    }
    size_t start = skip_non_letters(s, n);
    if (start == n) {
        buf[0] = 0;
        return FUZZY_OK;
    }

    buf[0] = to_upper(s[start]);
    size_t d = 1;
    char prev = 0;
    for (size_t i = start; i < n; i++) {
        char code = rsoundex_encode(s[i]);
        if (code != prev) {
            buf[d++] = code;
            prev = code;
        }
    }
    buf[d] = 0;
    return FUZZY_OK;
}

#pragma endregion
