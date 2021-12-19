// Created by Alex Wilson, MIT License
// https://github.com/mrwilson/squib/blob/master/pearson.c

// Pearson correlation coefficient.

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

typedef struct {
    float x_sum;
    float y_sum;
    float x_sqr;
    float y_sqr;
    float prod;
    int count;
} PrsCtx;

void validateTypes(sqlite3_context* context, int a, int b) {
    if (a == SQLITE_NULL || b == SQLITE_NULL) {
        sqlite3_result_error(context, "Null types.", -1);
    }

    if ((a != SQLITE_INTEGER && a != SQLITE_FLOAT) || a != b) {
        sqlite3_result_error(context, "Incompatible types.", -1);
    }
}

void pearsonStep(sqlite3_context* context, int argc, sqlite3_value** argv) {
    PrsCtx* p;
    int xType, yType;

    p = sqlite3_aggregate_context(context, sizeof(*p));

    xType = sqlite3_value_numeric_type(argv[0]);
    yType = sqlite3_value_numeric_type(argv[1]);

    validateTypes(context, xType, yType);

    if (p) {
        p->count++;
        double x, y;

        if (xType == SQLITE_INTEGER) {
            x = (double)sqlite3_value_int64(argv[0]);
            y = (double)sqlite3_value_int64(argv[1]);
        } else {
            x = sqlite3_value_double(argv[0]);
            y = sqlite3_value_double(argv[1]);
        }

        p->x_sum += x;
        p->y_sum += y;
        p->prod += x * y;
        p->x_sqr += x * x;
        p->y_sqr += y * y;
    }
}

double pearson(PrsCtx* p) {
    int n = p->count;

    double top = p->prod - (p->x_sum * p->y_sum / n), x_b = p->x_sqr - (p->x_sum * p->x_sum / n),
           y_b = p->y_sqr - (p->y_sum * p->y_sum / n);

    return top / sqrt(x_b * y_b);
}

void pearsonFinalize(sqlite3_context* context) {
    PrsCtx* p;
    p = sqlite3_aggregate_context(context, 0);

    if (!p || p->count <= 0) {
        sqlite3_result_error(context, "Error calculating value.", -1);
    }

    sqlite3_result_double(context, pearson(p));
}

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_pearson_init(sqlite3* db, char** pzErrMsg, const sqlite3_api_routines* pApi) {
    SQLITE_EXTENSION_INIT2(pApi);
    static const int flags = SQLITE_UTF8;
    sqlite3_create_function(db, "pearson", 2, flags, NULL, NULL, &pearsonStep, &pearsonFinalize);
    return SQLITE_OK;
}