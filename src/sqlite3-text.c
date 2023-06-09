// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// SQLite extension for working with text.

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

#include "text/extension.h"

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_text_init(sqlite3* db, char** errmsg_ptr, const sqlite3_api_routines* api) {
    (void)errmsg_ptr;
    SQLITE_EXTENSION_INIT2(api);
    return text_init(db);
}