// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

/*
 * PCRE wrapper.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "regexp/pcre2/pcre2.h"
#include "regexp/regexp.h"

/*
 * regexp_compile compiles and returns the compiled regexp.
 */
static pcre2_code* regexp_compile(const char* pattern) {
    size_t erroffset;
    int errcode;
    uint32_t options = PCRE2_UCP | PCRE2_UTF;
    pcre2_code* re = pcre2_compile((PCRE2_SPTR8)pattern, PCRE2_ZERO_TERMINATED, options, &errcode,
                                   &erroffset, NULL);
    return re;
}

/*
 * regexp_free frees the compiled regexp.
 */
static void regexp_free(pcre2_code* re) {
    pcre2_code_free(re);
}

/*
 * regexp_like checks if source string matches pattern.
 * returns:
 *     -1 if the pattern is invalid
 *      0 if there is no match
 *      1 if there is a match
 */
static int regexp_like(pcre2_code* re, const char* source) {
    if (re == NULL) {
        return -1;
    }

    pcre2_match_data* match_data;
    match_data = pcre2_match_data_create_from_pattern(re, NULL);

    size_t source_len = strlen(source);

    int rc = pcre2_match(re, (const unsigned char*)source, source_len, 0, 0, match_data, NULL);

    pcre2_match_data_free(match_data);

    if (rc <= 0) {
        return 0;
    } else {
        return 1;
    }
}

/*
 * regexp_extract extracts source substring matching pattern into `substr`.
 * if `group_idx` > 0, returns the corresponding group instead of the whole matched substring.
 * returns:
 *     -1 if the pattern is invalid
 *      0 if there is no match
 *      1 if there is a match
 */
static int regexp_extract(pcre2_code* re, const char* source, size_t group_idx, char** substr) {
    if (re == NULL) {
        return -1;
    }

    pcre2_match_data* match_data;
    match_data = pcre2_match_data_create_from_pattern(re, NULL);

    int rc = pcre2_match(re, (const unsigned char*)source, PCRE2_ZERO_TERMINATED, 0, 0, match_data,
                         NULL);

    if (rc <= 0) {
        pcre2_match_data_free(match_data);
        return 0;
    }

    if (group_idx >= rc) {
        pcre2_match_data_free(match_data);
        return 0;
    }

    size_t* ovector = pcre2_get_ovector_pointer(match_data);

    const char* substr_start = source + ovector[2 * group_idx];
    size_t substr_len = ovector[2 * group_idx + 1] - ovector[2 * group_idx];

    *substr = malloc(substr_len + 1);
    memcpy(*substr, substr_start, substr_len);
    (*substr)[substr_len] = '\0';

    pcre2_match_data_free(match_data);
    return 1;
}

/*
 * regexp_replace replaces matching substring with replacement string into `dest`.
 * returns:
 *     -1 if the pattern is invalid
 *      0 if there is no match
 *      1 if there is a match
 */
static int regexp_replace(pcre2_code* re, const char* source, const char* repl, char** dest) {
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
        free(output);
        return 0;
    }

    *dest = malloc(outlen + 1);
    memcpy(*dest, output, outlen);
    (*dest)[outlen] = '\0';

    pcre2_match_data_free(match_data);
    free(output);
    return 1;
}

struct regexp_ns regexp = {
    .compile = regexp_compile,
    .free = regexp_free,
    .like = regexp_like,
    .extract = regexp_extract,
    .replace = regexp_replace,
};
