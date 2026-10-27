// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Based on fileio.c by D. Richard Hipp, Public Domain
// https://www.sqlite.org/src/file/ext/misc/fileio.c

// SQLite extension for reading and writing files.

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT3

#include "fileio/binding.h"
#include "fileio/extension.h"
#include "fileio/fileio.h"

#pragma region Helpers

// MAX_PERM is the largest permissions value:
// the setuid, setgid and sticky bits, and rwx for everyone.
#define MAX_PERM 07777

// MAX_MODE is the largest file mode value: the file type and the permissions.
#define MAX_MODE 0177777

// result_errorf sets an error message, formatted with the path.
static void result_errorf(sqlite3_context* context, const char* format, const char* path) {
    char* msg = sqlite3_mprintf(format, path);
    if (msg == NULL) {
        sqlite3_result_error_nomem(context);
        return;
    }
    sqlite3_result_error(context, msg, -1);
    sqlite3_free(msg);
}

// result_fs_error sets the error for a failed file system operation,
// naming the path. Running out of memory is reported as such.
static void result_fs_error(sqlite3_context* context,
                            int err,
                            const char* format,
                            const char* path) {
    if (err == ENOMEM) {
        sqlite3_result_error_nomem(context);
        return;
    }
    result_errorf(context, format, path);
}

// arg_perm decodes a permissions argument into out.
// Returns false if the value is not an integer in the 0..07777 range.
static bool arg_perm(sqlite3_value* value, uint32_t* out) {
    int64_t n;
    if (!arg_int64(value, &n) || n < 0 || n > MAX_PERM) {
        return false;
    }
    *out = (uint32_t)n;
    return true;
}

// arg_data decodes a data argument as bytes: blobs as is, and other values
// as text. Returns false if SQLite runs out of memory.
static bool arg_data(sqlite3_value* value, const void** data, size_t* n) {
    // sqlite3_value_blob converts the value to bytes,
    // so it must be called before sqlite3_value_bytes.
    const void* buf = sqlite3_value_blob(value);
    int len = sqlite3_value_bytes(value);
    // An empty value has a NULL buffer, a non-empty one only if out of memory.
    if (buf == NULL && len > 0) {
        return false;
    }
    *data = buf;
    *n = (size_t)len;
    return true;
}

// decode_path decodes a path argument and handles a failure: an invalid path
// gives a NULL result, and running out of memory an error.
// Returns NULL on failure.
static const char* decode_path(sqlite3_context* context, sqlite3_value* value) {
    const char* path;
    switch (arg_path(value, &path)) {
        case ARG_OK:
            return path;
        case ARG_INVALID:
            sqlite3_result_null(context);
            return NULL;
        case ARG_NOMEM:
        default:
            sqlite3_result_error_nomem(context);
            return NULL;
    }
}

#pragma endregion

#pragma region Reading

// fileio_read(path[, offset[, limit]])
static void fn_read(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc >= 1 && argc <= 3);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* path = decode_path(context, argv[0]);
    if (path == NULL) {
        return;
    }
    int64_t offset = 0;
    if (argc >= 2 && (!arg_int64(argv[1], &offset) || offset < 0)) {
        sqlite3_result_null(context);
        return;
    }
    // limit 0 means no limit
    int64_t limit = 0;
    if (argc >= 3 && (!arg_int64(argv[2], &limit) || limit < 0)) {
        sqlite3_result_null(context);
        return;
    }

    // A file that doesn't exist, can't be read or is not a regular file is NULL.
    FILE* file;
    int err = fs_open(path, &file);
    if (err == ENOMEM) {
        sqlite3_result_error_nomem(context);
        return;
    }
    if (err != 0) {
        sqlite3_result_null(context);
        return;
    }
    int64_t size;
    if (fs_size(file, &size) != 0) {
        fclose(file);
        sqlite3_result_null(context);
        return;
    }

    int64_t n = offset < size ? size - offset : 0;
    if (limit > 0 && limit < n) {
        n = limit;
    }
    sqlite3* db = sqlite3_context_db_handle(context);
    if (n > sqlite3_limit(db, SQLITE_LIMIT_LENGTH, -1)) {
        fclose(file);
        sqlite3_result_error_toobig(context);
        return;
    }

    // sqlite3_malloc64(0) returns NULL, so allocate at least one byte.
    void* buf = sqlite3_malloc64(n > 0 ? (sqlite3_uint64)n : 1);
    if (buf == NULL) {
        fclose(file);
        sqlite3_result_error_nomem(context);
        return;
    }
    size_t nread = 0;
    err = n > 0 ? fs_read(file, offset, buf, (size_t)n, &nread) : 0;
    fclose(file);
    if (err != 0) {
        sqlite3_free(buf);
        sqlite3_result_error_code(context, SQLITE_IOERR);
        return;
    }
    // The file can shrink between getting its size and reading it.
    sqlite3_result_blob64(context, buf, nread, sqlite3_free);
}

#pragma endregion

#pragma region Writing

// fileio_write(path, data[, perm[, mtime]])
static void fn_write(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc >= 2 && argc <= 4);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* path = decode_path(context, argv[0]);
    if (path == NULL) {
        return;
    }
    const void* data;
    size_t n;
    if (!arg_data(argv[1], &data, &n)) {
        sqlite3_result_error_nomem(context);
        return;
    }
    uint32_t perm = 0666;
    if (argc >= 3 && !arg_perm(argv[2], &perm)) {
        sqlite3_result_null(context);
        return;
    }
    int64_t mtime;
    if (argc >= 4 && !arg_int64(argv[3], &mtime)) {
        sqlite3_result_null(context);
        return;
    }

    int err = fs_write(path, data, n, perm, argc >= 4 ? &mtime : NULL);
    // The system can't represent the time, so it's out of domain.
    if (err == EOVERFLOW) {
        sqlite3_result_null(context);
        return;
    }
    if (err != 0) {
        result_fs_error(context, err, "failed to write file: %s", path);
        return;
    }
    sqlite3_result_int64(context, (sqlite3_int64)n);
}

