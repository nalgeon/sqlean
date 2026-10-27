// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// File I/O tests.

// The domain tests call the file system functions directly. The rest run the
// extension in a real connection (statically linked SQLite, see the Makefile)
// and cover what the SQL tests can't: error codes and messages, function
// flags, limits, permissions, and out-of-memory handling.
//
// Every file is created in the DIR directory, relative to the working
// directory, which is removed at the end.

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fileio/extension.h"
#include "fileio/fileio.h"
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

#pragma region Domain

static void test_mode_string(void) {
    printf("test_mode_string...");
    struct {
        uint32_t mode;
        const char* want;
    } tests[] = {
        {0100644, "-rw-r--r--"},
        {040755, "drwxr-xr-x"},
        {0120777, "lrwxrwxrwx"},
        {010600, "prw-------"},
        {020620, "crw--w----"},
        {060660, "brw-rw----"},
        {0140755, "srwxr-xr-x"},
        {0, "?---------"},
        {0777, "?rwxrwxrwx"},
        // setuid, setgid and sticky bits are ignored
        {0107777, "-rwxrwxrwx"},
        // so are the bits above the file type
        {0xFFFF0000u | 0100644, "-rw-r--r--"},
    };
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        char buf[FS_MODE_SIZE];
        fs_mode_string(tests[i].mode, buf);
        if (strcmp(buf, tests[i].want) != 0) {
            printf("fs_mode_string(0%o): want %s, got %s\n", tests[i].mode, tests[i].want, buf);
        }
        assert(strcmp(buf, tests[i].want) == 0);
    }
    printf("OK\n");
}

static void test_join(void) {
    printf("test_join...");
    struct {
        const char* dir;
        const char* name;
        const char* want;
    } tests[] = {
        {"dir", "a.txt", "dir/a.txt"}, {"dir/", "a.txt", "dir/a.txt"}, {"/", "a.txt", "/a.txt"},
        {"", "a.txt", "/a.txt"},       {"a/b", "c", "a/b/c"},
    };
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        char buf[64];
        size_t n = fs_join(buf, sizeof(buf), tests[i].dir, tests[i].name);
        if (strcmp(buf, tests[i].want) != 0) {
            printf("fs_join(%s, %s): want %s, got %s\n", tests[i].dir, tests[i].name, tests[i].want,
                   buf);
        }
        assert(strcmp(buf, tests[i].want) == 0);
        assert(n == strlen(tests[i].want));
    }

    // the result is truncated to fit, and the full length returned
    char buf[6];
    assert(fs_join(buf, sizeof(buf), "dir", "a.txt") == 9);
    assert(strcmp(buf, "dir/a") == 0);
    assert(fs_join(buf, 1, "dir", "a.txt") == 9);
    assert(strcmp(buf, "") == 0);
    // a zero size writes nothing
    assert(fs_join(NULL, 0, "dir", "a.txt") == 9);
    printf("OK\n");
}

static void test_trim_eol(void) {
    printf("test_trim_eol...");
    struct {
        const char* line;
        size_t want;
    } tests[] = {
        {"", 0},      {"\n", 0},  {"\r", 0},    {"\r\n", 0},    {"a", 1},    {"a\n", 1},
        {"a\r\n", 1}, {"a\r", 1}, {"a\n\n", 2}, {"a\r\r\n", 2}, {"a\rb", 3}, {"a\n\r", 2},
    };
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        size_t got = fs_trim_eol(tests[i].line, strlen(tests[i].line));
        if (got != tests[i].want) {
            printf("fs_trim_eol(%zu): want %zu, got %zu\n", i, tests[i].want, got);
        }
        assert(got == tests[i].want);
    }
    printf("OK\n");
}

