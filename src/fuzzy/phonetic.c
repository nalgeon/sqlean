// Originally from the spellfix SQLite exension, Public Domain
// https://www.sqlite.org/src/file/ext/misc/spellfix.c
// Modified by Anton Zhiyanov, https://github.com/nalgeon/sqlean/, MIT License

// Spellfix phonetic hash.

#include <stdbool.h>
#include <stddef.h>

#include "fuzzy/common.h"
#include "fuzzy/fuzzy.h"

#pragma region Phonetic hash

// fuzzy_phonetic writes the spellfix phonetic hash of s into buf,
// which must hold at least FUZZY_PHONETIC_SIZE(n) bytes:
//
//   - Map characters by character class (see common.h).
//   - Omit double letters.
//   - Omit vowels beside R and L.
//   - Omit T when followed by CH.
//   - Omit W when followed by R.
//   - Omit D when followed by J or G.
//   - Omit K in KN or G in GN at the beginning of a word
//     (of more than two characters).
//
// Returns false if s is not ASCII.
bool fuzzy_phonetic(const char* s, size_t n, char* buf) {
    if (!is_ascii(s, n)) {
        return false;
    }
    if (n > 2 && (s[0] == 'g' || s[0] == 'k') && s[1] == 'n') {
        s++;
        n--;
    }

    size_t len = 0;
    unsigned char prev = 0x77;
    unsigned char prevx = 0x77;
    const unsigned char* classes = init_class;
    for (size_t i = 0; i < n; i++) {
        char c = s[i];
        if (i + 1 < n) {
            if (c == 'w' && s[i + 1] == 'r') {
                continue;
            }
            if (c == 'd' && (s[i + 1] == 'j' || s[i + 1] == 'g')) {
                continue;
            }
            if (i + 2 < n && c == 't' && s[i + 1] == 'c' && s[i + 2] == 'h') {
                continue;
            }
        }
        unsigned char cls = classes[(unsigned char)c];
        if (cls == CCLASS_SPACE) {
            continue;
        }
        if (cls == CCLASS_OTHER && prev != CCLASS_DIGIT) {
            continue;
        }
        classes = mid_class;
        if (cls == CCLASS_VOWEL && (prevx == CCLASS_R || prevx == CCLASS_L)) {
            // no vowels beside L or R
            continue;
        }
        if ((cls == CCLASS_R || cls == CCLASS_L) && prevx == CCLASS_VOWEL && len > 0) {
            // no vowels beside L or R
            len--;
        }
        prev = cls;
        if (cls == CCLASS_SILENT) {
            continue;
        }
        prevx = cls;
        char code = class_name[cls];
        if (len == 0 || code != buf[len - 1]) {
            buf[len++] = code;
        }
    }
    buf[len] = 0;
    return true;
}

#pragma endregion
