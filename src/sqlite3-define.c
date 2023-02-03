// Created by 0x09, Public Domain
// https://github.com/0x09/sqlite-statement-vtab/blob/master/statement_vtab.c

// eval() created by by D. Richard Hipp, Public Domain
// https://www.sqlite.org/src/file/ext/misc/eval.c

// Modified by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

#define DEFINE_CACHE 1

#pragma region statement cache

typedef struct cache_node {
    sqlite3_stmt* stmt;
    struct cache_node* next;
} cache_node;

cache_node* cache_head = NULL;
cache_node* cache_tail = NULL;

static int cache_add(sqlite3_stmt* stmt) {
    if (cache_head == NULL) {
        cache_head = (cache_node*)malloc(sizeof(cache_node));
        if (cache_head == NULL) {
            return SQLITE_ERROR;
        }
        cache_head->stmt = stmt;
        cache_head->next = NULL;
        cache_tail = cache_head;
        return SQLITE_OK;
    }
    cache_tail->next = (cache_node*)malloc(sizeof(cache_node));
    if (cache_tail->next == NULL) {
        return SQLITE_ERROR;
    }
    cache_tail = cache_tail->next;
    cache_tail->stmt = stmt;
    cache_tail->next = NULL;
    return SQLITE_OK;
}

static void cache_print() {
    if (cache_head == NULL) {
        printf("cache is empty");
        return;
    }
    cache_node* curr = cache_head;
    while (curr != NULL) {
        printf("%s\n", sqlite3_sql(curr->stmt));
        curr = curr->next;
    }
}

static void cache_free() {
    if (cache_head == NULL) {
        return;
    }
    cache_node* prev;
    cache_node* curr = cache_head;
    while (curr != NULL) {
        sqlite3_finalize(curr->stmt);
        prev = curr;
        curr = curr->next;
        free(prev);
    }
    cache_head = cache_tail = NULL;
}

#pragma endregion

#pragma region eval

/*
 * Structure used to accumulate the output
 */
struct EvalResult {
    char* z;              /* Accumulated output */
    const char* zSep;     /* Separator */
    int szSep;            /* Size of the separator string */
    sqlite3_int64 nAlloc; /* Number of bytes allocated for z[] */
    sqlite3_int64 nUsed;  /* Number of bytes of z[] actually used */
};

/*
 * Callback from sqlite_exec() for the eval() function.
 */
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
            /* Using sqlite3_realloc64() would be better, but it is a recent
            ** addition and will cause a segfault if loaded by an older version
            ** of SQLite.  */
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

/*
 * Implementation of the eval(X) and eval(X,Y) SQL functions.
 *
 * Evaluate the SQL text in X. Return the results, using string
 * Y as the separator. If Y is omitted, use a single space character.
 */
static void eval(sqlite3_context* context, int argc, sqlite3_value** argv) {
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
    if (rc != SQLITE_OK) {
        sqlite3_result_error(context, zErr, -1);
        sqlite3_free(zErr);
    } else if (x.zSep == 0) {
        sqlite3_result_error_nomem(context);
        sqlite3_free(x.z);
    } else {
        sqlite3_result_text(context, x.z, (int)x.nUsed, sqlite3_free);
    }
}

#pragma endregion

#pragma region define scalar function without caching

#ifndef DEFINE_CACHE
/*
 * Creates user-defined function and saves it to the database.
 */
static void define_function(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    sqlite3* db = sqlite3_context_db_handle(ctx);
    const char* name = (const char*)sqlite3_value_text(argv[0]);
    const char* body = (const char*)sqlite3_value_text(argv[1]);
    int ret;
    if ((ret = create_function(db, name, body)) != SQLITE_OK) {
        sqlite3_result_error_code(ctx, ret);
        return;
    }
    if ((ret = save_function(db, name, "scalar", body)) != SQLITE_OK) {
        sqlite3_result_error_code(ctx, ret);
        return;
    }
}

