// Copyright 2009 The Go Authors, BSD 3-Clause License
// https://github.com/golang/go/blob/master/LICENSE

// Ported to C by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// MD5 hash algorithm (RFC 1321).

#ifndef CRYPTO_MD5_H
#define CRYPTO_MD5_H

#include <stddef.h>
#include <stdint.h>

#define MD5_DIGEST_SIZE 16
#define MD5_BLOCK_SIZE 64

typedef struct {
    uint32_t h[4];              // state
    uint8_t x[MD5_BLOCK_SIZE];  // buffered input, not yet hashed
    size_t nx;                  // number of bytes in x
    uint64_t len;               // total number of bytes hashed
} md5_ctx;

// Incremental hashing: init, then update any number of times, then final.
// The context is owned by the caller. The data may be NULL if len is 0.
void md5_init(md5_ctx* ctx);
void md5_update(md5_ctx* ctx, const uint8_t* data, size_t len);
void md5_final(md5_ctx* ctx, uint8_t hash[MD5_DIGEST_SIZE]);

// One-shot hashing of len bytes. The data may be NULL if len is 0.
void md5_hash(const uint8_t* data, size_t len, uint8_t hash[MD5_DIGEST_SIZE]);

#endif  // CRYPTO_MD5_H
