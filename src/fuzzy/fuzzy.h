// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Fuzzy string matching and phonetics.

// All functions take a string as a pointer and a length in bytes, so the
// string doesn't have to be NUL-terminated and may contain NUL characters.
// The distance functions work with Unicode characters: they decode the
// strings as UTF-8, replacing each invalid byte with U+FFFD. The phonetic
// functions only accept ASCII strings.

#ifndef FUZZY_FUZZY_H
#define FUZZY_FUZZY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// FuzzyStatus is the outcome of a function that can fail
// because of bad input or because it runs out of memory.
typedef enum {
    FUZZY_OK,
    FUZZY_INVALID,
    FUZZY_NOMEM,
} FuzzyStatus;

// Distances.

// fuzzy_damlev sets dist to the Damerau-Levenshtein distance between a and b.
// Returns false if out of memory.
bool fuzzy_damlev(const char* a, size_t na, const char* b, size_t nb, size_t* dist);

// fuzzy_editdist sets dist to the spellfix edit distance between a and b.
// Returns FUZZY_INVALID if a or b is not ASCII, and FUZZY_NOMEM
// if out of memory.
FuzzyStatus fuzzy_editdist(const char* a, size_t na, const char* b, size_t nb, int64_t* dist);

// fuzzy_hamming sets dist to the Hamming distance between a and b.
// Returns false if a and b have a different number of characters.
bool fuzzy_hamming(const char* a, size_t na, const char* b, size_t nb, size_t* dist);

// fuzzy_jarowin sets sim to the Jaro-Winkler similarity between a and b.
// Returns false if out of memory.
bool fuzzy_jarowin(const char* a, size_t na, const char* b, size_t nb, double* sim);

// fuzzy_leven sets dist to the Levenshtein distance between a and b.
// Returns false if out of memory.
bool fuzzy_leven(const char* a, size_t na, const char* b, size_t nb, size_t* dist);

// fuzzy_osadist sets dist to the optimal string alignment distance
// between a and b. Returns false if out of memory.
bool fuzzy_osadist(const char* a, size_t na, const char* b, size_t nb, size_t* dist);

// Phonetic codes.

// The phonetic functions write a NUL-terminated code into buf,
// which must hold at least the number of bytes given by the size constant.

// FUZZY_CAVER_SIZE is the buffer size for fuzzy_caver:
// a 10-character code and the NUL.
#define FUZZY_CAVER_SIZE 11

// FUZZY_PHONETIC_SIZE is the buffer size for fuzzy_phonetic with n input
// bytes: at most one code character per input character, and the NUL.
#define FUZZY_PHONETIC_SIZE(n) ((n) + 1)

// FUZZY_SOUNDEX_SIZE is the buffer size for fuzzy_soundex:
// a 4-character code and the NUL.
#define FUZZY_SOUNDEX_SIZE 5

// FUZZY_RSOUNDEX_SIZE is the buffer size for fuzzy_rsoundex with n input
// bytes: the first letter, at most one digit per input character, and the NUL.
#define FUZZY_RSOUNDEX_SIZE(n) ((n) + 2)

// fuzzy_caver writes the Caverphone 2.0 code of s into buf.
// Returns FUZZY_INVALID if s is not ASCII, and FUZZY_NOMEM if out of memory.
FuzzyStatus fuzzy_caver(const char* s, size_t n, char* buf);

// fuzzy_phonetic writes the spellfix phonetic hash of s into buf.
// Returns false if s is not ASCII.
bool fuzzy_phonetic(const char* s, size_t n, char* buf);

// fuzzy_soundex writes the Soundex code of s into buf.
// Returns false if s is not ASCII.
bool fuzzy_soundex(const char* s, size_t n, char* buf);

// fuzzy_rsoundex writes the Refined Soundex code of s into buf.
// Returns false if s is not ASCII.
bool fuzzy_rsoundex(const char* s, size_t n, char* buf);

// Transliteration.

// FUZZY_TRANSLIT_SIZE is the buffer size for fuzzy_translit with n input
// bytes: a character transliterates to at most twice its UTF-8 length
// (the worst case is a two-byte character such as 'щ' becoming "shch";
// ASCII and invalid bytes take one byte each).
#define FUZZY_TRANSLIT_SIZE(n) ((n) * 2)

// fuzzy_translit writes the ASCII transliteration of s into buf and returns
// its length. The result is not NUL-terminated. Characters without
// a transliteration, and invalid UTF-8 bytes, become '?'.
size_t fuzzy_translit(const char* s, size_t n, char* buf);

// fuzzy_script returns the ISO 15924 numeric code of the script s is written
// in: 215 for Latin, 220 for Cyrillic, 200 for Greek, 125 for Hebrew,
// 160 for Arabic, 998 for a mix of these, and 999 for none of them.
int fuzzy_script(const char* s, size_t n);

#endif /* FUZZY_FUZZY_H */
