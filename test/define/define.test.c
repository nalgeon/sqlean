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
#include <stdbool.h>
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
    if (rc == SQLITE_OK) {
        rc = sqlite3_step(stmt);
    }
    if (rc != SQLITE_ROW) {
        printf("%s: %s\n", sql, sqlite3_errmsg(db));
    }
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

// twice is a C function that stands for a function from another extension.
static void twice(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    sqlite3_result_int64(ctx, sqlite3_value_int64(argv[0]) * 2);
}

static void test_load(void) {
    printf("test_load...");
    remove_db();
    sqlite3* db = open_db(DB_PATH);
    exec_ok(db,
            "insert into sqlean_define(name, type, body) values "
            "('broken', 'scalar', 'nosuch(?)'), "
            "('syntax', 'scalar', 'from'), "
            "('callsnext', 'scalar', 'nextone(?) + 1'), "
            "('nextone', 'scalar', '? * 2'), "
            "('external', 'scalar', 'twice(?) + 1'), "
            "('later', 'scalar', '? * 10'), "
            "(null, 'scalar', '1'), "
            "('nobody', 'scalar', null)");
    close_db(db);

    // functions that don't compile don't stop loading the rest
    db = open_db(DB_PATH);
    assert(query_int(db, "select later(2)") == 20);
    assert(query_int(db, "select nextone(5)") == 10);

    // a function that calls one loaded after it works
    assert(query_int(db, "select callsnext(5)") == 11);
    // but accepts any number of arguments in SQL, and checks the number when called
    Result res = exec(db, "select callsnext(1, 2)");
    assert(res.rc == SQLITE_ERROR);
    assert(strcmp(res.msg, "wrong number of arguments to function callsnext()") == 0);

    // calling a function that doesn't compile reports why
    res = exec(db, "select broken(1)");
    assert(res.rc == SQLITE_ERROR);
    assert(strcmp(res.msg, "no such function: nosuch") == 0);
    res = exec(db, "select syntax()");
    assert(res.rc == SQLITE_ERROR);
    assert(strcmp(res.msg, "near \"from\": syntax error") == 0);
    res = exec(db, "select nobody()");
    assert(strcmp(res.msg, "no such function: nobody") == 0);

    // a function from an extension loaded later works once it's there
    res = exec(db, "select external(1)");
    assert(strcmp(res.msg, "no such function: twice") == 0);
    sqlite3_create_function(db, "twice", 1, SQLITE_UTF8, NULL, twice, NULL, NULL);
    assert(query_int(db, "select external(1)") == 3);

    // a broken function can be fixed and deleted as usual
    exec_ok(db, "select define('broken', '? + 1')");
    assert(query_int(db, "select broken(1)") == 2);
    res = exec(db, "select broken()");
    assert(strcmp(res.msg, "wrong number of arguments to function broken()") == 0);
    exec_ok(db, "select undefine('syntax')");
    res = exec(db, "select syntax()");
    assert(strcmp(res.msg, "no such function: syntax") == 0);

    close_db(db);
    remove_db();
    printf("OK\n");
}

static void test_load_tables(void) {
    printf("test_load_tables...");
    remove_db();
    sqlite3* db = open_db(DB_PATH);
    exec_ok(db, "create virtual table half using define((select :x / 2 as value))");
    // a table function saved by an older version, with several statements
    exec_ok(db, "pragma writable_schema = on");
    exec_ok(db,
            "insert into sqlite_master(type, name, tbl_name, rootpage, sql) values "
            "('table', 'oldmulti', 'oldmulti', 0, "
            "'CREATE VIRTUAL TABLE oldmulti USING define((select 1 as a; select 2 as b))')");
    exec_ok(db,
            "insert into sqlean_define(name, type, body) values "
            "('oldmulti', 'table', '(select 1 as a; select 2 as b)')");
    close_db(db);

    // connects to it, ignoring the statements after the first one
    db = open_db(DB_PATH);
    assert(query_int(db, "select a from oldmulti") == 1);
    assert(query_int(db, "select count(*) from pragma_table_info('oldmulti')") == 1);
    close_db(db);

    // table functions work in a read-only database
    sqlite3_open_v2(DB_PATH, &db, SQLITE_OPEN_READONLY, NULL);
    assert(define_init(db) == SQLITE_OK);
    assert(query_int(db, "select value from half(10)") == 5);
    assert(query_int(db, "select a from oldmulti") == 1);
    // but can't be created
    Result res = exec(db, "create virtual table third using define((select :x / 3 as value))");
    assert(res.rc == SQLITE_READONLY);
    close_db(db);

    remove_db();
    printf("OK\n");
}

#pragma endregion

#pragma region Errors

