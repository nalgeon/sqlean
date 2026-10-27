// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Based on fileio.c by D. Richard Hipp, Public Domain
// https://www.sqlite.org/src/file/ext/misc/fileio.c

// fileio_ls(path[, recursive])
// Lists the file at path, and the files in it if it's a directory.
// Implemented as a table-valued function.

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT3

#include "fileio/binding.h"
#include "fileio/fileio.h"

#define COLUMN_NAME 0
#define COLUMN_MODE 1
#define COLUMN_MTIME 2
#define COLUMN_SIZE 3
#define COLUMN_PATH 4
#define COLUMN_DIR 5

// The query plans chosen by xBestIndex and passed to xFilter.
#define PLAN_NO_PATH 0
#define PLAN_PATH 1

typedef struct {
    sqlite3_vtab base;
    // the name the table function was called by
    char* name;
} Table;

// Level is a directory being listed.
typedef struct {
    FsDir* dir;
    char* path;
} Level;

typedef struct {
    sqlite3_vtab_cursor base;
    // the arguments, returned as the hidden columns
    sqlite3_value* path_arg;
    sqlite3_value* dir_arg;
    bool recursive;
    // the directories being listed, from the top down
    Level* levels;
    int nlevels;
    int cap_levels;
    // the current entry, NULL at the end
    char* path;
    FileInfo info;
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

    int rc = sqlite3_declare_vtab(
        db, "CREATE TABLE x(name, mode, mtime, size, path HIDDEN, dir HIDDEN)");
    if (rc != SQLITE_OK) {
        return rc;
    }
    Table* table = sqlite3_malloc64(sizeof(*table));
    if (table == NULL) {
        return SQLITE_NOMEM;
    }
    memset(table, 0, sizeof(*table));
    // argv[0] is the module name: fileio_ls or an alias.
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
    *curptr = &cursor->base;
    return SQLITE_OK;
}

// reset frees the cursor resources and moves it to the end.
static void reset(Cursor* cursor) {
    for (int i = 0; i < cursor->nlevels; i++) {
        fs_dir_close(cursor->levels[i].dir);
        sqlite3_free(cursor->levels[i].path);
    }
    sqlite3_free(cursor->levels);
    cursor->levels = NULL;
    cursor->nlevels = 0;
    cursor->cap_levels = 0;
    sqlite3_free(cursor->path);
    cursor->path = NULL;
    sqlite3_value_free(cursor->path_arg);
    cursor->path_arg = NULL;
    sqlite3_value_free(cursor->dir_arg);
    cursor->dir_arg = NULL;
    cursor->recursive = false;
    cursor->rowid = 0;
}

// xclose destroys the cursor.
static int xclose(sqlite3_vtab_cursor* cur) {
    Cursor* cursor = (Cursor*)cur;
    reset(cursor);
    sqlite3_free(cursor);
    return SQLITE_OK;
}

// is_dir reports whether the file mode is a directory.
static bool is_dir(uint32_t mode) {
    return (mode & 0170000) == 0040000;
}

// descend starts listing the directory at the current path.
// A directory that can't be read is not listed.
static int descend(Cursor* cursor) {
    FsDir* dir;
    int err = fs_dir_open(cursor->path, &dir);
    if (err == ENOMEM) {
        return SQLITE_NOMEM;
    }
    if (err != 0) {
        return SQLITE_OK;
    }
    if (cursor->nlevels == cursor->cap_levels) {
        int cap = cursor->cap_levels == 0 ? 4 : cursor->cap_levels * 2;
        Level* levels = sqlite3_realloc64(cursor->levels, (sqlite3_uint64)cap * sizeof(Level));
        if (levels == NULL) {
            fs_dir_close(dir);
            return SQLITE_NOMEM;
        }
        cursor->levels = levels;
        cursor->cap_levels = cap;
    }
    // The level takes over the path.
    Level* level = &cursor->levels[cursor->nlevels++];
    level->dir = dir;
    level->path = cursor->path;
    cursor->path = NULL;
    return SQLITE_OK;
}

// ascend stops listing the innermost directory.
static void ascend(Cursor* cursor) {
    Level* level = &cursor->levels[--cursor->nlevels];
    fs_dir_close(level->dir);
    sqlite3_free(level->path);
}

// join returns the path of the entry name in the directory dir,
// or NULL if out of memory.
static char* join(const char* dir, const char* name) {
    size_t len = fs_join(NULL, 0, dir, name);
    char* path = sqlite3_malloc64(len + 1);
    if (path == NULL) {
        return NULL;
    }
    fs_join(path, len + 1, dir, name);
    return path;
}

// xnext advances the cursor to the next entry. An entry that disappears
// or otherwise can't be described is skipped.
static int xnext(sqlite3_vtab_cursor* cur) {
    Cursor* cursor = (Cursor*)cur;
    cursor->rowid++;

    // The root is always listed, the nested directories only if recursive.
    if (is_dir(cursor->info.mode) && (cursor->nlevels == 0 || cursor->recursive)) {
        int rc = descend(cursor);
        if (rc != SQLITE_OK) {
            return rc;
        }
    }
    sqlite3_free(cursor->path);
    cursor->path = NULL;

    while (cursor->nlevels > 0) {
        Level* level = &cursor->levels[cursor->nlevels - 1];
        const char* name;
        int err = fs_dir_next(level->dir, &name);
        if (err == ENOMEM) {
            return SQLITE_NOMEM;
        }
        // A read error ends the directory, as if it were the last entry.
        if (err != 0 || name == NULL) {
            ascend(cursor);
            continue;
        }
        char* path = join(level->path, name);
        if (path == NULL) {
            return SQLITE_NOMEM;
        }
        err = fs_stat(path, &cursor->info);
        if (err == ENOMEM) {
            sqlite3_free(path);
            return SQLITE_NOMEM;
        }
        if (err != 0) {
            sqlite3_free(path);
            continue;
        }
        cursor->path = path;
        return SQLITE_OK;
    }
    return SQLITE_OK;
}

