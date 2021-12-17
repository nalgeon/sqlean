// Copyright (c) 2021 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Cube root function.

#include <assert.h>
#include <math.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

// cbrt(X)
// Calculates cube root of X for integer or real X.
// Returns NULL for other types.
static void sqlite3_cbrt(sqlite3_context* context, int argc, sqlite3_value** argv) {
    double x;
    assert(argc == 1);
    switch (sqlite3_value_numeric_type(argv[0])) {
        case SQLITE_INTEGER:
        case SQLITE_FLOAT:
            x = sqlite3_value_double(argv[0]);
            break;
        default:
            return;
    }
    double answer = cbrt(x);
    sqlite3_result_double(context, answer);
}

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_cbrt_init(sqlite3* db, char** pzErrMsg, const sqlite3_api_routines* pApi) {
    SQLITE_EXTENSION_INIT2(pApi);
    static const int flags = SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC;
    sqlite3_create_function(db, "cbrt", 1, flags, 0, sqlite3_cbrt, 0, 0);
    return SQLITE_OK;
}