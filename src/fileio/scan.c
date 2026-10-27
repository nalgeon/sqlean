// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// fileio_scan(path)
// Reads the file at path line by line.
// Implemented as a table-valued function.

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT3

#include "fileio/binding.h"
#include "fileio/fileio.h"

#define COLUMN_VALUE 0
#define COLUMN_NAME 1

// The query plans chosen by xBestIndex and passed to xFilter.
#define PLAN_NO_NAME 0
#define PLAN_NAME 1

// INITIAL_CAP is the initial size of the line buffer.
#define INITIAL_CAP 128

typedef struct {
    sqlite3_vtab base;
    sqlite3* db;
    // the name the table function was called by
    char* name;
} Table;

typedef struct {
    sqlite3_vtab_cursor base;
    // the argument, returned as the hidden column
    sqlite3_value* name_arg;
    FILE* file;
    bool eof;
    // the current line, with its end-of-line marker
    char* line;
    size_t len;
    size_t cap;
    sqlite3_int64 rowid;
} Cursor;

// xconnect creates the virtual table.
static int xconnect(sqlite3* db,
                    void* aux,
                    int argc,
                    const char* const* argv,
                    sqlite3_vtab** vtabptr,
                    char** errptr) {
    (void)aux;
    (void)argc;
    (void)errptr;

    int rc = sqlite3_declare_vtab(db, "CREATE TABLE x(value text, name hidden)");
    if (rc != SQLITE_OK) {
        return rc;
    }
    Table* table = sqlite3_malloc64(sizeof(*table));
    if (table == NULL) {
        return SQLITE_NOMEM;
    }
    memset(table, 0, sizeof(*table));
    table->db = db;
    // argv[0] is the module name: fileio_scan or an alias.
    table->name = sqlite3_mprintf("%s", argv[0]);
    if (table->name == NULL) {
        sqlite3_free(table);
        return SQLITE_NOMEM;
    }
    sqlite3_vtab_config(db, SQLITE_VTAB_DIRECTONLY);
    *vtabptr = &table->base;
    return SQLITE_OK;
}

// xdisconnect destroys the virtual table.
static int xdisconnect(sqlite3_vtab* vtab) {
    Table* table = (Table*)vtab;
    sqlite3_free(table->name);
    sqlite3_free(table);
    return SQLITE_OK;
}

// xopen creates a new cursor.
static int xopen(sqlite3_vtab* vtab, sqlite3_vtab_cursor** curptr) {
    (void)vtab;
    Cursor* cursor = sqlite3_malloc64(sizeof(*cursor));
    if (cursor == NULL) {
        return SQLITE_NOMEM;
    }
    memset(cursor, 0, sizeof(*cursor));
    cursor->eof = true;
    *curptr = &cursor->base;
    return SQLITE_OK;
}

// reset frees the cursor resources and moves it to the end.
static void reset(Cursor* cursor) {
    if (cursor->file != NULL) {
        fclose(cursor->file);
        cursor->file = NULL;
    }
    sqlite3_free(cursor->line);
    cursor->line = NULL;
    cursor->len = 0;
    cursor->cap = 0;
    sqlite3_value_free(cursor->name_arg);
    cursor->name_arg = NULL;
    cursor->eof = true;
    cursor->rowid = 0;
}

// xclose destroys the cursor.
static int xclose(sqlite3_vtab_cursor* cur) {
    Cursor* cursor = (Cursor*)cur;
    reset(cursor);
    sqlite3_free(cursor);
    return SQLITE_OK;
}

// grow doubles the line buffer, up to the maximum length of a value.
static int grow(Cursor* cursor) {
    Table* table = (Table*)cursor->base.pVtab;
    sqlite3_uint64 max = (sqlite3_uint64)sqlite3_limit(table->db, SQLITE_LIMIT_LENGTH, -1);
    // A line of max bytes, plus a \r\n to trim.
    max += 2;
    if (cursor->cap >= max) {
        return SQLITE_TOOBIG;
    }
    sqlite3_uint64 cap = cursor->cap == 0 ? INITIAL_CAP : (sqlite3_uint64)cursor->cap * 2;
    if (cap > max) {
        cap = max;
    }
    char* line = sqlite3_realloc64(cursor->line, cap);
    if (line == NULL) {
        return SQLITE_NOMEM;
    }
    cursor->line = line;
    cursor->cap = (size_t)cap;
    return SQLITE_OK;
}

// xnext reads the next line.
static int xnext(sqlite3_vtab_cursor* cur) {
    Cursor* cursor = (Cursor*)cur;
    cursor->rowid++;
    cursor->len = 0;
    for (;;) {
        if (cursor->len == cursor->cap) {
            int rc = grow(cursor);
            if (rc != SQLITE_OK) {
                return rc;
            }
        }
        size_t n;
        bool done;
        int err = fs_read_line(cursor->file, cursor->line + cursor->len, cursor->cap - cursor->len,
                               &n, &done);
        if (err != 0) {
            return SQLITE_IOERR;
        }
        cursor->len += n;
        if (done) {
            break;
        }
    }
    if (cursor->len == 0) {
        cursor->eof = true;
    }
    return SQLITE_OK;
}

