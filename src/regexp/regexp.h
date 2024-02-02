// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

#ifndef REGEXP_H
#define REGEXP_H

#include "regexp/pcre2/pcre2.h"

struct regexp_ns {
    pcre2_code* (*compile)(const char* pattern);
    void (*free)(pcre2_code* re);
    char* (*get_error)(const char* pattern);
    int (*like)(pcre2_code* re, const char* source);
    int (*extract)(pcre2_code* re, const char* source, size_t group_idx, char** substr);
    int (*replace)(pcre2_code* re, const char* source, const char* repl, char** dest);
};

extern struct regexp_ns regexp;

#endif /* REGEXP_H */
