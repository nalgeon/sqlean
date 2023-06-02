// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Text handling.

#ifndef TEXT_H
#define TEXT_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

int32_t* runes_from_cstring(const char* const str, size_t length);
char* runes_to_cstring(const int32_t* runes, size_t length);

typedef struct {
    const int32_t* runes;
    size_t length;
    size_t size;
    bool owning;
} RuneString;

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

    RuneString (*reverse)(RuneString str);

    RuneString (*pad_left)(RuneString str, size_t length, RuneString fill);
    RuneString (*pad_right)(RuneString str, size_t length, RuneString fill);

    void (*print)(RuneString str);
};

extern struct rstring_ns rstring;

typedef struct {
    const char* bytes;
    size_t length;
    bool owning;
} ByteString;

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

#endif /* TEXT_H */
