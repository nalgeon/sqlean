// Copyright (c) 2021 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Caverphone phonetic coding algorithm.
// https://en.wikipedia.org/wiki/Caverphone

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "fuzzy/common.h"
#include "fuzzy/fuzzy.h"

#pragma region Replacements

// The replacements work in place on the string s of n bytes and update n.
// None of them makes the string longer.

// remove_non_letters lowercases s and deletes everything
// except the letters a-z from it.
static void remove_non_letters(char* s, size_t* n) {
    size_t len = 0;
    for (size_t i = 0; i < *n; i++) {
        char c = to_lower(s[i]);
        if (c >= 'a' && c <= 'z') {
            s[len++] = c;
        }
    }
    *n = len;
}

// replace_start replaces old with new if s starts with old.
static void replace_start(char* s, size_t* n, const char* old, const char* new) {
    size_t old_len = strlen(old);
    size_t new_len = strlen(new);
    if (*n < old_len || memcmp(s, old, old_len) != 0) {
        return;
    }
    memcpy(s, new, new_len);
    memmove(s + new_len, s + old_len, *n - old_len);
    *n = *n - old_len + new_len;
}

// replace_end replaces old with new if s ends with old.
static void replace_end(char* s, size_t* n, const char* old, const char* new) {
    size_t old_len = strlen(old);
    size_t new_len = strlen(new);
    if (*n < old_len || memcmp(s + *n - old_len, old, old_len) != 0) {
        return;
    }
    memcpy(s + *n - old_len, new, new_len);
    *n = *n - old_len + new_len;
}

// replace replaces all occurrences of old with new in s.
// Since new is not longer than old, the write position
// never gets ahead of the read position.
static void replace(char* s, size_t* n, const char* old, const char* new) {
    size_t old_len = strlen(old);
    size_t new_len = strlen(new);
    size_t len = 0;
    size_t i = 0;
    while (i < *n) {
        if (*n - i >= old_len && memcmp(s + i, old, old_len) == 0) {
            memcpy(s + len, new, new_len);
            len += new_len;
            i += old_len;
        } else {
            s[len++] = s[i++];
        }
    }
    *n = len;
}

// replace_seq replaces every run of the old character with new in s.
static void replace_seq(char* s, size_t* n, char old, char new) {
    size_t len = 0;
    size_t i = 0;
    while (i < *n) {
        if (s[i] == old) {
            while (i < *n && s[i] == old) {
                i++;
            }
            s[len++] = new;
        } else {
            s[len++] = s[i++];
        }
    }
    *n = len;
}

#pragma endregion

#pragma region Caverphone

