// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Fuzzy matching tests.

// These tests run the extension in a real connection (statically linked
// SQLite, see the Makefile) and cover what the SQL tests can't: error codes
// and messages, and function flags.

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "fuzzy/extension.h"
#include "sqlite3.h"

typedef struct {
    int rc;
    char msg[256];
} Result;

// open_db opens an in-memory database and initializes the extension in it.
static sqlite3* open_db(void) {
    sqlite3* db;
    int rc = sqlite3_open(":memory:", &db);
    assert(rc == SQLITE_OK);
    rc = fuzzy_init(db);
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
        if (got.rc != test.rc || strcmp(got.msg, test.msg) != 0) {
            printf("%s: want %d '%s', got %d '%s'\n", test.sql, test.rc, test.msg, got.rc, got.msg);
        }
        assert(got.rc == test.rc);
        assert(strcmp(got.msg, test.msg) == 0);
    }
}

// A call to every function and alias with valid arguments.
static const char* const calls[] = {
    // distances
    "fuzzy_damlev(x, 'abc')",
    "dlevenshtein(x, 'abc')",
    "fuzzy_editdist(x, 'abc')",
    "edit_distance(x, 'abc')",
    "fuzzy_hamming(x, 'abc')",
    "hamming(x, 'abc')",
    "fuzzy_jarowin(x, 'abc')",
    "jaro_winkler(x, 'abc')",
    "fuzzy_leven(x, 'abc')",
    "levenshtein(x, 'abc')",
    "fuzzy_osadist(x, 'abc')",
    "osa_distance(x, 'abc')",
    // phonetic codes
    "fuzzy_caver(x)",
    "caverphone(x)",
    "fuzzy_phonetic(x)",
    "phonetic_hash(x)",
    "fuzzy_soundex(x)",
    "soundex(x)",
    "fuzzy_rsoundex(x)",
    "rsoundex(x)",
    // transliteration
    "fuzzy_translit(x)",
    "translit(x)",
    "fuzzy_script(x)",
    "script_code(x)",
};

#pragma region Errors

// The distance functions raise an error on NULL (current behavior).
static const ErrorTest null_errors[] = {
    {"select fuzzy_damlev(null, 'abc')", SQLITE_ERROR, "arguments should not be NULL"},
    {"select fuzzy_damlev('abc', null)", SQLITE_ERROR, "arguments should not be NULL"},
    {"select fuzzy_editdist(null, 'abc')", SQLITE_ERROR, "NULL input to editdist()"},
    {"select fuzzy_editdist('abc', null)", SQLITE_ERROR, "NULL input to editdist()"},
    {"select fuzzy_hamming(null, 'abc')", SQLITE_ERROR, "arguments should not be NULL"},
    {"select fuzzy_hamming('abc', null)", SQLITE_ERROR, "arguments should not be NULL"},
    {"select fuzzy_jarowin(null, 'abc')", SQLITE_ERROR, "arguments should not be NULL"},
    {"select fuzzy_jarowin('abc', null)", SQLITE_ERROR, "arguments should not be NULL"},
    {"select fuzzy_leven(null, 'abc')", SQLITE_ERROR, "arguments should not be NULL"},
    {"select fuzzy_leven('abc', null)", SQLITE_ERROR, "arguments should not be NULL"},
    {"select fuzzy_osadist(null, 'abc')", SQLITE_ERROR, "arguments should not be NULL"},
    {"select fuzzy_osadist('abc', null)", SQLITE_ERROR, "arguments should not be NULL"},
    // aliases
    {"select dlevenshtein(null, null)", SQLITE_ERROR, "arguments should not be NULL"},
    {"select edit_distance(null, null)", SQLITE_ERROR, "NULL input to editdist()"},
    {"select hamming(null, null)", SQLITE_ERROR, "arguments should not be NULL"},
    {"select jaro_winkler(null, null)", SQLITE_ERROR, "arguments should not be NULL"},
    {"select levenshtein(null, null)", SQLITE_ERROR, "arguments should not be NULL"},
    {"select osa_distance(null, null)", SQLITE_ERROR, "arguments should not be NULL"},
};

static void test_null_errors(void) {
    printf("test_null_errors...");
    sqlite3* db = open_db();
    check_errors(db, null_errors, sizeof(null_errors) / sizeof(null_errors[0]));
    close_db(db);
    printf("OK\n");
}

