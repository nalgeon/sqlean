// Copyright (c) 2022 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Manage defined functions.

#include <stdbool.h>
#include <stddef.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT3

#pragma region registry

// The registry holds the scalar functions defined in a connection, so that
// define() and undefine() can find them by name. SQLite can't replace or delete
// a function while any statement is running, and define() and undefine() run
// in a statement themselves. So instead of replacing the function, define()
// changes its body, and instead of deleting the function, undefine() marks it
// as undefined, and the function fails when called.
//
// The registry is shared by the management functions and by each defined
// function, and is freed when the last of them is destroyed (usually when the
// connection closes).

typedef struct registry registry;

typedef struct function {
    char* name;
    char* sql;    // "select <body>"
    int nparams;  // -1 if the body didn't compile when loaded, see load_function
    int version;  // changes with the body, see cache_get
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
static function* registry_add(registry* reg, const char* name, char* sql, int nparams) {
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
    fn->nparams = nparams;
    fn->version = 0;
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

// Function names are case-insensitive, and there may be several functions
// with the same name and a different number of parameters (but only one
// of them is defined at a time).

// registry_find returns the function with the given name and number
// of parameters, defined or not, or NULL if there is none.
static function* registry_find(registry* reg, const char* name, int nparams) {
    for (function* fn = reg->head; fn != NULL; fn = fn->next) {
        if (sqlite3_stricmp(fn->name, name) == 0 && fn->nparams == nparams) {
            return fn;
        }
    }
    return NULL;
}

// registry_find_name returns a function with the given name (only a defined
// one if defined_only is true), or NULL if there is none.
static function* registry_find_name(registry* reg, const char* name, bool defined_only) {
    for (function* fn = reg->head; fn != NULL; fn = fn->next) {
        if (sqlite3_stricmp(fn->name, name) == 0 && (fn->defined || !defined_only)) {
            return fn;
        }
    }
    return NULL;
}

// registry_undefine marks the functions with the given name as undefined,
// except the given one.
static void registry_undefine(registry* reg, const char* name, const function* except) {
    for (function* fn = reg->head; fn != NULL; fn = fn->next) {
        if (fn != except && sqlite3_stricmp(fn->name, name) == 0) {
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
    int version;  // of the function when the statement was prepared
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

// cache_find returns the cache node of the function, or NULL if there is none.
static cache_node* cache_find(sqlite3_context* ctx, const function* fn) {
    for (cache_node* node = sqlite3_get_auxdata(ctx, CACHE_SLOT); node != NULL; node = node->next) {
        if (node->fn == fn) {
            return node;
        }
    }
    return NULL;
}

// cache_get returns the statement cached for the function by the calling
// statement, or NULL if there is none, or if the function was redefined
// since (which may happen while the calling statement is running).
static sqlite3_stmt* cache_get(sqlite3_context* ctx, const function* fn) {
    cache_node* node = cache_find(ctx, fn);
    if (node == NULL || node->version != fn->version) {
        return NULL;
    }
    return node->stmt;
}

// cache_put caches the statement compiled from the given version of the
// function and takes ownership of it. If the statement can't be cached,
// it's finalized.
static void cache_put(sqlite3_context* ctx, const function* fn, int version, sqlite3_stmt* stmt) {
    cache_node* node = cache_find(ctx, fn);
    if (node != NULL) {
        // replace the statement compiled from an old body
        sqlite3_finalize(node->stmt);
        node->version = version;
        node->stmt = stmt;
        return;
    }
    node = sqlite3_malloc(sizeof(cache_node));
    if (node == NULL) {
        sqlite3_finalize(stmt);
        return;
    }
    node->fn = fn;
    node->version = version;
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

// define_save_function saves the function into the sqlean_define table
// and returns SQLITE_OK or an error code. Does nothing if a function with
// the same name is already saved.
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

// result_db_error reports an error with the code and the connection's error message.
static void result_db_error(sqlite3_context* ctx, sqlite3* db, int code) {
    sqlite3_result_error(ctx, sqlite3_errmsg(db), -1);
    sqlite3_result_error_code(ctx, code);
}

// result_name_error reports an error with a message made from a format and a name.
static void result_name_error(sqlite3_context* ctx, const char* format, const char* name) {
    char* msg = sqlite3_mprintf(format, name);
    if (msg == NULL) {
        sqlite3_result_error_nomem(ctx);
        return;
    }
    sqlite3_result_error(ctx, msg, -1);
    sqlite3_free(msg);
}

// define_exec executes the body of a defined function and returns its first value,
// or NULL if the body returns no rows. The body is compiled once per run of the
// calling statement (see the statement cache).
static void define_exec(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    const function* fn = sqlite3_user_data(ctx);
    if (!fn->defined) {
        // same messages as SQLite gives
        if (registry_find_name(fn->reg, fn->name, true) != NULL) {
            result_name_error(ctx, "wrong number of arguments to function %s()", fn->name);
        } else {
            result_name_error(ctx, "no such function: %s", fn->name);
        }
        return;
    }

    sqlite3* db = sqlite3_context_db_handle(ctx);
    sqlite3_stmt* stmt = cache_get(ctx, fn);
    bool cached = stmt != NULL;
    // the body itself may redefine the function
    int version = fn->version;
    int ret;
    if (!cached) {
        // the body may no longer compile, e.g. if a table it uses was dropped
        ret = sqlite3_prepare_v2(db, fn->sql, -1, &stmt, NULL);
        if (ret != SQLITE_OK) {
            result_db_error(ctx, db, ret);
            return;
        }
    }

    if (fn->nparams < 0 && sqlite3_bind_parameter_count(stmt) != argc) {
        // registered with any number of arguments, so check it here
        result_name_error(ctx, "wrong number of arguments to function %s()", fn->name);
        ret = SQLITE_MISUSE;
        goto end;
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
    } else if (ret == SQLITE_DONE) {
        // no rows, like a scalar subquery
        sqlite3_result_null(ctx);
    } else {
        result_db_error(ctx, db, ret);
    }

end:
    sqlite3_reset(stmt);
    if (!cached) {
        cache_put(ctx, fn, version, stmt);
    }
}

// compile_body compiles the function body to validate it and count its parameters,
// and returns SQLITE_OK or an error code. On success, sets sql to the statement
// to execute, which the caller must free.
static int compile_body(sqlite3* db, const char* body, char** sql, int* nparams) {
    *sql = sqlite3_mprintf("select %s", body);
    if (*sql == NULL) {
        return SQLITE_NOMEM;
    }
    sqlite3_stmt* stmt;
    int ret = sqlite3_prepare_v2(db, *sql, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        sqlite3_free(*sql);
        *sql = NULL;
        return ret;
    }
    *nparams = sqlite3_bind_parameter_count(stmt);
    sqlite3_finalize(stmt);
    return SQLITE_OK;
}

// register_function registers a new defined function in the connection and the registry,
// takes ownership of sql, and returns SQLITE_OK or an error code.
static int register_function(sqlite3* db, registry* reg, const char* name, char* sql, int nparams) {
    function* fn = registry_add(reg, name, sql, nparams);
    if (fn == NULL) {
        return SQLITE_NOMEM;
    }
    // The function holds a reference to the registry, which keeps it alive
    // (cache_get uses its address to tell functions apart). SQLite calls the
    // destructor even if registering fails.
    reg->refs++;
    int ret = sqlite3_create_function_v2(db, name, nparams, SQLITE_UTF8, fn, define_exec, NULL,
                                         NULL, function_release);
    if (ret != SQLITE_OK) {
        registry_remove(reg, fn);
    }
    return ret;
}

// load_function registers a function saved in the database, and returns SQLITE_OK
// or an error code. A body may fail to compile at this point, because it calls a function
// that's loaded later (by define_load or by another extension), or uses a table that's
// gone. Such a function is registered with any number of arguments, and its body is
// compiled when it's called (and fails with the compile error if it still doesn't compile).
static int load_function(sqlite3* db, registry* reg, const char* name, const char* body) {
    char* sql;
    int nparams;
    int ret = compile_body(db, body, &sql, &nparams);
    if (ret == SQLITE_NOMEM) {
        return ret;
    }
    if (ret != SQLITE_OK) {
        sql = sqlite3_mprintf("select %s", body);
        if (sql == NULL) {
            return SQLITE_NOMEM;
        }
        nparams = -1;
    }
    if (registry_find(reg, name, nparams) != NULL) {
        // a duplicate in another case, SQLite can't replace it while define_load runs
        sqlite3_free(sql);
        return SQLITE_OK;
    }
    return register_function(db, reg, name, sql, nparams);
}

// exec_with_name executes a statement with the name as its only parameter,
// ignoring its results, and returns SQLITE_OK or an error code.
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

// find_function looks the function up in sqlean_define and returns SQLITE_OK
// or an error code. Sets type to "scalar", "table", or NULL if there is no such
// function. The caller must free the type.
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

// function_exists checks if a function with the given name (any number of parameters)
// is registered in the connection, and returns SQLITE_OK or an error code.
static int function_exists(sqlite3* db, const char* name, bool* exists) {
    *exists = false;
    sqlite3_stmt* stmt;
    const char* sql = "select 1 from pragma_function_list where name = ? collate nocase";
    int ret = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        // SQLite is compiled without pragma_function_list,
        // so let sqlite3_create_function decide
        return SQLITE_OK;
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

// exec_with_name_body executes a statement with the name and the body as its parameters,
// and returns SQLITE_OK or an error code.
static int exec_with_name_body(sqlite3* db, const char* sql, const char* name, const char* body) {
    sqlite3_stmt* stmt;
    int ret = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        return ret;
    }
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, body, -1, SQLITE_STATIC);
    ret = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return ret == SQLITE_DONE ? SQLITE_OK : ret;
}

// save_scalar saves the scalar function into sqlean_define, replacing a saved
// function with the same name (in any case), and returns SQLITE_OK or an error code.
static int save_scalar(sqlite3* db, const char* name, const char* body) {
    // update in place, so that the functions load in the same order
    int ret = exec_with_name_body(
        db, "update sqlean_define set name = ?1, body = ?2 where name = ?1 collate nocase", name,
        body);
    if (ret != SQLITE_OK || sqlite3_changes(db) > 0) {
        return ret;
    }
    return exec_with_name_body(
        db, "insert into sqlean_define(name, type, body) values (?1, 'scalar', ?2)", name, body);
}

// define_function implements define(name, body): creates a scalar function, or changes
// the one this connection created with the same name, and saves it to the database.
// Returns NULL. Errors if the body doesn't compile, or if a function with this name
// exists but wasn't created by define().
static void define_function(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    sqlite3* db = sqlite3_context_db_handle(ctx);
    registry* reg = sqlite3_user_data(ctx);
    const char* name = (const char*)sqlite3_value_text(argv[0]);
    const char* body = (const char*)sqlite3_value_text(argv[1]);
    if (name == NULL) {
        sqlite3_result_error(ctx, "function name is NULL", -1);
        return;
    }
    if (body == NULL) {
        sqlite3_result_error(ctx, "function body is NULL", -1);
        return;
    }
    if (sqlite3_value_bytes(argv[0]) > 255) {
        // SQLite's limit
        sqlite3_result_error(ctx, "function name is too long", -1);
        return;
    }

    char* sql = NULL;
    char* type = NULL;
    int nparams;
    int ret = compile_body(db, body, &sql, &nparams);
    if (ret != SQLITE_OK) {
        result_db_error(ctx, db, ret);
        return;
    }

    // a function defined in this connection is redefined,
    // any other function (such as a built-in one) is left alone
    function* fn = registry_find(reg, name, nparams);
    if (registry_find_name(reg, name, false) == NULL) {
        bool exists;
        if ((ret = function_exists(db, name, &exists)) != SQLITE_OK) {
            goto db_error;
        }
        if (exists) {
            result_name_error(ctx, "function already exists: %s", name);
            goto end;
        }
    }
    if ((ret = find_function(db, name, &type)) != SQLITE_OK) {
        goto db_error;
    }
    if (type != NULL && sqlite3_stricmp(type, "scalar") != 0) {
        result_name_error(ctx, "table function already exists: %s", name);
        goto end;
    }

    // save first, so that nothing changes if the database is read-only
    if ((ret = save_scalar(db, name, body)) != SQLITE_OK) {
        goto db_error;
    }

    if (fn != NULL) {
        // same number of parameters: change the body
        sqlite3_free(fn->sql);
        fn->sql = sql;
        fn->version++;
        fn->defined = true;
        sql = NULL;
    } else {
        // new function or a different number of parameters
        ret = register_function(db, reg, name, sql, nparams);
        sql = NULL;
        if (ret != SQLITE_OK) {
            goto db_error;
        }
        fn = registry_find(reg, name, nparams);
    }
    registry_undefine(reg, name, fn);
    goto end;

db_error:
    result_db_error(ctx, db, ret);
end:
    sqlite3_free(sql);
    sqlite3_free(type);
}

// define_free implements define_free(), which does nothing. Prepared statements
// used to require freeing before closing the connection, now they are freed
// automatically.
static void define_free(sqlite3_context* ctx, int argc, sqlite3_value** argv) {}

// table_exists checks if the table function has its table, i.e. there is a virtual
// table with the name, created with the saved body, and returns SQLITE_OK or an error code.
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

// define_undefine implements undefine(name): deletes a scalar or table-valued
// function and returns NULL. Deletes only the functions listed in sqlean_define,
// and does nothing for other names.
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
        registry_undefine(reg, name, NULL);
    }

end:
    sqlite3_free(type);
    if (ret != SQLITE_OK) {
        sqlite3_result_error_code(ctx, ret);
    }
}

// define_load creates the sqlean_define table if it doesn't exist, registers
// the scalar functions saved there, and returns SQLITE_OK or an error code.
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

    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
        const char* name = (const char*)sqlite3_column_text(stmt, 0);
        const char* body = (const char*)sqlite3_column_text(stmt, 1);
        if (name == NULL || body == NULL) {
            // not a function define() would save
            continue;
        }
        if ((ret = load_function(db, reg, name, body)) != SQLITE_OK) {
            break;
        }
    }
    sqlite3_finalize(stmt);
    return ret == SQLITE_DONE ? SQLITE_OK : ret;
}

// define_manage_init registers define(), undefine() and define_free(), loads
// the saved scalar functions, and returns SQLITE_OK or an error code.
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
