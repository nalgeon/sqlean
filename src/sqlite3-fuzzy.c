// Copyright (c) 2021 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Fuzzy string matching and phonetics.

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

#include "fuzzy/extension.h"

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_fuzzy_init(sqlite3* db, char** errmsg_ptr, const sqlite3_api_routines* api) {
    (void)errmsg_ptr;
    SQLITE_EXTENSION_INIT2(api);
    return fuzzy_init(db);
}