// fuzzy_caver writes the Caverphone 2.0 code of s into buf, which must hold
// at least FUZZY_CAVER_SIZE bytes. The code is 10 characters long, except
// for a string without letters, which has an empty code. Non-letters
// are ignored.
// Returns FUZZY_INVALID if s is not ASCII, and FUZZY_NOMEM if out of memory.
FuzzyStatus fuzzy_caver(const char* s, size_t n, char* buf) {
    if (!is_ascii(s, n)) {
        return FUZZY_INVALID;
    }
    char* w = alloc_array(n, sizeof(char));
    if (w == NULL) {
        return FUZZY_NOMEM;
    }
    memcpy(w, s, n);

    // Convert to lowercase, remove anything not in the standard alphabet
    remove_non_letters(w, &n);
    if (n == 0) {
        buf[0] = 0;
        free(w);
        return FUZZY_OK;
    }
    // Remove final e
    replace_end(w, &n, "e", "");
    // If the name starts with *gh make it *2f
    replace_start(w, &n, "cough", "cou2f");
    replace_start(w, &n, "rough", "rou2f");
    replace_start(w, &n, "tough", "tou2f");
    replace_start(w, &n, "enough", "enou2f");
    replace_start(w, &n, "trough", "trou2f");
    // If the name starts with gn make it 2n
    replace_start(w, &n, "gn", "2n");
    // If the name ends with mb make it m2
    replace_end(w, &n, "mb", "m2");
    // replace cq with 2q
    replace(w, &n, "cq", "2q");
    // replace c[iey] with s[iey]
    replace(w, &n, "ci", "si");
    replace(w, &n, "ce", "se");
    replace(w, &n, "cy", "sy");
    // replace tch with 2ch
    replace(w, &n, "tch", "2ch");
    // replace [cqx] with k
    replace(w, &n, "c", "k");
    replace(w, &n, "q", "k");
    replace(w, &n, "x", "k");
    // replace v with f
    replace(w, &n, "v", "f");
    // replace dg with 2g
    replace(w, &n, "dg", "2g");
    // replace ti[oa] with si[oa]
    replace(w, &n, "tio", "sio");
    replace(w, &n, "tia", "sia");
    // replace d with t
    replace(w, &n, "d", "t");
    // replace ph with fh
    replace(w, &n, "ph", "fh");
    // replace b with p
    replace(w, &n, "b", "p");
    // replace sh with s2
    replace(w, &n, "sh", "s2");
    // replace z with s
    replace(w, &n, "z", "s");
    // replace an initial vowel [aeiou] with an A
    replace_start(w, &n, "a", "A");
    replace_start(w, &n, "e", "A");
    replace_start(w, &n, "i", "A");
    replace_start(w, &n, "o", "A");
    replace_start(w, &n, "u", "A");
    // replace all other vowels with a 3
    replace(w, &n, "a", "3");
    replace(w, &n, "e", "3");
    replace(w, &n, "i", "3");
    replace(w, &n, "o", "3");
    replace(w, &n, "u", "3");
    // replace j with y
    replace(w, &n, "j", "y");
    // replace an initial y3 with Y3
    replace_start(w, &n, "y3", "Y3");
    // replace an initial y with A
    replace_start(w, &n, "y", "A");
    // replace y with 3
    replace(w, &n, "y", "3");
    // replace 3gh3 with 3kh3
    replace(w, &n, "3gh3", "3kh3");
    // replace gh with 22
    replace(w, &n, "gh", "22");
    // replace g with k
    replace(w, &n, "g", "k");
    // replace sequence of the letter [stpkfmn] with an uppercased letter
    replace_seq(w, &n, 's', 'S');
    replace_seq(w, &n, 't', 'T');
    replace_seq(w, &n, 'p', 'P');
    replace_seq(w, &n, 'k', 'K');
    replace_seq(w, &n, 'f', 'F');
    replace_seq(w, &n, 'm', 'M');
    replace_seq(w, &n, 'n', 'N');
    // replace w3 with W3
    replace(w, &n, "w3", "W3");
    // replace wh3 with Wh3
    replace(w, &n, "wh3", "Wh3");
    // replace the final w with 3
    replace_end(w, &n, "w", "3");
    // replace w with 2
    replace(w, &n, "w", "2");
    // replace an initial h with an A
    replace_start(w, &n, "h", "A");
    // replace all other occurrences of h with a 2
    replace(w, &n, "h", "2");
    // replace r3 with R3
    replace(w, &n, "r3", "R3");
    // replace the final r with 3
    replace_end(w, &n, "r", "3");
    // replace r with 2
    replace(w, &n, "r", "2");
    // replace l3 with L3
    replace(w, &n, "l3", "L3");
    // replace the final l with 3
    replace_end(w, &n, "l", "3");
    // replace l with 2
    replace(w, &n, "l", "2");
    // remove all 2s
    replace(w, &n, "2", "");
    // replace the final 3 with A
    replace_end(w, &n, "3", "A");
    // remove all 3s
    replace(w, &n, "3", "");
    // put ten 1s on the end and take the first ten characters as the code
    size_t len = n < 10 ? n : 10;
    memcpy(buf, w, len);
    memset(buf + len, '1', 10 - len);
    buf[10] = 0;
    free(w);
    return FUZZY_OK;
}

#pragma endregion
