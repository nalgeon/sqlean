// Copyright (c) 2022 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// array_agg(expr)
// Aggregates a set of values into the array (reverse operation for unnest()).

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "extension.h"

#include "../sqlite3ext.h"
SQLITE_EXTENSION_INIT3

// Infers array type and element size.
// Infers the type from the type of the first argument.
// For numbers, infers the size from the type.
// For strings, sets the size equal to the value of the second argument.
static void infer_type(Array* arr, int argc, sqlite3_value** argv) {
    int value_type = sqlite3_value_type(argv[0]);
    if (value_type == SQLITE_INTEGER) {
        arr->type = TYPE_INT;
        arr->el_size = array.infer_size(arr->type);
        return;
    }
    if (value_type == SQLITE_FLOAT) {
        arr->type = TYPE_REAL;
        arr->el_size = array.infer_size(arr->type);
        return;
    }
    if (value_type == SQLITE_TEXT) {
        if (argc != 2 || sqlite3_value_type(argv[1]) != SQLITE_INTEGER) {
            return;
        }
        arr->type = TYPE_TEXT;
        arr->el_size = sqlite3_value_int(argv[1]) + 1;
        return;
    }
}

// Adds a row to aggregate.
static void xstep(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1 || argc == 2);

    Array* arr = sqlite3_aggregate_context(context, sizeof(Array));
    if (arr == NULL) {
        return;
    }
    if (arr->type == 0) {
        infer_type(arr, argc, argv);
    }

    if (sqlite3_value_type(argv[0]) != arr->type) {
        return;
    }

    STATUS err = STATUS_OK;
    if (arr->type == TYPE_INT) {
        int64_t val = sqlite3_value_int64(argv[0]);
        err = array.append(arr, &val);
    } else if (arr->type == TYPE_REAL) {
        double val = sqlite3_value_double(argv[0]);
        err = array.append(arr, &val);
    } else if (arr->type == TYPE_TEXT) {
        const unsigned char* str = sqlite3_value_text(argv[0]);
        uint8_t* val = array.fix_width(str, arr->el_size - 1);
        err = array.append(arr, val);
        free(val);
    }

    if (err == STATUS_NOMEM) {
        sqlite3_result_error_nomem(context);
    }
}

// Removes the oldest presently aggregated result from the current window.
static void xinverse(sqlite3_context* context, int argc, sqlite3_value** argv) {
    // The error checking and array type inference can be omitted,
    // as this function is only ever called after xStep() (so the aggregate
    // context has already been allocated) and with a value that has already
    // been passed to xStep() without error.

    (void)argc;
    Array* arr = sqlite3_aggregate_context(context, sizeof(Array));

    STATUS err = STATUS_OK;
    if (arr->type == TYPE_INT) {
        int64_t val = sqlite3_value_int64(argv[0]);
        err = array.remove(arr, &val);
    } else if (arr->type == TYPE_REAL) {
        double val = sqlite3_value_double(argv[0]);
        err = array.remove(arr, &val);
    } else if (arr->type == TYPE_TEXT) {
        const unsigned char* str = sqlite3_value_text(argv[0]);
        uint8_t* val = array.fix_width(str, arr->el_size - 1);
        err = array.remove(arr, val);
        free(val);
    }

    if (err == STATUS_NOMEM) {
        sqlite3_result_error_nomem(context);
    }
}

// Returns the current value of the aggregate.
static void xvalue(sqlite3_context* context) {
    Array* arr = sqlite3_aggregate_context(context, sizeof(Array));
    if (arr == NULL) {
        return;
    }
    if (arr->type == 0) {
        // aggregate is empty
        return;
    }
    sqlite3_result_blob(context, array.to_blob(arr), array.size(arr), SQLITE_TRANSIENT);
}

// Returns the current value of the aggregate
// and frees the resources allocated by xstep.
static void xfinal(sqlite3_context* context) {
    Array* arr = sqlite3_aggregate_context(context, sizeof(Array));
    if (arr == NULL) {
        return;
    }
    if (arr->type == 0) {
        // aggregate is empty
        return;
    }
    sqlite3_result_blob(context, array.to_blob(arr), array.size(arr), SQLITE_TRANSIENT);
    free(arr->data);
    // the array itself is freed by sqlite
}

int arrayagg_init(sqlite3* db) {
    const int flags = SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC;
    sqlite3_create_window_function(db, "array_agg", -1, flags, 0, xstep, xfinal, xvalue, xinverse,
                                   0);
    return SQLITE_OK;
}
