// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Based on series.c by D. Richard Hipp, Public Domain
// https://sqlite.org/src/file/ext/misc/series.c

// stats_seq(start[, stop[, step]])
// Generates a series of integers from start to stop, step apart.
// Implemented as a table-valued function.

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT3

#include "stats/binding.h"
#include "stats/common.h"
#include "stats/stats.h"

#define COLUMN_VALUE 0
#define COLUMN_START 1
#define COLUMN_STOP 2
#define COLUMN_STEP 3

// The query plan chosen by xBestIndex and passed to xFilter
// is a bitmask of the arguments it has.
#define PLAN_START 1
#define PLAN_STOP 2
#define PLAN_STEP 4

// The default stop is 2^32-1, as in SQLite's generate_series.
#define DEFAULT_STOP 4294967295
#define DEFAULT_STEP 1

typedef struct {
    sqlite3_vtab base;
    // the name the table function was called by
    char* name;
} Table;

typedef struct {
    sqlite3_vtab_cursor base;
    // the arguments, with the defaults, returned as the hidden columns
    int64_t start;
    int64_t stop;
    int64_t step;
    Series series;
    // the index of the current value in the series
    uint64_t index;
    bool eof;
} Cursor;

// arg_int64 decodes an integer argument into out, converting numeric text.
// A real is accepted if it's a whole number, so stats_seq(1, 1e6) works.
// Returns false if the value is not an integer.
static bool arg_int64(sqlite3_value* value, int64_t* out) {
    switch (sqlite3_value_numeric_type(value)) {
        case SQLITE_INTEGER:
            *out = sqlite3_value_int64(value);
            return true;
        case SQLITE_FLOAT: {
            double d = sqlite3_value_double(value);
            // Converting a double outside the int64 range is UB.
            // 2^63 is exact as a double, unlike INT64_MAX.
            if (!(d >= -9223372036854775808.0 && d < 9223372036854775808.0) || d != trunc(d)) {
                return false;
            }
            *out = (int64_t)d;
            return true;
        }
        default:
            return false;
    }
}

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

    int rc =
        sqlite3_declare_vtab(db, "CREATE TABLE x(value, start hidden, stop hidden, step hidden)");
    if (rc != SQLITE_OK) {
        return rc;
    }
    Table* table = sqlite3_malloc64(sizeof(*table));
    if (table == NULL) {
        return SQLITE_NOMEM;
    }
    memset(table, 0, sizeof(*table));
    // argv[0] is the module name: stats_seq or an alias.
    table->name = sqlite3_mprintf("%s", argv[0]);
    if (table->name == NULL) {
        sqlite3_free(table);
        return SQLITE_NOMEM;
    }
    sqlite3_vtab_config(db, SQLITE_VTAB_INNOCUOUS);
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

// xclose destroys the cursor.
static int xclose(sqlite3_vtab_cursor* cur) {
    sqlite3_free(cur);
    return SQLITE_OK;
}

// xnext moves to the next value.
static int xnext(sqlite3_vtab_cursor* cur) {
    Cursor* cursor = (Cursor*)cur;
    // Checking before incrementing, the index never overflows.
    if (cursor->index == cursor->series.last) {
        cursor->eof = true;
    } else {
        cursor->index++;
    }
    return SQLITE_OK;
}

// xcolumn returns a column value of the current row.
static int xcolumn(sqlite3_vtab_cursor* cur, sqlite3_context* ctx, int col) {
    Cursor* cursor = (Cursor*)cur;
    switch (col) {
        case COLUMN_VALUE:
            sqlite3_result_int64(ctx, series_at(cursor->series, cursor->index));
            break;
        case COLUMN_START:
            sqlite3_result_int64(ctx, cursor->start);
            break;
        case COLUMN_STOP:
            sqlite3_result_int64(ctx, cursor->stop);
            break;
        case COLUMN_STEP:
            sqlite3_result_int64(ctx, cursor->step);
            break;
        default:
            break;
    }
    return SQLITE_OK;
}

