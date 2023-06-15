// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Byte string data structure.

#ifndef BSTRING_H
#define BSTRING_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// ByteString is a string composed of bytes.
typedef struct {
    // array of bytes
    const char* bytes;
    // number of bytes in the string
    size_t length;
    // indicates whether the string owns the array
    // and should free the memory when destroyed
    bool owning;
} ByteString;

// ByteString methods.
struct bstring_ns {
    ByteString (*new)(void);
    ByteString (*from_cstring)(const char* const cstring, size_t length);
    const char* (*to_cstring)(ByteString str);
    void (*free)(ByteString str);

    char (*at)(ByteString str, size_t idx);
    ByteString (*slice)(ByteString str, int start, int end);
    ByteString (*substring)(ByteString str, size_t start, size_t length);

    int (*index)(ByteString str, ByteString other);
    int (*last_index)(ByteString str, ByteString other);
    bool (*contains)(ByteString str, ByteString other);
    bool (*equals)(ByteString str, ByteString other);
    bool (*has_prefix)(ByteString str, ByteString other);
    bool (*has_suffix)(ByteString str, ByteString other);
    size_t (*count)(ByteString str, ByteString other);

    ByteString (*split_part)(ByteString str, ByteString sep, size_t part);
    ByteString (*join)(ByteString* strings, size_t count, ByteString sep);
    ByteString (*concat)(ByteString* strings, size_t count);
    ByteString (*repeat)(ByteString str, size_t count);

    ByteString (*replace)(ByteString str, ByteString old, ByteString new, size_t max_count);
    ByteString (*replace_all)(ByteString str, ByteString old, ByteString new);
    ByteString (*reverse)(ByteString str);

    ByteString (*trim_left)(ByteString str);
    ByteString (*trim_right)(ByteString str);
    ByteString (*trim)(ByteString str);

    void (*print)(ByteString str);
};

extern struct bstring_ns bstring;

#endif /* BSTRING_H */
