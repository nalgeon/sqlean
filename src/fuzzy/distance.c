// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Based on the go-edlib package by Hugo Bollon, MIT License
// https://github.com/hbollon/go-edlib

// String distances: Damerau-Levenshtein, Hamming, Jaro-Winkler,
// Levenshtein and optimal string alignment.

// Deviations from go-edlib:
//   - Jaro-Winkler is computed in double rather than float32 precision.
//   - The Damerau-Levenshtein alphabet is a sorted array rather than a map.
//   - Jaro-Winkler similarity of two empty strings is 1 (they are equal)
//     rather than 0.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fuzzy/common.h"
#include "fuzzy/fuzzy.h"

#pragma region Helpers

// min2 returns the smaller of a and b.
static size_t min2(size_t a, size_t b) {
    return a < b ? a : b;
}

// min3 returns the smallest of a, b and c.
static size_t min3(size_t a, size_t b, size_t c) {
    return min2(min2(a, b), c);
}

// runes_equal reports whether the code point arrays a and b are equal.
static bool runes_equal(const uint32_t* a, size_t na, const uint32_t* b, size_t nb) {
    return na == nb && (na == 0 || memcmp(a, b, na * sizeof(uint32_t)) == 0);
}

// Runes is a string decoded into code points.
typedef struct {
    uint32_t* p;
    size_t n;
} Runes;

// decode_pair decodes a and b into ra and rb. Returns false if out
// of memory. On success, the caller frees ra->p and rb->p.
static bool decode_pair(const char* a, size_t na, const char* b, size_t nb, Runes* ra, Runes* rb) {
    ra->p = utf8_runes(a, na, &ra->n);
    if (ra->p == NULL) {
        return false;
    }
    rb->p = utf8_runes(b, nb, &rb->n);
    if (rb->p == NULL) {
        free(ra->p);
        return false;
    }
    return true;
}

// RuneDistance computes an edit distance between the non-empty, different
// code point arrays a and b. Returns false if out of memory.
typedef bool (
    *RuneDistance)(const uint32_t* a, size_t na, const uint32_t* b, size_t nb, size_t* dist);

// edit_distance sets dist to the edit distance between a and b computed
// by distance. The distance to an empty string is the length of the other
// string, and the distance between equal strings is 0, so distance only
// handles the rest. Returns FUZZY_NOMEM if out of memory.
static FuzzyStatus edit_distance(const char* a,
                                 size_t na,
                                 const char* b,
                                 size_t nb,
                                 RuneDistance distance,
                                 size_t* dist) {
    Runes ra, rb;
    if (!decode_pair(a, na, b, nb, &ra, &rb)) {
        return FUZZY_NOMEM;
    }
    bool ok = true;
    if (ra.n == 0) {
        *dist = rb.n;
    } else if (rb.n == 0) {
        *dist = ra.n;
    } else if (runes_equal(ra.p, ra.n, rb.p, rb.n)) {
        *dist = 0;
    } else {
        ok = distance(ra.p, ra.n, rb.p, rb.n, dist);
    }
    free(ra.p);
    free(rb.p);
    return ok ? FUZZY_OK : FUZZY_NOMEM;
}

// alloc_matrix allocates a rows x cols matrix of size_t values.
// Returns NULL if out of memory or if the size overflows.
static size_t* alloc_matrix(size_t rows, size_t cols) {
    if (cols != 0 && rows > SIZE_MAX / cols) {
        return NULL;
    }
    return alloc_array(rows * cols, sizeof(size_t));
}

// compare_runes orders code points for qsort and bsearch.
static int compare_runes(const void* a, const void* b) {
    uint32_t x = *(const uint32_t*)a;
    uint32_t y = *(const uint32_t*)b;
    return (x > y) - (x < y);
}

#pragma endregion

#pragma region Damerau-Levenshtein

