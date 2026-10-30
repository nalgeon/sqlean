// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// SQLite extension for working with UUIDs.

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT3

#include "uuid/uuid.h"

#pragma region Helpers

// result_text sets the canonical string form of u as the result.
static void result_text(sqlite3_context* context, UUID u) {
    char buf[UUID_STR_SIZE];
    size_t n = uuid_fmt(buf, sizeof(buf), u);
    sqlite3_result_text(context, buf, (int)n, SQLITE_TRANSIENT);
}

// arg_uuid decodes a UUID argument into out: a 16-byte blob, or text that
// uuid_parse accepts (numbers are converted to text). Sets the NULL result
// for anything else, and the error if out of memory. Returns false on failure.
static bool arg_uuid(sqlite3_context* context, sqlite3_value* value, UUID* out) {
    switch (sqlite3_value_type(value)) {
        case SQLITE_NULL:
            sqlite3_result_null(context);
            return false;
        case SQLITE_BLOB:
            if (sqlite3_value_bytes(value) != UUID_SIZE) {
                sqlite3_result_null(context);
                return false;
            }
            memcpy(out->b, sqlite3_value_blob(value), UUID_SIZE);
            return true;
        default: {
            const char* s = (const char*)sqlite3_value_text(value);
            if (s == NULL) {
                sqlite3_result_error_nomem(context);
                return false;
            }
            size_t n = (size_t)sqlite3_value_bytes(value);
            if (!uuid_parse(s, n, out)) {
                sqlite3_result_null(context);
                return false;
            }
            return true;
        }
    }
}

// arg_int64 decodes an integer argument into out, converting numeric text.
// Returns false if the value is not an integer.
static bool arg_int64(sqlite3_value* value, int64_t* out) {
    if (sqlite3_value_numeric_type(value) != SQLITE_INTEGER) {
        return false;
    }
    *out = sqlite3_value_int64(value);
    return true;
}

#pragma endregion

#pragma region Constructors

// uuid4()
static void fn_v4(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 0);
    uint8_t rand[UUID_SIZE];
    sqlite3_randomness(sizeof(rand), rand);
    result_text(context, uuid_v4(rand));
}

// uuid7([seconds])
static void fn_v7(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 0 || argc == 1);
    uint8_t rand[UUID_SIZE];
    sqlite3_randomness(sizeof(rand), rand);
    UUID u;
    if (argc == 0) {
        if (!uuid_v7_now(rand, &u)) {
            sqlite3_result_null(context);
            return;
        }
        result_text(context, u);
        return;
    }

    // Check the range before multiplying, so sec * 1000 can't overflow.
    int64_t sec;
    if (!arg_int64(argv[0], &sec) || sec < 0 || sec > UUID_V7_MAX_MS / 1000 ||
        !uuid_v7(sec * 1000, rand, &u)) {
        sqlite3_result_null(context);
        return;
    }
    result_text(context, u);
}

#pragma endregion

#pragma region Fields

// uuid7_timestamp_ms(uuid)
static void fn_v7_timestamp_ms(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    UUID u;
    if (!arg_uuid(context, argv[0], &u)) {
        return;
    }
    int64_t ms;
    if (!uuid_get_unix_ms(u, &ms)) {
        sqlite3_result_null(context);
        return;
    }
    sqlite3_result_int64(context, ms);
}

#pragma endregion

#pragma region Conversion

// uuid_str(uuid)
static void fn_str(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    UUID u;
    if (!arg_uuid(context, argv[0], &u)) {
        return;
    }
    result_text(context, u);
}

// uuid_blob(uuid)
static void fn_blob(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    UUID u;
    if (!arg_uuid(context, argv[0], &u)) {
        return;
    }
    sqlite3_result_blob(context, u.b, UUID_SIZE, SQLITE_TRANSIENT);
}

#pragma endregion

#pragma region Registration

#define FLAGS (SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC)
#define FLAGS_ND (SQLITE_UTF8 | SQLITE_INNOCUOUS)

// Function describes an SQL function to register.
typedef struct {
    const char* name;
    int nargs;
    int flags;
    void (*fn)(sqlite3_context*, int, sqlite3_value**);
} Function;

static const Function functions[] = {
    // constructors
    {"uuid4", 0, FLAGS_ND, fn_v4},
    {"uuid7", 0, FLAGS_ND, fn_v7},
    {"uuid7", 1, FLAGS_ND, fn_v7},

    // fields
    {"uuid7_timestamp_ms", 1, FLAGS, fn_v7_timestamp_ms},

    // conversion
    {"uuid_str", 1, FLAGS, fn_str},
    {"uuid_blob", 1, FLAGS, fn_blob},

    // aliases
    {"gen_random_uuid", 0, FLAGS_ND, fn_v4},
};

int uuid_init(sqlite3* db) {
    for (size_t i = 0; i < sizeof(functions) / sizeof(functions[0]); i++) {
        const Function* f = &functions[i];
        int rc = sqlite3_create_function(db, f->name, f->nargs, f->flags, 0, f->fn, 0, 0);
        if (rc != SQLITE_OK) {
            return rc;
        }
    }
    return SQLITE_OK;
}

#pragma endregion