static const ErrorTest define_errors[] = {
    {"select define(null, '1')", SQLITE_ERROR, "function name is NULL"},
    {"select define('f', null)", SQLITE_ERROR, "function body is NULL"},
    {"select define('f', '')", SQLITE_ERROR, "incomplete input"},
    {"select define('f', 'from')", SQLITE_ERROR, "near \"from\": syntax error"},
    {"select define('f', 'nosuch(?)')", SQLITE_ERROR, "no such function: nosuch"},
    {"select define('f', '1; select 2')", SQLITE_ERROR, "function body must be a single statement"},
    {"select define('f', '1; delete from data')", SQLITE_ERROR,
     "function body must be a single statement"},
    {"select define('f', '1; garbage')", SQLITE_ERROR, "function body must be a single statement"},
    {"select define('abs', 'abs(?)')", SQLITE_ERROR, "function already exists: abs"},
    {"select define('ABS', '1')", SQLITE_ERROR, "function already exists: ABS"},
    {"select define('half', '1')", SQLITE_ERROR, "table function already exists: half"},
    {"select overflow()", SQLITE_ERROR, "integer overflow"},  // runtime error in the body
    {"select answer(1)", SQLITE_ERROR, "wrong number of arguments to function answer()"},
};

static void test_define_errors(void) {
    printf("test_define_errors...");
    sqlite3* db = open_db(":memory:");
    exec_ok(db, "create table data(x)");
    exec_ok(db, "select define('answer', '42')");
    exec_ok(db, "select define('overflow', 'abs(-9223372036854775808)')");
    exec_ok(db, "create virtual table half using define((select :x / 2 as value))");
    check_errors(db, define_errors, sizeof(define_errors) / sizeof(define_errors[0]));
    // failed definitions are not saved
    assert(query_int(db, "select count(*) from sqlean_define") == 3);
    assert(query_int(db, "select answer()") == 42);
    assert(query_int(db, "select abs(-1)") == 1);
    // a body that returns no rows gives NULL, like a scalar subquery
    exec_ok(db, "select define('zero', '1 where 0')");
    assert(query_int(db, "select zero() is null") == 1);
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
    {"create virtual table t using define((select 1 as a; select 2 as b))", SQLITE_ERROR,
     "function body must be a single statement"},
    // parameters accept only equality constraints
    {"select * from half where x > 1", SQLITE_ERROR, "no query solution"},
    // a runtime error in the body
    {"select * from overflow", SQLITE_ERROR, "integer overflow"},
    // a table the body uses is gone
    {"select * from fromsrc", SQLITE_ERROR, "no such table: src"},
    {"select * from fromsrc(1)", SQLITE_ERROR, "no such table: src"},
};

static void test_table_errors(void) {
    printf("test_table_errors...");
    sqlite3* db = open_db(":memory:");
    exec_ok(db, "create table data(x)");
    exec_ok(db, "create virtual table half using define((select :x / 2 as value))");
    exec_ok(db,
            "create virtual table overflow using define((select abs(-9223372036854775808) as v))");
    exec_ok(db, "create table src(x)");
    exec_ok(db, "create virtual table fromsrc using define((select x + ? as y from src))");
    exec_ok(db, "drop table src");
    check_errors(db, table_errors, sizeof(table_errors) / sizeof(table_errors[0]));
    // failed definitions are not saved
    assert(query_int(db, "select count(*) from sqlean_define") == 3);
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
    assert(strcmp(res.msg, "no such function: plus") == 0);

    // undefine doesn't touch functions it didn't define
    res = exec(db, "select undefine('abs')");
    assert(res.rc == SQLITE_OK);
    assert(query_int(db, "select abs(-1)") == 1);

    close_db(db);
    printf("OK\n");
}

