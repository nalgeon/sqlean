// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Substrings, trimming and padding.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "text/common.h"
#include "text/text.h"

#pragma region Substrings

// span returns the part of s between the character counts first and last
// (0-based, last not included), which must not exceed the length of s.
static TextSpan span(const char* s, size_t n, uint64_t first, uint64_t last) {
    if (first >= last) {
        return (TextSpan){0, 0};
    }
    size_t start = utf8_skip(s, n, 0, first);
    size_t end = utf8_skip(s, n, start, last - first);
    return (TextSpan){start, end - start};
}

// text_substring returns the part of s that has count characters and starts
// at the position start. A start before position 1 counts the characters
// before the string, so it shortens the part: text_substring('abc', 0, 2)
// is 'a'. A count less than 1 gives an empty part, and the part ends where
// s does.
TextSpan text_substring(const char* s, size_t n, int64_t start, int64_t count) {
    if (count <= 0) {
        return (TextSpan){0, 0};
    }
    if (start < 1) {
        // count is positive and start is not, so start + count can't overflow.
        if (start + count <= 1) {
            return (TextSpan){0, 0};
        }
        count = start + count - 1;
        start = 1;
    }
    size_t from = utf8_skip(s, n, 0, (uint64_t)start - 1);
    size_t to = utf8_skip(s, n, from, (uint64_t)count);
    return (TextSpan){from, to - from};
}

// slice_index converts the position i, as text_slice takes it, into
// a 0-based character count in a string of len characters.
static uint64_t slice_index(int64_t i, int64_t len) {
    if (i > 0) {
        return (uint64_t)(i - 1 < len ? i - 1 : len);
    }
    if (i == 0) {
        return 0;
    }
    // len is not negative, so -len can't overflow, while -i could.
    if (i < -len) {
        return 0;
    }
    return (uint64_t)(len + i);
}

// text_slice returns the part of s from the position start up to,
// but not including, the position end. A negative position counts from
// the end, so -1 is the last character. A position past either end is
// clamped to it, and 0 is the same as 1, the beginning of the string.
// An end before the start gives an empty part.
TextSpan text_slice(const char* s, size_t n, int64_t start, int64_t end) {
    int64_t len = size_to_int64(utf8_count(s, n));
    return span(s, n, slice_index(start, len), slice_index(end, len));
}

// text_left returns the first count characters of s, or all of them if
// there are fewer. A negative count drops that many characters from the end
// instead, so text_left('abc', -1) is 'ab'.
TextSpan text_left(const char* s, size_t n, int64_t count) {
    if (count >= 0) {
        return span(s, n, 0, (uint64_t)count);
    }
    int64_t len = size_to_int64(utf8_count(s, n));
    // len is not negative, so -len can't overflow, while -count could.
    if (count < -len) {
        return (TextSpan){0, 0};
    }
    return span(s, n, 0, (uint64_t)(len + count));
}

// text_right returns the last count characters of s, or all of them if
// there are fewer. A negative count drops that many characters from the
// beginning instead, so text_right('abc', -1) is 'bc'.
TextSpan text_right(const char* s, size_t n, int64_t count) {
    int64_t len = size_to_int64(utf8_count(s, n));
    int64_t keep;
    if (count >= 0) {
        keep = count < len ? count : len;
    } else {
        // len is not negative, so -len can't overflow, while -count could.
        keep = count < -len ? 0 : len + count;
    }
    return span(s, n, (uint64_t)(len - keep), (uint64_t)len);
}

#pragma endregion

#pragma region Trim and pad

// in_set reports whether the character c (nc bytes) is one of the
// characters of set.
static bool in_set(const char* set, size_t nset, const char* c, size_t nc) {
    for (size_t i = 0; i < nset;) {
        size_t start = i;
        utf8_next(set, nset, &i);
        if (i - start == nc && memcmp(set + start, c, nc) == 0) {
            return true;
        }
    }
    return false;
}

// text_ltrim returns s without the leading characters that are in chars.
// Characters match by their bytes, ill-formed ones included.
TextSpan text_ltrim(const char* s, size_t n, const char* chars, size_t nchars) {
    size_t i = 0;
    while (i < n) {
        size_t next = i;
        utf8_next(s, n, &next);
        if (!in_set(chars, nchars, s + i, next - i)) {
            break;
        }
        i = next;
    }
    return (TextSpan){i, n - i};
}

// text_rtrim returns s without the trailing characters that are in chars.
// Characters match by their bytes, ill-formed ones included.
TextSpan text_rtrim(const char* s, size_t n, const char* chars, size_t nchars) {
    // An ill-formed sequence can't be decoded backwards reliably,
    // so walk forward and remember where the last kept character ends.
    size_t end = 0;
    for (size_t i = 0; i < n;) {
        size_t start = i;
        utf8_next(s, n, &i);
        if (!in_set(chars, nchars, s + start, i - start)) {
            end = i;
        }
    }
    return (TextSpan){0, end};
}

// text_trim returns s without the leading and trailing characters
// that are in chars. Characters match by their bytes, ill-formed ones
// included.
TextSpan text_trim(const char* s, size_t n, const char* chars, size_t nchars) {
    TextSpan left = text_ltrim(s, n, chars, nchars);
    TextSpan both = text_rtrim(s + left.start, left.len, chars, nchars);
    return (TextSpan){left.start, both.len};
}

// pad writes s padded with fill to length characters to out, on the left
// or on the right, and returns the size of the result.
static size_t
pad(const char* s, size_t n, int64_t length, const char* fill, size_t nfill, bool left, char* out) {
    uint64_t want = length > 0 ? (uint64_t)length : 0;
    size_t len = utf8_count(s, n);
    if (want <= len) {
        return put(out, 0, s, utf8_skip(s, n, 0, want));
    }
    if (nfill == 0) {
        return put(out, 0, s, n);
    }

    // The padding is the fill repeated full times, then its first rest characters.
    uint64_t npad = want - len;
    size_t fill_len = utf8_count(fill, nfill);
    uint64_t full = npad / fill_len;
    size_t rest = utf8_skip(fill, nfill, 0, npad % fill_len);
    if (out == NULL) {
        return size_add(size_add(size_mul(nfill, full), rest), n);
    }

    size_t pos = 0;
    if (!left) {
        pos = put(out, pos, s, n);
    }
    for (uint64_t i = 0; i < full; i++) {
        pos = put(out, pos, fill, nfill);
    }
    pos = put(out, pos, fill, rest);
    if (left) {
        pos = put(out, pos, s, n);
    }
    return pos;
}

// text_lpad writes s, padded on the left with fill to length characters,
// to out, and returns the size of the result. A string longer than length
// is truncated on the right, and a negative length is 0. An empty fill
// doesn't pad, so it gives s as is.
size_t text_lpad(const char* s,
                 size_t n,
                 int64_t length,
                 const char* fill,
                 size_t nfill,
                 char* out) {
    return pad(s, n, length, fill, nfill, true, out);
}

// text_rpad writes s, padded on the right with fill to length characters,
// to out, and returns the size of the result. A string longer than length
// is truncated on the right, and a negative length is 0. An empty fill
// doesn't pad, so it gives s as is.
size_t text_rpad(const char* s,
                 size_t n,
                 int64_t length,
                 const char* fill,
                 size_t nfill,
                 char* out) {
    return pad(s, n, length, fill, nfill, false, out);
}

#pragma endregion

#pragma region Properties

// text_length returns the number of characters in s.
int64_t text_length(const char* s, size_t n) {
    return size_to_int64(utf8_count(s, n));
}

#pragma endregion