/*
 * Creates user-defined function without caching the prepared statement.
 */
static int create_function(sqlite3* db, const char* name, const char* body) {
    char* sql = sqlite3_mprintf("select %s", body);
    if (!sql) {
        return SQLITE_NOMEM;
    }

    sqlite3_stmt* stmt;
    int ret = sqlite3_prepare_v3(db, sql, -1, SQLITE_PREPARE_PERSISTENT, &stmt, NULL);
    if (ret != SQLITE_OK) {
        sqlite3_free(sql);
        return ret;
    }
    int nparams = sqlite3_bind_parameter_count(stmt);
    sqlite3_finalize(stmt);

    return sqlite3_create_function_v2(db, name, nparams, SQLITE_UTF8, sql, exec_function, NULL,
                                      NULL, sqlite3_free);
}

/*
 * Executes user-defined sql from the context.
 */
static void exec_function(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    int ret = SQLITE_OK;
    char* sql = sqlite3_user_data(ctx);
    sqlite3_stmt* stmt;
    // sqlite3_close requires all prepared statements to be closed before destroying functions, so
    // we have to re-create this every call
    if ((ret = sqlite3_prepare_v2(sqlite3_context_db_handle(ctx), sql, -1, &stmt, NULL)) !=
        SQLITE_OK) {
        sqlite3_result_error_code(ctx, ret);
        return;
    }
    for (int i = 0; i < argc; i++)
        if ((ret = sqlite3_bind_value(stmt, i + 1, argv[i])) != SQLITE_OK)
            goto end;
    if ((ret = sqlite3_step(stmt)) != SQLITE_ROW) {
        if (ret == SQLITE_DONE)
            ret = SQLITE_MISUSE;
        goto end;
    }
    sqlite3_result_value(ctx, sqlite3_column_value(stmt, 0));

end:
    sqlite3_finalize(stmt);
    if (ret != SQLITE_ROW)
        sqlite3_result_error_code(ctx, ret);
}
#endif

#pragma endregion

#pragma region define scalar function with caching

/*
 * Executes compiled prepared statement from the context.
 */
static void exec_compiled(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    int ret = SQLITE_OK;
    sqlite3_stmt* stmt = sqlite3_user_data(ctx);
    for (int i = 0; i < argc; i++) {
        if ((ret = sqlite3_bind_value(stmt, i + 1, argv[i])) != SQLITE_OK) {
            sqlite3_reset(stmt);
            sqlite3_result_error_code(ctx, ret);
            return;
        }
    }
    if ((ret = sqlite3_step(stmt)) != SQLITE_ROW) {
        if (ret == SQLITE_DONE) {
            ret = SQLITE_MISUSE;
        }
        sqlite3_reset(stmt);
        sqlite3_result_error_code(ctx, ret);
        return;
    }
    sqlite3_result_value(ctx, sqlite3_column_value(stmt, 0));
    sqlite3_reset(stmt);
}

/*
 * Saves user-defined function into the database.
 */
static int save_function(sqlite3* db, const char* name, const char* type, const char* body) {
    char* sql =
        "insert into sqlean_define(name, type, body) values (?, ?, ?) "
        "on conflict do nothing";
    sqlite3_stmt* stmt;
    int ret = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        return ret;
    }
    sqlite3_bind_text(stmt, 1, name, -1, NULL);
    sqlite3_bind_text(stmt, 2, type, -1, NULL);
    sqlite3_bind_text(stmt, 3, body, -1, NULL);
    ret = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (ret != SQLITE_DONE) {
        return ret;
    }
    return SQLITE_OK;
}

/*
 * Creates user-defined function and caches the prepared statement.
 */
