// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Regular expressions based on the PCRE2 engine.

// Patterns are compiled in UTF mode with Unicode properties (PCRE2_UTF and
// PCRE2_UCP): they work on characters, and \d, \w, \b and case folding follow
// Unicode. Every string is a pointer and a length in bytes, so it doesn't have
// to be NUL-terminated and may contain NUL characters. The pointer must not be
// NULL, even for an empty string.

// The functions that can fail return a RegexpStatus and pass the result
// through out-parameters. Those that take an err buffer of REGEXP_ERR_BUF_SIZE
// bytes write a NUL-terminated message to it on REGEXP_BAD_PATTERN,
// REGEXP_BAD_REPLACEMENT and REGEXP_FAILED.

#ifndef REGEXP_REGEXP_H
#define REGEXP_REGEXP_H

#include <stdbool.h>
#include <stddef.h>

// RegexpStatus is the outcome of a function that can fail.
typedef enum {
    REGEXP_OK,
    REGEXP_INVALID,          // the subject or the replacement is not valid UTF-8
    REGEXP_BAD_PATTERN,      // the pattern does not compile
    REGEXP_BAD_GROUP,        // the pattern has no such capturing group
    REGEXP_BAD_REPLACEMENT,  // the replacement is malformed or refers to a missing group
    REGEXP_FAILED,           // the match hit a limit (backtracking, depth, heap) or failed
    REGEXP_NOMEM,            // out of memory
} RegexpStatus;

// REGEXP_ERR_BUF_SIZE is the size of the err buffer that is always enough:
// the longest PCRE2 message is 91 characters, followed by " at offset "
// (11 characters), a size_t offset (up to 20 digits) and the NUL.
#define REGEXP_ERR_BUF_SIZE 128

// Regexp is a compiled pattern along with the memory for matching it.
// Matching modifies the memory, so a Regexp must not be used by two
// threads at once.
typedef struct Regexp Regexp;

// Compiling.

// regexp_compile compiles the pattern into out. Free it with regexp_free.
// Returns REGEXP_BAD_PATTERN if the pattern is invalid, and REGEXP_NOMEM
// if out of memory.
RegexpStatus regexp_compile(const char* pattern, size_t n, Regexp** out, char* err);

// regexp_free frees the compiled pattern. Does nothing for NULL.
void regexp_free(Regexp* re);

// Matching.

// regexp_like sets matched to whether the pattern matches somewhere in s.
// Returns REGEXP_INVALID if s is not valid UTF-8, REGEXP_FAILED if the match
// fails, and REGEXP_NOMEM if out of memory.
RegexpStatus regexp_like(Regexp* re, const char* s, size_t n, bool* matched, char* err);

// regexp_capture finds the first match of the pattern in s, and sets
// [start, start+len) to the bytes of s the capturing group matched.
// Group 0 is the whole match. Sets found to false if there is no match,
// or if the group did not participate in it. Returns REGEXP_BAD_GROUP if
// the pattern has fewer groups, REGEXP_INVALID if s is not valid UTF-8,
// REGEXP_FAILED if the match fails, and REGEXP_NOMEM if out of memory.
RegexpStatus regexp_capture(Regexp* re,
                            const char* s,
                            size_t n,
                            size_t group,
                            bool* found,
                            size_t* start,
                            size_t* len,
                            char* err);

// Replacing.

// regexp_replace replaces every match of the pattern in s with repl, writes
// the result to buf of size bytes followed by a NUL, and sets len to the
// result length. As with snprintf, if len >= size, the result did not fit
// and buf holds no meaningful data: call again with a buffer of len + 1.
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
                            char* err);

#endif /* REGEXP_REGEXP_H */
