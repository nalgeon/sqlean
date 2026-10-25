// Created by: Peter Tripp (@notpeter), Public Domain
// blake3 reference implementation, Public Domain
// https://github.com/oconnor663/blake3_reference_impl_c

#ifndef __BLAKE3_H__
#define __BLAKE3_H__

#include <stddef.h>
#include <stdint.h>

#define BLAKE3_OUT_LEN 32
#define BLAKE3_KEY_LEN 32
#define BLAKE3_BLOCK_LEN 64
#define BLAKE3_CHUNK_LEN 1024

// This struct is private.
typedef struct _blake3_chunk_state {
    uint32_t chaining_value[8];
    uint64_t chunk_counter;
    uint8_t block[BLAKE3_BLOCK_LEN];
    uint8_t block_len;
    uint8_t blocks_compressed;
    uint32_t flags;
} _blake3_chunk_state;

// An incremental hasher that can accept any number of writes.
typedef struct blake3_hasher {
    _blake3_chunk_state chunk_state;
    uint32_t key_words[8];
    uint32_t cv_stack[8 * 54];  // Space for 54 subtree chaining values:
    uint8_t cv_stack_len;       // 2^54 * CHUNK_LEN = 2^64
    uint32_t flags;
} blake3_hasher;

// Incremental hashing: init, then update any number of times, then final.
// The hasher is owned by the caller. The data may be NULL if len is 0.
void blake3_init(blake3_hasher* ctx);
void blake3_update(blake3_hasher* ctx, const uint8_t* data, size_t len);
void blake3_final(blake3_hasher* ctx, uint8_t hash[BLAKE3_OUT_LEN]);

// One-shot hashing of len bytes. The data may be NULL if len is 0.
void blake3_hash(const uint8_t* data, size_t len, uint8_t hash[BLAKE3_OUT_LEN]);

#endif
