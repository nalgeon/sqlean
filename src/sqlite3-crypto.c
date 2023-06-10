// Copyright (c) 2021 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// SQLite hash and encode/decode functions.

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

#include "crypto/extension.h"

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_crypto_init(sqlite3* db, char** errmsg_ptr, const sqlite3_api_routines* api) {
    (void)errmsg_ptr;
    SQLITE_EXTENSION_INIT2(api);
    return crypto_init(db);
}