static int create_compiled(sqlite3* db, const char* name, const char* body) {
    char* sql = sqlite3_mprintf("select %s", body);
    if (!sql) {
        return SQLITE_NOMEM;
    }

    sqlite3_stmt* stmt;
    int ret = sqlite3_prepare_v3(db, sql, -1, SQLITE_PREPARE_PERSISTENT, &stmt, NULL);
    sqlite3_free(sql);
    if (ret != SQLITE_OK) {
        return ret;
    }
    int nparams = sqlite3_bind_parameter_count(stmt);
    if ((ret = cache_add(stmt)) != SQLITE_OK) {
        return ret;
    }

    return sqlite3_create_function(db, name, nparams, SQLITE_UTF8, stmt, exec_compiled, NULL, NULL);
}

/*
 * Loads user-defined functions from the database.
 */
static int load_functions(sqlite3* db) {
    char* sql =
        "create table if not exists sqlean_define"
        "(name text primary key, type text, body text)";
    int ret = sqlite3_exec(db, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK) {
        return ret;
    }

    sqlite3_stmt* stmt;
    sql = "select name, body from sqlean_define where type = 'scalar'";
    if ((ret = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL)) != SQLITE_OK) {
        return ret;
    }

    const char* name;
    const char* body;
    while (sqlite3_step(stmt) != SQLITE_DONE) {
        name = (const char*)sqlite3_column_text(stmt, 0);
        body = (const char*)sqlite3_column_text(stmt, 1);
        ret = create_compiled(db, name, body);
        if (ret != SQLITE_OK) {
            break;
        }
    }
    return sqlite3_finalize(stmt);
}

/*
 * Creates compiled user-defined function and saves it to the database.
 */
static void define_compiled(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    sqlite3* db = sqlite3_context_db_handle(ctx);
    const char* name = (const char*)sqlite3_value_text(argv[0]);
    const char* body = (const char*)sqlite3_value_text(argv[1]);
    int ret;
    if ((ret = create_compiled(db, name, body)) != SQLITE_OK) {
        sqlite3_result_error_code(ctx, ret);
        return;
    }
    if ((ret = save_function(db, name, "scalar", body)) != SQLITE_OK) {
        sqlite3_result_error_code(ctx, ret);
        return;
    }
}

/*
 * Frees prepared statements compiled by user-defined functions.
 */
static void free_compiled(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    cache_free();
}

/*
 * Prints prepared statements cache contents.
 */
static void print_cache(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    cache_print();
}

#pragma endregion

#pragma region define table valued function

struct define_vtab {
    sqlite3_vtab base;
    sqlite3* db;
    char* sql;
    size_t sql_len;
    int num_inputs;
    int num_outputs;
};

struct define_cursor {
    sqlite3_vtab_cursor base;
    sqlite3_stmt* stmt;
    int rowid;
    int param_argc;
    sqlite3_value** param_argv;
};

static char* build_create_statement(sqlite3_stmt* stmt) {
    sqlite3_str* sql = sqlite3_str_new(NULL);
    sqlite3_str_appendall(sql, "CREATE TABLE x( ");
    for (int i = 0, nout = sqlite3_column_count(stmt); i < nout; i++) {
        const char* name = sqlite3_column_name(stmt, i);
        if (!name) {
            sqlite3_free(sqlite3_str_finish(sql));
            return NULL;
        }
        const char* type = sqlite3_column_decltype(stmt, i);
        sqlite3_str_appendf(sql, "%Q %s,", name, (type ? type : ""));
    }
    for (int i = 0, nargs = sqlite3_bind_parameter_count(stmt); i < nargs; i++) {
        const char* name = sqlite3_bind_parameter_name(stmt, i + 1);
        if (name)
            sqlite3_str_appendf(sql, "%Q hidden,", name + 1);
        else
            sqlite3_str_appendf(sql, "'%d' hidden,", i + 1);
    }
    if (sqlite3_str_length(sql))
        sqlite3_str_value(sql)[sqlite3_str_length(sql) - 1] = ')';
    return sqlite3_str_finish(sql);
}

