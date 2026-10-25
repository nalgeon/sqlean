/*********************************************************************
 * Filename:   md5.h
 * Author:     Brad Conte (brad AT bradconte.com)
 * Source:     https://github.com/B-Con/crypto-algorithms
 * License:    Public Domain
 * Details:    Defines the API for the corresponding MD5 implementation.
 *********************************************************************/

#ifndef MD5_H
#define MD5_H

#include <stddef.h>
#include <stdint.h>

#define MD5_DIGEST_SIZE 16

typedef struct {
    uint8_t data[64];
    uint32_t datalen;
    uint64_t bitlen;
    uint32_t state[4];
} md5_ctx;

// Incremental hashing: init, then update any number of times, then final.
// The context is owned by the caller. The data may be NULL if len is 0.
void md5_init(md5_ctx* ctx);
void md5_update(md5_ctx* ctx, const uint8_t* data, size_t len);
void md5_final(md5_ctx* ctx, uint8_t hash[MD5_DIGEST_SIZE]);

// One-shot hashing of len bytes. The data may be NULL if len is 0.
void md5_hash(const uint8_t* data, size_t len, uint8_t hash[MD5_DIGEST_SIZE]);

#endif  // MD5_H
