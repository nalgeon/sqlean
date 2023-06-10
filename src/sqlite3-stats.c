// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Statistical functions for SQLite.

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

#include "stats/extension.h"

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_stats_init(sqlite3* db, char** errmsg_ptr, const sqlite3_api_routines* api) {
    (void)errmsg_ptr;
    SQLITE_EXTENSION_INIT2(api);
    return stats_init(db);
}
