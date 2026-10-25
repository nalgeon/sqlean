// Adapted from https://sqlite.org/src/file/ext/misc/sha1.c
// Public domain

#ifndef __SHA1_H__
#define __SHA1_H__

#include <stddef.h>
#include <stdint.h>

#define SHA1_DIGEST_SIZE 20

typedef struct {
    uint32_t state[5];
    uint64_t count;  // number of bytes hashed so far
    uint8_t buffer[64];
} sha1_ctx;

// Incremental hashing: init, then update any number of times, then final.
// The context is owned by the caller. The data may be NULL if len is 0.
void sha1_init(sha1_ctx* ctx);
void sha1_update(sha1_ctx* ctx, const uint8_t* data, size_t len);
void sha1_final(sha1_ctx* ctx, uint8_t hash[SHA1_DIGEST_SIZE]);

// One-shot hashing of len bytes. The data may be NULL if len is 0.
void sha1_hash(const uint8_t* data, size_t len, uint8_t hash[SHA1_DIGEST_SIZE]);

#endif
