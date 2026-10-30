// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Extension tests.

// These run the extension in a real connection (statically linked SQLite,
// see the Makefile) and cover what the SQL tests can't: errors.

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "sqlite3.h"
#include "stats/extension.h"

#define LEN(a) (sizeof(a) / sizeof(a[0]))

typedef struct {
    const char* sql;
    const char* msg;
} ErrorTest;

static void test_errors(void) {
    printf("test_errors...");
    static const ErrorTest tests[] = {
        {"select stats_perc(1, 101)", "percentile should be a number between 0 and 100"},
        {"select stats_perc(1, -1)", "percentile should be a number between 0 and 100"},
        {"select stats_perc(1, 'abc')", "percentile should be a number between 0 and 100"},
        {"select stats_perc(1, x'01')", "percentile should be a number between 0 and 100"},
        {"select percentile(1, 9e999)", "percentile should be a number between 0 and 100"},
        // the error is raised after rows were added, and the sample is freed
        {"select stats_perc(value, iif(value < 100, 50, 200)) from stats_seq(1, 100)",
         "percentile should be a number between 0 and 100"},
        {"select stats_perc(value, value) from stats_seq(1, 100)",
         "percentile should be the same for all rows"},
        {"select stats_perc(value, value) over (rows 1 preceding) from stats_seq(1, 3)",
         "percentile should be the same for all rows"},
        {"select * from stats_seq", "table function stats_seq requires an argument"},
        {"select * from generate_series where stop = 3",
         "table function generate_series requires an argument"},
    };

    sqlite3* db;
    int rc = sqlite3_open(":memory:", &db);
    assert(rc == SQLITE_OK);
    rc = stats_init(db);
    assert(rc == SQLITE_OK);

    for (size_t i = 0; i < LEN(tests); i++) {
        sqlite3_stmt* stmt;
        rc = sqlite3_prepare_v2(db, tests[i].sql, -1, &stmt, NULL);
        if (rc == SQLITE_OK) {
            while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            }
        }
        if (rc != SQLITE_ERROR || strcmp(sqlite3_errmsg(db), tests[i].msg) != 0) {
            printf("%s: rc = %d, %s\n", tests[i].sql, rc, sqlite3_errmsg(db));
        }
        assert(rc == SQLITE_ERROR);
        assert(strcmp(sqlite3_errmsg(db), tests[i].msg) == 0);
        sqlite3_finalize(stmt);
    }

    rc = sqlite3_close(db);
    assert(rc == SQLITE_OK);
    printf("OK\n");
}

int main(void) {
    test_errors();
    return 0;
}
