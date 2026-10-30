// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// PCRE2 build constants.

// The vendored PCRE2 sources expect these before any of their own includes,
// so the Makefile force-includes this file (-include) into every compilation
// unit of the module, instead of the sources including it.

#ifndef REGEXP_CONSTANTS_H
#define REGEXP_CONSTANTS_H

#define PCRE2_CODE_UNIT_WIDTH 8
#define LINK_SIZE 2
#define HAVE_CONFIG_H
#define SUPPORT_UNICODE

#if defined(_WIN32)
#define PCRE2_STATIC
#endif

#endif /* REGEXP_CONSTANTS_H */
