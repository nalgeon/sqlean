// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// SQLite hash and encode/decode functions.

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT3

#include "crypto/base32.h"
#include "crypto/base64.h"
#include "crypto/base85.h"
#include "crypto/blake3.h"
#include "crypto/hex.h"
#include "crypto/md5.h"
#include "crypto/sha1.h"
#include "crypto/sha2.h"
#include "crypto/url.h"
#include "crypto/xxhash.h"

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

// arg_data reads a data argument as bytes: a blob as is, and any other
// value as its text representation. Sets a nomem error and returns false
// if the conversion fails. The data may be NULL if len is 0.
static bool arg_data(sqlite3_context* context,
                     sqlite3_value* value,
                     const uint8_t** data,
                     size_t* len) {
    if (sqlite3_value_type(value) == SQLITE_BLOB) {
        *data = sqlite3_value_blob(value);
    } else {
        *data = sqlite3_value_text(value);
    }
    *len = (size_t)sqlite3_value_bytes(value);
    if (*data == NULL && *len > 0) {
        sqlite3_result_error_nomem(context);
        return false;
    }
    return true;
}

#pragma endregion

#pragma region Hashing

// Hash describes a hash algorithm: its one-shot function for the scalar
// functions, and its incremental API for the aggregates.
typedef struct {
    size_t size;  // digest size in bytes
    void (*hash)(const uint8_t* data, size_t len, uint8_t* out);
    size_t ctx_size;  // context size in bytes
    void (*init)(void* ctx);
    void (*update)(void* ctx, const uint8_t* data, size_t len);
    void (*final)(void* ctx, uint8_t* out);
} Hash;

// The largest digest size of all the algorithms.
#define MAX_DIGEST_SIZE SHA512_DIGEST_SIZE

// DEFINE_HASH defines hash_<name>, with adapters that give the incremental
// functions of the algorithm a common signature.
#define DEFINE_HASH(name, ctx_type, digest_size)                                           \
    _Static_assert(digest_size <= MAX_DIGEST_SIZE, #name " digest too large");             \
    _Static_assert(_Alignof(ctx_type) <= 8, #name " context misaligned");                  \
    static void name##_init_any(void* ctx) {                                               \
        name##_init(ctx);                                                                  \
    }                                                                                      \
    static void name##_update_any(void* ctx, const uint8_t* data, size_t len) {            \
        name##_update(ctx, data, len);                                                     \
    }                                                                                      \
    static void name##_final_any(void* ctx, uint8_t* out) {                                \
        name##_final(ctx, out);                                                            \
    }                                                                                      \
    static const Hash hash_##name = {digest_size,     name##_hash,       sizeof(ctx_type), \
                                     name##_init_any, name##_update_any, name##_final_any}

DEFINE_HASH(blake3, blake3_ctx, BLAKE3_DIGEST_SIZE);
DEFINE_HASH(md5, md5_ctx, MD5_DIGEST_SIZE);
DEFINE_HASH(sha1, sha1_ctx, SHA1_DIGEST_SIZE);
DEFINE_HASH(sha256, sha256_ctx, SHA256_DIGEST_SIZE);
DEFINE_HASH(sha384, sha384_ctx, SHA384_DIGEST_SIZE);
DEFINE_HASH(sha512, sha512_ctx, SHA512_DIGEST_SIZE);
DEFINE_HASH(xxh32, xxh32_ctx, XXH32_DIGEST_SIZE);
DEFINE_HASH(xxh64, xxh64_ctx, XXH64_DIGEST_SIZE);
DEFINE_HASH(xxh3_64, xxh3_64_ctx, XXH3_64_DIGEST_SIZE);
DEFINE_HASH(xxh3_128, xxh3_128_ctx, XXH3_128_DIGEST_SIZE);

// crypto_blake3(data)
// crypto_md5(data)
// crypto_sha1(data)
// crypto_sha256(data)
// crypto_sha384(data)
// crypto_sha512(data)
// crypto_xxh32(data)
// crypto_xxh64(data)
// crypto_xxh3_64(data)
// crypto_xxh3_128(data)
static void fn_hash(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }

    const uint8_t* data;
    size_t len;
    if (!arg_data(context, argv[0], &data, &len)) {
        return;
    }

    const Hash* hash = sqlite3_user_data(context);
    uint8_t digest[MAX_DIGEST_SIZE];
    hash->hash(data, len, digest);
    sqlite3_result_blob(context, digest, (int)hash->size, SQLITE_TRANSIENT);
}

