// Originally from SQLite 3.42.0 source code (func.c), Public Domain

// Modified by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean/

// SQLite math functions.

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

#include "math/extension.h"

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_math_init(sqlite3* db, char** errmsg_ptr, const sqlite3_api_routines* api) {
    (void)errmsg_ptr;
    SQLITE_EXTENSION_INIT2(api);
    return math_init(db);
}
