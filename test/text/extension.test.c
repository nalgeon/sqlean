// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Extension tests.

// These run the extension in a real connection (statically linked SQLite,
// see the Makefile) and cover what the SQL tests can't: errors.

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "sqlite3.h"
#include "text/extension.h"

#define LEN(a) (sizeof(a) / sizeof(a[0]))

typedef struct {
    const char* sql;
    const char* msg;
} ErrorTest;

static void test_errors(void) {
    printf("test_errors...");
    static const ErrorTest tests[] = {
        {"select text_join('|')", "wrong number of arguments to function text_join()"},
        {"select text_join()", "wrong number of arguments to function text_join()"},
        {"select text_concat()", "wrong number of arguments to function text_concat()"},
        // fixed arities fail at prepare time
        {"select text_ltrim()", "wrong number of arguments to function text_ltrim()"},
        {"select text_lpad('a')", "wrong number of arguments to function text_lpad()"},
        {"select text_substring('a', 1, 2, 3)",
         "wrong number of arguments to function text_substring()"},
        // results longer than SQLITE_LIMIT_LENGTH (set to 1000 below)
        {"select text_repeat('a', 1001)", "string or blob too big"},
        {"select text_repeat('ab', 9223372036854775807)", "string or blob too big"},
        {"select text_lpad('a', 1001)", "string or blob too big"},
        {"select text_rpad('a', 9223372036854775807, 'xy')", "string or blob too big"},
        {"select text_replace(text_repeat('a', 500), 'a', 'bbb')", "string or blob too big"},
        {"select text_join(text_repeat('a', 500), 'x', 'y', 'z')", "string or blob too big"},
        {"select text_concat(text_repeat('a', 600), text_repeat('a', 600))",
         "string or blob too big"},
        {"select text_translate(text_repeat('a', 600), 'a', 'ж')", "string or blob too big"},
        {"select text_upper(text_repeat('ɐ', 400))", "string or blob too big"},
    };

    sqlite3* db;
    int rc = sqlite3_open(":memory:", &db);
    assert(rc == SQLITE_OK);
    rc = text_init(db);
    assert(rc == SQLITE_OK);
    sqlite3_limit(db, SQLITE_LIMIT_LENGTH, 1000);

    for (size_t i = 0; i < LEN(tests); i++) {
        sqlite3_stmt* stmt;
        rc = sqlite3_prepare_v2(db, tests[i].sql, -1, &stmt, NULL);
        if (rc == SQLITE_OK) {
            while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            }
        }
        if ((rc != SQLITE_ERROR && rc != SQLITE_TOOBIG) ||
            strcmp(sqlite3_errmsg(db), tests[i].msg) != 0) {
            printf("%s: rc = %d, %s\n", tests[i].sql, rc, sqlite3_errmsg(db));
        }
        assert(rc == SQLITE_ERROR || rc == SQLITE_TOOBIG);
        assert(strcmp(sqlite3_errmsg(db), tests[i].msg) == 0);
        sqlite3_finalize(stmt);
    }

    rc = sqlite3_close(db);
    assert(rc == SQLITE_OK);
    printf("OK\n");
}

typedef struct {
    const char* sql;
    const char* want;
} ResultTest;

// test_limit checks that results up to SQLITE_LIMIT_LENGTH still work.
static void test_limit(void) {
    printf("test_limit...");
    static const ResultTest tests[] = {
        {"select length(text_repeat('a', 1000))", "1000"},
        {"select length(text_lpad('a', 1000, 'xy'))", "1000"},
        {"select text_repeat('', 9223372036854775807)", ""},
        {"select text_lpad('a', 9223372036854775807, '')", "a"},
    };

    sqlite3* db;
    int rc = sqlite3_open(":memory:", &db);
    assert(rc == SQLITE_OK);
    rc = text_init(db);
    assert(rc == SQLITE_OK);
    sqlite3_limit(db, SQLITE_LIMIT_LENGTH, 1000);

    for (size_t i = 0; i < LEN(tests); i++) {
        sqlite3_stmt* stmt;
        rc = sqlite3_prepare_v2(db, tests[i].sql, -1, &stmt, NULL);
        assert(rc == SQLITE_OK);
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_ROW) {
            printf("%s: rc = %d, %s\n", tests[i].sql, rc, sqlite3_errmsg(db));
        }
        assert(rc == SQLITE_ROW);
        assert(strcmp((const char*)sqlite3_column_text(stmt, 0), tests[i].want) == 0);
        sqlite3_finalize(stmt);
    }

    rc = sqlite3_close(db);
    assert(rc == SQLITE_OK);
    printf("OK\n");
}

int main(void) {
    test_errors();
    test_limit();
    return 0;
}