// The aggregate memory holds a flag telling whether the hash context is
// initialized, followed by the context itself. SQLite gives zeroed, 8-byte
// aligned memory, and the flag is 8 bytes, so the context is aligned too.
typedef struct {
    uint64_t initialized;
    uint8_t ctx[];
} AggHash;

// crypto_blake3_agg(data)
// crypto_md5_agg(data)
// crypto_sha1_agg(data)
// crypto_sha256_agg(data)
// crypto_sha384_agg(data)
// crypto_sha512_agg(data)
// crypto_xxh32_agg(data)
// crypto_xxh64_agg(data)
// crypto_xxh3_64_agg(data)
// crypto_xxh3_128_agg(data)
static void agg_hash_step(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        return;
    }

    const uint8_t* data;
    size_t len;
    if (!arg_data(context, argv[0], &data, &len)) {
        return;
    }

    const Hash* hash = sqlite3_user_data(context);
    AggHash* agg = sqlite3_aggregate_context(context, (int)(sizeof(AggHash) + hash->ctx_size));
    if (agg == NULL) {
        sqlite3_result_error_nomem(context);
        return;
    }
    if (!agg->initialized) {
        hash->init(agg->ctx);
        agg->initialized = 1;
    }
    hash->update(agg->ctx, data, len);
}

static void agg_hash_final(sqlite3_context* context) {
    AggHash* agg = sqlite3_aggregate_context(context, 0);
    if (agg == NULL) {
        // No rows, or only NULLs.
        sqlite3_result_null(context);
        return;
    }

    const Hash* hash = sqlite3_user_data(context);
    uint8_t digest[MAX_DIGEST_SIZE];
    hash->final(agg->ctx, digest);
    sqlite3_result_blob(context, digest, (int)hash->size, SQLITE_TRANSIENT);
}

#pragma endregion

#pragma region Encoding

// Codec describes a binary-to-text encoding.
typedef struct {
    const char* name;
    size_t (*encoded_size)(size_t len);
    size_t (*encode)(const uint8_t* src, size_t len, uint8_t* dst);
    size_t (*decoded_size)(const uint8_t* src, size_t len);
    bool (*decode)(const uint8_t* src, size_t len, uint8_t* dst, size_t* out_len);
} Codec;

static const Codec codecs[] = {
    {"base32", base32_encoded_size, base32_encode, base32_decoded_size, base32_decode},
    {"base64", base64_encoded_size, base64_encode, base64_decoded_size, base64_decode},
    {"base85", base85_encoded_size, base85_encode, base85_decoded_size, base85_decode},
    {"hex", hex_encoded_size, hex_encode, hex_decoded_size, hex_decode},
    {"url", url_encoded_size, url_encode, url_decoded_size, url_decode},
};

// arg_codec decodes an encoding name argument. Encoding names are spec
// arguments, so an invalid one is an error, not NULL. Sets the error and
// returns NULL if the value is not a known encoding name.
static const Codec* arg_codec(sqlite3_context* context, sqlite3_value* value) {
    if (sqlite3_value_type(value) == SQLITE_BLOB) {
        sqlite3_result_error(context, "encoding name should be text", -1);
        return NULL;
    }
    const char* name = (const char*)sqlite3_value_text(value);
    if (name == NULL) {
        sqlite3_result_error_nomem(context);
        return NULL;
    }

    for (size_t i = 0; i < sizeof(codecs) / sizeof(codecs[0]); i++) {
        if (strcmp(name, codecs[i].name) == 0) {
            return &codecs[i];
        }
    }

    char* msg = sqlite3_mprintf("unknown encoding: %s", name);
    if (msg == NULL) {
        sqlite3_result_error_nomem(context);
        return NULL;
    }
    sqlite3_result_error(context, msg, -1);
    sqlite3_free(msg);
    return NULL;
}

