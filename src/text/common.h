// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Helpers shared by the domain files.

#ifndef TEXT_COMMON_H
#define TEXT_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#pragma region Sizes

// size_add returns a+b, or SIZE_MAX if the sum doesn't fit.
static inline size_t size_add(size_t a, size_t b) {
    return a > SIZE_MAX - b ? SIZE_MAX : a + b;
}

// size_mul returns a*b, or SIZE_MAX if the product doesn't fit.
static inline size_t size_mul(size_t a, uint64_t b) {
    if (a == 0 || b == 0) {
        return 0;
    }
    if (b > SIZE_MAX / a) {
        return SIZE_MAX;
    }
    return a * (size_t)b;
}

// put copies the len bytes of src to out at offset pos, unless out is NULL,
// and returns the offset after them, saturating at SIZE_MAX. Calling it with
// a NULL out measures the result without writing it.
static inline size_t put(char* out, size_t pos, const char* src, size_t len) {
    // memcpy with a NULL pointer is UB even when len is 0.
    if (out != NULL && len > 0) {
        memcpy(out + pos, src, len);
    }
    return size_add(pos, len);
}

// size_to_int64 converts n to int64_t, saturating at INT64_MAX, which
// the size of a string in memory never reaches.
static inline int64_t size_to_int64(size_t n) {
    return (uint64_t)n > (uint64_t)INT64_MAX ? INT64_MAX : (int64_t)n;
}

#pragma endregion

#pragma region UTF-8

// UTF8_INVALID_MIN is the smallest value utf8_next returns for an ill-formed
// sequence. It is above every code point, so the two never mix.
#define UTF8_INVALID_MIN 0x800000u

// utf8_next decodes the character that starts at byte *i of s, which has n
// bytes, and advances *i past it. *i must be less than n.
//
// A valid sequence gives its code point. An ill-formed one gives its bytes
// packed big-endian into the low 24 bits, a value of at least
// UTF8_INVALID_MIN that compares in byte order and that no code point equals.
// Each maximal ill-formed subpart (Unicode 16.0, section 3.9, D93b) is one
// character: the bytes that form a valid prefix of a sequence stay together,
// and the byte that broke the sequence is left for the next call, since it
// may start a valid one. So the subpart has 1 to 3 bytes.
static inline uint32_t utf8_next(const char* s, size_t n, size_t* i) {
    const unsigned char* p = (const unsigned char*)s + *i;
    const size_t avail = n - *i;
    const unsigned char b0 = p[0];
    if (b0 < 0x80) {
        *i += 1;
        return b0;
    }

    // The well-formed byte sequences, from Table 3-7 of the Unicode standard.
    // lo and hi bound the second byte; the following ones are 0x80..0xBF.
    size_t need;
    uint32_t c;
    unsigned char lo = 0x80, hi = 0xBF;
    if (b0 >= 0xC2 && b0 <= 0xDF) {
        need = 1;
        c = b0 & 0x1F;
    } else if (b0 >= 0xE0 && b0 <= 0xEF) {
        need = 2;
        c = b0 & 0x0F;
        if (b0 == 0xE0) {
            lo = 0xA0;  // overlong
        } else if (b0 == 0xED) {
            hi = 0x9F;  // surrogates
        }
    } else if (b0 >= 0xF0 && b0 <= 0xF4) {
        need = 3;
        c = b0 & 0x07;
        if (b0 == 0xF0) {
            lo = 0x90;  // overlong
        } else if (b0 == 0xF4) {
            hi = 0x8F;  // above U+10FFFF
        }
    } else {
        // A continuation byte, or a byte that never appears in UTF-8.
        *i += 1;
        return (uint32_t)b0 << 16;
    }

    size_t k = 1;
    while (k <= need && k < avail && p[k] >= lo && p[k] <= hi) {
        c = (c << 6) | (p[k] & 0x3F);
        lo = 0x80;
        hi = 0xBF;
        k++;
    }
    *i += k;
    if (k == need + 1) {
        return c;
    }
    // k <= need <= 3 here, so the bytes fit into 24 bits.
    uint32_t packed = 0;
    for (size_t j = 0; j < 3; j++) {
        packed = (packed << 8) | (j < k ? p[j] : 0);
    }
    return packed;
}

// utf8_valid reports whether c, returned by utf8_next, is a code point.
static inline bool utf8_valid(uint32_t c) {
    return c < UTF8_INVALID_MIN;
}

// utf8_encode writes the UTF-8 encoding of the code point c to out, which
// has room for 4 bytes, and returns the number of bytes written.
static inline size_t utf8_encode(char* out, uint32_t c) {
    if (c < 0x80) {
        out[0] = (char)c;
        return 1;
    }
    if (c < 0x800) {
        out[0] = (char)(0xC0 | (c >> 6));
        out[1] = (char)(0x80 | (c & 0x3F));
        return 2;
    }
    if (c < 0x10000) {
        out[0] = (char)(0xE0 | (c >> 12));
        out[1] = (char)(0x80 | ((c >> 6) & 0x3F));
        out[2] = (char)(0x80 | (c & 0x3F));
        return 3;
    }
    out[0] = (char)(0xF0 | (c >> 18));
    out[1] = (char)(0x80 | ((c >> 12) & 0x3F));
    out[2] = (char)(0x80 | ((c >> 6) & 0x3F));
    out[3] = (char)(0x80 | (c & 0x3F));
    return 4;
}

// utf8_count returns the number of characters in the n bytes of s.
static inline size_t utf8_count(const char* s, size_t n) {
    size_t count = 0;
    for (size_t i = 0; i < n; count++) {
        utf8_next(s, n, &i);
    }
    return count;
}

// utf8_skip returns the byte offset count characters after the offset i
// in s, which has n bytes, or n if s ends first.
static inline size_t utf8_skip(const char* s, size_t n, size_t i, uint64_t count) {
    for (; count > 0 && i < n; count--) {
        utf8_next(s, n, &i);
    }
    return i;
}

#pragma endregion

#pragma region Search

// find returns the byte offset of the first occurrence of sub (nsub bytes)
// in s (n bytes) at or after the offset from, or SIZE_MAX if there is none.
// An empty sub is found at from.
static inline size_t find(const char* s, size_t n, const char* sub, size_t nsub, size_t from) {
    if (nsub == 0) {
        return from;
    }
    while (from < n && n - from >= nsub) {
        const char* p = memchr(s + from, sub[0], n - from - nsub + 1);
        if (p == NULL) {
            return SIZE_MAX;
        }
        size_t at = (size_t)(p - s);
        if (memcmp(p, sub, nsub) == 0) {
            return at;
        }
        from = at + 1;
    }
    return SIZE_MAX;
}

#pragma endregion

#endif /* TEXT_COMMON_H */
