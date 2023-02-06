// Copyright (c) 2022 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

#include "define/extension.h"

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_define_init(sqlite3* db, char** errmsg_ptr, const sqlite3_api_routines* api) {
    (void)errmsg_ptr;
    SQLITE_EXTENSION_INIT2(api);
    int status = define_manage_init(db);
    define_eval_init(db);
    define_module_init(db);
    return status;
}
