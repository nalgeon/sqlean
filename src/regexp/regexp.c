// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Regular expressions based on the PCRE2 engine.

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "regexp/pcre2/pcre2.h"
#include "regexp/regexp.h"

struct Regexp {
    pcre2_code* code;
    pcre2_match_data* match;  // the ovector and backtracking frames, reused by every match
    uint32_t ngroups;         // the number of capturing groups
};

static const uint32_t COMPILE_OPTIONS = PCRE2_UTF | PCRE2_UCP;

// Replace every match, support the extended replacement syntax, insert an
// empty string for an unset group, and report the length a too-small buffer
// would need instead of only failing.
static const uint32_t SUBSTITUTE_OPTIONS = PCRE2_SUBSTITUTE_GLOBAL | PCRE2_SUBSTITUTE_EXTENDED |
                                           PCRE2_SUBSTITUTE_UNSET_EMPTY |
                                           PCRE2_SUBSTITUTE_OVERFLOW_LENGTH;

#pragma region Helpers

// write_error writes the message for a PCRE2 error code to err,
// followed by the offset unless it is PCRE2_UNSET.
static void write_error(char* err, int code, size_t offset) {
    // The longest message is 91 characters. The size leaves room for the
    // offset in err, so the compiler can tell the snprintf doesn't truncate.
    PCRE2_UCHAR msg[96];
    if (pcre2_get_error_message(code, msg, sizeof(msg)) == PCRE2_ERROR_BADDATA) {
        snprintf(err, REGEXP_ERR_BUF_SIZE, "error %d", code);
        return;
    }
    if (offset == PCRE2_UNSET) {
        snprintf(err, REGEXP_ERR_BUF_SIZE, "%s", (const char*)msg);
    } else {
        snprintf(err, REGEXP_ERR_BUF_SIZE, "%s at offset %llu", (const char*)msg,
                 (unsigned long long)offset);
    }
}

// is_utf_error reports whether a PCRE2 error code is an invalid UTF-8 error.
static bool is_utf_error(int rc) {
    return rc <= PCRE2_ERROR_UTF8_ERR1 && rc >= PCRE2_ERROR_UTF8_ERR21;
}

// match_status returns the status for an error code from pcre2_match,
// writing the message to err for REGEXP_FAILED.
static RegexpStatus match_status(int rc, char* err) {
    if (is_utf_error(rc)) {
        return REGEXP_INVALID;
    }
    if (rc == PCRE2_ERROR_NOMEMORY) {
        return REGEXP_NOMEM;
    }
    // A match, depth or heap limit, a recursion loop or an internal error.
    write_error(err, rc, PCRE2_UNSET);
    return REGEXP_FAILED;
}

#pragma endregion

#pragma region Compiling

// regexp_compile compiles the pattern into out. Free it with regexp_free.
// Returns REGEXP_BAD_PATTERN if the pattern is invalid, and REGEXP_NOMEM
// if out of memory.
RegexpStatus regexp_compile(const char* pattern, size_t n, Regexp** out, char* err) {
    // n can't be PCRE2_ZERO_TERMINATED (SIZE_MAX), since no string is that long.
    int errcode;
    PCRE2_SIZE erroffset;
    pcre2_code* code =
        pcre2_compile((PCRE2_SPTR)pattern, n, COMPILE_OPTIONS, &errcode, &erroffset, NULL);
    if (code == NULL) {
        if (errcode == PCRE2_ERROR_HEAP_FAILED) {
            return REGEXP_NOMEM;
        }
        write_error(err, errcode, erroffset);
        return REGEXP_BAD_PATTERN;
    }

    Regexp* re = malloc(sizeof(*re));
    if (re == NULL) {
        pcre2_code_free(code);
        return REGEXP_NOMEM;
    }
    re->code = code;
    re->match = pcre2_match_data_create_from_pattern(code, NULL);
    if (re->match == NULL) {
        regexp_free(re);
        return REGEXP_NOMEM;
    }
    pcre2_pattern_info(code, PCRE2_INFO_CAPTURECOUNT, &re->ngroups);
    *out = re;
    return REGEXP_OK;
}

// regexp_free frees the compiled pattern. Does nothing for NULL.
void regexp_free(Regexp* re) {
    if (re == NULL) {
        return;
    }
    pcre2_match_data_free(re->match);
    pcre2_code_free(re->code);
    free(re);
}

#pragma endregion

#pragma region Matching

// regexp_like sets matched to whether the pattern matches somewhere in s.
// Returns REGEXP_INVALID if s is not valid UTF-8, REGEXP_FAILED if the match
// fails, and REGEXP_NOMEM if out of memory.
RegexpStatus regexp_like(Regexp* re, const char* s, size_t n, bool* matched, char* err) {
    int rc = pcre2_match(re->code, (PCRE2_SPTR)s, n, 0, 0, re->match, NULL);
    if (rc == PCRE2_ERROR_NOMATCH) {
        *matched = false;
        return REGEXP_OK;
    }
    if (rc < 0) {
        return match_status(rc, err);
    }
    // rc = 0 would mean a match with the ovector too small for the groups,
    // which can't happen with match data created from the pattern.
    *matched = true;
    return REGEXP_OK;
}

