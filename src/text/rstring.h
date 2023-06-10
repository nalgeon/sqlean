// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Rune (UTF-8) string data structure.

#ifndef RSTRING_H
#define RSTRING_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// RuneString is a string composed of UTF-8 characters (runes).
typedef struct {
    // array of utf-8 characters
    const int32_t* runes;
    // number of characters in the string
    size_t length;
    // number of bytes in the string
    size_t size;
    // indicates whether the string owns the array
    // and should free the memory when destroyed
    bool owning;
} RuneString;

// RuneString methods.
struct rstring_ns {
    RuneString (*new)(void);
    RuneString (*from_cstring)(const char* const utf8str);
    char* (*to_cstring)(RuneString str);
    void (*free)(RuneString str);

    int32_t (*at)(RuneString str, size_t idx);
    RuneString (*slice)(RuneString str, int start, int end);
    RuneString (*substring)(RuneString str, size_t start, size_t length);

    int (*index)(RuneString str, RuneString other);
    int (*last_index)(RuneString str, RuneString other);

    RuneString (*translate)(RuneString str, RuneString from, RuneString to);
    RuneString (*reverse)(RuneString str);

    RuneString (*trim_left)(RuneString str, RuneString chars);
    RuneString (*trim_right)(RuneString str, RuneString chars);
    RuneString (*trim)(RuneString str, RuneString chars);
    RuneString (*pad_left)(RuneString str, size_t length, RuneString fill);
    RuneString (*pad_right)(RuneString str, size_t length, RuneString fill);

    void (*print)(RuneString str);
};

extern struct rstring_ns rstring;

#endif /* RSTRING_H */