// xrowid returns the rowid of the current row: 1 for the first one,
// and one more for each next one.
static int xrowid(sqlite3_vtab_cursor* cur, sqlite3_int64* rowid) {
    Cursor* cursor = (Cursor*)cur;
    // index < last <= UINT64_MAX here, so index + 1 doesn't wrap;
    // past INT64_MAX rows (never reached in practice) it becomes negative.
    *rowid = uint64_to_int64(cursor->index + 1);
    return SQLITE_OK;
}

// xeof reports whether the cursor is past the last row.
static int xeof(sqlite3_vtab_cursor* cur) {
    Cursor* cursor = (Cursor*)cur;
    return cursor->eof;
}

// xfilter starts the series given in the arguments.
// A bad argument (NULL or not an integer) gives no rows.
static int xfilter(sqlite3_vtab_cursor* cur,
                   int idx_num,
                   const char* idx_str,
                   int argc,
                   sqlite3_value** argv) {
    (void)idx_str;
    Cursor* cursor = (Cursor*)cur;
    cursor->eof = true;
    cursor->index = 0;

    if ((idx_num & PLAN_START) == 0) {
        Table* table = (Table*)cursor->base.pVtab;
        sqlite3_free(table->base.zErrMsg);
        table->base.zErrMsg =
            sqlite3_mprintf("table function %s requires an argument", table->name);
        return table->base.zErrMsg == NULL ? SQLITE_NOMEM : SQLITE_ERROR;
    }

    // start, stop and step, in the order xBestIndex passes them.
    int64_t args[3] = {0, DEFAULT_STOP, DEFAULT_STEP};
    static const int plans[3] = {PLAN_START, PLAN_STOP, PLAN_STEP};
    int argi = 0;
    for (int i = 0; i < 3; i++) {
        if ((idx_num & plans[i]) == 0) {
            continue;
        }
        if (argi >= argc || !arg_int64(argv[argi++], &args[i])) {
            return SQLITE_OK;
        }
    }

    cursor->start = args[0];
    cursor->stop = args[1];
    cursor->step = args[2];
    cursor->series = series_make(cursor->start, cursor->stop, cursor->step);
    cursor->eof = cursor->series.empty;
    return SQLITE_OK;
}

// xbest_index passes the start, stop and step arguments to xFilter.
// Other constraints, LIMIT and ORDER BY are left to SQLite.
static int xbest_index(sqlite3_vtab* vtab, sqlite3_index_info* info) {
    (void)vtab;
    // The constraint index for each of start, stop and step.
    int idx[3] = {-1, -1, -1};
    bool unusable[3] = {false, false, false};
    for (int i = 0; i < info->nConstraint; i++) {
        const struct sqlite3_index_constraint* c = &info->aConstraint[i];
        if (c->op != SQLITE_INDEX_CONSTRAINT_EQ || c->iColumn < COLUMN_START) {
            continue;
        }
        int arg = c->iColumn - COLUMN_START;
        if (c->usable) {
            idx[arg] = i;
        } else {
            unusable[arg] = true;
        }
    }

    // A plan without a usable argument can't be executed,
    // so SQLite has to choose another one.
    int plan = 0;
    int argc = 0;
    for (int i = 0; i < 3; i++) {
        if (idx[i] < 0) {
            if (unusable[i]) {
                return SQLITE_CONSTRAINT;
            }
            continue;
        }
        plan |= 1 << i;
        info->aConstraintUsage[idx[i]].argvIndex = ++argc;
        info->aConstraintUsage[idx[i]].omit = 1;
    }
    info->idxNum = plan;

    if ((plan & PLAN_START) == 0) {
        // xFilter reports the missing argument.
        info->estimatedRows = 0x7fffffff;
        return SQLITE_OK;
    }
    if (plan & PLAN_STOP) {
        info->estimatedCost = (plan & PLAN_STEP) ? 1.0 : 2.0;
        info->estimatedRows = 1000;
    } else {
        // The default stop makes a huge series.
        info->estimatedRows = 0x7fffffff;
    }
    return SQLITE_OK;
}

static sqlite3_module seq_module = {
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

int stats_seq_init(sqlite3* db) {
    int rc = sqlite3_create_module(db, "stats_seq", &seq_module, 0);
    if (rc != SQLITE_OK) {
        return rc;
    }
    // alias as in SQLite and Postgres
    return sqlite3_create_module(db, "generate_series", &seq_module, 0);
}
