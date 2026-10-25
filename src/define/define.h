// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// User-defined functions in SQLite.

#ifndef DEFINE_INTERNAL_H
#define DEFINE_INTERNAL_H

#include "sqlite3ext.h"

// define_save_function saves the function into the sqlean_define table
// and returns SQLITE_OK or an error code.
int define_save_function(sqlite3* db, const char* name, const char* type, const char* body);

// define_eval_init registers eval() and returns SQLITE_OK.
int define_eval_init(sqlite3* db);

// define_manage_init registers define(), undefine() and define_free(), loads
// the saved scalar functions, and returns SQLITE_OK or an error code.
int define_manage_init(sqlite3* db);

// define_module_init registers the "define" module for table functions,
// and returns SQLITE_OK.
int define_module_init(sqlite3* db);

#endif /* DEFINE_INTERNAL_H */