static void test_read_line(void) {
    printf("test_read_line...");
    FILE* file = tmpfile();
    assert(file != NULL);
    fputs("one\ntwo-long\n\nlast", file);
    rewind(file);

    char buf[4];
    size_t n;
    bool done;
    // a line that fits
    assert(fs_read_line(file, buf, sizeof(buf), &n, &done) == 0);
    assert(n == 4 && done && memcmp(buf, "one\n", 4) == 0);
    // a line that doesn't fit is read in parts
    assert(fs_read_line(file, buf, sizeof(buf), &n, &done) == 0);
    assert(n == 4 && !done && memcmp(buf, "two-", 4) == 0);
    assert(fs_read_line(file, buf, sizeof(buf), &n, &done) == 0);
    assert(n == 4 && !done && memcmp(buf, "long", 4) == 0);
    assert(fs_read_line(file, buf, sizeof(buf), &n, &done) == 0);
    assert(n == 1 && done && buf[0] == '\n');
    // an empty line
    assert(fs_read_line(file, buf, sizeof(buf), &n, &done) == 0);
    assert(n == 1 && done && buf[0] == '\n');
    // the last line without a newline
    assert(fs_read_line(file, buf, sizeof(buf), &n, &done) == 0);
    assert(n == 4 && !done && memcmp(buf, "last", 4) == 0);
    assert(fs_read_line(file, buf, sizeof(buf), &n, &done) == 0);
    assert(n == 0 && done);
    // the end of the file
    assert(fs_read_line(file, buf, sizeof(buf), &n, &done) == 0);
    assert(n == 0 && done);
    // a zero size reads nothing
    rewind(file);
    assert(fs_read_line(file, buf, 0, &n, &done) == 0);
    assert(n == 0 && !done);

    fclose(file);
    printf("OK\n");
}

static void test_read(void) {
    printf("test_read...");
    FILE* file;
    assert(fs_open(DIR "/hello.txt", &file) == 0);
    int64_t size;
    assert(fs_size(file, &size) == 0);
    assert(size == 11);

    char buf[16];
    size_t n;
    assert(fs_read(file, 6, buf, sizeof(buf), &n) == 0);
    assert(n == 5 && memcmp(buf, "world", 5) == 0);
    // past the end
    assert(fs_read(file, 100, buf, sizeof(buf), &n) == 0);
    assert(n == 0);
    // a zero size reads nothing, even with a NULL buffer
    assert(fs_read(file, 0, NULL, 0, &n) == 0);
    assert(n == 0);
    assert(fs_read(file, -1, buf, sizeof(buf), &n) == EINVAL);
    fclose(file);

    assert(fs_open(DIR "/nosuch.txt", &file) == ENOENT);
    assert(fs_open(DIR "/sub", &file) == EISDIR);
    printf("OK\n");
}

static void test_write(void) {
    printf("test_write...");
    int64_t mtime = 1000000000;
    assert(fs_write(DIR "/w/x/w.txt", "hello", 5, 0600, &mtime) == 0);
    FileInfo info;
    assert(fs_stat(DIR "/w/x/w.txt", &info) == 0);
    assert(info.mode == 0100600);
    assert(info.size == 5);
    assert(info.mtime == 1000000000);
    assert(fs_stat(DIR "/w/x", &info) == 0);
    assert((info.mode & 0170000) == 040000);

    // an empty write, with a NULL buffer
    assert(fs_write(DIR "/w/x/w.txt", NULL, 0, 0644, NULL) == 0);
    assert(fs_stat(DIR "/w/x/w.txt", &info) == 0);
    assert(info.mode == 0100644);
    assert(info.size == 0);

    // a file in the way of the parent directories
    assert(fs_write(DIR "/hello.txt/w.txt", "x", 1, 0644, NULL) == ENOTDIR);
    assert(fs_write(DIR "/sub", "x", 1, 0644, NULL) == EISDIR);
    sh("rm -rf " DIR "/w");
    printf("OK\n");
}

static void test_dir(void) {
    printf("test_dir...");
    FsDir* dir;
    assert(fs_dir_open(DIR "/sub", &dir) == 0);
    const char* name;
    assert(fs_dir_next(dir, &name) == 0);
    assert(name != NULL && strcmp(name, "a.txt") == 0);
    assert(fs_dir_next(dir, &name) == 0);
    assert(name == NULL);
    fs_dir_close(dir);
    fs_dir_close(NULL);

    assert(fs_dir_open(DIR "/nosuch", &dir) == ENOENT);
    assert(fs_dir_open(DIR "/hello.txt", &dir) == ENOTDIR);

    // an existing directory is fine, an existing file is not
    assert(fs_mkdir(DIR "/sub", 0755) == 0);
    assert(fs_mkdir(DIR "/hello.txt", 0755) == EEXIST);
    assert(fs_mkdir(DIR "/nosuch/d", 0755) == ENOENT);
    assert(fs_mkdir_parents(DIR "/p/q/r") == 0);
    FileInfo info;
    assert(fs_stat(DIR "/p/q", &info) == 0);
    assert(fs_stat(DIR "/p/q/r", &info) == ENOENT);
    assert(fs_mkdir_parents(DIR "/hello.txt/q/r") == ENOTDIR);
    sh("rm -rf " DIR "/p");
    printf("OK\n");
}

