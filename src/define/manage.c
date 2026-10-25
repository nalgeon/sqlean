// Copyright (c) 2022 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Manage defined functions.

#include <stdbool.h>
#include <stddef.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT3

#pragma region registry

// The registry holds the scalar functions defined in a connection, so that
// undefine() can find them by name. SQLite can't delete a function while any
// statement is running, and undefine() itself runs in a statement. So instead
// of deleting the function, undefine() marks it as undefined, and the function
// fails when called.
//
// The registry is shared by the management functions and by each defined
// function, and is freed when the last of them is destroyed (usually when the
// connection closes).

typedef struct registry registry;

typedef struct function {
    char* name;
    char* sql;  // "select <body>"
    bool defined;
    registry* reg;
    struct function* next;
} function;

struct registry {
    function* head;
    int refs;
};

// registry_new creates an empty registry with no references.
static registry* registry_new(void) {
    registry* reg = sqlite3_malloc(sizeof(registry));
    if (reg == NULL) {
        return NULL;
    }
    reg->head = NULL;
    reg->refs = 0;
    return reg;
}

// registry_release drops a reference to the registry,
// and frees it with all its functions when it's the last one.
static void registry_release(void* ptr) {
    registry* reg = ptr;
    if (--reg->refs > 0) {
        return;
    }
    function* fn = reg->head;
    while (fn != NULL) {
        function* next = fn->next;
        sqlite3_free(fn->name);
        sqlite3_free(fn->sql);
        sqlite3_free(fn);
        fn = next;
    }
    sqlite3_free(reg);
}

// registry_add adds a function to the registry and takes ownership of sql.
// Returns NULL (and frees sql) if out of memory.
static function* registry_add(registry* reg, const char* name, char* sql) {
    function* fn = sqlite3_malloc(sizeof(function));
    char* fn_name = sqlite3_mprintf("%s", name);
    if (fn == NULL || fn_name == NULL) {
        sqlite3_free(fn);
        sqlite3_free(fn_name);
        sqlite3_free(sql);
        return NULL;
    }
    fn->name = fn_name;
    fn->sql = sql;
    fn->defined = true;
    fn->reg = reg;
    fn->next = reg->head;
    reg->head = fn;
    return fn;
}

// registry_remove removes a function from the registry and frees it.
static void registry_remove(registry* reg, function* fn) {
    for (function** ptr = &reg->head; *ptr != NULL; ptr = &(*ptr)->next) {
        if (*ptr == fn) {
            *ptr = fn->next;
            sqlite3_free(fn->name);
            sqlite3_free(fn->sql);
            sqlite3_free(fn);
            return;
        }
    }
}

// registry_undefine marks the functions with the given name as undefined.
// Function names are case-insensitive, and there may be several functions
// with the same name and a different number of parameters.
static void registry_undefine(registry* reg, const char* name) {
    for (function* fn = reg->head; fn != NULL; fn = fn->next) {
        if (sqlite3_stricmp(fn->name, name) == 0) {
            fn->defined = false;
        }
    }
}

// function_release is the destructor of a defined function.
static void function_release(void* ptr) {
    registry_release(((function*)ptr)->reg);
}

#pragma endregion

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
    const function* fn;
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
static sqlite3_stmt* cache_get(sqlite3_context* ctx, const function* fn) {
    for (cache_node* node = sqlite3_get_auxdata(ctx, CACHE_SLOT); node != NULL; node = node->next) {
        if (node->fn == fn) {
            return node->stmt;
        }
    }
    return NULL;
}

// cache_put caches the statement for the function and takes ownership of it.
// If the statement can't be cached, it's finalized.
static void cache_put(sqlite3_context* ctx, const function* fn, sqlite3_stmt* stmt) {
    cache_node* node = sqlite3_malloc(sizeof(cache_node));
    if (node == NULL) {
        sqlite3_finalize(stmt);
        return;
    }
    node->fn = fn;
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
    const function* fn = sqlite3_user_data(ctx);
    if (!fn->defined) {
        // same message as SQLite gives for an unknown function
        char* msg = sqlite3_mprintf("no such function: %s", fn->name);
        if (msg == NULL) {
            sqlite3_result_error_nomem(ctx);
            return;
        }
        sqlite3_result_error(ctx, msg, -1);
        sqlite3_free(msg);
        return;
    }

    sqlite3_stmt* stmt = cache_get(ctx, fn);
    bool cached = stmt != NULL;
    int ret;
    if (!cached) {
        ret = sqlite3_prepare_v2(sqlite3_context_db_handle(ctx), fn->sql, -1, &stmt, NULL);
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
        cache_put(ctx, fn, stmt);
    }
}

/*
 * Creates user-defined function.
 */
static int define_create(sqlite3* db, registry* reg, const char* name, const char* body) {
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

    function* fn = registry_add(reg, name, sql);
    if (fn == NULL) {
        return SQLITE_NOMEM;
    }
    // The function holds a reference to the registry, which keeps it alive
    // (cache_get uses its address to tell functions apart). SQLite calls the
    // destructor even if registering fails.
    reg->refs++;
    ret = sqlite3_create_function_v2(db, name, nparams, SQLITE_UTF8, fn, define_exec, NULL, NULL,
                                     function_release);
    if (ret != SQLITE_OK) {
        registry_remove(reg, fn);
    }
    return ret;
}

