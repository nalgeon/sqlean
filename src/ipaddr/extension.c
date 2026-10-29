// Copyright (c) 2021 Vincent Bernat, MIT License
// https://github.com/nalgeon/sqlean

// IP address manipulation in SQLite.

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT3

#include "ipaddr/extension.h"
#include "ipaddr/ipaddr.h"

#pragma region Helpers

// has_null reports whether any of the arguments is NULL.
static bool has_null(int argc, sqlite3_value** argv) {
    for (int i = 0; i < argc; i++) {
        if (sqlite3_value_type(argv[i]) == SQLITE_NULL) {
            return true;
        }
    }
    return false;
}

// arg_prefix decodes an address or prefix argument into p. Numbers are
// converted to text. Sets the NULL result for a blob or a malformed value,
// and the error if out of memory. Returns false on failure.
static bool arg_prefix(sqlite3_context* context, sqlite3_value* arg, IPPrefix* p) {
    if (sqlite3_value_type(arg) == SQLITE_BLOB) {
        sqlite3_result_null(context);
        return false;
    }
    const char* s = (const char*)sqlite3_value_text(arg);
    if (s == NULL) {
        sqlite3_result_error_nomem(context);
        return false;
    }
    size_t n = (size_t)sqlite3_value_bytes(arg);
    if (!ipprefix_parse(s, n, p)) {
        sqlite3_result_null(context);
        return false;
    }
    return true;
}

#pragma endregion

#pragma region Addresses

// ip_family(ip)
static void fn_family(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    IPPrefix p;
    if (!arg_prefix(context, argv[0], &p)) {
        return;
    }
    sqlite3_result_int(context, (int)p.addr.family);
}

// ip_host(ip)
static void fn_host(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    IPPrefix p;
    if (!arg_prefix(context, argv[0], &p)) {
        return;
    }
    char buf[IPADDR_FMT_BUF_SIZE];
    size_t len = ipaddr_fmt(buf, sizeof(buf), p.addr);
    sqlite3_result_text(context, buf, (int)len, SQLITE_TRANSIENT);
}

#pragma endregion

#pragma region Prefixes

// ip_masklen(ip)
static void fn_masklen(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    IPPrefix p;
    if (!arg_prefix(context, argv[0], &p)) {
        return;
    }
    sqlite3_result_int(context, p.bits);
}

// ip_network(ip)
static void fn_network(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    IPPrefix p;
    if (!arg_prefix(context, argv[0], &p)) {
        return;
    }
    char buf[IPADDR_FMT_BUF_SIZE];
    size_t len = ipprefix_fmt(buf, sizeof(buf), ipprefix_masked(p));
    sqlite3_result_text(context, buf, (int)len, SQLITE_TRANSIENT);
}

// ip_contains(subnet, ip)
static void fn_contains(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    IPPrefix subnet, ip;
    if (!arg_prefix(context, argv[0], &subnet) || !arg_prefix(context, argv[1], &ip)) {
        return;
    }
    sqlite3_result_int(context, ipprefix_contains(subnet, ip));
}

#pragma endregion

#pragma region Registration

#define FLAGS (SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC)

// Function describes an SQL function to register.
typedef struct {
    const char* name;
    int nargs;
    int flags;
    void* user_data;
    void (*fn)(sqlite3_context* context, int argc, sqlite3_value** argv);
} Function;

static const Function functions[] = {
    // addresses
    {"ip_family", 1, FLAGS, 0, fn_family},
    {"ip_host", 1, FLAGS, 0, fn_host},

    // prefixes
    {"ip_masklen", 1, FLAGS, 0, fn_masklen},
    {"ip_network", 1, FLAGS, 0, fn_network},
    {"ip_contains", 2, FLAGS, 0, fn_contains},

    // aliases
    {"ipfamily", 1, FLAGS, 0, fn_family},
    {"iphost", 1, FLAGS, 0, fn_host},
    {"ipmasklen", 1, FLAGS, 0, fn_masklen},
    {"ipnetwork", 1, FLAGS, 0, fn_network},
    {"ipcontains", 2, FLAGS, 0, fn_contains},
};

int ipaddr_init(sqlite3* db) {
    for (size_t i = 0; i < sizeof(functions) / sizeof(functions[0]); i++) {
        const Function* f = &functions[i];
        int rc =
            sqlite3_create_function(db, f->name, f->nargs, f->flags, f->user_data, f->fn, 0, 0);
        if (rc != SQLITE_OK) {
            return rc;
        }
    }
    return SQLITE_OK;
}

#pragma endregion
