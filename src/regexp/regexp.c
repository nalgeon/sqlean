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

/*
 * regexp_like checks if source string matches pattern.
 * returns:
 *     -1 if the pattern is invalid
 *      0 if there is no match
 *      1 if there is a match
 */
static int regexp_like(const char* pattern, const char* source) {
    pcre2_code* re;
    size_t erroffset;
    int errcode;

    re = pcre2_compile((const unsigned char*)pattern, PCRE2_ZERO_TERMINATED, 0, &errcode,
                       &erroffset, NULL);
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
    pcre2_code* re;
    size_t erroffset;
    int errcode;

    re = pcre2_compile((const unsigned char*)pattern, PCRE2_ZERO_TERMINATED, 0, &errcode,
                       &erroffset, NULL);
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
    pcre2_code* re;
    size_t erroffset;
    int errcode;

    re = pcre2_compile((const unsigned char*)pattern, PCRE2_ZERO_TERMINATED, 0, &errcode,
                       &erroffset, NULL);
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