// Copyright 2009 The Go Authors, BSD 3-Clause License
// https://github.com/golang/go/blob/master/LICENSE

// Ported to C by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// SHA-256, SHA-384 and SHA-512 hash algorithms (FIPS 180-4).

#ifndef CRYPTO_SHA2_H
#define CRYPTO_SHA2_H

#include <stddef.h>
#include <stdint.h>

#define SHA256_DIGEST_SIZE 32
#define SHA384_DIGEST_SIZE 48
#define SHA512_DIGEST_SIZE 64

#define SHA256_BLOCK_SIZE 64
#define SHA512_BLOCK_SIZE 128

typedef struct {
    uint32_t h[8];                 // state
    uint8_t x[SHA256_BLOCK_SIZE];  // buffered input, not yet hashed
    size_t nx;                     // number of bytes in x
    uint64_t len;                  // total number of bytes hashed
} sha256_ctx;

typedef struct {
    uint64_t h[8];                 // state
    uint8_t x[SHA512_BLOCK_SIZE];  // buffered input, not yet hashed
    size_t nx;                     // number of bytes in x
    uint64_t len;                  // total number of bytes hashed
} sha512_ctx;

// SHA-384 is SHA-512 with a different initial state and a truncated digest.
typedef sha512_ctx sha384_ctx;

// Incremental hashing: init, then update any number of times, then final.
// The context is owned by the caller. The data may be NULL if len is 0.

void sha256_init(sha256_ctx* ctx);
void sha256_update(sha256_ctx* ctx, const uint8_t* data, size_t len);
void sha256_final(sha256_ctx* ctx, uint8_t digest[SHA256_DIGEST_SIZE]);

void sha384_init(sha384_ctx* ctx);
void sha384_update(sha384_ctx* ctx, const uint8_t* data, size_t len);
void sha384_final(sha384_ctx* ctx, uint8_t digest[SHA384_DIGEST_SIZE]);

void sha512_init(sha512_ctx* ctx);
void sha512_update(sha512_ctx* ctx, const uint8_t* data, size_t len);
void sha512_final(sha512_ctx* ctx, uint8_t digest[SHA512_DIGEST_SIZE]);

// One-shot hashing of len bytes. The data may be NULL if len is 0.

void sha256_hash(const uint8_t* data, size_t len, uint8_t digest[SHA256_DIGEST_SIZE]);
void sha384_hash(const uint8_t* data, size_t len, uint8_t digest[SHA384_DIGEST_SIZE]);
void sha512_hash(const uint8_t* data, size_t len, uint8_t digest[SHA512_DIGEST_SIZE]);

#endif  // CRYPTO_SHA2_H
