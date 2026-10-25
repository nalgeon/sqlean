// Copyright 2009 The Go Authors, BSD 3-Clause License
// https://github.com/golang/go/blob/master/LICENSE

// Ported to C by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// SHA-1 hash algorithm (RFC 3174).

#ifndef CRYPTO_SHA1_H
#define CRYPTO_SHA1_H

#include <stddef.h>
#include <stdint.h>

#define SHA1_DIGEST_SIZE 20
#define SHA1_BLOCK_SIZE 64

typedef struct {
    uint32_t h[5];               // state
    uint8_t x[SHA1_BLOCK_SIZE];  // buffered input, not yet hashed
    size_t nx;                   // number of bytes in x
    uint64_t len;                // total number of bytes hashed
} sha1_ctx;

// Incremental hashing: init, then update any number of times, then final.
// The context is owned by the caller. The data may be NULL if len is 0.
void sha1_init(sha1_ctx* ctx);
void sha1_update(sha1_ctx* ctx, const uint8_t* data, size_t len);
void sha1_final(sha1_ctx* ctx, uint8_t hash[SHA1_DIGEST_SIZE]);

// One-shot hashing of len bytes. The data may be NULL if len is 0.
void sha1_hash(const uint8_t* data, size_t len, uint8_t hash[SHA1_DIGEST_SIZE]);

#endif  // CRYPTO_SHA1_H
