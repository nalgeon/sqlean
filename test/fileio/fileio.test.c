// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// File I/O tests.

// The fileio extension has no domain layer separate from SQLite yet, so these
// tests run it in a real connection (statically linked SQLite, see the
// Makefile). They cover what the SQL tests can't: error codes and messages,
// function flags, limits, permissions, and out-of-memory handling.
//
// Every file is created in the DIR directory, relative to the working
// directory, which is removed at the end.

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fileio/extension.h"
#include "sqlite3.h"

#define DIR "fileio.test.tmp"

typedef struct {
    int rc;
    char msg[256];
} Result;

// sh runs a shell command, which must succeed.
static void sh(const char* cmd) {
    int rc = system(cmd);
    assert(rc == 0);
}

// setup_dir creates DIR with the fixtures:
//   hello.txt  'hello world' (11 bytes)
//   lines.txt  'one\ntwo\nthree\n'
//   sub/       a.txt
static void setup_dir(void) {
    sh("chmod -R u+rwx " DIR " 2>/dev/null; rm -rf " DIR);
    sh("mkdir -p " DIR "/sub");
    sh("printf 'hello world' > " DIR "/hello.txt");
    sh("printf 'one\\ntwo\\nthree\\n' > " DIR "/lines.txt");
    sh("touch " DIR "/sub/a.txt");
}

// remove_dir deletes DIR, even if a test made parts of it unreadable.
static void remove_dir(void) {
    sh("chmod -R u+rwx " DIR " 2>/dev/null; rm -rf " DIR);
}

