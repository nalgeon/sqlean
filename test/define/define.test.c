// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// User-defined function tests.

// The define extension is a thin layer over SQLite itself, so these tests
// run it in real connections (statically linked SQLite, see the Makefile)
// instead of testing a domain layer in isolation. They cover what the SQL
// tests can't: error codes and messages, function flags, and the behavior
// across several connections.
//
// Defined scalar functions keep their prepared statements in a process-wide
// cache, and a connection with cached statements can't be closed until
// define_free() is called. Tests close such connections with sqlite3_close_v2,
// which defers the actual close until test_define_free frees the cache.
// test_define_free must run last: after define_free(), define() and defined
// functions fail in every connection of the process.

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "define/extension.h"
#include "sqlite3.h"

#define DB_PATH "define.test.db"

typedef struct {
    int rc;
    char msg[256];
} Result;

// open_db opens a database and initializes the extension in it.
static sqlite3* open_db(const char* path) {
    sqlite3* db;
    int rc = sqlite3_open(path, &db);
    assert(rc == SQLITE_OK);
    rc = define_init(db);
    assert(rc == SQLITE_OK);
    return db;
}

// remove_db deletes the test database file.
static void remove_db(void) {
    remove(DB_PATH);
    remove(DB_PATH "-journal");
}

// exec runs a single statement to completion and returns its result code
// (SQLITE_OK on success) and the error message.
static Result exec(sqlite3* db, const char* sql) {
    Result res = {SQLITE_OK, ""};
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        }
        if (rc == SQLITE_DONE) {
            rc = SQLITE_OK;
        }
    }
    res.rc = rc;
    if (rc != SQLITE_OK) {
        snprintf(res.msg, sizeof(res.msg), "%s", sqlite3_errmsg(db));
    }
    sqlite3_finalize(stmt);
    return res;
}

// exec_ok runs a statement that must succeed.
static void exec_ok(sqlite3* db, const char* sql) {
    Result res = exec(db, sql);
    if (res.rc != SQLITE_OK) {
        printf("%s: %s\n", sql, res.msg);
    }
    assert(res.rc == SQLITE_OK);
}

// query_int returns the first column of the first row as an integer.
static int64_t query_int(sqlite3* db, const char* sql) {
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    assert(rc == SQLITE_OK);
    rc = sqlite3_step(stmt);
    assert(rc == SQLITE_ROW);
    int64_t val = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    return val;
}

typedef struct {
    const char* sql;
    int rc;
    const char* msg;
} ErrorTest;

// check_errors runs each statement and checks its result code and message.
static void check_errors(sqlite3* db, const ErrorTest* tests, size_t n) {
    for (size_t i = 0; i < n; i++) {
        ErrorTest test = tests[i];
        Result got = exec(db, test.sql);
        // printf("%s: want %d %s, got %d %s\n", test.sql, test.rc, test.msg, got.rc, got.msg);
        assert(got.rc == test.rc);
        assert(strcmp(got.msg, test.msg) == 0);
    }
}

#pragma region Init

static void test_init(void) {
    printf("test_init...");
    remove_db();

    // creates sqlean_define
    sqlite3* db = open_db(DB_PATH);
    assert(query_int(db, "select count(*) from sqlite_master where name = 'sqlean_define'") == 1);
    exec_ok(db, "create table data(x)");
    sqlite3_close(db);

    // fails on a read-only database without sqlean_define
    remove_db();
    db = open_db(DB_PATH);
    exec_ok(db, "drop table sqlean_define");
    sqlite3_close(db);
    sqlite3_open_v2(DB_PATH, &db, SQLITE_OPEN_READONLY, NULL);
    assert(define_init(db) == SQLITE_READONLY);
    sqlite3_close(db);

    // works on a read-only database with sqlean_define
    remove_db();
    db = open_db(DB_PATH);
    exec_ok(db, "insert into sqlean_define values ('plus', 'scalar', '? + 1')");
    sqlite3_close(db);
    sqlite3_open_v2(DB_PATH, &db, SQLITE_OPEN_READONLY, NULL);
    assert(define_init(db) == SQLITE_OK);
    assert(query_int(db, "select plus(1)") == 2);
    sqlite3_close_v2(db);

    remove_db();
    printf("OK\n");
}

