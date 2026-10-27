// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Internal declarations and helpers shared by the SQLite binding layer:
// the scalar functions and the table-valued functions.

#ifndef FILEIO_BINDING_H
#define FILEIO_BINDING_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "sqlite3ext.h"

// fileio_ls_init registers the fileio_ls table-valued function.
int fileio_ls_init(sqlite3* db);

// fileio_scan_init registers the fileio_scan table-valued function.
int fileio_scan_init(sqlite3* db);

// ArgStatus is the result of decoding an argument that can fail
// because of bad data or because SQLite runs out of memory.
typedef enum {
    ARG_OK,
    ARG_INVALID,
    ARG_NOMEM,
} ArgStatus;

// has_null reports whether any of the arguments is NULL.
static inline bool has_null(int argc, sqlite3_value** argv) {
    for (int i = 0; i < argc; i++) {
        if (sqlite3_value_type(argv[i]) == SQLITE_NULL) {
            return true;
        }
    }
    return false;
}

// arg_path decodes a path argument into out. A path is text without
// embedded NULs: a blob or a string with a NUL is ARG_INVALID.
static inline ArgStatus arg_path(sqlite3_value* value, const char** out) {
    if (sqlite3_value_type(value) == SQLITE_BLOB) {
        return ARG_INVALID;
    }
    const char* path = (const char*)sqlite3_value_text(value);
    if (path == NULL) {
        return ARG_NOMEM;
    }
    if (strlen(path) != (size_t)sqlite3_value_bytes(value)) {
        return ARG_INVALID;
    }
    *out = path;
    return ARG_OK;
}

// arg_int64 decodes an integer argument into out, converting numeric text.
// Returns false if the value is not an integer.
static inline bool arg_int64(sqlite3_value* value, int64_t* out) {
    if (sqlite3_value_numeric_type(value) != SQLITE_INTEGER) {
        return false;
    }
    *out = sqlite3_value_int64(value);
    return true;
}

#endif /* FILEIO_BINDING_H */
