// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Math function tests.

// The math extension has no domain layer, and the SQL tests cover its
// functions. These tests run it in a real connection (statically linked
// SQLite, see the Makefile) to cover what the SQL tests can't: the behavior
// under a locale set by the host application.

#include <assert.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>

#include "math/extension.h"
#include "sqlite3.h"

// Locales with a ',' decimal point, one of which is likely installed.
static const char* const comma_locales[] = {
    "de_DE.UTF-8", "de_DE.utf8", "de_DE", "fr_FR.UTF-8", "fr_FR.utf8", "ru_RU.UTF-8", NULL,
};

// set_comma_locale sets the numeric locale to one with a ',' decimal point
// and returns its name, or NULL if none is installed.
static const char* set_comma_locale(void) {
    for (const char* const* name = comma_locales; *name != NULL; name++) {
        if (setlocale(LC_NUMERIC, *name) != NULL && strcmp(localeconv()->decimal_point, ",") == 0) {
            return *name;
        }
    }
    setlocale(LC_NUMERIC, "C");
    return NULL;
}

// query_double runs a query that returns a single double.
static double query_double(sqlite3* db, const char* sql) {
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    assert(rc == SQLITE_OK);
    rc = sqlite3_step(stmt);
    assert(rc == SQLITE_ROW);
    double val = sqlite3_column_double(stmt, 0);
    sqlite3_finalize(stmt);
    return val;
}

// math_round does not depend on the locale's decimal point.
static void test_round_locale(void) {
    printf("test_round_locale...");
    const char* locale = set_comma_locale();
    if (locale == NULL) {
        printf("SKIP (no locale with a ',' decimal point)\n");
        return;
    }

    sqlite3* db;
    int rc = sqlite3_open(":memory:", &db);
    assert(rc == SQLITE_OK);
    rc = math_init(db);
    assert(rc == SQLITE_OK);

    const struct {
        const char* sql;
        double want;
    } tests[] = {
        {"select math_round(42.123456789, 2)", 42.12},
        {"select math_round(-42.125, 2)", -42.13},     // away from zero
        {"select math_round(0.5, 1)", 0.5},            // no integer part to keep
        {"select math_round(42.123456789, 0)", 42.0},  // no decimal point to replace
        {"select math_round(1e20, 2)", 1e20},          // nothing to round
    };
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        double got = query_double(db, tests[i].sql);
        if (got != tests[i].want) {
            printf("\n%s (%s): got %.17g, want %.17g\n", tests[i].sql, locale, got, tests[i].want);
        }
        assert(got == tests[i].want);
    }

    rc = sqlite3_close(db);
    assert(rc == SQLITE_OK);
    setlocale(LC_NUMERIC, "C");
    printf("OK\n");
}

int main(void) {
    test_round_locale();
    return 0;
}
