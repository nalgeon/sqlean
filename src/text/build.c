// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Splitting, joining and modifying strings.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "text/common.h"
#include "text/text.h"

#pragma region Split and join

// count_parts returns the number of parts s splits into by the non-empty sep.
static uint64_t count_parts(const char* s, size_t n, const char* sep, size_t nsep) {
    uint64_t count = 1;
    for (size_t at = find(s, n, sep, nsep, 0); at != SIZE_MAX;
         at = find(s, n, sep, nsep, at + nsep)) {
        count++;
    }
    return count;
}

// text_split splits s by sep and sets out to the part with the number part,
// counting from 1. A negative part counts from the end, so -1 is the last
// part. A part past either end is empty, and so is an out-of-range one.
// An empty sep doesn't split, so s is the only part. Returns false if part
// is 0, which is neither from the beginning nor from the end.
bool text_split(const char* s,
                size_t n,
                const char* sep,
                size_t nsep,
                int64_t part,
                TextSpan* out) {
    if (part == 0) {
        return false;
    }
    *out = (TextSpan){0, 0};
    if (nsep == 0) {
        if (part == 1 || part == -1) {
            *out = (TextSpan){0, n};
        }
        return true;
    }

    uint64_t index;  // 0-based
    if (part > 0) {
        index = (uint64_t)part - 1;
    } else {
        uint64_t nparts = count_parts(s, n, sep, nsep);
        // Compare in unsigned: -part overflows for INT64_MIN, while
        // 0 - (uint64_t)part is its absolute value.
        uint64_t back = 0 - (uint64_t)part;
        if (back > nparts) {
            return true;
        }
        index = nparts - back;
    }

    size_t start = 0;
    for (uint64_t i = 0;; i++) {
        size_t at = find(s, n, sep, nsep, start);
        if (i == index) {
            size_t end = at == SIZE_MAX ? n : at;
            *out = (TextSpan){start, end - start};
            return true;
        }
        if (at == SIZE_MAX) {
            return true;
        }
        start = at + nsep;
    }
}

// text_join writes the count strings parts (with sizes in sizes) to out,
// separated by sep, and returns the size of the result.
size_t text_join(const char* sep,
                 size_t nsep,
                 const char* const* parts,
                 const size_t* sizes,
                 size_t count,
                 char* out) {
    size_t pos = 0;
    for (size_t i = 0; i < count; i++) {
        if (i > 0) {
            pos = put(out, pos, sep, nsep);
        }
        pos = put(out, pos, parts[i], sizes[i]);
    }
    return pos;
}

// text_repeat writes s repeated count times to out, and returns the size
// of the result. A negative count is 0, which gives an empty string.
size_t text_repeat(const char* s, size_t n, int64_t count, char* out) {
    // An empty s gives an empty result, without looping count times.
    if (count <= 0 || n == 0) {
        return 0;
    }
    if (out == NULL) {
        return size_mul(n, (uint64_t)count);
    }
    size_t pos = 0;
    for (int64_t i = 0; i < count; i++) {
        pos = put(out, pos, s, n);
    }
    return pos;
}

#pragma endregion

#pragma region Modify

// text_replace writes s with the first count occurrences of old replaced by
// new to out, and returns the size of the result. Occurrences don't overlap,
// and a count less than 1 replaces nothing. An empty old matches nothing,
// as in SQLite's replace.
size_t text_replace(const char* s,
                    size_t n,
                    const char* old,
                    size_t nold,
                    const char* new,
                    size_t nnew,
                    int64_t count,
                    char* out) {
    if (nold == 0 || count <= 0) {
        return put(out, 0, s, n);
    }
    size_t pos = 0;
    size_t from = 0;
    for (int64_t i = 0; i < count; i++) {
        size_t at = find(s, n, old, nold, from);
        if (at == SIZE_MAX) {
            break;
        }
        pos = put(out, pos, s + from, at - from);
        pos = put(out, pos, new, nnew);
        from = at + nold;
    }
    return put(out, pos, s + from, n - from);
}

// char_at returns the k-th character of s (0-based) as a span,
// or an empty span at n if s has fewer characters.
static TextSpan char_at(const char* s, size_t n, size_t k) {
    size_t start = utf8_skip(s, n, 0, k);
    size_t end = start;
    if (end < n) {
        utf8_next(s, n, &end);
    }
    return (TextSpan){start, end - start};
}

// index_in returns the 0-based index of the first character of set equal
// to the character c (nc bytes), or SIZE_MAX if there is none.
static size_t index_in(const char* set, size_t nset, const char* c, size_t nc) {
    size_t k = 0;
    for (size_t i = 0; i < nset; k++) {
        size_t start = i;
        utf8_next(set, nset, &i);
        if (i - start == nc && memcmp(set + start, c, nc) == 0) {
            return k;
        }
    }
    return SIZE_MAX;
}

// text_translate writes s with each character in from replaced by the
// character at the same position in to, to out, and returns the size of
// the result. A character in from that has no counterpart (to is shorter)
// is deleted, and extra characters in to are ignored. If a character
// appears in from more than once, its first position counts. Characters
// match by their bytes, ill-formed ones included.
size_t text_translate(const char* s,
                      size_t n,
                      const char* from,
                      size_t nfrom,
                      const char* to,
                      size_t nto,
                      char* out) {
    size_t pos = 0;
    for (size_t i = 0; i < n;) {
        size_t start = i;
        utf8_next(s, n, &i);
        size_t k = index_in(from, nfrom, s + start, i - start);
        if (k == SIZE_MAX) {
            pos = put(out, pos, s + start, i - start);
            continue;
        }
        TextSpan c = char_at(to, nto, k);
        pos = put(out, pos, to + c.start, c.len);
    }
    return pos;
}

// text_reverse writes the characters of s in reverse order to out,
// which has room for n bytes. An ill-formed sequence moves as a whole,
// with its bytes in their original order.
void text_reverse(const char* s, size_t n, char* out) {
    for (size_t i = 0; i < n;) {
        size_t start = i;
        utf8_next(s, n, &i);
        memcpy(out + n - i, s + start, i - start);
    }
}

#pragma endregion