static void test_redefine(void) {
    printf("test_redefine...");
    remove_db();
    sqlite3* db = open_db(DB_PATH);
    exec_ok(db, "create table data(x)");
    exec_ok(db, "insert into data values (1), (2), (3)");
    exec_ok(db, "select define('plus', '? + 1')");
    exec_ok(db, "select define('plustwice', 'plus(plus(?))')");
    sqlite3* other = open_db(DB_PATH);

    // same number of parameters: the body changes,
    // including in statements prepared before
    sqlite3_stmt* stmt;
    assert(sqlite3_prepare_v2(db, "select sum(plus(x)) from data", -1, &stmt, NULL) == SQLITE_OK);
    exec_ok(db, "select define('Plus', '? + 2')");
    assert(sqlite3_step(stmt) == SQLITE_ROW);
    assert(sqlite3_column_int(stmt, 0) == 12);
    sqlite3_finalize(stmt);
    assert(query_int(db, "select plustwice(1)") == 5);
    assert(query_int(db, "select count(*) from sqlean_define where name like 'plus'") == 1);
    assert(query_int(db, "select count(*) from sqlean_define where body = '? + 2'") == 1);

    // in the middle of a query (the first row is computed with the old body)
    assert(query_int(db,
                     "select sum(plus(x) + iif(x = 1, coalesce(define('plus', '? + 3'), 0), 0)) "
                     "from data") == 3 + 5 + 6);
    assert(query_int(db, "select plus(1)") == 4);

    // by its own body
    exec_ok(db, "select define('selfdef', 'coalesce(define(''selfdef'', ''2''), 1)')");
    assert(query_int(db, "select selfdef() + selfdef()") == 3);
    assert(query_int(db, "select selfdef()") == 2);

    // a different number of parameters: the old function stops working
    exec_ok(db, "select define('plus', '? + ?')");
    assert(query_int(db, "select plus(1, 2)") == 3);
    Result res = exec(db, "select plus(1)");
    assert(res.rc == SQLITE_ERROR);
    assert(strcmp(res.msg, "wrong number of arguments to function plus()") == 0);

    // and back
    exec_ok(db, "select define('plus', '? + 10')");
    assert(query_int(db, "select plus(1)") == 11);
    res = exec(db, "select plus(1, 2)");
    assert(strcmp(res.msg, "wrong number of arguments to function plus()") == 0);

    // other connections keep the old body until reconnect
    assert(query_int(other, "select plus(1)") == 2);
    close_db(other);
    other = open_db(DB_PATH);
    assert(query_int(other, "select plus(1)") == 11);
    assert(query_int(other, "select plustwice(1)") == 21);
    close_db(other);

    // after undefine
    exec_ok(db, "select undefine('plus')");
    exec_ok(db, "select define('plus', '? + 20')");
    assert(query_int(db, "select plus(1)") == 21);

    // plus is saved after plustwice now, and both still load
    other = open_db(DB_PATH);
    assert(query_int(other, "select plustwice(1)") == 41);
    close_db(other);

    // a failed redefinition changes nothing
    res = exec(db, "select define('plus', 'nosuch(?)')");
    assert(res.rc == SQLITE_ERROR);
    assert(query_int(db, "select plus(1)") == 21);

    close_db(db);
    remove_db();
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
    assert(strcmp(res.msg, "integer overflow") == 0);
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

#pragma region Out of memory

// The allocator fails every allocation after the first fail_after ones
// (if fail_after is not negative).
static sqlite3_mem_methods default_mem;
static int fail_after = -1;

static bool should_fail(void) {
    if (fail_after < 0) {
        return false;
    }
    if (fail_after == 0) {
        return true;
    }
    fail_after--;
    return false;
}

static void* failing_malloc(int n) {
    return should_fail() ? NULL : default_mem.xMalloc(n);
}

static void* failing_realloc(void* p, int n) {
    return should_fail() ? NULL : default_mem.xRealloc(p, n);
}

// Each case runs after the setup statements, in a new connection.
static const char* nomem_setup[] = {
    "create table data(x)",
    "insert into data values (1), (2), (3)",
    "select define('plus', '? + 1')",
    "create virtual table half using define((select :x / 2 as value))",
};

static const char* nomem_cases[] = {
    "select define('minus', '? - 1')",
    "select define('plus', '? + 2')",
    "select define('plus', '? + ?')",
    "select define('abs', '1')",
    "select define('f', '1; select 2')",
    "select sum(plus(x)) from data",
    "select undefine('plus')",
    "select undefine('half')",
    "create virtual table third using define((select :x / 3 as value))",
    "select sum(value) from data, half(data.x)",
    "select eval('select 1, 2; select 3')",
};

static void test_nomem(void) {
    printf("test_nomem...");
    sqlite3_shutdown();
    sqlite3_config(SQLITE_CONFIG_GETMALLOC, &default_mem);
    sqlite3_mem_methods mem = default_mem;
    mem.xMalloc = failing_malloc;
    mem.xRealloc = failing_realloc;
    sqlite3_config(SQLITE_CONFIG_MALLOC, &mem);
    sqlite3_initialize();

    for (size_t i = 0; i < sizeof(nomem_cases) / sizeof(nomem_cases[0]); i++) {
        // fail the n-th allocation of the case, until it runs without failing
        int want = SQLITE_OK;
        for (int n = -1;; n++) {
            sqlite3* db = open_db(":memory:");
            for (size_t j = 0; j < sizeof(nomem_setup) / sizeof(nomem_setup[0]); j++) {
                exec_ok(db, nomem_setup[j]);
            }
            fail_after = n;
            Result res = exec(db, nomem_cases[i]);
            bool failed = fail_after == 0;
            fail_after = -1;
            close_db(db);
            if (n < 0) {
                // without failures
                want = res.rc;
                continue;
            }
            // if an allocation failed, the statement reports it or recovers
            if (res.rc != want && res.rc != SQLITE_NOMEM) {
                printf("%s, allocation %d: %d %s\n", nomem_cases[i], n, res.rc, res.msg);
            }
            assert(res.rc == want || res.rc == SQLITE_NOMEM);
            if (!failed) {
                break;
            }
        }
    }

    sqlite3_shutdown();
    sqlite3_config(SQLITE_CONFIG_MALLOC, &default_mem);
    sqlite3_initialize();
    printf("OK\n");
}

#pragma endregion

int main(void) {
    // Init
    test_init();
    test_load();
    test_load_tables();

    // Errors
    test_define_errors();
    test_table_errors();
    test_eval_errors();

    // Flags
    test_flags();

    // Connections
    test_connections();
    test_undefine();
    test_redefine();

    // Cache
    test_cache();
    test_cache_connections();

    // Out of memory
    test_nomem();
}
