// Created by: Peter Tripp (@notpeter), Public Domain
// blake3 reference implementation, Public Domain
// https://github.com/oconnor663/blake3_reference_impl_c

// Modified by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// BLAKE3 hash algorithm, default hash mode with a 32-byte output.

#ifndef CRYPTO_BLAKE3_H
#define CRYPTO_BLAKE3_H

#include <stddef.h>
#include <stdint.h>

#define BLAKE3_DIGEST_SIZE 32
#define BLAKE3_BLOCK_SIZE 64
#define BLAKE3_CHUNK_SIZE 1024

// The state of the chunk being hashed (private).
typedef struct {
    uint32_t cv[8];                    // chaining value
    uint64_t counter;                  // chunk index
    uint8_t block[BLAKE3_BLOCK_SIZE];  // buffered input, not yet compressed
    uint8_t block_len;                 // number of bytes in block
    uint8_t blocks_compressed;         // number of blocks compressed so far
} blake3_chunk;

typedef struct {
    blake3_chunk chunk;        // current chunk
    uint32_t cv_stack[54][8];  // chaining values of completed subtrees:
    uint8_t cv_stack_len;      // 54 levels hold 2^54 chunks = 2^64 bytes
} blake3_ctx;

// Incremental hashing: init, then update any number of times, then final.
// The context is owned by the caller. The data may be NULL if len is 0.
void blake3_init(blake3_ctx* ctx);
void blake3_update(blake3_ctx* ctx, const uint8_t* data, size_t len);
void blake3_final(blake3_ctx* ctx, uint8_t hash[BLAKE3_DIGEST_SIZE]);

// One-shot hashing of len bytes. The data may be NULL if len is 0.
void blake3_hash(const uint8_t* data, size_t len, uint8_t hash[BLAKE3_DIGEST_SIZE]);

#endif  // CRYPTO_BLAKE3_H