#pragma endregion

#pragma region Errors

// Most errors lose the original message and report a bare result code
// (current behavior).
static const ErrorTest define_errors[] = {
    {"select define(null, '1')", SQLITE_MISUSE, "bad parameter or other API misuse"},
    {"select define('f', null)", SQLITE_ERROR, "SQL logic error"},  // compiles 'select '
    {"select define('f', '')", SQLITE_ERROR, "SQL logic error"},
    {"select define('f', 'from')", SQLITE_ERROR, "SQL logic error"},  // syntax error
    {"select define('f', 'nosuch(?)')", SQLITE_ERROR, "SQL logic error"},
    {"select define('answer', '24')", SQLITE_BUSY, "database is locked"},   // redefine
    {"select define('abs', 'abs(?)')", SQLITE_BUSY, "database is locked"},  // override built-in
    {"select zero()", SQLITE_MISUSE, "bad parameter or other API misuse"},  // no rows
    {"select overflow()", SQLITE_ERROR, "SQL logic error"},  // runtime error in the body
    {"select answer(1)", SQLITE_ERROR, "wrong number of arguments to function answer()"},
};

static void test_define_errors(void) {
    printf("test_define_errors...");
    sqlite3* db = open_db(":memory:");
    exec_ok(db, "select define('answer', '42')");
    exec_ok(db, "select define('zero', '1 where 0')");
    exec_ok(db, "select define('overflow', 'abs(-9223372036854775808)')");
    check_errors(db, define_errors, sizeof(define_errors) / sizeof(define_errors[0]));
    // failed definitions are not saved
    assert(query_int(db, "select count(*) from sqlean_define") == 3);
    assert(query_int(db, "select answer()") == 42);
    sqlite3_close_v2(db);
    printf("OK\n");
}

static const ErrorTest table_errors[] = {
    {"create virtual table t using define()", SQLITE_MISUSE, "no statement provided"},
    {"create virtual table t using define(select 1)", SQLITE_MISUSE,
     "statement must be parenthesized"},
    {"create virtual table t using define((select from))", SQLITE_ERROR,
     "near \"from\": syntax error"},
    {"create virtual table t using define((select * from nosuch))", SQLITE_ERROR,
     "no such table: nosuch"},
    {"create virtual table t using define((insert into data values (1)))", SQLITE_ERROR,
     "Statement must be read only."},
    {"create virtual table t using define((select 1 as a, 2 as a))", SQLITE_ERROR,
     "duplicate column name: a"},
    // parameters accept only equality constraints
    {"select * from half where x > 1", SQLITE_ERROR, "no query solution"},
    // a runtime error in the body loses its message (current behavior)
    {"select * from overflow", SQLITE_ERROR, "SQL logic error"},
};

static void test_table_errors(void) {
    printf("test_table_errors...");
    sqlite3* db = open_db(":memory:");
    exec_ok(db, "create table data(x)");
    exec_ok(db, "create virtual table half using define((select :x / 2 as value))");
    exec_ok(db,
            "create virtual table overflow using define((select abs(-9223372036854775808) as v))");
    check_errors(db, table_errors, sizeof(table_errors) / sizeof(table_errors[0]));
    // failed definitions are not saved
    assert(query_int(db, "select count(*) from sqlean_define") == 2);
    assert(query_int(db, "select count(*) from sqlite_master where name = 't'") == 0);
    sqlite3_close(db);
    printf("OK\n");
}

static const ErrorTest eval_errors[] = {
    {"select eval('bogus')", SQLITE_ERROR, "near \"bogus\": syntax error"},
    {"select eval('select * from nosuch')", SQLITE_ERROR, "no such table: nosuch"},
    {"select eval('select abs(-9223372036854775808)')", SQLITE_ERROR, "integer overflow"},
    // statements before the failing one take effect
    {"select eval('insert into data values (1); select * from nosuch')", SQLITE_ERROR,
     "no such table: nosuch"},
};

