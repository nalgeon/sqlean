// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Unicode support for SQLite.

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

#include "unicode/extension.h"

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_unicode_init(sqlite3* db, char** errmsg_ptr, const sqlite3_api_routines* api) {
    (void)errmsg_ptr;
    SQLITE_EXTENSION_INIT2(api);
    return unicode_init(db);
}