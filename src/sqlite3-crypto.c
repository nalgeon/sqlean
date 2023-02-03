// Copyright (c) 2021 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

/*
 * SQLite secure hash functions.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "crypto/md5.h"
#include "crypto/sha1.h"
#include "crypto/sha2.h"
#include "sqlite3ext.h"

SQLITE_EXTENSION_INIT1

/**
 * Generic compute hash function. Algorithm is encoded in the user data field.
 */
static void sqlite3_hash(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);

    void* (*init_func)() = NULL;
    void (*update_func)(void*, void*, size_t) = NULL;
    int (*final_func)(void*, void*) = NULL;
    int algo = (intptr_t)sqlite3_user_data(context);

    switch (algo) {
        case 1: /* Hardened SHA1 */
            init_func = (void*)sha1_init;
            update_func = (void*)sha1_update;
            final_func = (void*)sha1_final;
            algo = 1;
            break;
        case 5: /* MD5 */
            init_func = (void*)md5_init;
            update_func = (void*)md5_update;
            final_func = (void*)md5_final;
            algo = 1;
            break;
        case 2256: /* SHA2-256 */
            init_func = (void*)sha256_init;
            update_func = (void*)sha256_update;
            final_func = (void*)sha256_final;
            algo = 1;
            break;
        case 2384: /* SHA2-384 */
            init_func = (void*)sha384_init;
            update_func = (void*)sha384_update;
            final_func = (void*)sha384_final;
            algo = 1;
            break;
        case 2512: /* SHA2-512 */
            init_func = (void*)sha512_init;
            update_func = (void*)sha512_update;
            final_func = (void*)sha512_final;
            algo = 1;
            break;
        default:
            sqlite3_result_error(context, "Unknown Algorithm", -1);
            return;
    }

    void* ctx = NULL;
    if (algo) {
        ctx = init_func();
    }
    if (!ctx) {
        sqlite3_result_error(context, "Algorithm could not allocate it's context", -1);
        return;
    }

    void* data = NULL;
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        sqlite3_result_null(context);
        return;
    } else if (sqlite3_value_type(argv[0]) == SQLITE_BLOB) {
        data = (void*)sqlite3_value_blob(argv[0]);
    } else {
        data = (void*)sqlite3_value_text(argv[0]);
    }
    size_t datalen = sqlite3_value_bytes(argv[0]);
    if (datalen > 0)
        update_func(ctx, data, datalen);

    unsigned char hash[128] = {0};
    int hashlen = final_func(ctx, hash);
    sqlite3_result_blob(context, hash, hashlen, SQLITE_TRANSIENT);
}

/*
 * Registers the extension.
 */
#ifdef _WIN32
__declspec(dllexport)
#endif

    int sqlite3_crypto_init(sqlite3* db, char** pzErrMsg, const sqlite3_api_routines* pApi) {
    SQLITE_EXTENSION_INIT2(pApi);
    static const int flags = SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC;
    sqlite3_create_function(db, "md5", 1, flags, (void*)5, sqlite3_hash, 0, 0);
    sqlite3_create_function(db, "sha1", 1, flags, (void*)1, sqlite3_hash, 0, 0);
    sqlite3_create_function(db, "sha256", -1, flags, (void*)2256, sqlite3_hash, 0, 0);
    sqlite3_create_function(db, "sha384", -1, flags, (void*)2384, sqlite3_hash, 0, 0);
    sqlite3_create_function(db, "sha512", -1, flags, (void*)2512, sqlite3_hash, 0, 0);
    return SQLITE_OK;
}