// alloc_buf allocates a result buffer of the given size (non-zero).
// Sets an error and returns NULL if the allocation fails.
static uint8_t* alloc_buf(sqlite3_context* context, size_t size) {
    assert(size > 0);
    if (size == SIZE_MAX) {
        // The size does not fit into size_t (see the *_size functions).
        sqlite3_result_error_toobig(context);
        return NULL;
    }
    uint8_t* buf = sqlite3_malloc64(size);
    if (buf == NULL) {
        sqlite3_result_error_nomem(context);
        return NULL;
    }
    return buf;
}

// crypto_encode(data, encoding)
static void fn_encode(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }

    const Codec* codec = arg_codec(context, argv[1]);
    if (codec == NULL) {
        return;
    }
    const uint8_t* data;
    size_t len;
    if (!arg_data(context, argv[0], &data, &len)) {
        return;
    }

    size_t size = codec->encoded_size(len);
    if (size == 0) {
        sqlite3_result_text(context, "", 0, SQLITE_STATIC);
        return;
    }
    uint8_t* buf = alloc_buf(context, size);
    if (buf == NULL) {
        return;
    }
    size_t n = codec->encode(data, len, buf);
    assert(n <= size);
    // The encodings are ASCII, so the result is valid UTF-8.
    sqlite3_result_text64(context, (const char*)buf, n, sqlite3_free, SQLITE_UTF8);
}

// crypto_decode(text, encoding)
static void fn_decode(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }

    const Codec* codec = arg_codec(context, argv[1]);
    if (codec == NULL) {
        return;
    }
    const uint8_t* data;
    size_t len;
    if (!arg_data(context, argv[0], &data, &len)) {
        return;
    }

    size_t size = codec->decoded_size(data, len);
    if (size == 0) {
        // Nothing to write, but the input may still be invalid (e.g. '=').
        size_t n;
        uint8_t dummy;
        if (!codec->decode(data, len, &dummy, &n)) {
            sqlite3_result_null(context);
            return;
        }
        assert(n == 0);
        sqlite3_result_zeroblob(context, 0);
        return;
    }
    uint8_t* buf = alloc_buf(context, size);
    if (buf == NULL) {
        return;
    }
    size_t n;
    if (!codec->decode(data, len, buf, &n)) {
        sqlite3_free(buf);
        sqlite3_result_null(context);
        return;
    }
    assert(n <= size);
    sqlite3_result_blob64(context, buf, n, sqlite3_free);
}

#pragma endregion

#define FLAGS (SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC)

// Function describes an SQL function to register.
typedef struct {
    const char* name;
    int nargs;
    int flags;
    const void* user_data;
    // a scalar function has fn, an aggregate has step and final
    void (*fn)(sqlite3_context* context, int argc, sqlite3_value** argv);
    void (*step)(sqlite3_context* context, int argc, sqlite3_value** argv);
    void (*final)(sqlite3_context* context);
} Function;

