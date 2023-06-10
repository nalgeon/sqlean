// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Sqlean extensions bundle.

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

// include most of the extensions,
// except ipaddr, which does not support Windows
#include "crypto/extension.h"
#include "define/extension.h"
#include "fileio/extension.h"
#include "fuzzy/extension.h"
#include "math/extension.h"
#include "regexp/extension.h"
#include "stats/extension.h"
#include "text/extension.h"
#include "unicode/extension.h"
#include "uuid/extension.h"
#include "vsv/extension.h"

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_sqlean_init(sqlite3* db, char** errmsg_ptr, const sqlite3_api_routines* api) {
    (void)errmsg_ptr;
    SQLITE_EXTENSION_INIT2(api);
    crypto_init(db);
    define_init(db);
    fileio_init(db);
    fuzzy_init(db);
    math_init(db);
    regexp_init(db);
    stats_init(db);
    text_init(db);
    unicode_init(db);
    uuid_init(db);
    vsv_init(db);
    return SQLITE_OK;
}