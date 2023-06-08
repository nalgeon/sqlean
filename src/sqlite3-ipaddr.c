// Copyright (c) 2021 Vincent Bernat, MIT License
// https://github.com/nalgeon/sqlean

// IP address manipulation in SQLite.

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

#include "ipaddr/extension.h"

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_ipaddr_init(sqlite3* db, char** errmsg_ptr, const sqlite3_api_routines* api) {
    (void)errmsg_ptr;
    SQLITE_EXTENSION_INIT2(api);
    return ipaddr_init(db);
}