// damlev computes the true Damerau-Levenshtein distance between
// the non-empty code point arrays a and b. Returns false if out of memory.
static bool damlev(const uint32_t* a, size_t na, const uint32_t* b, size_t nb, size_t* dist) {
    // The alphabet of both strings: a sorted array of unique code points.
    // da[k] is the last row where the alphabet's k-th character occurs in a.
    // na + nb doesn't overflow: both are counts of array elements.
    uint32_t* alphabet = alloc_array(na + nb, sizeof(uint32_t));
    size_t* ia = alloc_array(na, sizeof(size_t));
    size_t* ib = alloc_array(nb, sizeof(size_t));
    size_t* da = alloc_array(na + nb, sizeof(size_t));
    // matrix[0..na+1][0..nb+1]
    size_t cols = nb + 2;
    size_t* matrix = alloc_matrix(na + 2, cols);
    bool ok = alphabet && ia && ib && da && matrix;
    if (!ok) {
        goto done;
    }

    memcpy(alphabet, a, na * sizeof(uint32_t));
    memcpy(alphabet + na, b, nb * sizeof(uint32_t));
    qsort(alphabet, na + nb, sizeof(uint32_t), compare_runes);
    size_t nalpha = 1;
    for (size_t i = 1; i < na + nb; i++) {
        if (alphabet[i] != alphabet[nalpha - 1]) {
            alphabet[nalpha++] = alphabet[i];
        }
    }
    for (size_t i = 0; i < na; i++) {
        const uint32_t* found = bsearch(&a[i], alphabet, nalpha, sizeof(uint32_t), compare_runes);
        ia[i] = (size_t)(found - alphabet);
    }
    for (size_t j = 0; j < nb; j++) {
        const uint32_t* found = bsearch(&b[j], alphabet, nalpha, sizeof(uint32_t), compare_runes);
        ib[j] = (size_t)(found - alphabet);
    }
    memset(da, 0, nalpha * sizeof(size_t));

#define M(i, j) matrix[(i) * cols + (j)]
    // maximum possible distance
    size_t max_dist = na + nb;
    M(0, 0) = max_dist;
    for (size_t i = 0; i <= na; i++) {
        M(i + 1, 0) = max_dist;
        M(i + 1, 1) = i;
    }
    for (size_t j = 0; j <= nb; j++) {
        M(0, j + 1) = max_dist;
        M(1, j + 1) = j;
    }

    for (size_t i = 1; i <= na; i++) {
        size_t db = 0;
        for (size_t j = 1; j <= nb; j++) {
            size_t i1 = da[ib[j - 1]];
            size_t j1 = db;
            size_t cost = 1;
            if (a[i - 1] == b[j - 1]) {
                cost = 0;
                db = j;
            }
            // i1 < i and j1 < j, so the transposition cost doesn't underflow
            size_t trans = M(i1, j1) + (i - i1 - 1) + 1 + (j - j1 - 1);
            M(i + 1, j + 1) = min2(min3(M(i + 1, j) + 1,  // insertion
                                        M(i, j + 1) + 1,  // deletion
                                        M(i, j) + cost),  // substitution
                                   trans);                // transposition
        }
        da[ia[i - 1]] = i;
    }
    *dist = M(na + 1, nb + 1);
#undef M

done:
    free(alphabet);
    free(ia);
    free(ib);
    free(da);
    free(matrix);
    return ok;
}

// fuzzy_damlev sets dist to the Damerau-Levenshtein distance between a and b:
// the minimum number of insertions, deletions, substitutions and transpositions
// of adjacent characters that turn a into b. Unlike the optimal string
// alignment distance, it allows editing a substring more than once.
// Returns FUZZY_NOMEM if out of memory.
FuzzyStatus fuzzy_damlev(const char* a, size_t na, const char* b, size_t nb, size_t* dist) {
    return edit_distance(a, na, b, nb, damlev, dist);
}

#pragma endregion

#pragma region Hamming

// fuzzy_hamming sets dist to the Hamming distance between a and b:
// the number of positions where their characters differ.
// Returns FUZZY_INVALID if a and b have a different number of characters.
FuzzyStatus fuzzy_hamming(const char* a, size_t na, const char* b, size_t nb, size_t* dist) {
    size_t i = 0, j = 0, count = 0;
    while (i < na && j < nb) {
        size_t sa, sb;
        uint32_t ca = utf8_decode(a + i, na - i, &sa);
        uint32_t cb = utf8_decode(b + j, nb - j, &sb);
        if (ca != cb) {
            count++;
        }
        i += sa;
        j += sb;
    }
    if (i < na || j < nb) {
        return FUZZY_INVALID;
    }
    *dist = count;
    return FUZZY_OK;
}

#pragma endregion

#pragma region Jaro-Winkler

// jaro sets sim to the Jaro similarity between the non-empty, different
// code point arrays a and b. Returns false if out of memory.
static bool jaro(const uint32_t* a, size_t na, const uint32_t* b, size_t nb, double* sim) {
    bool* matched = calloc(na + nb, sizeof(bool));
    if (matched == NULL) {
        return false;
    }
    bool* amatch = matched;
    bool* bmatch = matched + na;

    // maximum matching distance allowed: max(na, nb) / 2 - 1, or 0
    // (go-edlib lets it go negative, which gives the same result)
    size_t longest = na > nb ? na : nb;
    size_t max_dist = longest / 2 > 0 ? longest / 2 - 1 : 0;

    // find the matching characters in both strings
    size_t match = 0;
    for (size_t i = 0; i < na; i++) {
        size_t lo = i > max_dist ? i - max_dist : 0;
        size_t hi = min2(nb, i + max_dist + 1);
        for (size_t j = lo; j < hi; j++) {
            if (a[i] == b[j] && !bmatch[j]) {
                amatch[i] = true;
                bmatch[j] = true;
                match++;
                break;
            }
        }
    }
    if (match == 0) {
        free(matched);
        *sim = 0.0;
        return true;
    }

    // count the transpositions: matching characters in a different order
    size_t unaligned = 0;
    size_t p = 0;
    for (size_t i = 0; i < na; i++) {
        if (amatch[i]) {
            while (!bmatch[p]) {
                p++;
            }
            if (a[i] != b[p]) {
                unaligned++;
            }
            p++;
        }
    }
    free(matched);

    double m = (double)match;
    double t = (double)unaligned / 2;
    *sim = (m / (double)na + m / (double)nb + (m - t) / m) / 3.0;
    return true;
}

