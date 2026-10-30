// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Extension tests.

// These run the extension in a real connection (statically linked SQLite,
// see the Makefile) and cover what the SQL tests can't: errors.

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "sqlite3.h"
#include "vsv/extension.h"

#define LEN(a) (sizeof(a) / sizeof(a[0]))

#define FILE_PATH "vsv.test.csv"

typedef struct {
    const char* sql;
    int rc;
    const char* msg;
} ErrorTest;

static sqlite3* open_db(void) {
    sqlite3* db;
    int rc = sqlite3_open(":memory:", &db);
    assert(rc == SQLITE_OK);
    rc = vsv_init(db);
    assert(rc == SQLITE_OK);
    return db;
}

static void close_db(sqlite3* db) {
    int rc = sqlite3_close(db);
    if (rc != SQLITE_OK) {
        printf("sqlite3_close: %s\n", sqlite3_errmsg(db));
    }
    assert(rc == SQLITE_OK);
}

// exec runs the statements in sql to completion, and returns the result code
// of the first one that fails (or SQLITE_OK).
static int exec(sqlite3* db, const char* sql) {
    while (sql[0] != '\0') {
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, &sql);
        if (rc == SQLITE_OK && stmt != NULL) {
            while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            }
            rc = rc == SQLITE_DONE ? SQLITE_OK : rc;
        }
        sqlite3_finalize(stmt);
        if (rc != SQLITE_OK) {
            return rc;
        }
    }
    return SQLITE_OK;
}

static void exec_ok(sqlite3* db, const char* sql) {
    int rc = exec(db, sql);
    if (rc != SQLITE_OK) {
        printf("%s: %s\n", sql, sqlite3_errmsg(db));
    }
    assert(rc == SQLITE_OK);
}

// check_errors runs each statement and checks its result code and message.
// The vsv table is dropped after each one.
static void check_errors(sqlite3* db, const ErrorTest* tests, size_t n) {
    for (size_t i = 0; i < n; i++) {
        int rc = exec(db, tests[i].sql);
        const char* msg = sqlite3_errmsg(db);
        if (rc != tests[i].rc || strcmp(msg, tests[i].msg) != 0) {
            printf("%s: rc = %d, %s\n", tests[i].sql, rc, msg);
        }
        assert(rc == tests[i].rc);
        assert(strcmp(msg, tests[i].msg) == 0);
        exec_ok(db, "drop table if exists temp.t");
    }
}

static void write_file(const char* path, const char* data) {
    FILE* f = fopen(path, "wb");
    assert(f != NULL);
    fputs(data, f);
    fclose(f);
}