static const Function functions[] = {
    // hashing
    {"crypto_blake3", 1, FLAGS, &hash_blake3, fn_hash, 0, 0},
    {"crypto_md5", 1, FLAGS, &hash_md5, fn_hash, 0, 0},
    {"crypto_sha1", 1, FLAGS, &hash_sha1, fn_hash, 0, 0},
    {"crypto_sha256", 1, FLAGS, &hash_sha256, fn_hash, 0, 0},
    {"crypto_sha384", 1, FLAGS, &hash_sha384, fn_hash, 0, 0},
    {"crypto_sha512", 1, FLAGS, &hash_sha512, fn_hash, 0, 0},
    {"crypto_xxh32", 1, FLAGS, &hash_xxh32, fn_hash, 0, 0},
    {"crypto_xxh64", 1, FLAGS, &hash_xxh64, fn_hash, 0, 0},
    {"crypto_xxh3_64", 1, FLAGS, &hash_xxh3_64, fn_hash, 0, 0},
    {"crypto_xxh3_128", 1, FLAGS, &hash_xxh3_128, fn_hash, 0, 0},

    // hash aggregates
    {"crypto_blake3_agg", 1, FLAGS, &hash_blake3, 0, agg_hash_step, agg_hash_final},
    {"crypto_md5_agg", 1, FLAGS, &hash_md5, 0, agg_hash_step, agg_hash_final},
    {"crypto_sha1_agg", 1, FLAGS, &hash_sha1, 0, agg_hash_step, agg_hash_final},
    {"crypto_sha256_agg", 1, FLAGS, &hash_sha256, 0, agg_hash_step, agg_hash_final},
    {"crypto_sha384_agg", 1, FLAGS, &hash_sha384, 0, agg_hash_step, agg_hash_final},
    {"crypto_sha512_agg", 1, FLAGS, &hash_sha512, 0, agg_hash_step, agg_hash_final},
    {"crypto_xxh32_agg", 1, FLAGS, &hash_xxh32, 0, agg_hash_step, agg_hash_final},
    {"crypto_xxh64_agg", 1, FLAGS, &hash_xxh64, 0, agg_hash_step, agg_hash_final},
    {"crypto_xxh3_64_agg", 1, FLAGS, &hash_xxh3_64, 0, agg_hash_step, agg_hash_final},
    {"crypto_xxh3_128_agg", 1, FLAGS, &hash_xxh3_128, 0, agg_hash_step, agg_hash_final},

    // encoding
    {"crypto_encode", 2, FLAGS, 0, fn_encode, 0, 0},
    {"crypto_decode", 2, FLAGS, 0, fn_decode, 0, 0},

    // aliases
    {"blake3", 1, FLAGS, &hash_blake3, fn_hash, 0, 0},
    {"md5", 1, FLAGS, &hash_md5, fn_hash, 0, 0},
    {"sha1", 1, FLAGS, &hash_sha1, fn_hash, 0, 0},
    {"sha256", 1, FLAGS, &hash_sha256, fn_hash, 0, 0},
    {"sha384", 1, FLAGS, &hash_sha384, fn_hash, 0, 0},
    {"sha512", 1, FLAGS, &hash_sha512, fn_hash, 0, 0},
    {"xxh32", 1, FLAGS, &hash_xxh32, fn_hash, 0, 0},
    {"xxh64", 1, FLAGS, &hash_xxh64, fn_hash, 0, 0},
    {"xxh3_64", 1, FLAGS, &hash_xxh3_64, fn_hash, 0, 0},
    {"xxh3_128", 1, FLAGS, &hash_xxh3_128, fn_hash, 0, 0},
    {"blake3_agg", 1, FLAGS, &hash_blake3, 0, agg_hash_step, agg_hash_final},
    {"md5_agg", 1, FLAGS, &hash_md5, 0, agg_hash_step, agg_hash_final},
    {"sha1_agg", 1, FLAGS, &hash_sha1, 0, agg_hash_step, agg_hash_final},
    {"sha256_agg", 1, FLAGS, &hash_sha256, 0, agg_hash_step, agg_hash_final},
    {"sha384_agg", 1, FLAGS, &hash_sha384, 0, agg_hash_step, agg_hash_final},
    {"sha512_agg", 1, FLAGS, &hash_sha512, 0, agg_hash_step, agg_hash_final},
    {"xxh32_agg", 1, FLAGS, &hash_xxh32, 0, agg_hash_step, agg_hash_final},
    {"xxh64_agg", 1, FLAGS, &hash_xxh64, 0, agg_hash_step, agg_hash_final},
    {"xxh3_64_agg", 1, FLAGS, &hash_xxh3_64, 0, agg_hash_step, agg_hash_final},
    {"xxh3_128_agg", 1, FLAGS, &hash_xxh3_128, 0, agg_hash_step, agg_hash_final},
    {"encode", 2, FLAGS, 0, fn_encode, 0, 0},
    {"decode", 2, FLAGS, 0, fn_decode, 0, 0},
};

int crypto_init(sqlite3* db) {
    for (size_t i = 0; i < sizeof(functions) / sizeof(functions[0]); i++) {
        const Function* f = &functions[i];
        // The user data is never written to, so casting away const is safe.
        int rc = sqlite3_create_function(db, f->name, f->nargs, f->flags, (void*)f->user_data,
                                         f->fn, f->step, f->final);
        if (rc != SQLITE_OK) {
            return rc;
        }
    }
    return SQLITE_OK;
}