// regexp_capture finds the first match of the pattern in s, and sets
// [start, start+len) to the bytes of s the capturing group matched.
// Group 0 is the whole match. Sets found to false if there is no match,
// or if the group did not participate in it, as (b) in 'a(b)?c' with 'ac'.
// A group that matched an empty string is found with len = 0.
// Returns REGEXP_BAD_GROUP if the pattern has fewer groups, REGEXP_INVALID
// if s is not valid UTF-8, REGEXP_FAILED if the match fails, and
// REGEXP_NOMEM if out of memory.
RegexpStatus regexp_capture(Regexp* re,
                            const char* s,
                            size_t n,
                            size_t group,
                            bool* found,
                            size_t* start,
                            size_t* len,
                            char* err) {
    if (group > re->ngroups) {
        return REGEXP_BAD_GROUP;
    }
    int rc = pcre2_match(re->code, (PCRE2_SPTR)s, n, 0, 0, re->match, NULL);
    if (rc == PCRE2_ERROR_NOMATCH) {
        *found = false;
        return REGEXP_OK;
    }
    if (rc < 0) {
        return match_status(rc, err);
    }

    // The ovector has a pair of offsets for each group, both PCRE2_UNSET
    // for a group that did not participate. Computing s + PCRE2_UNSET would
    // be pointer overflow, so check before using them.
    const PCRE2_SIZE* ovector = pcre2_get_ovector_pointer(re->match);
    PCRE2_SIZE from = ovector[2 * group];
    PCRE2_SIZE to = ovector[2 * group + 1];
    // A \K in a lookahead could make from > to, but PCRE2 rejects \K
    // in lookarounds by default, so only an unset group gets here.
    if (from == PCRE2_UNSET || to < from) {
        *found = false;
        return REGEXP_OK;
    }
    *found = true;
    *start = from;
    *len = to - from;
    return REGEXP_OK;
}

#pragma endregion

#pragma region Replacing

// regexp_replace replaces every match of the pattern in s with repl, writes
// the result to buf of size bytes followed by a NUL, and sets len to the
// result length. As with snprintf, if len >= size, the result did not fit
// and buf holds no meaningful data: call again with a buffer of len + 1.
//
// The replacement syntax is PCRE2's extended one: $n, ${n} and ${name}
// insert a group, $0 the whole match, $$ a dollar sign; \U, \L, \u, \l and
// \E change the case; \ escapes a character as in a pattern. An unset group
// inserts an empty string, a group the pattern doesn't have is an error.
//
// Returns REGEXP_BAD_REPLACEMENT if repl is invalid, REGEXP_INVALID if s or
// repl is not valid UTF-8, REGEXP_FAILED if the match fails, and
// REGEXP_NOMEM if out of memory. size must be at least 1.
RegexpStatus regexp_replace(Regexp* re,
                            const char* s,
                            size_t n,
                            const char* repl,
                            size_t nrepl,
                            char* buf,
                            size_t size,
                            size_t* len,
                            char* err) {
    assert(size > 0);
    PCRE2_SIZE outlen = size;
    int rc = pcre2_substitute(re->code, (PCRE2_SPTR)s, n, 0, SUBSTITUTE_OPTIONS, re->match, NULL,
                              (PCRE2_SPTR)repl, nrepl, (PCRE2_UCHAR*)buf, &outlen);
    if (rc >= 0) {
        // The number of replacements, outlen is the result length.
        *len = outlen;
        return REGEXP_OK;
    }

    switch (rc) {
        case PCRE2_ERROR_NOMEMORY:
            // With PCRE2_SUBSTITUTE_OVERFLOW_LENGTH, a buffer too small sets
            // outlen to the size needed, including the NUL. A real allocation
            // failure leaves it PCRE2_UNSET.
            if (outlen == PCRE2_UNSET) {
                return REGEXP_NOMEM;
            }
            *len = outlen - 1;
            return REGEXP_OK;

        case PCRE2_ERROR_BADREPESCAPE:
        case PCRE2_ERROR_BADREPLACEMENT:
        case PCRE2_ERROR_BADSUBSPATTERN:
        case PCRE2_ERROR_BADSUBSTITUTION:
        case PCRE2_ERROR_NOSUBSTRING:
        case PCRE2_ERROR_REPMISSINGBRACE:
        case PCRE2_ERROR_UNSET:
            // outlen is the offset of the error in the replacement, if known.
            write_error(err, rc, outlen);
            return REGEXP_BAD_REPLACEMENT;
    }

    // PCRE2 checks both the replacement and the subject, with the same
    // error codes, so either may be invalid.
    if (is_utf_error(rc)) {
        return REGEXP_INVALID;
    }
    return match_status(rc, err);
}

#pragma endregion