static int define_vtab_destroy(sqlite3_vtab* pVTab) {
    sqlite3_free(((struct define_vtab*)pVTab)->sql);
    sqlite3_free(pVTab);
    return SQLITE_OK;
}

static int define_vtab_create(sqlite3* db,
                              void* pAux,
                              int argc,
                              const char* const* argv,
                              sqlite3_vtab** ppVtab,
                              char** pzErr) {
    size_t len;
    if (argc < 4 || (len = strlen(argv[3])) < 3) {
        if (!(*pzErr = sqlite3_mprintf("no statement provided")))
            return SQLITE_NOMEM;
        return SQLITE_MISUSE;
    }
    if (argv[3][0] != '(' || argv[3][len - 1] != ')') {
        if (!(*pzErr = sqlite3_mprintf("statement must be parenthesized")))
            return SQLITE_NOMEM;
        return SQLITE_MISUSE;
    }

    int ret;
    sqlite3_stmt* stmt = NULL;
    char* create = NULL;

    struct define_vtab* vtab = sqlite3_malloc64(sizeof(*vtab));
    if (!vtab) {
        return SQLITE_NOMEM;
    }
    memset(vtab, 0, sizeof(*vtab));
    *ppVtab = &vtab->base;

    vtab->db = db;
    vtab->sql_len = len - 2;
    if (!(vtab->sql = sqlite3_mprintf("%.*s", vtab->sql_len, argv[3] + 1))) {
        ret = SQLITE_NOMEM;
        goto error;
    }

    ret = sqlite3_prepare_v2(db, vtab->sql, vtab->sql_len, &stmt, NULL);
    if (ret != SQLITE_OK) {
        goto sqlite_error;
    }

    if (!sqlite3_stmt_readonly(stmt)) {
        ret = SQLITE_ERROR;
        if (!(*pzErr = sqlite3_mprintf("Statement must be read only.")))
            ret = SQLITE_NOMEM;
        goto error;
    }

    vtab->num_inputs = sqlite3_bind_parameter_count(stmt);
    vtab->num_outputs = sqlite3_column_count(stmt);

    if (!(create = build_create_statement(stmt))) {
        ret = SQLITE_NOMEM;
        goto error;
    }

    if ((ret = sqlite3_declare_vtab(db, create)) != SQLITE_OK) {
        goto sqlite_error;
    }

    if ((ret = save_function(db, argv[2], "table", argv[3])) != SQLITE_OK) {
        goto error;
    }

    sqlite3_free(create);
    sqlite3_finalize(stmt);
    return SQLITE_OK;

sqlite_error:
    if (!(*pzErr = sqlite3_mprintf("%s", sqlite3_errmsg(db))))
        ret = SQLITE_NOMEM;
error:
    sqlite3_free(create);
    sqlite3_finalize(stmt);
    define_vtab_destroy(*ppVtab);
    *ppVtab = NULL;
    return ret;
}

// if these point to the literal same function sqlite makes define_vtab eponymous, which we don't
// want
static int define_vtab_connect(sqlite3* db,
                               void* pAux,
                               int argc,
                               const char* const* argv,
                               sqlite3_vtab** ppVtab,
                               char** pzErr) {
    return define_vtab_create(db, pAux, argc, argv, ppVtab, pzErr);
}

static int define_vtab_open(sqlite3_vtab* pVTab, sqlite3_vtab_cursor** ppCursor) {
    struct define_vtab* vtab = (struct define_vtab*)pVTab;
    struct define_cursor* cur = sqlite3_malloc64(sizeof(*cur));
    if (!cur)
        return SQLITE_NOMEM;

    *ppCursor = &cur->base;
    cur->param_argv = sqlite3_malloc(sizeof(*cur->param_argv) * vtab->num_inputs);
    return sqlite3_prepare_v2(vtab->db, vtab->sql, vtab->sql_len, &cur->stmt, NULL);
}

