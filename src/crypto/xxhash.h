// Copyright (c) 2025 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// xxHash family of non-cryptographic hash functions (seed 0).
// Digests are written in canonical (big-endian) byte order.

#ifndef CRYPTO_XXHASH_H
#define CRYPTO_XXHASH_H

#include <stddef.h>
#include <stdint.h>

#define XXH32_DIGEST_SIZE 4
#define XXH64_DIGEST_SIZE 8
#define XXH3_64_DIGEST_SIZE 8
#define XXH3_128_DIGEST_SIZE 16

#define XXH32_STRIPE_SIZE 16
#define XXH64_STRIPE_SIZE 32

typedef struct {
    uint32_t v[4];                 // accumulators
    uint8_t x[XXH32_STRIPE_SIZE];  // buffered input, not yet mixed
    size_t nx;                     // number of bytes in x
    uint64_t len;                  // total number of bytes hashed
} xxh32_ctx;

typedef struct {
    uint64_t v[4];                 // accumulators
    uint8_t x[XXH64_STRIPE_SIZE];  // buffered input, not yet mixed
    size_t nx;                     // number of bytes in x
    uint64_t len;                  // total number of bytes hashed
} xxh64_ctx;

// Storage for xxHash's own XXH3 state, which is private (xxh3.c checks that
// it fits). It must be 64-byte aligned: declared objects are, but memory
// from malloc may not be.
typedef struct {
    _Alignas(64) uint64_t state[72];
} xxh3_64_ctx;

typedef xxh3_64_ctx xxh3_128_ctx;

// Incremental hashing: init, then update any number of times, then final.
// The context is owned by the caller. The data may be NULL if len is 0.
void xxh32_init(xxh32_ctx* ctx);
void xxh32_update(xxh32_ctx* ctx, const uint8_t* data, size_t len);
void xxh32_final(xxh32_ctx* ctx, uint8_t hash[XXH32_DIGEST_SIZE]);

void xxh64_init(xxh64_ctx* ctx);
void xxh64_update(xxh64_ctx* ctx, const uint8_t* data, size_t len);
void xxh64_final(xxh64_ctx* ctx, uint8_t hash[XXH64_DIGEST_SIZE]);

void xxh3_64_init(xxh3_64_ctx* ctx);
void xxh3_64_update(xxh3_64_ctx* ctx, const uint8_t* data, size_t len);
void xxh3_64_final(xxh3_64_ctx* ctx, uint8_t hash[XXH3_64_DIGEST_SIZE]);

void xxh3_128_init(xxh3_128_ctx* ctx);
void xxh3_128_update(xxh3_128_ctx* ctx, const uint8_t* data, size_t len);
void xxh3_128_final(xxh3_128_ctx* ctx, uint8_t hash[XXH3_128_DIGEST_SIZE]);

// One-shot hashing of len bytes. The data may be NULL if len is 0.
void xxh32_hash(const uint8_t* data, size_t len, uint8_t hash[XXH32_DIGEST_SIZE]);
void xxh64_hash(const uint8_t* data, size_t len, uint8_t hash[XXH64_DIGEST_SIZE]);
void xxh3_64_hash(const uint8_t* data, size_t len, uint8_t hash[XXH3_64_DIGEST_SIZE]);
void xxh3_128_hash(const uint8_t* data, size_t len, uint8_t hash[XXH3_128_DIGEST_SIZE]);

#endif  // CRYPTO_XXHASH_H
