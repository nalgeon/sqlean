// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Based on SQLite's sqlite3AffinityType and sqlite3VdbeIntegerAffinity,
// Public Domain, https://sqlite.org/src/file/src/build.c

// SQLite's type affinity rules.

#include <stdbool.h>
#include <stdint.h>

#include "vsv/vsv.h"

// vsv_affinity returns the affinity of a column with the declared type.
// The first matching rule wins:
//
// 1. "INT" anywhere: INTEGER.
// 2. "CHAR", "CLOB" or "TEXT": TEXT.
// 3. "BLOB": BLOB.
// 4. "REAL", "FLOA" or "DOUB": REAL.
// 5. Otherwise: NUMERIC.
//
// SQLite scans the type once, keeping the last four characters in a hash,
// so that's what this does too.
VsvAffinity vsv_affinity(const char* type) {
    if (type[0] == '\0') {
        return VSV_AFF_BLOB;
    }
    uint32_t h = 0;
    VsvAffinity aff = VSV_AFF_NUMERIC;
    for (const unsigned char* s = (const unsigned char*)type; *s != '\0'; s++) {
        unsigned char c = *s;
        if (c >= 'A' && c <= 'Z') {
            c += 'a' - 'A';
        }
        h = (h << 8) + c;
        if (h == (('c' << 24) + ('h' << 16) + ('a' << 8) + 'r') ||
            h == (('c' << 24) + ('l' << 16) + ('o' << 8) + 'b') ||
            h == (('t' << 24) + ('e' << 16) + ('x' << 8) + 't')) {
            aff = VSV_AFF_TEXT;
        } else if (h == (('b' << 24) + ('l' << 16) + ('o' << 8) + 'b') &&
                   (aff == VSV_AFF_NUMERIC || aff == VSV_AFF_REAL)) {
            aff = VSV_AFF_BLOB;
        } else if ((h == (('r' << 24) + ('e' << 16) + ('a' << 8) + 'l') ||
                    h == (('f' << 24) + ('l' << 16) + ('o' << 8) + 'a') ||
                    h == (('d' << 24) + ('o' << 16) + ('u' << 8) + 'b')) &&
                   aff == VSV_AFF_NUMERIC) {
            aff = VSV_AFF_REAL;
        } else if ((h & 0x00ffffff) == (('i' << 16) + ('n' << 8) + 't')) {
            return VSV_AFF_INTEGER;
        }
    }
    return aff;
}

// vsv_real_to_int reports whether SQLite stores r as an integer
// in an INTEGER or NUMERIC column, and sets i to its value.
bool vsv_real_to_int(double r, int64_t* i) {
    // The largest double below 2^63. SQLite saturates anything beyond it,
    // which never round-trips. Checking the range first also avoids
    // converting a double outside the range of int64_t (or NaN), which is UB.
    const double limit = 9223372036854774784.0;
    if (!(r >= -limit && r <= limit)) {
        return false;
    }
    int64_t v = (int64_t)r;
    if ((double)v != r) {
        return false;
    }
    *i = v;
    return true;
}
