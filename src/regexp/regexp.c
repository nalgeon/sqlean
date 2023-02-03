// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

/*
 * PCRE wrapper.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pcre2/pcre2.h"
#include "regexp.h"

#pragma region Expression cache

#ifdef REGEXP_CACHE

#define SEED 0x12345678
#define MAX_CACHED_EXPRESSIONS 16

typedef struct CachedExpr {
    pcre2_code* re;
    size_t hash;
} CachedExpr;

CachedExpr regexp_cache[MAX_CACHED_EXPRESSIONS];

static size_t murmur(const unsigned char* str, uint32_t h) {
    // One-byte-at-a-time hash based on Murmur's mix
    // Source: https://github.com/aappleby/smhasher/blob/master/src/Hashes.cpp
    for (; *str; ++str) {
        h ^= *str;
        h *= 0x5bd1e995;
        h ^= h >> 15;
    }
    return h;
}

static pcre2_code* regexp_compile(PCRE2_SPTR pattern, PCRE2_SIZE pattern_length, int options) {
    size_t pattern_hash = murmur(pattern, SEED);

    // Check if the expression is already in the cache
    for (int i = 0; i < MAX_CACHED_EXPRESSIONS; i++) {
        CachedExpr cached = regexp_cache[i];
        if (cached.hash == 0) {
            continue;
        }
        // Compare the expression to the cached expression
        if (cached.hash == pattern_hash) {
            // The expression is already in the cache, return a copy
            return pcre2_code_copy(cached.re);
        }
    }

    // The expression is not in the cache, compile it
    size_t erroffset;
    int errcode;
    pcre2_code* re = pcre2_compile(pattern, pattern_length, options, &errcode, &erroffset, NULL);

    // Cache the expression
    size_t idx;
    for (idx = 0; idx < MAX_CACHED_EXPRESSIONS; idx++) {
        CachedExpr cached = regexp_cache[idx];
        if (cached.hash == 0) {
            cached.re = pcre2_code_copy(re);
            cached.hash = pattern_hash;
            regexp_cache[idx] = cached;
            break;
        }
    }

    // Free cache space if necessary
    size_t next_idx = (idx + 1) % MAX_CACHED_EXPRESSIONS;
    if (regexp_cache[next_idx].hash != 0) {
        pcre2_code_free(regexp_cache[next_idx].re);
        CachedExpr empty = {};
        regexp_cache[next_idx] = empty;
    }

    return re;
}

#else

static pcre2_code* regexp_compile(PCRE2_SPTR pattern) {
    size_t erroffset;
    int errcode;
    uint32_t options = PCRE2_UCP | PCRE2_UTF;
    pcre2_code* re =
        pcre2_compile(pattern, PCRE2_ZERO_TERMINATED, options, &errcode, &erroffset, NULL);
    return re;
}

#endif /* REGEXP_CACHE */

#pragma endregion

/*
 * regexp_like checks if source string matches pattern.
 * returns:
 *     -1 if the pattern is invalid
 *      0 if there is no match
 *      1 if there is a match
 */
static int regexp_like(const char* pattern, const char* source) {
    pcre2_code* re = regexp_compile((const unsigned char*)pattern);
    if (re == NULL) {
        return -1;
    }

    pcre2_match_data* match_data;
    match_data = pcre2_match_data_create_from_pattern(re, NULL);

    size_t source_len = strlen(source);

    int rc = pcre2_match(re, (const unsigned char*)source, source_len, 0, 0, match_data, NULL);

    pcre2_match_data_free(match_data);
    pcre2_code_free(re);

    if (rc <= 0) {
        return 0;
    } else {
        return 1;
    }
}

/*
 * regexp_substr extracts source substring matching pattern into `substr`.
 * returns:
 *     -1 if the pattern is invalid
 *      0 if there is no match
 *      1 if there is a match
 */
static int regexp_substr(const char* pattern, const char* source, char** substr) {
    pcre2_code* re = regexp_compile((const unsigned char*)pattern);
    if (re == NULL) {
        return -1;
    }

    pcre2_match_data* match_data;
    match_data = pcre2_match_data_create_from_pattern(re, NULL);

    int rc = pcre2_match(re, (const unsigned char*)source, PCRE2_ZERO_TERMINATED, 0, 0, match_data,
                         NULL);

    if (rc <= 0) {
        pcre2_match_data_free(match_data);
        pcre2_code_free(re);
        return 0;
    }

    size_t* ovector = pcre2_get_ovector_pointer(match_data);

    const char* substr_start = source + ovector[0];
    size_t substr_len = ovector[1] - ovector[0];

    *substr = malloc(substr_len + 1);
    memcpy(*substr, substr_start, substr_len);
    (*substr)[substr_len] = '\0';

    pcre2_match_data_free(match_data);
    pcre2_code_free(re);
    return 1;
}

/*
 * regexp_replace replaces matching substring with replacement string into `dest`.
 * returns:
 *     -1 if the pattern is invalid
 *      0 if there is no match
 *      1 if there is a match
 */
static int regexp_replace(const char* pattern, const char* source, const char* repl, char** dest) {
    pcre2_code* re = regexp_compile((const unsigned char*)pattern);
    if (re == NULL) {
        return -1;
    }

    pcre2_match_data* match_data;
    match_data = pcre2_match_data_create_from_pattern(re, NULL);

    const int options = PCRE2_SUBSTITUTE_GLOBAL | PCRE2_SUBSTITUTE_EXTENDED;
    size_t source_len = strlen(source);
    size_t outlen = source_len + 1024;
    char* output = malloc(outlen);
    int rc = pcre2_substitute(re, (const unsigned char*)source, PCRE2_ZERO_TERMINATED, 0, options,
                              match_data, NULL, (const unsigned char*)repl, PCRE2_ZERO_TERMINATED,
                              (unsigned char*)output, &outlen);

    if (rc <= 0) {
        pcre2_match_data_free(match_data);
        pcre2_code_free(re);
        free(output);
        return 0;
    }

    *dest = malloc(outlen + 1);
    memcpy(*dest, output, outlen);
    (*dest)[outlen] = '\0';

    pcre2_match_data_free(match_data);
    pcre2_code_free(re);
    free(output);
    return 1;
}

struct regexp_ns regexp = {.like = regexp_like, .substr = regexp_substr, .replace = regexp_replace};