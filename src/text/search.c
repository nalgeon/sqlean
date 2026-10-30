// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Searching and matching.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "text/common.h"
#include "text/text.h"

#pragma region Search

// position converts the byte offset off in s to a 1-based character position.
// An offset inside a character gives the position of that character. Only an
// ill-formed sub can match there, since a valid one starts with a lead byte.
static int64_t position(const char* s, size_t n, size_t off) {
    // Positions don't overflow: a string has fewer than INT64_MAX characters.
    int64_t pos = 1;
    for (size_t i = 0; i < off;) {
        utf8_next(s, n, &i);
        if (i <= off) {
            pos++;
        }
    }
    return pos;
}

// text_index returns the position of the first occurrence of sub in s,
// or 0 if there is none. The search compares bytes, so a sub that is
// ill-formed UTF-8 only matches the same bytes, and may match inside
// a character, which gives the position of that character. An empty sub
// is found at 1.
int64_t text_index(const char* s, size_t n, const char* sub, size_t nsub) {
    size_t at = find(s, n, sub, nsub, 0);
    return at == SIZE_MAX ? 0 : position(s, n, at);
}

// text_last_index returns the position of the last occurrence of sub in s,
// or 0 if there is none. An empty sub is found after the last character,
// at the length of s plus one.
int64_t text_last_index(const char* s, size_t n, const char* sub, size_t nsub) {
    if (nsub > n) {
        return 0;
    }
    for (size_t at = n - nsub;; at--) {
        // An empty sub is found at the end. memcmp isn't called for it,
        // since a NULL pointer is UB even with 0 bytes.
        if (nsub == 0 || memcmp(s + at, sub, nsub) == 0) {
            return position(s, n, at);
        }
        if (at == 0) {
            return 0;
        }
    }
}

// text_contains reports whether s contains sub. Every s contains
// the empty string.
bool text_contains(const char* s, size_t n, const char* sub, size_t nsub) {
    return find(s, n, sub, nsub, 0) != SIZE_MAX;
}

// text_has_prefix reports whether s starts with prefix. Every s starts
// with the empty string.
bool text_has_prefix(const char* s, size_t n, const char* prefix, size_t nprefix) {
    return nprefix <= n && (nprefix == 0 || memcmp(s, prefix, nprefix) == 0);
}

// text_has_suffix reports whether s ends with suffix. Every s ends
// with the empty string.
bool text_has_suffix(const char* s, size_t n, const char* suffix, size_t nsuffix) {
    return nsuffix <= n && (nsuffix == 0 || memcmp(s + n - nsuffix, suffix, nsuffix) == 0);
}

// text_count returns the number of non-overlapping occurrences of sub in s.
// An empty sub occurs before each character and at the end, so it counts
// as the length of s plus one.
int64_t text_count(const char* s, size_t n, const char* sub, size_t nsub) {
    if (nsub == 0) {
        return size_to_int64(utf8_count(s, n)) + 1;
    }
    int64_t count = 0;
    for (size_t at = find(s, n, sub, nsub, 0); at != SIZE_MAX;
         at = find(s, n, sub, nsub, at + nsub)) {
        count++;
    }
    return count;
}

// text_like reports whether s matches the pattern p. In p, % matches any
// number of characters, _ matches exactly one, and any other character
// matches itself regardless of case (by simple case folding, for all of
// Unicode). There is no escape character, so p can't match a literal % or _.
bool text_like(const char* p, size_t np, const char* s, size_t n) {
    // Greedy matching with backtracking to the last %: the characters
    // after it are matched at each following position in turn. Each %
    // only needs to retry the latest one, so this is O(np * n).
    size_t pi = 0, si = 0;
    size_t star_pi = SIZE_MAX, star_si = 0;
    while (si < n) {
        if (pi < np) {
            size_t pnext = pi;
            uint32_t pc = utf8_next(p, np, &pnext);
            if (pc == '%') {
                star_pi = pi = pnext;
                star_si = si;
                continue;
            }
            size_t snext = si;
            uint32_t sc = utf8_next(s, n, &snext);
            if (pc == '_' || rune_casefold(pc) == rune_casefold(sc)) {
                pi = pnext;
                si = snext;
                continue;
            }
        }
        if (star_pi == SIZE_MAX) {
            return false;
        }
        // Let the last % match one more character, and retry after it.
        utf8_next(s, n, &star_si);
        pi = star_pi;
        si = star_si;
    }
    while (pi < np && p[pi] == '%') {
        pi++;
    }
    return pi == np;
}

#pragma endregion
