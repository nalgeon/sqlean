// Copyright (c) 2022 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Manage defined functions.

#include <stdbool.h>
#include <stddef.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT3

#pragma region statement cache

// A defined function compiles its body once per run of the calling statement,
// not once per call. The first call prepares the body and attaches it to the
// calling statement as auxiliary data, and later calls reuse it. SQLite frees
// the auxiliary data when the calling statement finishes, is reset or is
// finalized, so the prepared body never outlives it, and the connection can be
// closed as usual.
//
// Auxiliary data in a non-negative slot belongs to a function argument, and
// SQLite discards it after each call unless the argument is constant. Data in
// a negative slot lives until the calling statement finishes, but the slot is
// shared by all the functions called from that statement. So the slot holds a
// list of prepared bodies, one per defined function.
//
// Negative slots are undocumented, so if SQLite refuses to keep the data,
// functions fall back to preparing the body on every call.

#define CACHE_SLOT -0x5ea1

typedef struct cache_node {
    const char* sql;  // identifies the function, see define_create
    sqlite3_stmt* stmt;
    struct cache_node* next;
} cache_node;

// cache_free finalizes and frees the cached statements.
static void cache_free(void* head) {
    cache_node* node = head;
    while (node != NULL) {
        cache_node* next = node->next;
        sqlite3_finalize(node->stmt);
        sqlite3_free(node);
        node = next;
    }
}

// cache_get returns the statement cached for the function by the calling
// statement, or NULL if there is none.
static sqlite3_stmt* cache_get(sqlite3_context* ctx, const char* sql) {
    for (cache_node* node = sqlite3_get_auxdata(ctx, CACHE_SLOT); node != NULL; node = node->next) {
        if (node->sql == sql) {
            return node->stmt;
        }
    }
    return NULL;
}

// cache_put caches the statement for the function and takes ownership of it.
// If the statement can't be cached, it's finalized.
static void cache_put(sqlite3_context* ctx, const char* sql, sqlite3_stmt* stmt) {
    cache_node* node = sqlite3_malloc(sizeof(cache_node));
    if (node == NULL) {
        sqlite3_finalize(stmt);
        return;
    }
    node->sql = sql;
    node->stmt = stmt;
    node->next = NULL;
    cache_node* head = sqlite3_get_auxdata(ctx, CACHE_SLOT);
    if (head != NULL) {
        node->next = head->next;
        head->next = node;
        return;
    }
    // calls cache_free on failure
    sqlite3_set_auxdata(ctx, CACHE_SLOT, node, cache_free);
}

#pragma endregion

/*
 * Saves user-defined function into the database.
 */
int define_save_function(sqlite3* db, const char* name, const char* type, const char* body) {
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
 * Executes the function body, compiled once per run of the calling statement.
 */
static void define_exec(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    const char* sql = sqlite3_user_data(ctx);
    sqlite3_stmt* stmt = cache_get(ctx, sql);
    bool cached = stmt != NULL;
    int ret;
    if (!cached) {
        ret = sqlite3_prepare_v2(sqlite3_context_db_handle(ctx), sql, -1, &stmt, NULL);
        if (ret != SQLITE_OK) {
            sqlite3_result_error_code(ctx, ret);
            return;
        }
    }

    for (int i = 0; i < argc; i++) {
        if ((ret = sqlite3_bind_value(stmt, i + 1, argv[i])) != SQLITE_OK) {
            break;
        }
    }
    if (ret == SQLITE_OK) {
        ret = sqlite3_step(stmt);
    }
    if (ret == SQLITE_ROW) {
        sqlite3_result_value(ctx, sqlite3_column_value(stmt, 0));
    } else {
        sqlite3_result_error_code(ctx, ret == SQLITE_DONE ? SQLITE_MISUSE : ret);
    }
    sqlite3_reset(stmt);

    if (!cached) {
        cache_put(ctx, sql, stmt);
    }
}

/*
 * Creates user-defined function.
 */
static int define_create(sqlite3* db, const char* name, const char* body) {
    char* sql = sqlite3_mprintf("select %s", body);
    if (!sql) {
        return SQLITE_NOMEM;
    }

    // compile the body to validate it and count the parameters
    sqlite3_stmt* stmt;
    int ret = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        sqlite3_free(sql);
        return ret;
    }
    int nparams = sqlite3_bind_parameter_count(stmt);
    sqlite3_finalize(stmt);

    // The function owns the SQL text until it's deleted or the connection closes,
    // and cache_get uses its address to tell functions apart. Calling statements
    // can't outlive the function: replacing or deleting it requires that no
    // statement is running.
    return sqlite3_create_function_v2(db, name, nparams, SQLITE_UTF8, sql, define_exec, NULL, NULL,
                                      sqlite3_free);
}

/*
 * Creates user-defined function and saves it to the database.
 */
static void define_function(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    sqlite3* db = sqlite3_context_db_handle(ctx);
    const char* name = (const char*)sqlite3_value_text(argv[0]);
    const char* body = (const char*)sqlite3_value_text(argv[1]);
    int ret;
    if ((ret = define_create(db, name, body)) != SQLITE_OK) {
        sqlite3_result_error_code(ctx, ret);
        return;
    }
    if ((ret = define_save_function(db, name, "scalar", body)) != SQLITE_OK) {
        sqlite3_result_error_code(ctx, ret);
        return;
    }
}

/*
 * Does nothing. Prepared statements used to require freeing before closing
 * the connection, now they are freed automatically.
 */
static void define_free(sqlite3_context* ctx, int argc, sqlite3_value** argv) {}

/*
 * Deletes user-defined function (scalar or table-valued)
 */
static void define_undefine(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    char* template =
        "delete from sqlean_define where name = '%q';"
        "drop table if exists \"%w\";";
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

/*
 * Loads user-defined functions from the database.
 */
static int define_load(sqlite3* db) {
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
        ret = define_create(db, name, body);
        if (ret != SQLITE_OK) {
            break;
        }
    }
    return sqlite3_finalize(stmt);
}

int define_manage_init(sqlite3* db) {
    const int flags = SQLITE_UTF8 | SQLITE_DIRECTONLY;
    sqlite3_create_function(db, "define", 2, flags, NULL, define_function, NULL, NULL);
    sqlite3_create_function(db, "define_free", 0, flags, NULL, define_free, NULL, NULL);
    sqlite3_create_function(db, "undefine", 1, flags, NULL, define_undefine, NULL, NULL);
    return define_load(db);
}
