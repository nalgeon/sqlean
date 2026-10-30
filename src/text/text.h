// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Unicode string functions.

#ifndef TEXT_TEXT_H
#define TEXT_TEXT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// A string is n bytes of UTF-8 at s, which is never NULL. It may contain
// zero bytes, which are characters like any other. Positions and lengths
// count characters, starting at 1.
//
// A string may also contain ill-formed UTF-8. Each maximal ill-formed subpart
// (1 to 3 bytes that can't be decoded) counts as one character, and functions
// keep its bytes as they are: they are searched, compared and copied, but
// never changed or replaced with U+FFFD.
//
// Functions that build a new string write it into a caller's buffer and
// return its size in bytes. With a NULL buffer, they only measure the result,
// saturating at SIZE_MAX, so the caller can check the size and allocate
// before calling again. The result is not zero-terminated.

// TextSpan is a part of a string: len bytes starting at byte offset start.
typedef struct {
    size_t start;
    size_t len;
} TextSpan;

#pragma region Runes

// The case mappings cover the Basic Multilingual Plane (U+0000 to U+FFFF).
// Code points above it, such as the Deseret letters, have no case.

// rune_casefold returns the simple case folding of the code point c,
// or c if it has none.
uint32_t rune_casefold(uint32_t c);

// rune_tolower returns the lowercase of the code point c, or c if it has none.
uint32_t rune_tolower(uint32_t c);

// rune_toupper returns the uppercase of the code point c, or c if it has none.
uint32_t rune_toupper(uint32_t c);

// rune_totitle returns the titlecase of the code point c, or c if it has none.
uint32_t rune_totitle(uint32_t c);

// rune_isword reports whether the code point c is a letter, a digit
// or a connector punctuation (such as an underscore).
bool rune_isword(uint32_t c);

#pragma endregion

#pragma region Substrings

// text_substring returns the part of s that has count characters and starts
// at the position start.
TextSpan text_substring(const char* s, size_t n, int64_t start, int64_t count);

// text_slice returns the part of s from the position start up to,
// but not including, the position end.
TextSpan text_slice(const char* s, size_t n, int64_t start, int64_t end);

// text_left returns the first count characters of s.
TextSpan text_left(const char* s, size_t n, int64_t count);

// text_right returns the last count characters of s.
TextSpan text_right(const char* s, size_t n, int64_t count);

#pragma endregion

#pragma region Search

// text_index returns the position of the first occurrence of sub in s,
// or 0 if there is none.
int64_t text_index(const char* s, size_t n, const char* sub, size_t nsub);

// text_last_index returns the position of the last occurrence of sub in s,
// or 0 if there is none.
int64_t text_last_index(const char* s, size_t n, const char* sub, size_t nsub);

// text_contains reports whether s contains sub.
bool text_contains(const char* s, size_t n, const char* sub, size_t nsub);

// text_has_prefix reports whether s starts with prefix.
bool text_has_prefix(const char* s, size_t n, const char* prefix, size_t nprefix);

// text_has_suffix reports whether s ends with suffix.
bool text_has_suffix(const char* s, size_t n, const char* suffix, size_t nsuffix);

// text_count returns the number of non-overlapping occurrences of sub in s.
int64_t text_count(const char* s, size_t n, const char* sub, size_t nsub);

// text_like reports whether s matches the pattern p, case-insensitively.
// In p, % matches any number of characters and _ matches one.
bool text_like(const char* p, size_t np, const char* s, size_t n);

#pragma endregion

#pragma region Split and join

// text_split splits s by sep and sets out to the part with the number part.
// Returns false if part is 0.
bool text_split(const char* s, size_t n, const char* sep, size_t nsep, int64_t part, TextSpan* out);

// text_join writes the count strings parts (with sizes in sizes) to out,
// separated by sep, and returns the size of the result.
size_t text_join(const char* sep,
                 size_t nsep,
                 const char* const* parts,
                 const size_t* sizes,
                 size_t count,
                 char* out);

// text_repeat writes s repeated count times to out, and returns the size
// of the result.
size_t text_repeat(const char* s, size_t n, int64_t count, char* out);

#pragma endregion

#pragma region Trim and pad

// text_ltrim returns s without the leading characters that are in chars.
TextSpan text_ltrim(const char* s, size_t n, const char* chars, size_t nchars);

// text_rtrim returns s without the trailing characters that are in chars.
TextSpan text_rtrim(const char* s, size_t n, const char* chars, size_t nchars);

// text_trim returns s without the leading and trailing characters
// that are in chars.
TextSpan text_trim(const char* s, size_t n, const char* chars, size_t nchars);

// text_lpad writes s, padded on the left with fill to length characters,
// to out, and returns the size of the result.
size_t text_lpad(const char* s,
                 size_t n,
                 int64_t length,
                 const char* fill,
                 size_t nfill,
                 char* out);

// text_rpad writes s, padded on the right with fill to length characters,
// to out, and returns the size of the result.
size_t text_rpad(const char* s,
                 size_t n,
                 int64_t length,
                 const char* fill,
                 size_t nfill,
                 char* out);

#pragma endregion

#pragma region Case

// text_upper writes s in uppercase to out, and returns the size of the result.
size_t text_upper(const char* s, size_t n, char* out);

// text_lower writes s in lowercase to out, and returns the size of the result.
size_t text_lower(const char* s, size_t n, char* out);

// text_title writes s in title case to out, and returns the size of the result.
size_t text_title(const char* s, size_t n, char* out);

// text_casefold writes s case-folded to out, and returns the size of the result.
size_t text_casefold(const char* s, size_t n, char* out);

// text_casecmp compares s1 and s2 case-insensitively, and returns a negative
// number, zero or a positive number if s1 is less than, equal to or greater
// than s2.
int text_casecmp(const char* s1, size_t n1, const char* s2, size_t n2);

#pragma endregion

#pragma region Modify

// text_replace writes s with the first count occurrences of old replaced by
// new to out, and returns the size of the result.
size_t text_replace(const char* s,
                    size_t n,
                    const char* old,
                    size_t nold,
                    const char* new,
                    size_t nnew,
                    int64_t count,
                    char* out);

// text_translate writes s with each character in from replaced by the
// character at the same position in to, to out, and returns the size of
// the result.
size_t text_translate(const char* s,
                      size_t n,
                      const char* from,
                      size_t nfrom,
                      const char* to,
                      size_t nto,
                      char* out);

// text_reverse writes the characters of s in reverse order to out,
// which has room for n bytes.
void text_reverse(const char* s, size_t n, char* out);

#pragma endregion

#pragma region Properties

// text_length returns the number of characters in s.
int64_t text_length(const char* s, size_t n);

#pragma endregion

#endif /* TEXT_TEXT_H */