// open_db opens an in-memory database and initializes the extension in it.
static sqlite3* open_db(void) {
    sqlite3* db;
    int rc = sqlite3_open(":memory:", &db);
    assert(rc == SQLITE_OK);
    rc = fileio_init(db);
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

#pragma region Errors

static const ErrorTest read_errors[] = {
    {"select fileio_read('" DIR "/hello.txt', -1)", SQLITE_ERROR, "offset must be >= 0"},
    {"select fileio_read('" DIR "/hello.txt', 0, -1)", SQLITE_ERROR, "limit must be >= 0"},
    // the offset is truncated to 32 bits, so 2^31 is negative (current behavior)
    {"select fileio_read('" DIR "/hello.txt', 2147483648)", SQLITE_ERROR, "offset must be >= 0"},
};

static void test_read_errors(void) {
    printf("test_read_errors...");
    sqlite3* db = open_db();
    check_errors(db, read_errors, sizeof(read_errors) / sizeof(read_errors[0]));
    close_db(db);
    printf("OK\n");
}

// Arity errors name the legacy function (current behavior).
static const ErrorTest write_errors[] = {
    // a failure is NULL with two arguments, and an error with more (current behavior)
    {"select fileio_write('" DIR "/sub', 'x', 420)", SQLITE_ERROR,
     "failed to write file: " DIR "/sub"},
    {"select fileio_write('" DIR "/sub', 'x', 420, 0)", SQLITE_ERROR,
     "failed to write file: " DIR "/sub"},
    {"select fileio_write('" DIR "/hello.txt/x.txt', 'x', 420)", SQLITE_ERROR,
     "failed to write file: " DIR "/hello.txt/x.txt"},
    {"select fileio_write('" DIR "/x.txt')", SQLITE_ERROR,
     "wrong number of arguments to function writefile()"},
    {"select fileio_write('" DIR "/x.txt', 'x', 420, 0, 1)", SQLITE_ERROR,
     "wrong number of arguments to function writefile()"},
};

static void test_write_errors(void) {
    printf("test_write_errors...");
    sqlite3* db = open_db();
    check_errors(db, write_errors, sizeof(write_errors) / sizeof(write_errors[0]));
    close_db(db);
    printf("OK\n");
}

static const ErrorTest append_errors[] = {
    {"select fileio_append('" DIR "/sub', 'x')", SQLITE_ERROR, "failed to open file"},
    {"select fileio_append('" DIR "/hello.txt/x.txt', 'x')", SQLITE_ERROR, "failed to open file"},
    {"select fileio_append('" DIR "/x.txt')", SQLITE_ERROR,
     "wrong number of arguments to function fileio_append()"},
};

static void test_append_errors(void) {
    printf("test_append_errors...");
    sqlite3* db = open_db();
    check_errors(db, append_errors, sizeof(append_errors) / sizeof(append_errors[0]));
    close_db(db);
    printf("OK\n");
}

// Arity errors name the legacy function (current behavior).
static const ErrorTest mkdir_errors[] = {
    {"select fileio_mkdir('" DIR "/hello.txt')", SQLITE_ERROR,
     "failed to create directory: " DIR "/hello.txt"},
    // parent directories are not created
    {"select fileio_mkdir('" DIR "/nosuch/sub')", SQLITE_ERROR,
     "failed to create directory: " DIR "/nosuch/sub"},
    {"select fileio_mkdir()", SQLITE_ERROR, "wrong number of arguments to function mkdir()"},
    {"select fileio_mkdir('" DIR "/d', 493, 1)", SQLITE_ERROR,
     "wrong number of arguments to function mkdir()"},
};

static void test_mkdir_errors(void) {
    printf("test_mkdir_errors...");
    sqlite3* db = open_db();
    check_errors(db, mkdir_errors, sizeof(mkdir_errors) / sizeof(mkdir_errors[0]));
    close_db(db);
    printf("OK\n");
}

static const ErrorTest symlink_errors[] = {
    {"select fileio_symlink('hello.txt')", SQLITE_ERROR,
     "wrong number of arguments to function fileio_symlink()"},
};

static void test_symlink_errors(void) {
    printf("test_symlink_errors...");
    sqlite3* db = open_db();
    check_errors(db, symlink_errors, sizeof(symlink_errors) / sizeof(symlink_errors[0]));
    close_db(db);
    printf("OK\n");
}

// Errors name the legacy table function (current behavior).
static const ErrorTest ls_errors[] = {
    {"select * from fileio_ls()", SQLITE_ERROR, "table function lsdir requires an argument"},
    {"select * from fileio_ls(null)", SQLITE_ERROR,
     "table function lsdir requires a non-NULL argument"},
    {"select * from fileio_ls('" DIR "', true, 1)", SQLITE_ERROR,
     "too many arguments on fileio_ls() - max 2"},
};

static void test_ls_errors(void) {
    printf("test_ls_errors...");
    sqlite3* db = open_db();
    check_errors(db, ls_errors, sizeof(ls_errors) / sizeof(ls_errors[0]));
    close_db(db);
    printf("OK\n");
}

// Errors name the legacy table function (current behavior).
static const ErrorTest scan_errors[] = {
    {"select * from fileio_scan('" DIR "/nosuch.txt')", SQLITE_ERROR,
     "cannot open '" DIR "/nosuch.txt' for reading"},
    {"select * from fileio_scan()", SQLITE_ERROR, "scanfile() expects a single constraint (name)"},
    {"select * from fileio_scan('" DIR "/lines.txt', 1)", SQLITE_ERROR,
     "too many arguments on fileio_scan() - max 1"},
    // any other constraint fails the query, even a LIMIT (current behavior)
    {"select * from fileio_scan('" DIR "/lines.txt') limit 1", SQLITE_ERROR,
     "scanfile() expects a single constraint (name)"},
    {"select * from fileio_scan('" DIR "/lines.txt') where value = 'two'", SQLITE_ERROR,
     "scanfile() expects a single constraint (name)"},
    {"select * from fileio_scan('" DIR "/lines.txt') where rowid = 2", SQLITE_ERROR,
     "scanfile() expects a single constraint (name)"},
    {"select * from fileio_scan('" DIR "/lines.txt') where name = 'x'", SQLITE_ERROR,
     "scanfile() expects a single constraint (name)"},
};

static void test_scan_errors(void) {
    printf("test_scan_errors...");
    sqlite3* db = open_db();
    check_errors(db, scan_errors, sizeof(scan_errors) / sizeof(scan_errors[0]));
    close_db(db);
    printf("OK\n");
}

static const ErrorTest mode_errors[] = {
    {"select fileio_mode()", SQLITE_ERROR, "wrong number of arguments to function fileio_mode()"},
    {"select fileio_mode(420, 1)", SQLITE_ERROR,
     "wrong number of arguments to function fileio_mode()"},
};

static void test_mode_errors(void) {
    printf("test_mode_errors...");
    sqlite3* db = open_db();
    check_errors(db, mode_errors, sizeof(mode_errors) / sizeof(mode_errors[0]));
    close_db(db);
    printf("OK\n");
}

#pragma endregion

#pragma region Limits

static void test_length_limit(void) {
    printf("test_length_limit...");
    sh("printf '%0100d' 0 > " DIR "/big.txt");
    sqlite3* db = open_db();
    sqlite3_limit(db, SQLITE_LIMIT_LENGTH, 64);

    // a file longer than the limit is an error
    Result res = exec(db, "select fileio_read('" DIR "/big.txt')");
    assert(res.rc == SQLITE_TOOBIG);
    assert(strcmp(res.msg, "string or blob too big") == 0);
    res = exec(db, "select fileio_read('" DIR "/big.txt', 35)");
    assert(res.rc == SQLITE_TOOBIG);

    // the limit applies to the part being read
    assert(query_int(db, "select length(fileio_read('" DIR "/big.txt', 36))") == 64);
    assert(query_int(db, "select length(fileio_read('" DIR "/big.txt', 0, 64))") == 64);

    close_db(db);
    sh("rm " DIR "/big.txt");
    printf("OK\n");
}

#pragma endregion

#pragma region Permissions

// Permission checks don't apply to root, so these tests only run as
// a regular user.
static const ErrorTest permission_errors[] = {
    {"select fileio_write('" DIR "/locked/x.txt', 'x', 420)", SQLITE_ERROR,
     "failed to write file: " DIR "/locked/x.txt"},
    {"select fileio_append('" DIR "/locked/x.txt', 'x')", SQLITE_ERROR, "failed to open file"},
    {"select fileio_mkdir('" DIR "/locked/d')", SQLITE_ERROR,
     "failed to create directory: " DIR "/locked/d"},
    {"select * from fileio_scan('" DIR "/secret.txt')", SQLITE_ERROR,
     "cannot open '" DIR "/secret.txt' for reading"},
    // the message loses the path (current behavior)
    {"select * from fileio_ls('" DIR "/locked')", SQLITE_ERROR, "cannot read directory: "},
    // an unreadable subdirectory fails the whole listing
    {"select * from fileio_ls('" DIR "', true)", SQLITE_ERROR, "cannot read directory: "},
};

static void test_permissions(void) {
    printf("test_permissions...");
    if (geteuid() == 0) {
        printf("SKIP (root)\n");
        return;
    }
    sh("printf 'secret' > " DIR "/secret.txt && chmod 000 " DIR "/secret.txt");
    sh("mkdir " DIR "/locked && chmod 000 " DIR "/locked");
    sqlite3* db = open_db();

    // an unreadable file is NULL
    assert(query_int(db, "select fileio_read('" DIR "/secret.txt') is null") == 1);
    // a failed write with two arguments is NULL
    assert(query_int(db, "select fileio_write('" DIR "/locked/x.txt', 'x') is null") == 1);
    // the symlink failure is ignored (current behavior)
    assert(query_int(db, "select fileio_symlink('x', '" DIR "/locked/link') is null") == 1);
    // a directory that can't be read is still listed from its parent
    assert(query_int(
               db, "select count(*) from fileio_ls('" DIR "') where name = '" DIR "/locked'") == 1);

    check_errors(db, permission_errors, sizeof(permission_errors) / sizeof(permission_errors[0]));

    close_db(db);
    sh("chmod 700 " DIR "/locked && rmdir " DIR "/locked");
    sh("chmod 600 " DIR "/secret.txt && rm " DIR "/secret.txt");
    printf("OK\n");
}

#pragma endregion

#pragma region Flags

// Functions that touch the file system are direct-only, so they can't be
// used in views or triggers, whatever trusted_schema says.
static const ErrorTest direct_only_errors[] = {
    {"select * from v_read", SQLITE_ERROR, "unsafe use of fileio_read()"},
    {"select * from v_write", SQLITE_ERROR, "unsafe use of fileio_write()"},
    {"select * from v_append", SQLITE_ERROR, "unsafe use of fileio_append()"},
    {"select * from v_mkdir", SQLITE_ERROR, "unsafe use of fileio_mkdir()"},
    {"select * from v_symlink", SQLITE_ERROR, "unsafe use of fileio_symlink()"},
    {"select * from v_ls", SQLITE_ERROR, "unsafe use of virtual table \"fileio_ls\""},
    {"select * from v_scan", SQLITE_ERROR, "unsafe use of virtual table \"fileio_scan\""},
    {"select * from v_readfile", SQLITE_ERROR, "unsafe use of readfile()"},
    {"select * from v_lsdir", SQLITE_ERROR, "unsafe use of virtual table \"lsdir\""},
    {"insert into data values (1)", SQLITE_ERROR, "unsafe use of fileio_write()"},
};

static void test_flags(void) {
    printf("test_flags...");
    sqlite3* db = open_db();
    exec_ok(db, "create table data(x)");
    exec_ok(db, "create view v_read as select fileio_read('" DIR "/hello.txt')");
    exec_ok(db, "create view v_write as select fileio_write('" DIR "/v.txt', 'x')");
    exec_ok(db, "create view v_append as select fileio_append('" DIR "/v.txt', 'x')");
    exec_ok(db, "create view v_mkdir as select fileio_mkdir('" DIR "/v')");
    exec_ok(db, "create view v_symlink as select fileio_symlink('x', '" DIR "/v.lnk')");
    exec_ok(db, "create view v_ls as select * from fileio_ls('" DIR "')");
    exec_ok(db, "create view v_scan as select * from fileio_scan('" DIR "/lines.txt')");
    exec_ok(db, "create view v_readfile as select readfile('" DIR "/hello.txt')");
    exec_ok(db, "create view v_lsdir as select * from lsdir('" DIR "')");
    exec_ok(db,
            "create trigger t_write after insert on data "
            "begin select fileio_write('" DIR "/t.txt', 'x'); end");

    exec_ok(db, "pragma trusted_schema = 1");
    check_errors(db, direct_only_errors,
                 sizeof(direct_only_errors) / sizeof(direct_only_errors[0]));
    // nothing was written
    assert(query_int(db, "select count(*) from fileio_ls('" DIR "') where name like '%/v%' or "
                         "name like '%/t.txt'") == 0);

    // fileio_mode is not innocuous, so it's allowed in views only with
    // a trusted schema (current behavior)
    exec_ok(db, "create view v_mode as select fileio_mode(420) as value");
    exec_ok(db, "select * from v_mode");
    exec_ok(db, "pragma trusted_schema = 0");
    Result res = exec(db, "select * from v_mode");
    assert(res.rc == SQLITE_ERROR);
    assert(strcmp(res.msg, "unsafe use of fileio_mode()") == 0);

    // fileio_mode is not deterministic, so it's not allowed in indexes
    // (current behavior)
    res = exec(db, "create index data_mode on data(fileio_mode(x))");
    assert(res.rc == SQLITE_ERROR);
    assert(strcmp(res.msg, "non-deterministic functions prohibited in index expressions") == 0);

    close_db(db);
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

// Each case runs after the setup statements, in a new connection,
// with DIR/new removed.
static const char* nomem_setup[] = {
    "create table data(x)",
    "insert into data values ('one'), ('two'), ('three')",
};

typedef struct {
    const char* sql;
    // the result code reported instead of SQLITE_NOMEM, if any (current behavior)
    int nomem_rc;
} NomemTest;

static const NomemTest nomem_cases[] = {
    {"select fileio_read('" DIR "/hello.txt')", SQLITE_NOMEM},
    {"select fileio_read('" DIR "/hello.txt', 6, 2)", SQLITE_NOMEM},
    {"select fileio_write('" DIR "/w.txt', 'hello', 420, 0)", SQLITE_NOMEM},
    // failing to create the parent directory is a NULL result
    {"select fileio_write('" DIR "/new/sub/w.txt', 'hello')", SQLITE_OK},
    // or an error
    {"select fileio_write('" DIR "/new/sub/w.txt', 'hello', 420)", SQLITE_ERROR},
    {"select sum(fileio_append('" DIR "/ap.txt', x)) from data", SQLITE_NOMEM},
    {"select sum(fileio_append('" DIR "/new/ap.txt', x)) from data", SQLITE_ERROR},
    {"select fileio_mkdir('" DIR "/new')", SQLITE_NOMEM},
    {"select fileio_symlink('hello.txt', '" DIR "/new')", SQLITE_NOMEM},
    {"select count(*) from fileio_ls('" DIR "/hello.txt')", SQLITE_NOMEM},
    {"select count(*) from fileio_ls('" DIR "', true)", SQLITE_NOMEM},
    {"select count(*) from fileio_scan('" DIR "/lines.txt')", SQLITE_NOMEM},
    {"select fileio_mode(420)", SQLITE_NOMEM},
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
            sh("rm -rf " DIR "/new");
            sqlite3* db = open_db();
            for (size_t j = 0; j < sizeof(nomem_setup) / sizeof(nomem_setup[0]); j++) {
                exec_ok(db, nomem_setup[j]);
            }
            fail_after = n;
            Result res = exec(db, nomem_cases[i].sql);
            bool failed = fail_after == 0;
            fail_after = -1;
            close_db(db);
            if (n < 0) {
                // without failures
                want = res.rc;
                continue;
            }
            // if an allocation failed, the statement reports it or recovers
            bool ok = res.rc == want || res.rc == SQLITE_NOMEM || res.rc == nomem_cases[i].nomem_rc;
            if (!ok) {
                printf("%s, allocation %d: %d %s\n", nomem_cases[i].sql, n, res.rc, res.msg);
            }
            assert(ok);
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
    setup_dir();

    // Errors
    test_read_errors();
    test_write_errors();
    test_append_errors();
    test_mkdir_errors();
    test_symlink_errors();
    test_ls_errors();
    test_scan_errors();
    test_mode_errors();

    // Limits
    test_length_limit();

    // Permissions
    test_permissions();

    // Flags
    test_flags();

    // Out of memory
    test_nomem();

    remove_dir();
}