// The distance functions and most of the phonetic functions raise an error
// on non-ASCII text (current behavior).
static const ErrorTest ascii_errors[] = {
    // distances
    {"select fuzzy_damlev('é', 'e')", SQLITE_ERROR, "arguments should be ASCII strings"},
    {"select fuzzy_damlev('e', 'é')", SQLITE_ERROR, "arguments should be ASCII strings"},
    {"select fuzzy_editdist('é', 'e')", SQLITE_ERROR, "non-ASCII input to editdist()"},
    {"select fuzzy_editdist('e', 'é')", SQLITE_ERROR, "non-ASCII input to editdist()"},
    {"select fuzzy_hamming('é', 'ab')", SQLITE_ERROR, "arguments should be ASCII strings"},
    {"select fuzzy_hamming('ab', 'é')", SQLITE_ERROR, "arguments should be ASCII strings"},
    {"select fuzzy_jarowin('é', 'e')", SQLITE_ERROR, "arguments should be ASCII strings"},
    {"select fuzzy_jarowin('e', 'é')", SQLITE_ERROR, "arguments should be ASCII strings"},
    {"select fuzzy_leven('é', 'e')", SQLITE_ERROR, "arguments should be ASCII strings"},
    {"select fuzzy_leven('e', 'é')", SQLITE_ERROR, "arguments should be ASCII strings"},
    {"select fuzzy_osadist('é', 'e')", SQLITE_ERROR, "arguments should be ASCII strings"},
    {"select fuzzy_osadist('e', 'é')", SQLITE_ERROR, "arguments should be ASCII strings"},
    // a blob is checked as text
    {"select fuzzy_leven(x'ff', 'e')", SQLITE_ERROR, "arguments should be ASCII strings"},
    {"select fuzzy_editdist(x'ff', 'e')", SQLITE_ERROR, "non-ASCII input to editdist()"},
    // phonetic codes (fuzzy_phonetic accepts any text)
    {"select fuzzy_caver('é')", SQLITE_ERROR, "argument should be ASCII string"},
    {"select fuzzy_soundex('é')", SQLITE_ERROR, "argument should be ASCII string"},
    {"select fuzzy_rsoundex('é')", SQLITE_ERROR, "argument should be ASCII string"},
    {"select fuzzy_soundex(x'ff')", SQLITE_ERROR, "argument should be ASCII string"},
};

static void test_ascii_errors(void) {
    printf("test_ascii_errors...");
    sqlite3* db = open_db();
    check_errors(db, ascii_errors, sizeof(ascii_errors) / sizeof(ascii_errors[0]));
    close_db(db);
    printf("OK\n");
}

// Every function is registered with a fixed number of arguments.
static const ErrorTest arity_errors[] = {
    {"select fuzzy_damlev('abc')", SQLITE_ERROR,
     "wrong number of arguments to function fuzzy_damlev()"},
    {"select fuzzy_editdist('abc', 'abc', 'abc')", SQLITE_ERROR,
     "wrong number of arguments to function fuzzy_editdist()"},
    {"select fuzzy_hamming()", SQLITE_ERROR,
     "wrong number of arguments to function fuzzy_hamming()"},
    {"select fuzzy_jarowin('abc')", SQLITE_ERROR,
     "wrong number of arguments to function fuzzy_jarowin()"},
    {"select fuzzy_leven('abc')", SQLITE_ERROR,
     "wrong number of arguments to function fuzzy_leven()"},
    {"select fuzzy_osadist('abc')", SQLITE_ERROR,
     "wrong number of arguments to function fuzzy_osadist()"},
    {"select fuzzy_caver()", SQLITE_ERROR, "wrong number of arguments to function fuzzy_caver()"},
    {"select fuzzy_phonetic('abc', 'abc')", SQLITE_ERROR,
     "wrong number of arguments to function fuzzy_phonetic()"},
    {"select fuzzy_soundex()", SQLITE_ERROR,
     "wrong number of arguments to function fuzzy_soundex()"},
    {"select fuzzy_rsoundex('abc', 'abc')", SQLITE_ERROR,
     "wrong number of arguments to function fuzzy_rsoundex()"},
    {"select fuzzy_translit()", SQLITE_ERROR,
     "wrong number of arguments to function fuzzy_translit()"},
    {"select fuzzy_script('abc', 'abc')", SQLITE_ERROR,
     "wrong number of arguments to function fuzzy_script()"},
    {"select levenshtein('abc')", SQLITE_ERROR,
     "wrong number of arguments to function levenshtein()"},
    {"select soundex()", SQLITE_ERROR, "wrong number of arguments to function soundex()"},
};

static void test_arity_errors(void) {
    printf("test_arity_errors...");
    sqlite3* db = open_db();
    check_errors(db, arity_errors, sizeof(arity_errors) / sizeof(arity_errors[0]));
    close_db(db);
    printf("OK\n");
}

#pragma endregion

#pragma region Flags

// test_flags checks that every function is innocuous (allowed in views with
// an untrusted schema) and deterministic (allowed in index expressions).
static void test_flags(void) {
    printf("test_flags...");
    sqlite3* db = open_db();
    exec_ok(db, "create table data(x)");
    exec_ok(db, "insert into data values ('abc')");
    exec_ok(db, "pragma trusted_schema = 0");

    char sql[256];
    for (size_t i = 0; i < sizeof(calls) / sizeof(calls[0]); i++) {
        snprintf(sql, sizeof(sql), "create view v%zu as select %s from data", i, calls[i]);
        exec_ok(db, sql);
        snprintf(sql, sizeof(sql), "select * from v%zu", i);
        exec_ok(db, sql);

        snprintf(sql, sizeof(sql), "create index data_%zu on data(%s)", i, calls[i]);
        exec_ok(db, sql);
    }

    // the indexes are maintained on insert
    exec_ok(db, "insert into data values ('abd')");

    close_db(db);
    printf("OK\n");
}

#pragma endregion

int main(void) {
    // Errors
    test_null_errors();
    test_ascii_errors();
    test_arity_errors();

    // Flags
    test_flags();
}
