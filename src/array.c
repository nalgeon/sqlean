// Copyright (c) 2022 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// One-dimensional arrays for SQLite.
// Supports integers, real numbers and strings (with limited max size).
// Uses 1-based indexing. Stores itself as a blob value.

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

#include "array/extension.h"

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_array_init(sqlite3* db, char** errmsg_ptr, const sqlite3_api_routines* api) {
    (void)errmsg_ptr;
    SQLITE_EXTENSION_INIT2(api);
    arrayscalar_init(db);
    arrayagg_init(db);
    unnest_init(db);
    return SQLITE_OK;
}