// xcolumn returns a column value of the current entry.
static int xcolumn(sqlite3_vtab_cursor* cur, sqlite3_context* ctx, int col) {
    Cursor* cursor = (Cursor*)cur;
    switch (col) {
        case COLUMN_NAME:
            sqlite3_result_text(ctx, cursor->path, -1, SQLITE_TRANSIENT);
            break;
        case COLUMN_MODE:
            sqlite3_result_int64(ctx, cursor->info.mode);
            break;
        case COLUMN_MTIME:
            sqlite3_result_int64(ctx, cursor->info.mtime);
            break;
        case COLUMN_SIZE:
            sqlite3_result_int64(ctx, cursor->info.size);
            break;
        case COLUMN_PATH:
            if (cursor->path_arg != NULL) {
                sqlite3_result_value(ctx, cursor->path_arg);
            }
            break;
        case COLUMN_DIR:
            if (cursor->dir_arg != NULL) {
                sqlite3_result_value(ctx, cursor->dir_arg);
            }
            break;
        default:
            break;
    }
    return SQLITE_OK;
}

// xrowid returns the rowid of the current entry: 1 for the first one,
// and one more for each next one.
static int xrowid(sqlite3_vtab_cursor* cur, sqlite3_int64* rowid) {
    Cursor* cursor = (Cursor*)cur;
    *rowid = cursor->rowid;
    return SQLITE_OK;
}

// xeof reports whether the cursor is past the last entry.
static int xeof(sqlite3_vtab_cursor* cur) {
    Cursor* cursor = (Cursor*)cur;
    return cursor->path == NULL;
}

// xfilter starts listing from the path given in the arguments. Bad arguments
// (NULL, a path that is not text, a non-integer recursive flag) and a path
// that doesn't exist give no rows.
static int xfilter(sqlite3_vtab_cursor* cur,
                   int idx_num,
                   const char* idx_str,
                   int argc,
                   sqlite3_value** argv) {
    (void)idx_str;
    Cursor* cursor = (Cursor*)cur;
    reset(cursor);

    if (idx_num == PLAN_NO_PATH) {
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
    if (argc >= 2) {
        int64_t recursive;
        if (!arg_int64(argv[1], &recursive)) {
            return SQLITE_OK;
        }
        cursor->recursive = recursive != 0;
    }

    cursor->path_arg = sqlite3_value_dup(argv[0]);
    cursor->dir_arg = argc >= 2 ? sqlite3_value_dup(argv[1]) : NULL;
    cursor->path = sqlite3_mprintf("%s", path);
    if (cursor->path_arg == NULL || (argc >= 2 && cursor->dir_arg == NULL) ||
        cursor->path == NULL) {
        reset(cursor);
        return SQLITE_NOMEM;
    }

    int err = fs_stat(cursor->path, &cursor->info);
    if (err != 0) {
        sqlite3_free(cursor->path);
        cursor->path = NULL;
        return err == ENOMEM ? SQLITE_NOMEM : SQLITE_OK;
    }
    cursor->rowid = 1;
    return SQLITE_OK;
}

// xbest_index passes the path and dir arguments to xFilter.
static int xbest_index(sqlite3_vtab* vtab, sqlite3_index_info* info) {
    (void)vtab;
    int path_idx = -1;
    int dir_idx = -1;
    bool unusable_path = false;
    bool unusable_dir = false;
    for (int i = 0; i < info->nConstraint; i++) {
        const struct sqlite3_index_constraint* c = &info->aConstraint[i];
        if (c->op != SQLITE_INDEX_CONSTRAINT_EQ) {
            continue;
        }
        if (c->iColumn != COLUMN_PATH && c->iColumn != COLUMN_DIR) {
            continue;
        }
        bool is_path = c->iColumn == COLUMN_PATH;
        if (!c->usable) {
            *(is_path ? &unusable_path : &unusable_dir) = true;
        } else if (is_path) {
            path_idx = i;
        } else {
            dir_idx = i;
        }
    }

    // A plan without a usable argument can't be executed,
    // so SQLite has to choose another one.
    if ((unusable_path && path_idx < 0) || (unusable_dir && dir_idx < 0)) {
        return SQLITE_CONSTRAINT;
    }

    if (path_idx < 0) {
        // xFilter reports the missing argument.
        info->idxNum = PLAN_NO_PATH;
        info->estimatedRows = 0x7fffffff;
        return SQLITE_OK;
    }

    info->aConstraintUsage[path_idx].argvIndex = 1;
    info->aConstraintUsage[path_idx].omit = 1;
    if (dir_idx >= 0) {
        info->aConstraintUsage[dir_idx].argvIndex = 2;
        info->aConstraintUsage[dir_idx].omit = 1;
    }
    info->idxNum = PLAN_PATH;
    info->estimatedCost = 100.0;
    return SQLITE_OK;
}

static sqlite3_module ls_module = {
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

int fileio_ls_init(sqlite3* db) {
    int rc = sqlite3_create_module(db, "fileio_ls", &ls_module, 0);
    if (rc != SQLITE_OK) {
        return rc;
    }
    // alias for backward compatibility
    return sqlite3_create_module(db, "lsdir", &ls_module, 0);
}