static int define_vtab_close(sqlite3_vtab_cursor* cur) {
    struct define_cursor* stmtcur = (struct define_cursor*)cur;
    sqlite3_finalize(stmtcur->stmt);
    sqlite3_free(stmtcur->param_argv);
    sqlite3_free(cur);
    return SQLITE_OK;
}

static int define_vtab_next(sqlite3_vtab_cursor* cur) {
    struct define_cursor* stmtcur = (struct define_cursor*)cur;
    int ret = sqlite3_step(stmtcur->stmt);
    if (ret == SQLITE_ROW) {
        stmtcur->rowid++;
        return SQLITE_OK;
    }
    return ret == SQLITE_DONE ? SQLITE_OK : ret;
}

static int define_vtab_rowid(sqlite3_vtab_cursor* cur, sqlite_int64* pRowid) {
    *pRowid = ((struct define_cursor*)cur)->rowid;
    return SQLITE_OK;
}

static int define_vtab_eof(sqlite3_vtab_cursor* cur) {
    return !sqlite3_stmt_busy(((struct define_cursor*)cur)->stmt);
}

static int define_vtab_column(sqlite3_vtab_cursor* cur, sqlite3_context* ctx, int i) {
    struct define_cursor* stmtcur = (struct define_cursor*)cur;
    int num_outputs = ((struct define_vtab*)cur->pVtab)->num_outputs;
    if (i < num_outputs)
        sqlite3_result_value(ctx, sqlite3_column_value(stmtcur->stmt, i));
    else if (i - num_outputs < stmtcur->param_argc)
        sqlite3_result_value(ctx, stmtcur->param_argv[i - num_outputs]);
    return SQLITE_OK;
}

// xBestIndex needs to communicate which columns are constrained by the where clause to xFilter;
// in terms of a statement table this translates to which parameters will be available to bind.
static int define_vtab_filter(sqlite3_vtab_cursor* cur,
                              int idxNum,
                              const char* idxStr,
                              int argc,
                              sqlite3_value** argv) {
    struct define_cursor* stmtcur = (struct define_cursor*)cur;
    stmtcur->rowid = 1;
    sqlite3_stmt* stmt = stmtcur->stmt;
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);

    int ret;
    for (int i = 0; i < argc; i++)
        if ((ret = sqlite3_bind_value(stmt, idxStr ? ((int*)idxStr)[i] : i + 1, argv[i])) !=
            SQLITE_OK)
            return ret;
    ret = sqlite3_step(stmt);
    if (!(ret == SQLITE_ROW || ret == SQLITE_DONE))
        return ret;

    assert(((struct define_vtab*)cur->pVtab)->num_inputs >= argc);
    if ((stmtcur->param_argc = argc))  // these seem to persist for the remainder of the statement,
                                       // so just shallow copy
        memcpy(stmtcur->param_argv, argv, sizeof(*stmtcur->param_argv) * argc);

    return SQLITE_OK;
}