// close_file closes the file kept as auxiliary data.
static void close_file(void* file) {
    fclose((FILE*)file);
}

// fileio_append(path, data)
static void fn_append(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* path = decode_path(context, argv[0]);
    if (path == NULL) {
        return;
    }
    const void* data;
    size_t n;
    if (!arg_data(argv[1], &data, &n)) {
        sqlite3_result_error_nomem(context);
        return;
    }

    // SQLite keeps the file open between the calls while the path is constant,
    // and closes it at the end of the statement or when the path changes.
    FILE* file = sqlite3_get_auxdata(context, 0);
    if (file == NULL) {
        int err = fs_open_append(path, &file);
        if (err != 0) {
            result_fs_error(context, err, "failed to open file: %s", path);
            return;
        }
        // sqlite3_set_auxdata calls close_file right away if out of memory.
        sqlite3_set_auxdata(context, 0, file, close_file);
        file = sqlite3_get_auxdata(context, 0);
        if (file == NULL) {
            sqlite3_result_error_nomem(context);
            return;
        }
    }

    int err = fs_append(file, data, n);
    if (err == 0 && fflush(file) != 0) {
        err = errno != 0 ? errno : EIO;
    }
    if (err != 0) {
        result_fs_error(context, err, "failed to append to file: %s", path);
        return;
    }
    sqlite3_result_int64(context, (sqlite3_int64)n);
}

// fileio_symlink(target, path)
static void fn_symlink(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* target = decode_path(context, argv[0]);
    if (target == NULL) {
        return;
    }
    const char* path = decode_path(context, argv[1]);
    if (path == NULL) {
        return;
    }
    int err = fs_symlink(target, path);
    if (err != 0) {
        result_fs_error(context, err, "failed to create symlink: %s", path);
        return;
    }
    sqlite3_result_null(context);
}

#pragma endregion

#pragma region Directories

// fileio_mkdir(path[, perm])
static void fn_mkdir(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1 || argc == 2);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* path = decode_path(context, argv[0]);
    if (path == NULL) {
        return;
    }
    uint32_t perm = 0777;
    if (argc >= 2 && !arg_perm(argv[1], &perm)) {
        sqlite3_result_null(context);
        return;
    }
    int err = fs_mkdir(path, perm);
    if (err != 0) {
        result_fs_error(context, err, "failed to create directory: %s", path);
        return;
    }
    sqlite3_result_null(context);
}

#pragma endregion

#pragma region File mode

// fileio_mode(mode)
static void fn_mode(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    int64_t mode;
    if (!arg_int64(argv[0], &mode) || mode < 0 || mode > MAX_MODE) {
        sqlite3_result_null(context);
        return;
    }
    char buf[FS_MODE_SIZE];
    fs_mode_string((uint32_t)mode, buf);
    sqlite3_result_text(context, buf, -1, SQLITE_TRANSIENT);
}

#pragma endregion

// FLAGS are for functions that don't touch the file system.
#define FLAGS (SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC)
// FLAGS_FS are for functions that do: they can only be called
// from top-level SQL, not from views, triggers or schema.
#define FLAGS_FS (SQLITE_UTF8 | SQLITE_DIRECTONLY)

typedef struct {
    const char* name;
    int nargs;
    int flags;
    void (*fn)(sqlite3_context*, int, sqlite3_value**);
} Function;

static const Function functions[] = {
    {"fileio_read", 1, FLAGS_FS, fn_read},
    {"fileio_read", 2, FLAGS_FS, fn_read},
    {"fileio_read", 3, FLAGS_FS, fn_read},
    {"fileio_write", 2, FLAGS_FS, fn_write},
    {"fileio_write", 3, FLAGS_FS, fn_write},
    {"fileio_write", 4, FLAGS_FS, fn_write},
    {"fileio_append", 2, FLAGS_FS, fn_append},
    {"fileio_symlink", 2, FLAGS_FS, fn_symlink},
    {"fileio_mkdir", 1, FLAGS_FS, fn_mkdir},
    {"fileio_mkdir", 2, FLAGS_FS, fn_mkdir},
    {"fileio_mode", 1, FLAGS, fn_mode},

    // aliases for backward compatibility
    {"readfile", 1, FLAGS_FS, fn_read},
    {"readfile", 2, FLAGS_FS, fn_read},
    {"readfile", 3, FLAGS_FS, fn_read},
    {"writefile", 2, FLAGS_FS, fn_write},
    {"writefile", 3, FLAGS_FS, fn_write},
    {"writefile", 4, FLAGS_FS, fn_write},
    {"symlink", 2, FLAGS_FS, fn_symlink},
    {"mkdir", 1, FLAGS_FS, fn_mkdir},
    {"mkdir", 2, FLAGS_FS, fn_mkdir},
    {"lsmode", 1, FLAGS, fn_mode},
};

int fileio_init(sqlite3* db) {
    for (size_t i = 0; i < sizeof(functions) / sizeof(functions[0]); i++) {
        const Function* f = &functions[i];
        int rc = sqlite3_create_function(db, f->name, f->nargs, f->flags, 0, f->fn, 0, 0);
        if (rc != SQLITE_OK) {
            return rc;
        }
    }
    int rc = fileio_ls_init(db);
    if (rc != SQLITE_OK) {
        return rc;
    }
    return fileio_scan_init(db);
}