// xcolumn returns a column value of the current line.
static int xcolumn(sqlite3_vtab_cursor* cur, sqlite3_context* ctx, int col) {
    Cursor* cursor = (Cursor*)cur;
    switch (col) {
        case COLUMN_VALUE: {
            size_t len = fs_trim_eol(cursor->line, cursor->len);
            sqlite3_result_text64(ctx, cursor->line, len, SQLITE_TRANSIENT, SQLITE_UTF8);
            break;
        }
        case COLUMN_NAME:
            sqlite3_result_value(ctx, cursor->name_arg);
            break;
        default:
            break;
    }
    return SQLITE_OK;
}

// xrowid returns the rowid of the current line: 1 for the first one,
// and one more for each next one.
static int xrowid(sqlite3_vtab_cursor* cur, sqlite3_int64* rowid) {
    Cursor* cursor = (Cursor*)cur;
    *rowid = cursor->rowid;
    return SQLITE_OK;
}

// xeof reports whether the cursor is past the last line.
static int xeof(sqlite3_vtab_cursor* cur) {
    Cursor* cursor = (Cursor*)cur;
    return cursor->eof;
}

// xfilter opens the file given in the argument and reads the first line.
// A bad argument (NULL or a path that is not text) and a file that doesn't
// exist, can't be read or is a directory give no rows.
static int xfilter(sqlite3_vtab_cursor* cur,
                   int idx_num,
                   const char* idx_str,
                   int argc,
                   sqlite3_value** argv) {
    (void)idx_str;
    Cursor* cursor = (Cursor*)cur;
    reset(cursor);

    if (idx_num == PLAN_NO_NAME) {
        Table* table = (Table*)cursor->base.pVtab;
        sqlite3_free(table->base.zErrMsg);
        table->base.zErrMsg =
            sqlite3_mprintf("table function %s requires an argument", table->name);
        return table->base.zErrMsg == NULL ? SQLITE_NOMEM : SQLITE_ERROR;
    }

    if (has_null(argc, argv)) {
        return SQLITE_OK;
    }
    const char* path;
    ArgStatus status = arg_path(argv[0], &path);
    if (status == ARG_NOMEM) {
        return SQLITE_NOMEM;
    }
    if (status == ARG_INVALID) {
        return SQLITE_OK;
    }
    int err = fs_open(path, &cursor->file);
    if (err == ENOMEM) {
        return SQLITE_NOMEM;
    }
    if (err != 0) {
        cursor->file = NULL;
        return SQLITE_OK;
    }
    cursor->name_arg = sqlite3_value_dup(argv[0]);
    if (cursor->name_arg == NULL) {
        reset(cursor);
        return SQLITE_NOMEM;
    }
    cursor->eof = false;
    return xnext(cur);
}

// xbest_index passes the name argument to xFilter. Other constraints
// are left to SQLite.
static int xbest_index(sqlite3_vtab* vtab, sqlite3_index_info* info) {
    (void)vtab;
    int name_idx = -1;
    bool unusable = false;
    for (int i = 0; i < info->nConstraint; i++) {
        const struct sqlite3_index_constraint* c = &info->aConstraint[i];
        if (c->op != SQLITE_INDEX_CONSTRAINT_EQ || c->iColumn != COLUMN_NAME) {
            continue;
        }
        if (c->usable) {
            name_idx = i;
        } else {
            unusable = true;
        }
    }

    // A plan without a usable argument can't be executed,
    // so SQLite has to choose another one.
    if (unusable && name_idx < 0) {
        return SQLITE_CONSTRAINT;
    }

    if (name_idx < 0) {
        // xFilter reports the missing argument.
        info->idxNum = PLAN_NO_NAME;
        info->estimatedRows = 0x7fffffff;
        return SQLITE_OK;
    }

    info->aConstraintUsage[name_idx].argvIndex = 1;
    info->aConstraintUsage[name_idx].omit = 1;
    info->idxNum = PLAN_NAME;
    info->estimatedCost = 1000.0;
    info->estimatedRows = 1000;
    return SQLITE_OK;
}

static sqlite3_module scan_module = {
    .xConnect = xconnect,
    .xBestIndex = xbest_index,
    .xDisconnect = xdisconnect,
    .xOpen = xopen,
    .xClose = xclose,
    .xFilter = xfilter,
    .xNext = xnext,
    .xEof = xeof,
    .xColumn = xcolumn,
    .xRowid = xrowid,
};

int fileio_scan_init(sqlite3* db) {
    int rc = sqlite3_create_module(db, "fileio_scan", &scan_module, 0);
    if (rc != SQLITE_OK) {
        return rc;
    }
    // alias for backward compatibility
    return sqlite3_create_module(db, "scanfile", &scan_module, 0);
}