static int define_vtab_best_index(sqlite3_vtab* pVTab, sqlite3_index_info* index_info) {
    int num_outputs = ((struct define_vtab*)pVTab)->num_outputs;
    int out_constraints = 0;
    index_info->orderByConsumed = 0;
    index_info->estimatedCost = 1;
    index_info->estimatedRows = 1;
    int col_max = 0;
    sqlite3_uint64 used_cols = 0;
    for (int i = 0; i < index_info->nConstraint; i++) {
        // skip if this is a constraint on one of our output columns
        if (index_info->aConstraint[i].iColumn < num_outputs)
            continue;
        // a given query plan is only usable if all provided "input" columns are usable and have
        // equal constraints only is this redundant / an EQ constraint ever unusable?
        if (!index_info->aConstraint[i].usable ||
            index_info->aConstraint[i].op != SQLITE_INDEX_CONSTRAINT_EQ)
            return SQLITE_CONSTRAINT;

        int col_index = index_info->aConstraint[i].iColumn - num_outputs;
        index_info->aConstraintUsage[i].argvIndex = col_index + 1;
        index_info->aConstraintUsage[i].omit = 1;

        if (col_index + 1 > col_max)
            col_max = col_index + 1;
        if (col_index < 64)
            used_cols |= 1ull << col_index;

        out_constraints++;
    }

    // if the constrained columns are contiguous then we can just tell sqlite to order the arg
    // vector provided to xFilter in the same order as our column bindings, so there's no need to
    // map between these (this will always be the case when calling the vtab as a table-valued
    // function) only support this optimization for up to 64 constrained columns since checking for
    // continuity more generally would cost as much as just allocating the mapping
    sqlite_uint64 required_cols = (col_max < 64 ? 1ull << col_max : 0ull) - 1;
    if (!out_constraints || (col_max <= 64 && used_cols == required_cols))
        return SQLITE_OK;

    // otherwise map the constraint index as provided to xFilter to column index for bindings
    // if this is sparse e.g. where arg1 = x and arg3 = y then we store this separately in idxStr
    int* colmap = sqlite3_malloc64(sizeof(*colmap) * out_constraints);
    if (!colmap)
        return SQLITE_NOMEM;

    int argc = 0;
    int old_index;
    for (int i = 0; i < index_info->nConstraint; i++)
        if ((old_index = index_info->aConstraintUsage[i].argvIndex)) {
            colmap[argc] = old_index;
            index_info->aConstraintUsage[i].argvIndex = ++argc;
        }

    index_info->idxStr = (char*)colmap;
    index_info->needToFreeIdxStr = 1;

    return SQLITE_OK;
}

static sqlite3_module define_module = {
    .xCreate = define_vtab_create,
    .xConnect = define_vtab_connect,
    .xBestIndex = define_vtab_best_index,
    .xDisconnect = define_vtab_destroy,
    .xDestroy = define_vtab_destroy,
    .xOpen = define_vtab_open,
    .xClose = define_vtab_close,
    .xFilter = define_vtab_filter,
    .xNext = define_vtab_next,
    .xEof = define_vtab_eof,
    .xColumn = define_vtab_column,
    .xRowid = define_vtab_rowid,
};

#pragma endregion

/*
 * Deletes user-defined function (scalar or table-valued)
 */
static void undefine_function(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    char* template =
        "delete from sqlean_define where name = '%s';"
        "drop table if exists \"%s\";";
    const char* name = (const char*)sqlite3_value_text(argv[0]);
    char* sql = sqlite3_mprintf(template, name, name);
    if (!sql) {
        sqlite3_result_error_code(ctx, SQLITE_NOMEM);
        return;
    }

    sqlite3* db = sqlite3_context_db_handle(ctx);
    int ret = sqlite3_exec(db, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK) {
        sqlite3_result_error_code(ctx, ret);
    }
    sqlite3_free(sql);
}

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_define_init(sqlite3* db, char** pzErrMsg, const sqlite3_api_routines* pApi) {
    SQLITE_EXTENSION_INIT2(pApi);
    const int flags = SQLITE_UTF8 | SQLITE_DIRECTONLY;
    sqlite3_create_function(db, "define", 2, flags, NULL, define_compiled, NULL, NULL);
    sqlite3_create_function(db, "define_free", 0, flags, NULL, free_compiled, NULL, NULL);
    sqlite3_create_function(db, "define_cache", 0, flags, NULL, print_cache, NULL, NULL);
    sqlite3_create_function(db, "eval", 1, flags, NULL, eval, NULL, NULL);
    sqlite3_create_function(db, "eval", 2, flags, NULL, eval, NULL, NULL);
    sqlite3_create_function(db, "undefine", 1, flags, NULL, undefine_function, NULL, NULL);
    sqlite3_create_module(db, "define", &define_module, NULL);
    return load_functions(db);
}
