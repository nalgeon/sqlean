// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// User-defined function tests.

// The define extension is a thin layer over SQLite itself, so these tests
// run it in real connections (statically linked SQLite, see the Makefile)
// instead of testing a domain layer in isolation. They cover what the SQL
// tests can't: error codes and messages, function flags, and the behavior
// across several connections.
//
// Every test closes its connections with close_db, which fails if anything
// (such as a cached statement) keeps the connection open.

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

// close_db closes a database connection, which must succeed.
static void close_db(sqlite3* db) {
    int rc = sqlite3_close(db);
    if (rc != SQLITE_OK) {
        printf("sqlite3_close: %s\n", sqlite3_errmsg(db));
    }
    assert(rc == SQLITE_OK);
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

// count_stmts returns the number of prepared statements in the connection.
static int count_stmts(sqlite3* db) {
    int n = 0;
    for (sqlite3_stmt* stmt = sqlite3_next_stmt(db, NULL); stmt != NULL;
         stmt = sqlite3_next_stmt(db, stmt)) {
        n++;
    }
    return n;
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
    close_db(db);

    // fails on a read-only database without sqlean_define
    remove_db();
    db = open_db(DB_PATH);
    exec_ok(db, "drop table sqlean_define");
    close_db(db);
    sqlite3_open_v2(DB_PATH, &db, SQLITE_OPEN_READONLY, NULL);
    assert(define_init(db) == SQLITE_READONLY);
    close_db(db);

    // works on a read-only database with sqlean_define
    remove_db();
    db = open_db(DB_PATH);
    exec_ok(db, "insert into sqlean_define values ('plus', 'scalar', '? + 1')");
    close_db(db);
    sqlite3_open_v2(DB_PATH, &db, SQLITE_OPEN_READONLY, NULL);
    assert(define_init(db) == SQLITE_OK);
    assert(query_int(db, "select plus(1)") == 2);
    close_db(db);

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
    close_db(db);
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
    close_db(db);
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
    close_db(db);
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

    close_db(db);
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

    // undefine removes the function from its own connection at once,
    // other open connections keep it until they reconnect,
    // and new connections don't load it
    exec_ok(db1, "select undefine('plus')");
    res = exec(db1, "select plus(1)");
    assert(res.rc == SQLITE_ERROR);
    assert(strcmp(res.msg, "no such function: plus") == 0);
    assert(query_int(db3, "select plus(1)") == 2);
    sqlite3* db4 = open_db(DB_PATH);
    res = exec(db4, "select plus(1)");
    assert(res.rc == SQLITE_ERROR);
    assert(strcmp(res.msg, "no such function: plus") == 0);

    // table functions are removed from all connections at once
    exec_ok(db1, "select undefine('half')");
    res = exec(db3, "select value from half(10)");
    assert(res.rc == SQLITE_ERROR);
    assert(strcmp(res.msg, "no such table: half") == 0);

    // the same name can be defined again in a connection that doesn't have it
    exec_ok(db4, "select define('plus', '? + 2')");
    assert(query_int(db4, "select plus(1)") == 3);
    assert(query_int(db3, "select plus(1)") == 2);

    close_db(db1);
    close_db(db2);
    close_db(db3);
    close_db(db4);
    remove_db();
    printf("OK\n");
}

static void test_undefine(void) {
    printf("test_undefine...");
    sqlite3* db = open_db(":memory:");
    exec_ok(db, "create table data(x)");
    exec_ok(db, "insert into data values (1), (2), (3)");
    exec_ok(db, "select define('plus', '? + 1')");
    exec_ok(db, "select define('plustwice', 'plus(plus(?))')");

    // a statement prepared before undefine fails when run after it
    sqlite3_stmt* stmt;
    assert(sqlite3_prepare_v2(db, "select sum(plus(x)) from data", -1, &stmt, NULL) == SQLITE_OK);
    assert(sqlite3_step(stmt) == SQLITE_ROW);
    assert(sqlite3_column_int(stmt, 0) == 9);
    sqlite3_reset(stmt);

    // names are case-insensitive
    exec_ok(db, "select undefine('PLUS')");
    assert(query_int(db, "select count(*) from sqlean_define where name = 'plus'") == 0);
    assert(sqlite3_step(stmt) == SQLITE_ERROR);
    assert(strcmp(sqlite3_errmsg(db), "no such function: plus") == 0);
    sqlite3_finalize(stmt);

    // so do functions calling an undefined function
    Result res = exec(db, "select plustwice(1)");
    assert(res.rc == SQLITE_ERROR);
    assert(strcmp(res.msg, "SQL logic error") == 0);  // the message is lost (current behavior)

    // the name can't be defined again in the same connection (current behavior)
    res = exec(db, "select define('plus', '? + 2')");
    assert(res.rc == SQLITE_BUSY);

    // undefine doesn't touch functions it didn't define
    res = exec(db, "select undefine('abs')");
    assert(res.rc == SQLITE_OK);
    assert(query_int(db, "select abs(-1)") == 1);

    close_db(db);
    printf("OK\n");
}

#pragma endregion

#pragma region Cache

static void test_cache(void) {
    printf("test_cache...");
    sqlite3* db = open_db(":memory:");
    exec_ok(db, "create table data(x)");
    exec_ok(db,
            "insert into data with recursive c(i) as (select 1 union all select i + 1 from c "
            "where i < 1000) select i from c");
    exec_ok(db, "select define('plus', '? + 1')");
    exec_ok(db, "select define('times2', '? * 2')");
    exec_ok(db, "select define('plustimes2', 'times2(plus(?))')");
    exec_ok(db, "select define('evalf', 'eval(?)')");
    exec_ok(db, "select define('absf', 'abs(?)')");

    // the body is compiled once per run of the calling statement,
    // and each function gets its own body
    assert(query_int(db, "select sum(plus(x)) from data") == 501500);
    assert(query_int(db, "select sum(plus(x) + times2(x) + plus(x)) from data") == 2004000);
    assert(query_int(db, "select sum(times2(plus(x))) from data") == 1003000);
    assert(query_int(db, "select sum(plustimes2(x)) from data") == 1003000);
    assert(query_int(db, "select sum(plus(x)) from data where times2(x) > 1000") == 375750);
    assert(
        query_int(
            db, "select plus(times2(x)) from data where x = (select max(plus(x)) - 1 from data)") ==
        2001);

    // a calling statement can be run again
    sqlite3_stmt* stmt;
    assert(sqlite3_prepare_v2(db, "select plus(?)", -1, &stmt, NULL) == SQLITE_OK);
    for (int i = 0; i < 3; i++) {
        sqlite3_bind_int(stmt, 1, i);
        assert(sqlite3_step(stmt) == SQLITE_ROW);
        assert(sqlite3_column_int(stmt, 0) == i + 1);
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);

    // the compiled bodies are statements of the connection, alive while
    // the calling statement runs
    assert(sqlite3_prepare_v2(db, "select plus(x), times2(x), plus(x) from data", -1, &stmt,
                              NULL) == SQLITE_OK);
    assert(count_stmts(db) == 1);
    assert(sqlite3_step(stmt) == SQLITE_ROW);
    assert(sqlite3_column_int(stmt, 0) == 2);
    assert(count_stmts(db) == 3);
    assert(sqlite3_step(stmt) == SQLITE_ROW);
    assert(count_stmts(db) == 3);
    sqlite3_reset(stmt);
    assert(count_stmts(db) == 1);

    // and freed when it's finalized without finishing
    assert(sqlite3_step(stmt) == SQLITE_ROW);
    assert(count_stmts(db) == 3);
    sqlite3_finalize(stmt);
    assert(count_stmts(db) == 0);

    // nested calls of the same function
    assert(query_int(db, "select evalf('select evalf(''select 42'')')") == 42);
    assert(query_int(db, "select sum(evalf('select ' || plus(x))) from data") == 501500);

    // in triggers
    exec_ok(db, "create table log(x)");
    exec_ok(db,
            "create trigger t_log after insert on log begin "
            "update log set x = plus(new.x) where rowid = new.rowid; end");
    exec_ok(db, "insert into log select x from data");
    assert(query_int(db, "select sum(x) from log") == 501500);

    // a runtime error stops the calling statement, but the function keeps working
    exec_ok(db, "insert into data values (-9223372036854775807 - 1)");
    Result res = exec(db, "select sum(absf(x)) from data");
    assert(res.rc == SQLITE_ERROR);
    assert(strcmp(res.msg, "SQL logic error") == 0);
    assert(query_int(db, "select absf(-42)") == 42);

    // define_free does nothing, and functions keep working after it
    exec_ok(db, "select define_free()");
    assert(query_int(db, "select plus(1)") == 2);
    exec_ok(db, "select define('minus', '? - 1')");
    assert(query_int(db, "select minus(1)") == 0);

    // the connection closes without define_free
    close_db(db);
    printf("OK\n");
}

static void test_cache_connections(void) {
    printf("test_cache_connections...");
    sqlite3* db1 = open_db(":memory:");
    sqlite3* db2 = open_db(":memory:");
    exec_ok(db1, "select define('f', '? + 1')");
    exec_ok(db2, "select define('f', '? + 2')");

    // functions with the same name in different connections are independent
    assert(query_int(db1, "select f(1)") == 2);
    assert(query_int(db2, "select f(1)") == 3);

    // define_free and close in one connection don't affect the others
    exec_ok(db1, "select define_free()");
    close_db(db1);
    assert(query_int(db2, "select f(1)") == 3);
    exec_ok(db2, "select define('g', '? * 2')");
    assert(query_int(db2, "select g(f(1))") == 6);
    close_db(db2);
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
    test_undefine();

    // Cache
    test_cache();
    test_cache_connections();
}