static void test_params(void) {
    printf("test_params...");
    static const ErrorTest tests[] = {
        // the input
        {"create virtual table temp.t using vsv()", SQLITE_ERROR,
         "must specify either filename or data, but not both"},
        {"create virtual table temp.t using vsv(header)", SQLITE_ERROR,
         "must specify either filename or data, but not both"},
        {"create virtual table temp.t using vsv(filename=x.csv, data='a')", SQLITE_ERROR,
         "must specify either filename or data, but not both"},
        {"create virtual table temp.t using vsv(filename=nosuch.csv)", SQLITE_ERROR,
         "cannot open file: 'nosuch.csv'"},
        {"create virtual table temp.t using vsv(filename=nosuch.csv, schema='create table x(a)')",
         SQLITE_ERROR, "cannot open file: 'nosuch.csv'"},
        {"create virtual table temp.t using vsv(filename='')", SQLITE_ERROR,
         "cannot open file: ''"},
        // names
        {"create virtual table temp.t using vsv(data='a', nosuch=1)", SQLITE_ERROR,
         "unknown parameter: nosuch"},
        {"create virtual table temp.t using vsv(data='a', nosuch)", SQLITE_ERROR,
         "unknown parameter: nosuch"},
        {"create virtual table temp.t using vsv(data='a', =1)", SQLITE_ERROR,
         "unknown parameter: "},
        {"create virtual table temp.t using vsv(data='a', file=x.csv)", SQLITE_ERROR,
         "unknown parameter: file"},
        {"create virtual table temp.t using vsv(data='a', rsep=';')", SQLITE_ERROR,
         "unsupported parameter: rsep"},
        {"create virtual table temp.t using vsv(data='a', dsep=',')", SQLITE_ERROR,
         "unsupported parameter: dsep"},
        {"create virtual table temp.t using vsv(data='a', nulls)", SQLITE_ERROR,
         "unsupported parameter: nulls"},
        {"create virtual table temp.t using vsv(data='a', skip=1)", SQLITE_ERROR,
         "unsupported parameter: skip"},
        {"create virtual table temp.t using vsv(data='a', validatetext)", SQLITE_ERROR,
         "unsupported parameter: validatetext"},
        {"create virtual table temp.t using vsv(data='a', AFFINITY=integer)", SQLITE_ERROR,
         "unsupported parameter: affinity"},
        // values
        {"create virtual table temp.t using vsv(data)", SQLITE_ERROR, "missing value for data"},
        {"create virtual table temp.t using vsv(Schema, data='a')", SQLITE_ERROR,
         "missing value for Schema"},
        {"create virtual table temp.t using vsv(data='a', data='b')", SQLITE_ERROR,
         "more than one data parameter"},
        {"create virtual table temp.t using vsv(data='a', fsep=';', fsep=';')", SQLITE_ERROR,
         "more than one fsep parameter"},
        {"create virtual table temp.t using vsv(data='a', header=maybe)", SQLITE_ERROR,
         "invalid value for header: 'maybe'"},
        {"create virtual table temp.t using vsv(data='a', header=)", SQLITE_ERROR,
         "invalid value for header: ''"},
        {"create virtual table temp.t using vsv(data='a', header=2)", SQLITE_ERROR,
         "invalid value for header: '2'"},
        // fsep is a single character other than NUL, ", \r, \n and U+FFFD
        {"create virtual table temp.t using vsv(data='a', fsep='')", SQLITE_ERROR,
         "invalid value for fsep: ''"},
        {"create virtual table temp.t using vsv(data='a', fsep=';;')", SQLITE_ERROR,
         "invalid value for fsep: ';;'"},
        {"create virtual table temp.t using vsv(data='a', fsep='\"')", SQLITE_ERROR,
         "invalid value for fsep: '\"'"},
        {"create virtual table temp.t using vsv(data='a', fsep='\n')", SQLITE_ERROR,
         "invalid value for fsep: '\n'"},
        {"create virtual table temp.t using vsv(data='a', fsep='\xef\xbf\xbd')", SQLITE_ERROR,
         "invalid value for fsep: '\xef\xbf\xbd'"},
        {"create virtual table temp.t using vsv(data='a', fsep='\xff')", SQLITE_ERROR,
         "invalid value for fsep: '\xff'"},
        // the escapes are \t and \xHH for ASCII, with a lowercase x
        {"create virtual table temp.t using vsv(data='a', fsep='\\n')", SQLITE_ERROR,
         "invalid value for fsep: '\\n'"},
        {"create virtual table temp.t using vsv(data='a', fsep='\\X7C')", SQLITE_ERROR,
         "invalid value for fsep: '\\X7C'"},
        {"create virtual table temp.t using vsv(data='a', fsep='\\x7')", SQLITE_ERROR,
         "invalid value for fsep: '\\x7'"},
        {"create virtual table temp.t using vsv(data='a', fsep='\\xzz')", SQLITE_ERROR,
         "invalid value for fsep: '\\xzz'"},
        {"create virtual table temp.t using vsv(data='a', fsep='\\x80')", SQLITE_ERROR,
         "invalid value for fsep: '\\x80'"},
        {"create virtual table temp.t using vsv(data='a', fsep='\\x00')", SQLITE_ERROR,
         "invalid value for fsep: '\\x00'"},
        {"create virtual table temp.t using vsv(data='a', fsep='\\x0a')", SQLITE_ERROR,
         "invalid value for fsep: '\\x0a'"},
        {"create virtual table temp.t using vsv(data='a', fsep='\\x22')", SQLITE_ERROR,
         "invalid value for fsep: '\\x22'"},
    };
    sqlite3* db = open_db();
    check_errors(db, tests, LEN(tests));
    close_db(db);
    printf("OK\n");
}