// fuzzy_jarowin sets sim to the Jaro-Winkler similarity between a and b:
// from 0 for no similarity to 1 for an exact match. It's the Jaro similarity
// boosted by 0.1 for each character of a common prefix of up to 4 characters.
// Returns FUZZY_NOMEM if out of memory.
FuzzyStatus fuzzy_jarowin(const char* a, size_t na, const char* b, size_t nb, double* sim) {
    Runes ra, rb;
    if (!decode_pair(a, na, b, nb, &ra, &rb)) {
        return FUZZY_NOMEM;
    }
    bool ok = true;
    if (runes_equal(ra.p, ra.n, rb.p, rb.n)) {
        *sim = 1.0;
    } else if (ra.n == 0 || rb.n == 0) {
        *sim = 0.0;
    } else {
        ok = jaro(ra.p, ra.n, rb.p, rb.n, sim);
        if (ok) {
            size_t prefix = 0;
            size_t limit = min3(ra.n, rb.n, 4);
            while (prefix < limit && ra.p[prefix] == rb.p[prefix]) {
                prefix++;
            }
            *sim += 0.1 * (double)prefix * (1.0 - *sim);
        }
    }
    free(ra.p);
    free(rb.p);
    return ok ? FUZZY_OK : FUZZY_NOMEM;
}

#pragma endregion

#pragma region Levenshtein

// leven computes the Levenshtein distance between the non-empty
// code point arrays a and b. Returns false if out of memory.
static bool leven(const uint32_t* a, size_t na, const uint32_t* b, size_t nb, size_t* dist) {
    // na + 1 doesn't overflow: na is a count of array elements
    size_t* column = alloc_array(na + 1, sizeof(size_t));
    if (column == NULL) {
        return false;
    }
    for (size_t y = 0; y <= na; y++) {
        column[y] = y;
    }
    for (size_t x = 1; x <= nb; x++) {
        column[0] = x;
        size_t lastkey = x - 1;
        for (size_t y = 1; y <= na; y++) {
            size_t oldkey = column[y];
            size_t cost = a[y - 1] != b[x - 1] ? 1 : 0;
            column[y] = min3(column[y] + 1,      // insertion
                             column[y - 1] + 1,  // deletion
                             lastkey + cost);    // substitution
            lastkey = oldkey;
        }
    }
    *dist = column[na];
    free(column);
    return true;
}

// fuzzy_leven sets dist to the Levenshtein distance between a and b:
// the minimum number of insertions, deletions and substitutions
// that turn a into b. Returns FUZZY_NOMEM if out of memory.
FuzzyStatus fuzzy_leven(const char* a, size_t na, const char* b, size_t nb, size_t* dist) {
    return edit_distance(a, na, b, nb, leven, dist);
}

#pragma endregion

#pragma region Optimal string alignment

// osadist computes the optimal string alignment distance between the
// non-empty code point arrays a and b. Returns false if out of memory.
static bool osadist(const uint32_t* a, size_t na, const uint32_t* b, size_t nb, size_t* dist) {
    if (na < nb) {
        // the distance is symmetric, and a shorter b means shorter rows
        return osadist(b, nb, a, na, dist);
    }
    // three rolling rows of the matrix[0..na][0..nb]
    size_t cols = nb + 1;
    size_t* matrix = alloc_matrix(3, cols);
    if (matrix == NULL) {
        return false;
    }
#define M(i, j) matrix[((i) % 3) * cols + (j)]
    for (size_t j = 0; j <= nb; j++) {
        M(0, j) = j;
    }
    for (size_t i = 1; i <= na; i++) {
        M(i, 0) = i;
        for (size_t j = 1; j <= nb; j++) {
            size_t cost = a[i - 1] == b[j - 1] ? 0 : 1;
            M(i, j) = min3(M(i - 1, j) + 1,          // deletion
                           M(i, j - 1) + 1,          // insertion
                           M(i - 1, j - 1) + cost);  // substitution
            if (i > 1 && j > 1 && a[i - 1] == b[j - 2] && a[i - 2] == b[j - 1]) {
                M(i, j) = min2(M(i, j), M(i - 2, j - 2) + 1);  // transposition
            }
        }
    }
    *dist = M(na, nb);
#undef M
    free(matrix);
    return true;
}

// fuzzy_osadist sets dist to the optimal string alignment distance
// between a and b: the minimum number of insertions, deletions, substitutions
// and transpositions of adjacent characters that turn a into b, where
// no substring is edited more than once. Returns FUZZY_NOMEM if out of memory.
FuzzyStatus fuzzy_osadist(const char* a, size_t na, const char* b, size_t nb, size_t* dist) {
    return edit_distance(a, na, b, nb, osadist, dist);
}

#pragma endregion
