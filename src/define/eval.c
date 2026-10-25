// Created by by D. Richard Hipp, Public Domain
// https://www.sqlite.org/src/file/ext/misc/eval.c

// Modified by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean/

// Evaluate dynamic SQL.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT3

// EvalResult accumulates the output of eval().
struct EvalResult {
    char* z;               // accumulated output
    const char* zSep;      // separator, NULL when out of memory
    int szSep;             // size of the separator
    sqlite3_int64 nAlloc;  // number of bytes allocated for z[]
    sqlite3_int64 nUsed;   // number of bytes of z[] actually used
};

// eval_callback appends a row of the result to EvalResult.
// When out of memory, it frees the output, clears EvalResult and returns SQLITE_NOMEM.
static int eval_callback(void* pCtx, int argc, char** argv, char** colnames) {
    struct EvalResult* p = (struct EvalResult*)pCtx;
    int i;
    if (argv == 0) {
        return SQLITE_OK;
    }
    for (i = 0; i < argc; i++) {
        const char* z = argv[i] ? argv[i] : "";
        size_t sz = strlen(z);
        if ((sqlite3_int64)sz + p->nUsed + p->szSep + 1 > p->nAlloc) {
            char* zNew;
            p->nAlloc = p->nAlloc * 2 + sz + p->szSep + 1;
            zNew = p->nAlloc <= 0x7fffffff ? sqlite3_realloc64(p->z, p->nAlloc) : 0;
            if (zNew == 0) {
                sqlite3_free(p->z);
                memset(p, 0, sizeof(*p));
                return SQLITE_NOMEM;
            }
            p->z = zNew;
        }
        if (p->nUsed > 0) {
            memcpy(&p->z[p->nUsed], p->zSep, p->szSep);
            p->nUsed += p->szSep;
        }
        memcpy(&p->z[p->nUsed], z, sz);
        p->nUsed += sz;
    }
    return SQLITE_OK;
}

// define_eval implements eval(sql[, sep]). It executes the SQL text and returns
// all values of all rows, separated by sep (a single space by default).
static void define_eval(sqlite3_context* context, int argc, sqlite3_value** argv) {
    const char* zSql;
    sqlite3* db;
    char* zErr = 0;
    int rc;
    struct EvalResult x;

    memset(&x, 0, sizeof(x));
    x.zSep = " ";
    zSql = (const char*)sqlite3_value_text(argv[0]);
    if (zSql == 0) {
        return;
    }
    if (argc > 1) {
        x.zSep = (const char*)sqlite3_value_text(argv[1]);
        if (x.zSep == 0) {
            return;
        }
    }
    x.szSep = (int)strlen(x.zSep);
    db = sqlite3_context_db_handle(context);
    rc = sqlite3_exec(db, zSql, eval_callback, &x, &zErr);
    if (x.zSep == 0 || rc == SQLITE_NOMEM) {
        // eval_callback clears zSep when out of memory (then exec reports SQLITE_ABORT)
        sqlite3_result_error_nomem(context);
        sqlite3_free(x.z);
    } else if (rc != SQLITE_OK) {
        sqlite3_result_error(context, zErr ? zErr : sqlite3_errmsg(db), -1);
        sqlite3_result_error_code(context, rc);
        sqlite3_free(x.z);
    } else {
        sqlite3_result_text(context, x.z, (int)x.nUsed, sqlite3_free);
    }
    sqlite3_free(zErr);
}

// define_eval_init registers eval() and returns SQLITE_OK.
int define_eval_init(sqlite3* db) {
    const int flags = SQLITE_UTF8 | SQLITE_DIRECTONLY;
    sqlite3_create_function(db, "eval", 1, flags, NULL, define_eval, NULL, NULL);
    sqlite3_create_function(db, "eval", 2, flags, NULL, define_eval, NULL, NULL);
    return SQLITE_OK;
}