static void test_schema(void) {
    printf("test_schema...");
    static const ErrorTest tests[] = {
        {"create virtual table temp.t using vsv(data='a', schema='create table x(')", SQLITE_ERROR,
         "bad schema: 'create table x(' - incomplete input"},
        {"create virtual table temp.t using vsv(data='a', schema='select 1')", SQLITE_ERROR,
         "bad schema: 'select 1' - syntax error"},
        {"create virtual table temp.t using vsv(data='a', schema='')", SQLITE_ERROR,
         "bad schema: '' - syntax error"},
        {"create virtual table temp.t using vsv(data='a', schema='create table x(a, a)')",
         SQLITE_ERROR, "bad schema: 'create table x(a, a)' - duplicate column name: a"},
        {"create virtual table temp.t using vsv(data='a', schema='create temp table x(a)')",
         SQLITE_ERROR, "bad schema: 'create temp table x(a)' - syntax error"},
        {"create virtual table temp.t using vsv(data='a', schema='create table x(a) without "
         "rowid')",
         SQLITE_ERROR,
         "bad schema: 'create table x(a) without rowid' - PRIMARY KEY missing on table x"},
        // a header with duplicate names is a bad schema too
        {"create virtual table temp.t using vsv(data='a,b,a', header)", SQLITE_ERROR,
         "bad schema: 'create table x(\"a\",\"b\",\"a\")' - duplicate column name: a"},
    };
    sqlite3* db = open_db();
    check_errors(db, tests, LEN(tests));
    close_db(db);
    printf("OK\n");
}

static void test_records(void) {
    printf("test_records...");
    static const ErrorTest tests[] = {
        // the first record is read at create time, unless there's a schema
        {"create virtual table temp.t using vsv(data='a\"b')", SQLITE_ERROR,
         "parse error on line 1, column 2: bare \" in non-quoted-field"},
        {"create virtual table temp.t using vsv(data='\"a\"b', header)", SQLITE_ERROR,
         "parse error on line 1, column 3: extraneous or missing \" in quoted-field"},
        {"create virtual table temp.t using vsv(data='\"a\n\nb')", SQLITE_ERROR,
         "record on line 1; parse error on line 3, column 2: "
         "extraneous or missing \" in quoted-field"},
        // other records are read by queries
        {"create virtual table temp.t using vsv(data='a\"b', schema='create table x(a)');"
         "select * from t",
         SQLITE_ERROR, "parse error on line 1, column 2: bare \" in non-quoted-field"},
        {"create virtual table temp.t using vsv(data='a,b\nc,d\n\ne,f\"');"
         "select count(*) from t",
         SQLITE_ERROR, "parse error on line 4, column 4: bare \" in non-quoted-field"},
        {"create virtual table temp.t using vsv(data='a\n\"b\n\"c\"');"
         "select * from t",
         SQLITE_ERROR,
         "record on line 2; parse error on line 3, column 1: "
         "extraneous or missing \" in quoted-field"},
        // the header too
        {"create virtual table temp.t using vsv(data='a\"', schema='create table x(a)', header);"
         "select * from t",
         SQLITE_ERROR, "parse error on line 1, column 2: bare \" in non-quoted-field"},
    };
    sqlite3* db = open_db();
    check_errors(db, tests, LEN(tests));

    // the error is the same on every scan, and the rows before it are read
    exec_ok(db, "create virtual table temp.t using vsv(data='a\nb\nc\"')");
    exec_ok(db, "create table rows(x)");
    for (int i = 0; i < 2; i++) {
        int rc = exec(db, "insert into rows select c0 from t");
        assert(rc == SQLITE_ERROR);
        assert(strcmp(sqlite3_errmsg(db),
                      "parse error on line 3, column 2: bare \" in non-quoted-field") == 0);
    }
    // the statement fails as a whole
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, "select count(*) from rows", -1, &stmt, NULL);
    assert(rc == SQLITE_OK);
    assert(sqlite3_step(stmt) == SQLITE_ROW);
    assert(sqlite3_column_int(stmt, 0) == 0);
    sqlite3_finalize(stmt);
    // and the rows before it are returned one by one
    rc = sqlite3_prepare_v2(db, "select c0 from t", -1, &stmt, NULL);
    assert(rc == SQLITE_OK);
    assert(sqlite3_step(stmt) == SQLITE_ROW);
    assert(strcmp((const char*)sqlite3_column_text(stmt, 0), "a") == 0);
    assert(sqlite3_step(stmt) == SQLITE_ROW);
    assert(strcmp((const char*)sqlite3_column_text(stmt, 0), "b") == 0);
    assert(sqlite3_step(stmt) == SQLITE_ERROR);
    sqlite3_finalize(stmt);
    exec_ok(db, "drop table t; drop table rows");

    close_db(db);
    printf("OK\n");
}