static void test_eval_errors(void) {
    printf("test_eval_errors...");
    sqlite3* db = open_db(":memory:");
    exec_ok(db, "create table data(x)");
    check_errors(db, eval_errors, sizeof(eval_errors) / sizeof(eval_errors[0]));
    assert(query_int(db, "select count(*) from data") == 1);
    sqlite3_close(db);
    printf("OK\n");
}

#pragma endregion

#pragma region Flags

// The management functions and eval are direct-only, so they can't be used
// in views or triggers, whatever trusted_schema says.
static const ErrorTest direct_only_errors[] = {
    {"select * from v_define", SQLITE_ERROR, "unsafe use of define()"},
    {"select * from v_undefine", SQLITE_ERROR, "unsafe use of undefine()"},
    {"select * from v_define_free", SQLITE_ERROR, "unsafe use of define_free()"},
    {"select * from v_eval", SQLITE_ERROR, "unsafe use of eval()"},
    {"insert into data values (1)", SQLITE_ERROR, "unsafe use of eval()"},
};

static void test_flags(void) {
    printf("test_flags...");
    sqlite3* db = open_db(":memory:");
    exec_ok(db, "select define('plus', '? + 1')");
    exec_ok(db, "create table data(x)");
    exec_ok(db, "create table log(x)");
    exec_ok(db, "create view v_define as select define('f', '1')");
    exec_ok(db, "create view v_undefine as select undefine('plus')");
    exec_ok(db, "create view v_define_free as select define_free()");
    exec_ok(db, "create view v_eval as select eval('select 1')");
    exec_ok(db, "create trigger t_eval after insert on data begin select eval('select 1'); end");

    exec_ok(db, "pragma trusted_schema = 1");
    check_errors(db, direct_only_errors,
                 sizeof(direct_only_errors) / sizeof(direct_only_errors[0]));
    exec_ok(db, "drop trigger t_eval");

    // defined functions are not innocuous, so they are allowed in views
    // only with a trusted schema
    exec_ok(db, "create view v_plus as select plus(41) as value");
    assert(query_int(db, "select value from v_plus") == 42);
    exec_ok(db, "pragma trusted_schema = 0");
    Result res = exec(db, "select value from v_plus");
    assert(res.rc == SQLITE_ERROR);
    assert(strcmp(res.msg, "unsafe use of plus()") == 0);

    // a defined function wrapping eval makes eval usable in a view
    // (documented in the Security section of docs/define.md)
    exec_ok(db, "pragma trusted_schema = 1");
    exec_ok(db, "select define('evalf', 'eval(?)')");
    exec_ok(db, "create view v_evalf as select evalf('insert into log values (42)')");
    exec_ok(db, "select * from v_evalf");
    assert(query_int(db, "select count(*) from log") == 1);

    // so does a table function
    exec_ok(db, "create virtual table evalt using define((select eval(:sql) as r))");
    exec_ok(db, "create view v_evalt as select r from evalt('insert into log values (42)')");
    exec_ok(db, "select * from v_evalt");
    assert(query_int(db, "select count(*) from log") == 2);

    // trusted_schema = 0 blocks both
    exec_ok(db, "pragma trusted_schema = 0");
    res = exec(db, "select * from v_evalf");
    assert(res.rc == SQLITE_ERROR);
    assert(strcmp(res.msg, "unsafe use of evalf()") == 0);
    res = exec(db, "select * from v_evalt");
    assert(res.rc == SQLITE_ERROR);
    assert(strcmp(res.msg, "unsafe use of virtual table \"evalt\"") == 0);

    // except in CHECK constraints, which SQLite checks only since 3.50.0
    // (this fails after upgrading; update the Security section in docs/define.md)
    exec_ok(db, "create table checked(x check (evalf('insert into log values (42)') is null))");
    exec_ok(db, "insert into checked values (1)");
    assert(query_int(db, "select count(*) from log") == 3);

    sqlite3_close_v2(db);
    printf("OK\n");
}