/*
 * Creates user-defined function and saves it to the database.
 */
static void define_function(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    sqlite3* db = sqlite3_context_db_handle(ctx);
    registry* reg = sqlite3_user_data(ctx);
    const char* name = (const char*)sqlite3_value_text(argv[0]);
    const char* body = (const char*)sqlite3_value_text(argv[1]);
    int ret;
    if ((ret = define_create(db, reg, name, body)) != SQLITE_OK) {
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
 * Executes a statement with a single text parameter, ignoring its results.
 */
static int exec_with_name(sqlite3* db, const char* sql, const char* name) {
    sqlite3_stmt* stmt;
    int ret = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        return ret;
    }
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
    }
    sqlite3_finalize(stmt);
    return ret == SQLITE_DONE ? SQLITE_OK : ret;
}

/*
 * Returns the type of the user-defined function ("scalar" or "table")
 * or NULL if there is no such function. The caller must free the result.
 */
static int find_function(sqlite3* db, const char* name, char** type) {
    *type = NULL;
    sqlite3_stmt* stmt;
    int ret = sqlite3_prepare_v2(db, "select type from sqlean_define where name = ? collate nocase",
                                 -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        return ret;
    }
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    ret = sqlite3_step(stmt);
    if (ret == SQLITE_ROW) {
        const char* value = (const char*)sqlite3_column_text(stmt, 0);
        *type = sqlite3_mprintf("%s", value ? value : "");
        ret = *type ? SQLITE_DONE : SQLITE_NOMEM;
    }
    sqlite3_finalize(stmt);
    return ret == SQLITE_DONE ? SQLITE_OK : ret;
}

/*
 * Checks if the table function exists, i.e. there is a virtual table
 * with the name, created with the body.
 */
static int table_exists(sqlite3* db, const char* name, bool* exists) {
    *exists = false;
    const char* sql =
        "select 1 from main.sqlite_master "
        "join sqlean_define as d on d.name = ? collate nocase and d.type = 'table' "
        "where sqlite_master.type = 'table' and sqlite_master.name = d.name collate nocase "
        "and sqlite_master.sql like 'create virtual table %' "
        "and substr(sqlite_master.sql, -length(d.body) - 1) = d.body || ')'";
    sqlite3_stmt* stmt;
    int ret = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        return ret;
    }
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    ret = sqlite3_step(stmt);
    if (ret == SQLITE_ROW) {
        *exists = true;
        ret = SQLITE_DONE;
    }
    sqlite3_finalize(stmt);
    return ret == SQLITE_DONE ? SQLITE_OK : ret;
}

/*
 * Deletes user-defined function (scalar or table-valued). Deletes only
 * the functions listed in sqlean_define, and does nothing for other names.
 */
static void define_undefine(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    sqlite3* db = sqlite3_context_db_handle(ctx);
    registry* reg = sqlite3_user_data(ctx);
    const char* name = (const char*)sqlite3_value_text(argv[0]);
    if (name == NULL) {
        return;
    }

    char* type;
    int ret = find_function(db, name, &type);
    if (ret != SQLITE_OK || type == NULL) {
        goto end;
    }

    if (sqlite3_stricmp(type, "table") == 0) {
        bool exists;
        if ((ret = table_exists(db, name, &exists)) != SQLITE_OK) {
            goto end;
        }
        if (exists) {
            // the table deletes its sqlean_define row when dropped
            char* sql = sqlite3_mprintf("drop table main.\"%w\"", name);
            if (sql == NULL) {
                ret = SQLITE_NOMEM;
                goto end;
            }
            ret = sqlite3_exec(db, sql, NULL, NULL, NULL);
            sqlite3_free(sql);
            goto end;
        }
    }

    // a scalar function, or a table function whose table is gone
    ret = exec_with_name(db, "delete from sqlean_define where name = ? collate nocase", name);
    if (ret == SQLITE_OK && sqlite3_stricmp(type, "scalar") == 0) {
        registry_undefine(reg, name);
    }

end:
    sqlite3_free(type);
    if (ret != SQLITE_OK) {
        sqlite3_result_error_code(ctx, ret);
    }
}

/*
 * Loads user-defined functions from the database.
 */
static int define_load(sqlite3* db, registry* reg) {
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
        ret = define_create(db, reg, name, body);
        if (ret != SQLITE_OK) {
            break;
        }
    }
    return sqlite3_finalize(stmt);
}

int define_manage_init(sqlite3* db) {
    registry* reg = registry_new();
    if (reg == NULL) {
        return SQLITE_NOMEM;
    }
    // define and undefine hold a reference to the registry each, and so does
    // define_load while it runs; SQLite calls the destructor even if
    // registering fails
    reg->refs = 3;
    const int flags = SQLITE_UTF8 | SQLITE_DIRECTONLY;
    sqlite3_create_function_v2(db, "define", 2, flags, reg, define_function, NULL, NULL,
                               registry_release);
    sqlite3_create_function(db, "define_free", 0, flags, NULL, define_free, NULL, NULL);
    sqlite3_create_function_v2(db, "undefine", 1, flags, reg, define_undefine, NULL, NULL,
                               registry_release);
    int ret = define_load(db, reg);
    registry_release(reg);
    return ret;
}
