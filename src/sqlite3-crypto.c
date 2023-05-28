// Copyright (c) 2021 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// SQLite hash and encode/decode functions.

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crypto/base32.h"
#include "crypto/base64.h"
#include "crypto/md5.h"
#include "crypto/sha1.h"
#include "crypto/sha2.h"
#include "sqlean.h"
#include "sqlite3ext.h"

SQLITE_EXTENSION_INIT1

// encoder/decoder function
typedef uint8_t* (*encdec_fn)(const uint8_t* src, size_t len, size_t* out_len);

// Generic compute hash function. Algorithm is encoded in the user data field.
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

// Encodes binary data into a textual representation using the specified encoder.
static void encode(sqlite3_context* context, int argc, sqlite3_value** argv, encdec_fn encode_fn) {
    assert(argc == 1);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        sqlite3_result_null(context);
        return;
    }
    size_t source_len = sqlite3_value_bytes(argv[0]);
    const uint8_t* source = (uint8_t*)sqlite3_value_blob(argv[0]);
    size_t result_len = 0;
    const char* result = (char*)encode_fn(source, source_len, &result_len);
    sqlite3_result_text(context, result, -1, free);
}

// Encodes binary data into a textual representation using the specified algorithm.
// encode('hello', 'base64') = 'aGVsbG8='
static void sqlite3_encode(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    size_t n = sqlite3_value_bytes(argv[1]);
    const char* format = (char*)sqlite3_value_text(argv[1]);
    if (strncmp(format, "base32", n) == 0) {
        encode(context, 1, argv, base32_encode);
        return;
    }
    if (strncmp(format, "base64", n) == 0) {
        encode(context, 1, argv, base64_encode);
        return;
    }
    sqlite3_result_error(context, "unknown encoding", -1);
}

// Decodes binary data from a textual representation using the specified decoder.
static void decode(sqlite3_context* context, int argc, sqlite3_value** argv, encdec_fn decode_fn) {
    assert(argc == 1);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        sqlite3_result_null(context);
        return;
    }

    size_t source_len = sqlite3_value_bytes(argv[0]);
    const uint8_t* source = (uint8_t*)sqlite3_value_text(argv[0]);
    if (source_len == 0) {
        sqlite3_result_zeroblob(context, 0);
        return;
    }

    size_t result_len = 0;
    const uint8_t* result = decode_fn(source, source_len, &result_len);
    if (result == NULL) {
        sqlite3_result_error(context, "invalid input string", -1);
        return;
    }

    sqlite3_result_blob(context, result, result_len, free);
}

// Decodes binary data from a textual representation using the specified algorithm.
// decode('aGVsbG8=', 'base64') = cast('hello' as blob)
static void sqlite3_decode(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    size_t n = sqlite3_value_bytes(argv[1]);
    const char* format = (char*)sqlite3_value_text(argv[1]);
    if (strncmp(format, "base32", n) == 0) {
        decode(context, 1, argv, base32_decode);
        return;
    }
    if (strncmp(format, "base64", n) == 0) {
        decode(context, 1, argv, base64_decode);
        return;
    }
    sqlite3_result_error(context, "unknown encoding", -1);
}

// Returns the current Sqlean version.
static void sqlean_version(sqlite3_context* context, int argc, sqlite3_value** argv) {
    sqlite3_result_text(context, SQLEAN_VERSION, -1, SQLITE_STATIC);
}

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_crypto_init(sqlite3* db, char** errmsg_ptr, const sqlite3_api_routines* api) {
    (void)errmsg_ptr;
    SQLITE_EXTENSION_INIT2(api);
    static const int flags = SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC;
    sqlite3_create_function(db, "md5", 1, flags, (void*)5, sqlite3_hash, 0, 0);
    sqlite3_create_function(db, "sha1", 1, flags, (void*)1, sqlite3_hash, 0, 0);
    sqlite3_create_function(db, "sha256", 1, flags, (void*)2256, sqlite3_hash, 0, 0);
    sqlite3_create_function(db, "sha384", 1, flags, (void*)2384, sqlite3_hash, 0, 0);
    sqlite3_create_function(db, "sha512", 1, flags, (void*)2512, sqlite3_hash, 0, 0);
    sqlite3_create_function(db, "encode", 2, flags, 0, sqlite3_encode, 0, 0);
    sqlite3_create_function(db, "decode", 2, flags, 0, sqlite3_decode, 0, 0);
    sqlite3_create_function(db, "sqlean_version", 0, flags, 0, sqlean_version, 0, 0);
    return SQLITE_OK;
}