#pragma endregion

#pragma region Connections

static void test_connections(void) {
    printf("test_connections...");
    remove_db();
    sqlite3* db1 = open_db(DB_PATH);
    sqlite3* db2 = open_db(DB_PATH);

    // functions are loaded on init, so existing connections don't see new ones
    exec_ok(db1, "select define('plus', '? + 1')");
    exec_ok(db1, "create virtual table half using define((select :x / 2 as value))");
    assert(query_int(db1, "select plus(1)") == 2);
    Result res = exec(db2, "select plus(1)");
    assert(res.rc == SQLITE_ERROR);
    assert(strcmp(res.msg, "no such function: plus") == 0);
    // table functions are regular schema objects, so they are visible at once
    assert(query_int(db2, "select value from half(10)") == 5);

    // new connections load the functions
    sqlite3* db3 = open_db(DB_PATH);
    assert(query_int(db3, "select plus(1)") == 2);
    assert(query_int(db3, "select value from half(10)") == 5);

    // undefine doesn't unregister the function in any open connection,
    // but new connections don't load it
    exec_ok(db1, "select undefine('plus')");
    assert(query_int(db1, "select plus(1)") == 2);
    assert(query_int(db3, "select plus(1)") == 2);
    sqlite3* db4 = open_db(DB_PATH);
    res = exec(db4, "select plus(1)");
    assert(res.rc == SQLITE_ERROR);
    assert(strcmp(res.msg, "no such function: plus") == 0);

    // the same name can be defined again in a connection that doesn't have it
    exec_ok(db4, "select define('plus', '? + 2')");
    assert(query_int(db4, "select plus(1)") == 3);
    assert(query_int(db1, "select plus(1)") == 2);

    sqlite3_close_v2(db1);
    sqlite3_close_v2(db2);
    sqlite3_close_v2(db3);
    sqlite3_close_v2(db4);
    remove_db();
    printf("OK\n");
}

#pragma endregion

#pragma region define_free

// test_define_free must run last, see the comment at the top.
static void test_define_free(void) {
    printf("test_define_free...");
    sqlite3* db1 = open_db(":memory:");
    sqlite3* db2 = open_db(":memory:");
    exec_ok(db1, "select define('plus', '? + 1')");
    exec_ok(db2, "select define('minus', '? - 1')");

    // a connection with defined functions can't be closed before define_free
    assert(sqlite3_close(db1) == SQLITE_BUSY);
    // a connection without defined functions can
    sqlite3* db3 = open_db(":memory:");
    assert(sqlite3_close(db3) == SQLITE_OK);

    // define_free in one connection frees the statements of all connections,
    // including the ones closed with sqlite3_close_v2 by the previous tests
    exec_ok(db1, "select define_free()");
    assert(sqlite3_close(db1) == SQLITE_OK);

    // after that, defined functions and define() fail in every connection
    // (current behavior)
    Result res = exec(db2, "select minus(1)");
    assert(res.rc == SQLITE_MISUSE);
    res = exec(db2, "select define('times2', '? * 2')");
    assert(res.rc == SQLITE_MISUSE);
    sqlite3* db4 = open_db(":memory:");
    res = exec(db4, "select define('times2', '? * 2')");
    assert(res.rc == SQLITE_MISUSE);
    assert(sqlite3_close(db4) == SQLITE_OK);

    // eval keeps working
    assert(query_int(db2, "select eval('select 42')") == 42);
    assert(sqlite3_close(db2) == SQLITE_OK);
    printf("OK\n");
}

#pragma endregion

int main(void) {
    // Init
    test_init();

    // Errors
    test_define_errors();
    test_table_errors();
    test_eval_errors();

    // Flags
    test_flags();

    // Connections
    test_connections();

    // define_free
    test_define_free();
}