static void test_file(void) {
    printf("test_file...");
    sqlite3* db = open_db();
    write_file(FILE_PATH, "a,b\n1,2\n");
    exec_ok(db, "create virtual table temp.t using vsv(filename=" FILE_PATH ", header)");

    // the file is read on every scan
    write_file(FILE_PATH, "a,b\n1,2\n3,4\n");
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, "select count(*) from t", -1, &stmt, NULL);
    assert(rc == SQLITE_OK);
    assert(sqlite3_step(stmt) == SQLITE_ROW);
    assert(sqlite3_column_int(stmt, 0) == 2);
    sqlite3_finalize(stmt);

    // a file removed after the table is created is an error when queried
    remove(FILE_PATH);
    rc = exec(db, "select * from t");
    assert(rc == SQLITE_ERROR);
    assert(strcmp(sqlite3_errmsg(db), "cannot open file: '" FILE_PATH "'") == 0);

    exec_ok(db, "drop table t");
    close_db(db);
    printf("OK\n");
}

// repeat writes n copies of c to buf (with room for n + 1 bytes) and returns it.
static const char* repeat(char* buf, char c, size_t n) {
    memset(buf, c, n);
    buf[n] = '\0';
    return buf;
}

// exec_file writes data to the test file and runs sql.
static int exec_file(sqlite3* db, const char* data, const char* sql) {
    write_file(FILE_PATH, data);
    return exec(db, sql);
}

static void test_limit(void) {
    printf("test_limit...");
    // A record larger than SQLITE_LIMIT_LENGTH is an error. The data comes
    // from a file, since data= that long would make the create statement
    // too long to store in the schema.
    sqlite3* db = open_db();
    // SQLite builds statements to update the schema under the same limit,
    // so it can't be much lower.
    sqlite3_limit(db, SQLITE_LIMIT_LENGTH, 300);
    char data[1024];
    char field[512];

    // the first record is read at create time
    snprintf(data, sizeof(data), "%s\n", repeat(field, 'a', 301));
    int rc = exec_file(db, data, "create virtual table temp.t using vsv(filename=" FILE_PATH ")");
    assert(rc == SQLITE_TOOBIG);
    assert(strcmp(sqlite3_errmsg(db), "string or blob too big") == 0);

    // the others when queried
    snprintf(data, sizeof(data), "a\n%s,%s\n", repeat(field, 'b', 160), field);
    rc = exec_file(db, data,
                   "create virtual table temp.t using vsv(filename=" FILE_PATH
                   ");"
                   "select * from t");
    assert(rc == SQLITE_TOOBIG);
    assert(strcmp(sqlite3_errmsg(db), "string or blob too big") == 0);
    exec_ok(db, "drop table t");

    // a quoted field over several lines
    snprintf(data, sizeof(data), "\"%s\n%s\"\n", repeat(field, 'c', 160), field);
    rc = exec_file(db, data, "create virtual table temp.t using vsv(filename=" FILE_PATH ")");
    assert(rc == SQLITE_TOOBIG);
    assert(strcmp(sqlite3_errmsg(db), "string or blob too big") == 0);

    // a record within the limit is fine, with or without conversion
    snprintf(data, sizeof(data), "%s,%s\n", repeat(field, '1', 140), field);
    rc = exec_file(db, data,
                   "create virtual table temp.t using vsv("
                   "filename=" FILE_PATH
                   ", schema='create table x(a int, b)');"
                   "select * from t");
    assert(rc == SQLITE_OK);
    exec_ok(db, "drop table t");

    remove(FILE_PATH);
    close_db(db);
    printf("OK\n");
}

static void test_directonly(void) {
    printf("test_directonly...");
    sqlite3* db = open_db();
    // vsv tables can't be used from the schema, so a database file
    // can't make the user read files they didn't ask for
    exec_ok(db,
            "create virtual table main.t using vsv(data='a');"
            "create view main.v as select * from t");
    int rc = exec(db, "select * from v");
    assert(rc == SQLITE_ERROR);
    assert(strcmp(sqlite3_errmsg(db), "unsafe use of virtual table \"t\"") == 0);
    // but can be used from temp views
    exec_ok(db, "create temp view w as select * from t; select * from w");
    exec_ok(db, "drop view w; drop view v; drop table t");
    close_db(db);
    printf("OK\n");
}

int main(void) {
    test_params();
    test_schema();
    test_records();
    test_file();
    test_limit();
    test_directonly();
    return 0;
}
