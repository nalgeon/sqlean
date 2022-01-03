// Copyright (c) 2022 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// unnest(arr)
// Expands the array to a set of values (reverse operation for array_agg()).
// Implemented as a table-valued function.

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "extension.h"

#include "../sqlite3ext.h"
SQLITE_EXTENSION_INIT3

typedef struct {
    sqlite3_vtab_cursor base;
    sqlite3_int64 rowid;
    Array* arr;
} Cursor;

// Creates the virtual table.
static int xconnect(sqlite3* db,
                    void* aux,
                    int argc,
                    const char* const* argv,
                    sqlite3_vtab** vtabptr,
                    char** errptr) {
    (void)aux;
    (void)argc;
    (void)argv;
    (void)errptr;
    int rc = sqlite3_declare_vtab(db, "CREATE TABLE x(value, data hidden)");
    if (rc != SQLITE_OK) {
        return rc;
    }

    sqlite3_vtab* vtable = *vtabptr = sqlite3_malloc(sizeof(*vtable));
    if (vtable == NULL) {
        return SQLITE_NOMEM;
    }
    memset(vtable, 0, sizeof(*vtable));
    sqlite3_vtab_config(db, SQLITE_VTAB_INNOCUOUS);
    return rc;
}

// Destroys the virtual table.
static int xdisconnect(sqlite3_vtab* vtable) {
    sqlite3_free(vtable);
    return SQLITE_OK;
}

// Creates a new cursor.
static int xopen(sqlite3_vtab* vtable, sqlite3_vtab_cursor** curptr) {
    (void)vtable;
    Cursor* cursor = sqlite3_malloc(sizeof(*cursor));
    if (cursor == NULL) {
        return SQLITE_NOMEM;
    }
    memset(cursor, 0, sizeof(*cursor));
    *curptr = &cursor->base;
    return SQLITE_OK;
}

// Destroys the cursor.
static int xclose(sqlite3_vtab_cursor* cur) {
    Cursor* cursor = (Cursor*)cur;
    array.free(cursor->arr);
    sqlite3_free(cur);
    return SQLITE_OK;
}

// Advances the cursor to its next row of output.
static int xnext(sqlite3_vtab_cursor* cur) {
    Cursor* cursor = (Cursor*)cur;
    cursor->rowid++;
    return SQLITE_OK;
}

// Returns the current cursor value.
static int xcolumn(sqlite3_vtab_cursor* cur, sqlite3_context* ctx, int col_idx) {
    (void)col_idx;
    Cursor* cursor = (Cursor*)cur;

    // rowid is 1-based, array is 0-based
    uint8_t* valptr = array.at(cursor->arr, cursor->rowid - 1);
    if (valptr == NULL) {
        return SQLITE_ERROR;
    }

    switch (cursor->arr->type) {
        case TYPE_INT:
            sqlite3_result_int64(ctx, *((int64_t*)valptr));
            break;
        case TYPE_REAL:
            sqlite3_result_double(ctx, *((double*)valptr));
            break;
        case TYPE_TEXT:
            sqlite3_result_text(ctx, (const char*)valptr, -1, SQLITE_TRANSIENT);
            break;
        default:
            break;
    }

    return SQLITE_OK;
}

// Returns the rowid for the current row.
static int xrowid(sqlite3_vtab_cursor* cur, sqlite_int64* rowid_ptr) {
    Cursor* cursor = (Cursor*)cur;
    *rowid_ptr = cursor->rowid;
    return SQLITE_OK;
}

// Returns TRUE if the cursor has been moved off of the last row of output.
static int xeof(sqlite3_vtab_cursor* cur) {
    Cursor* cursor = (Cursor*)cur;
    // rowid is 1-based, array is 0-based
    return cursor->rowid > cursor->arr->length;
}

// Rewinds the cursor back to the first row of output.
static int xfilter(sqlite3_vtab_cursor* cur,
                   int idx_num,
                   const char* idx_str,
                   int argc,
                   sqlite3_value** argv) {
    (void)idx_num;
    (void)idx_str;

    if (argc != 1) {
        return SQLITE_ERROR;
    }

    uint8_t* blob = (uint8_t*)sqlite3_value_blob(argv[0]);
    if (blob == NULL) {
        return SQLITE_ERROR;
    }
    Array* arr = array.from_blob(blob);
    if (arr == NULL) {
        return SQLITE_ERROR;
    }

    Cursor* cursor = (Cursor*)cur;
    cursor->rowid = 1;
    cursor->arr = arr;
    return SQLITE_OK;
}

// Instructs SQLite to pass the first argument to xFilter.
static int xbest_index(sqlite3_vtab* vtable, sqlite3_index_info* index_info) {
    bool unusable = false;
    const struct sqlite3_index_constraint* constraint = index_info->aConstraint;
    for (int i = 0; i < index_info->nConstraint; i++, constraint++) {
        if (constraint->usable == 0 || constraint->op != SQLITE_INDEX_CONSTRAINT_EQ) {
            unusable = true;
            break;
        }
    }
    if (unusable) {
        // If there are unusable constraints then this plan is unusable
        // Happens with joins, when xbest_index is called multiple times.
        return SQLITE_CONSTRAINT;
    }

    if (index_info->nConstraint != 1) {
        vtable->zErrMsg = sqlite3_mprintf("unnest() expects a single argument");
        return SQLITE_ERROR;
    }

    // Pass the first argument to xFilter
    index_info->aConstraintUsage[0].argvIndex = 1;
    index_info->aConstraintUsage[0].omit = 1;
    index_info->estimatedCost = (double)10;
    index_info->estimatedRows = 1000;
    return SQLITE_OK;
}

static sqlite3_module module = {.iVersion = 0,
                                .xCreate = 0,
                                .xConnect = xconnect,
                                .xBestIndex = xbest_index,
                                .xDisconnect = xdisconnect,
                                .xDestroy = 0,
                                .xOpen = xopen,
                                .xClose = xclose,
                                .xFilter = xfilter,
                                .xNext = xnext,
                                .xEof = xeof,
                                .xColumn = xcolumn,
                                .xRowid = xrowid,
                                .xUpdate = 0,
                                .xBegin = 0,
                                .xSync = 0,
                                .xCommit = 0,
                                .xRollback = 0,
                                .xFindFunction = 0,
                                .xRename = 0,
                                .xSavepoint = 0,
                                .xRelease = 0,
                                .xRollbackTo = 0,
                                .xShadowName = 0};

int unnest_init(sqlite3* db) {
    sqlite3_create_module(db, "unnest", &module, 0);
    return SQLITE_OK;
}