#pragma endregion

#pragma region Errors

static const ErrorTest read_errors[] = {
    {"select fileio_read()", SQLITE_ERROR, "wrong number of arguments to function fileio_read()"},
    {"select fileio_read('" DIR "/hello.txt', 0, 5, 1)", SQLITE_ERROR,
     "wrong number of arguments to function fileio_read()"},
};

static void test_read_errors(void) {
    printf("test_read_errors...");
    sqlite3* db = open_db();
    check_errors(db, read_errors, sizeof(read_errors) / sizeof(read_errors[0]));
    close_db(db);
    printf("OK\n");
}

static const ErrorTest write_errors[] = {
    {"select fileio_write('" DIR "/sub', 'x')", SQLITE_ERROR, "failed to write file: " DIR "/sub"},
    {"select fileio_write('" DIR "/sub', 'x', 420, 0)", SQLITE_ERROR,
     "failed to write file: " DIR "/sub"},
    {"select fileio_write('" DIR "/hello.txt/x.txt', 'x')", SQLITE_ERROR,
     "failed to write file: " DIR "/hello.txt/x.txt"},
    {"select fileio_write('', 'x')", SQLITE_ERROR, "failed to write file: "},
    {"select fileio_write('" DIR "/x.txt')", SQLITE_ERROR,
     "wrong number of arguments to function fileio_write()"},
    {"select fileio_write('" DIR "/x.txt', 'x', 420, 0, 1)", SQLITE_ERROR,
     "wrong number of arguments to function fileio_write()"},
};

static void test_write_errors(void) {
    printf("test_write_errors...");
    sqlite3* db = open_db();
    check_errors(db, write_errors, sizeof(write_errors) / sizeof(write_errors[0]));
    close_db(db);
    printf("OK\n");
}

