// Originally from the spellfix SQLite exension, Public Domain
// https://www.sqlite.org/src/file/ext/misc/spellfix.c
// Modified by Anton Zhiyanov, https://github.com/nalgeon/sqlean/, MIT License

// Spellfix edit distance.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "fuzzy/common.h"
#include "fuzzy/fuzzy.h"

#pragma region Costs

// FINAL_INS_COST_DIV divides the insertion cost when appending
// to the end of the word.
#define FINAL_INS_COST_DIV 4

// char_at returns the i-th character of s of n bytes, or 0 past the end.
static char char_at(const char* s, size_t n, size_t i) {
    return i < n ? s[i] : 0;
}

// insert_or_delete_cost returns the cost of inserting or deleting
// character c immediately following character prev. If prev is 0,
// c is the first character of the word.
static int insert_or_delete_cost(char prev, char c, char next) {
    unsigned char class_c = char_class(prev, c);
    if (class_c == CCLASS_SILENT) {
        // insert or delete "silent" characters such as H or W
        return 1;
    }
    if (prev == c) {
        // repeated characters, or miss a repeat
        return 10;
    }
    if (class_c == CCLASS_VOWEL && (prev == 'r' || next == 'r')) {
        // insert a vowel before or after 'r'
        return 20;
    }
    unsigned char class_prev = char_class(prev, prev);
    if (class_c == class_prev) {
        if (class_c == CCLASS_VOWEL) {
            // remove or add a new vowel to a vowel cluster
            return 15;
        } else {
            // remove or add a consonant not in the same class
            return 50;
        }
    }
    // any other character insertion or deletion
    return 100;
}

// substitute_cost returns the cost of substituting to in place of from,
// assuming the previous character is prev. If prev is 0, to is the first
// character of the word.
static int substitute_cost(char prev, char from, char to) {
    if (from == to) {
        // exact match
        return 0;
    }
    if (from == (to ^ 0x20) && is_alpha(to)) {
        // differ only in case
        return 0;
    }
    unsigned char class_from = char_class(prev, from);
    unsigned char class_to = char_class(prev, to);
    if (class_from == class_to) {
        // same character class
        return 40;
    }
    if (class_from >= CCLASS_B && class_from <= CCLASS_Y && class_to >= CCLASS_B &&
        class_to <= CCLASS_Y) {
        // convert from one consonant to another, but in a different class
        return 75;
    }
    // any other subsitution
    return 100;
}

#pragma endregion

#pragma region Edit distance

// fuzzy_editdist sets dist to the spellfix edit distance between a and b:
// the cost of transforming a into b, where the costs of edits depend
// on the character classes involved (see the cost functions above).
// Smaller numbers mean a closer match. If a ends with '*', it's treated
// as a prefix of b, and extra characters at the end of b cost little.
// Returns FUZZY_INVALID if a or b is not ASCII, and FUZZY_NOMEM
// if out of memory.
FuzzyStatus fuzzy_editdist(const char* a, size_t na, const char* b, size_t nb, int64_t* dist) {
    if (!is_ascii(a, na) || !is_ascii(b, nb)) {
        return FUZZY_INVALID;
    }

    // skip any common prefix
    char dc = 0;  // north-west character value
    while (na > 0 && nb > 0 && a[0] == b[0]) {
        dc = a[0];
        a++;
        b++;
        na--;
        nb--;
    }
    if (na == 0 && nb == 0) {
        *dist = 0;
        return FUZZY_OK;
    }

    // special processing if either string is empty
    int64_t res = 0;
    if (na == 0) {
        char prev = dc;
        for (size_t xb = 0; xb < nb; xb++) {
            char cb = b[xb];
            res += insert_or_delete_cost(prev, cb, char_at(b, nb, xb + 1)) / FINAL_INS_COST_DIV;
            prev = cb;
        }
        *dist = res;
        return FUZZY_OK;
    }
    if (nb == 0) {
        char prev = dc;
        for (size_t xa = 0; xa < na; xa++) {
            char ca = a[xa];
            res += insert_or_delete_cost(prev, ca, char_at(a, na, xa + 1));
            prev = ca;
        }
        *dist = res;
        return FUZZY_OK;
    }

    // a is a prefix of b
    if (na == 1 && a[0] == '*') {
        *dist = 0;
        return FUZZY_OK;
    }

    // the Wagner matrix row, and the corresponding character values;
    // nb + 1 doesn't overflow: nb is the length of a string in memory
    int64_t* m = alloc_array(nb + 1, sizeof(int64_t));
    char* cx = alloc_array(nb + 1, sizeof(char));
    if (m == NULL || cx == NULL) {
        free(m);
        free(cx);
        return FUZZY_NOMEM;
    }

    // compute the Wagner edit distance
    m[0] = 0;
    cx[0] = dc;
    char bprev = dc;
    for (size_t xb = 1; xb <= nb; xb++) {
        char cb = b[xb - 1];
        cx[xb] = cb;
        m[xb] = m[xb - 1] + insert_or_delete_cost(bprev, cb, char_at(b, nb, xb));
        bprev = cb;
    }
    char ca = 0;
    char aprev = dc;
    for (size_t xa = 1; xa <= na; xa++) {
        bool last_a = (xa == na);
        ca = a[xa - 1];
        char anext = char_at(a, na, xa);
        if (ca == '*' && last_a) {
            break;
        }
        int64_t d = m[0];  // north-west cost value
        m[0] = d + insert_or_delete_cost(aprev, ca, anext);
        for (size_t xb = 1; xb <= nb; xb++) {
            char cb = b[xb - 1];
            char bnext = char_at(b, nb, xb);

            // cost to insert cb
            int ins_cost = insert_or_delete_cost(cx[xb - 1], cb, bnext);
            if (last_a) {
                ins_cost /= FINAL_INS_COST_DIV;
            }
            // cost to delete ca
            int del_cost = insert_or_delete_cost(cx[xb], ca, bnext);
            // cost to substitute ca->cb
            int sub_cost = substitute_cost(cx[xb - 1], ca, cb);

            // best cost
            int64_t total = ins_cost + m[xb - 1];
            char ncx = cb;
            if (del_cost + m[xb] < total) {
                total = del_cost + m[xb];
                ncx = ca;
            }
            if (sub_cost + d < total) {
                total = sub_cost + d;
            }

            // update the matrix
            d = m[xb];
            m[xb] = total;
            cx[xb] = ncx;
        }
        aprev = ca;
    }

    if (ca == '*') {
        // the best match of a prefix of b
        res = m[1];
        for (size_t xb = 1; xb <= nb; xb++) {
            if (m[xb] < res) {
                res = m[xb];
            }
        }
    } else {
        res = m[nb];
    }
    free(m);
    free(cx);
    *dist = res;
    return FUZZY_OK;
}

#pragma endregion