static const ErrorTest append_errors[] = {
    {"select fileio_append('" DIR "/sub', 'x')", SQLITE_ERROR, "failed to open file: " DIR "/sub"},
    {"select fileio_append('" DIR "/hello.txt/x.txt', 'x')", SQLITE_ERROR,
     "failed to open file: " DIR "/hello.txt/x.txt"},
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

static const ErrorTest mkdir_errors[] = {
    {"select fileio_mkdir('" DIR "/hello.txt')", SQLITE_ERROR,
     "failed to create directory: " DIR "/hello.txt"},
    // parent directories are not created
    {"select fileio_mkdir('" DIR "/nosuch/sub')", SQLITE_ERROR,
     "failed to create directory: " DIR "/nosuch/sub"},
    {"select fileio_mkdir()", SQLITE_ERROR, "wrong number of arguments to function fileio_mkdir()"},
    {"select fileio_mkdir('" DIR "/d', 493, 1)", SQLITE_ERROR,
     "wrong number of arguments to function fileio_mkdir()"},
};

static void test_mkdir_errors(void) {
    printf("test_mkdir_errors...");
    sqlite3* db = open_db();
    check_errors(db, mkdir_errors, sizeof(mkdir_errors) / sizeof(mkdir_errors[0]));
    close_db(db);
    printf("OK\n");
}

static const ErrorTest symlink_errors[] = {
    // the link path is taken
    {"select fileio_symlink('lines.txt', '" DIR "/hello.txt')", SQLITE_ERROR,
     "failed to create symlink: " DIR "/hello.txt"},
    // parent directories are not created
    {"select fileio_symlink('hello.txt', '" DIR "/nosuch/link')", SQLITE_ERROR,
     "failed to create symlink: " DIR "/nosuch/link"},
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

static const ErrorTest ls_errors[] = {
    {"select * from fileio_ls()", SQLITE_ERROR, "table function fileio_ls requires an argument"},
    {"select * from lsdir()", SQLITE_ERROR, "table function lsdir requires an argument"},
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

static const ErrorTest scan_errors[] = {
    {"select * from fileio_scan()", SQLITE_ERROR,
     "table function fileio_scan requires an argument"},
    {"select * from scanfile()", SQLITE_ERROR, "table function scanfile requires an argument"},
    {"select * from fileio_scan('" DIR "/lines.txt', 1)", SQLITE_ERROR,
     "too many arguments on fileio_scan() - max 1"},
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

    // so does a line longer than the limit
    res = exec(db, "select * from fileio_scan('" DIR "/big.txt')");
    assert(res.rc == SQLITE_TOOBIG);
    sh("printf '%064d\\r\\n%064d' 0 0 > " DIR "/big.txt");
    assert(query_int(db, "select sum(length(value)) from fileio_scan('" DIR "/big.txt')") == 128);

    close_db(db);
    sh("rm " DIR "/big.txt");
    printf("OK\n");
}

#pragma endregion

#pragma region Permissions

// Permission checks don't apply to root, so these tests only run as
// a regular user.
static const ErrorTest permission_errors[] = {
    {"select fileio_write('" DIR "/locked/x.txt', 'x')", SQLITE_ERROR,
     "failed to write file: " DIR "/locked/x.txt"},
    {"select fileio_append('" DIR "/locked/x.txt', 'x')", SQLITE_ERROR,
     "failed to open file: " DIR "/locked/x.txt"},
    {"select fileio_mkdir('" DIR "/locked/d')", SQLITE_ERROR,
     "failed to create directory: " DIR "/locked/d"},
    {"select fileio_symlink('x', '" DIR "/locked/link')", SQLITE_ERROR,
     "failed to create symlink: " DIR "/locked/link"},
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

    // an unreadable file is NULL, and has no lines
    assert(query_int(db, "select fileio_read('" DIR "/secret.txt') is null") == 1);
    assert(query_int(db, "select count(*) from fileio_scan('" DIR "/secret.txt')") == 0);
    // a directory that can't be read is listed, but not its contents
    assert(query_int(db, "select count(*) from fileio_ls('" DIR "/locked')") == 1);
    assert(query_int(
               db, "select count(*) from fileio_ls('" DIR "') where name = '" DIR "/locked'") == 1);
    assert(query_int(db, "select count(*) from fileio_ls('" DIR "', true) where name like '" DIR
                         "/locked%'") == 1);
    // the rest is listed recursively
    assert(query_int(db, "select count(*) from fileio_ls('" DIR "', true) where name = '" DIR
                         "/sub/a.txt'") == 1);

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

    // fileio_mode doesn't touch the file system, so it's innocuous
    // and allowed in views with an untrusted schema
    exec_ok(db, "create view v_mode as select fileio_mode(420) as value");
    exec_ok(db, "pragma trusted_schema = 0");
    exec_ok(db, "select * from v_mode");
    exec_ok(db, "select * from (select lsmode(420))");

    // and deterministic, so it's allowed in indexes
    exec_ok(db, "create index data_mode on data(fileio_mode(x))");

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

static const char* nomem_cases[] = {
    "select fileio_read('" DIR "/hello.txt')",
    "select fileio_read('" DIR "/hello.txt', 6, 2)",
    "select fileio_write('" DIR "/w.txt', 'hello', 420, 0)",
    "select fileio_write('" DIR "/new/sub/w.txt', 'hello')",
    "select fileio_write('" DIR "/sub', 'hello')",
    "select sum(fileio_append('" DIR "/ap.txt', x)) from data",
    "select sum(fileio_append('" DIR "/new/ap.txt', x)) from data",
    "select fileio_mkdir('" DIR "/new')",
    "select fileio_mkdir('" DIR "/hello.txt')",
    "select fileio_symlink('hello.txt', '" DIR "/new')",
    "select count(*) from fileio_ls('" DIR "/hello.txt')",
    "select count(*) from fileio_ls('" DIR "', true)",
    "select count(*) from fileio_ls()",
    "select count(*) from fileio_scan('" DIR "/lines.txt')",
    "select count(*) from fileio_scan()",
    "select fileio_mode(420)",
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
            bool ok = res.rc == want || res.rc == SQLITE_NOMEM;
            if (!ok) {
                printf("%s, allocation %d: %d %s\n", nomem_cases[i], n, res.rc, res.msg);
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

    // Domain
    test_mode_string();
    test_join();
    test_trim_eol();
    test_read_line();
    test_read();
    test_write();
    test_dir();

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
