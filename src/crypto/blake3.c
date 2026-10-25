// Created by: Peter Tripp (@notpeter), Public Domain
// blake3 reference implementation, Public Domain
// https://github.com/oconnor663/blake3_reference_impl_c

// Modified by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// BLAKE3 hash algorithm, following the reference implementation.
// Only the default hash mode with a 32-byte output is supported:
// no keyed hashing, key derivation or extendable output.

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "crypto/blake3.h"
#include "crypto/common.h"

// Domain separation flags.
enum {
    CHUNK_START = 1 << 0,
    CHUNK_END = 1 << 1,
    PARENT = 1 << 2,
    ROOT = 1 << 3,
};

static const uint32_t IV[8] = {
    0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19,
};

static const uint8_t MSG_PERMUTATION[16] = {2, 6, 3, 10, 7, 0, 4, 13, 1, 11, 12, 5, 9, 14, 15, 8};

#pragma region Compression

// The mixing function, G, which mixes either a column or a diagonal.
static void
g(uint32_t state[16], size_t a, size_t b, size_t c, size_t d, uint32_t mx, uint32_t my) {
    state[a] = state[a] + state[b] + mx;
    state[d] = rotr32(state[d] ^ state[a], 16);
    state[c] = state[c] + state[d];
    state[b] = rotr32(state[b] ^ state[c], 12);
    state[a] = state[a] + state[b] + my;
    state[d] = rotr32(state[d] ^ state[a], 8);
    state[c] = state[c] + state[d];
    state[b] = rotr32(state[b] ^ state[c], 7);
}

static void round_function(uint32_t state[16], const uint32_t m[16]) {
    // Mix the columns.
    g(state, 0, 4, 8, 12, m[0], m[1]);
    g(state, 1, 5, 9, 13, m[2], m[3]);
    g(state, 2, 6, 10, 14, m[4], m[5]);
    g(state, 3, 7, 11, 15, m[6], m[7]);
    // Mix the diagonals.
    g(state, 0, 5, 10, 15, m[8], m[9]);
    g(state, 1, 6, 11, 12, m[10], m[11]);
    g(state, 2, 7, 8, 13, m[12], m[13]);
    g(state, 3, 4, 9, 14, m[14], m[15]);
}

static void permute(uint32_t m[16]) {
    uint32_t permuted[16];
    for (size_t i = 0; i < 16; i++) {
        permuted[i] = m[MSG_PERMUTATION[i]];
    }
    memcpy(m, permuted, sizeof(permuted));
}

// compress mixes a 16-word block into the chaining value cv and writes
// the new chaining value to out. The full compression function produces
// 16 words, but only the first 8 are needed for a 32-byte output.
// out may alias cv.
static void compress(const uint32_t cv[8],
                     const uint32_t block[16],
                     uint64_t counter,
                     uint32_t block_len,
                     uint32_t flags,
                     uint32_t out[8]) {
    uint32_t state[16];
    memcpy(state, cv, 8 * sizeof(uint32_t));
    memcpy(state + 8, IV, 4 * sizeof(uint32_t));
    state[12] = (uint32_t)counter;
    state[13] = (uint32_t)(counter >> 32);
    state[14] = block_len;
    state[15] = flags;

    uint32_t m[16];
    memcpy(m, block, sizeof(m));
    UNROLL
    for (size_t r = 0; r < 7; r++) {
        round_function(state, m);
        if (r < 6) {
            permute(m);
        }
    }

    for (size_t i = 0; i < 8; i++) {
        out[i] = state[i] ^ state[i + 8];
    }
}

static void load_block(const uint8_t bytes[BLAKE3_BLOCK_SIZE], uint32_t words[16]) {
    for (size_t i = 0; i < 16; i++) {
        words[i] = load_le32(bytes + i * 4);
    }
}

#pragma endregion

#pragma region Tree nodes

// Each chunk or parent node can produce either an 8-word chaining value or,
// by setting the ROOT flag, the final hash. The output struct captures
// the state just prior to choosing between those two possibilities.
typedef struct {
    uint32_t cv[8];
    uint32_t block[16];
    uint64_t counter;
    uint32_t block_len;
    uint32_t flags;
} output;

static void output_cv(const output* self, uint32_t out[8]) {
    compress(self->cv, self->block, self->counter, self->block_len, self->flags, out);
}

static void output_root(const output* self, uint8_t hash[BLAKE3_DIGEST_SIZE]) {
    // The counter is the output block index, which is always 0
    // for a 32-byte output.
    uint32_t words[8];
    compress(self->cv, self->block, 0, self->block_len, self->flags | ROOT, words);
    for (size_t i = 0; i < 8; i++) {
        store_le32(hash + i * 4, words[i]);
    }
}

static void chunk_init(blake3_chunk* self, uint64_t counter) {
    memcpy(self->cv, IV, sizeof(self->cv));
    self->counter = counter;
    memset(self->block, 0, sizeof(self->block));
    self->block_len = 0;
    self->blocks_compressed = 0;
}

static size_t chunk_len(const blake3_chunk* self) {
    return BLAKE3_BLOCK_SIZE * (size_t)self->blocks_compressed + self->block_len;
}

static uint32_t chunk_start_flag(const blake3_chunk* self) {
    return self->blocks_compressed == 0 ? CHUNK_START : 0;
}

static void chunk_update(blake3_chunk* self, const uint8_t* input, size_t len) {
    while (len > 0) {
        // If the block buffer is full, compress it and clear it. More input is
        // coming, so this compression is not CHUNK_END.
        if (self->block_len == BLAKE3_BLOCK_SIZE) {
            uint32_t block[16];
            load_block(self->block, block);
            compress(self->cv, block, self->counter, BLAKE3_BLOCK_SIZE, chunk_start_flag(self),
                     self->cv);
            self->blocks_compressed++;
            memset(self->block, 0, sizeof(self->block));
            self->block_len = 0;
        }

        // Copy input bytes into the block buffer.
        size_t take = BLAKE3_BLOCK_SIZE - self->block_len;
        if (take > len) {
            take = len;
        }
        memcpy(self->block + self->block_len, input, take);
        self->block_len += (uint8_t)take;
        input += take;
        len -= take;
    }
}

static output chunk_output(const blake3_chunk* self) {
    output ret;
    memcpy(ret.cv, self->cv, sizeof(ret.cv));
    load_block(self->block, ret.block);
    ret.counter = self->counter;
    ret.block_len = self->block_len;
    ret.flags = chunk_start_flag(self) | CHUNK_END;
    return ret;
}

static output parent_output(const uint32_t left_cv[8], const uint32_t right_cv[8]) {
    output ret;
    memcpy(ret.cv, IV, sizeof(ret.cv));
    memcpy(ret.block, left_cv, 8 * sizeof(uint32_t));
    memcpy(ret.block + 8, right_cv, 8 * sizeof(uint32_t));
    ret.counter = 0;                    // Always 0 for parent nodes.
    ret.block_len = BLAKE3_BLOCK_SIZE;  // Always the full block for parent nodes.
    ret.flags = PARENT;
    return ret;
}

// Section 5.1.2 of the BLAKE3 spec explains this algorithm in more detail.
static void add_chunk_cv(blake3_ctx* ctx, uint32_t new_cv[8], uint64_t total_chunks) {
    // This chunk might complete some subtrees. For each completed subtree, its
    // left child will be the current top entry in the CV stack, and its right
    // child will be the current value of `new_cv`. Pop each left child off the
    // stack, merge it with `new_cv`, and overwrite `new_cv` with the result.
    // After all these merges, push the final value of `new_cv` onto the stack.
    // The number of completed subtrees is given by the number of trailing 0-bits
    // in the new total number of chunks.
    while ((total_chunks & 1) == 0) {
        ctx->cv_stack_len--;
        output parent = parent_output(ctx->cv_stack[ctx->cv_stack_len], new_cv);
        output_cv(&parent, new_cv);
        total_chunks >>= 1;
    }
    memcpy(ctx->cv_stack[ctx->cv_stack_len], new_cv, sizeof(ctx->cv_stack[0]));
    ctx->cv_stack_len++;
}

#pragma endregion

#pragma region Public API

void blake3_init(blake3_ctx* ctx) {
    chunk_init(&ctx->chunk, 0);
    ctx->cv_stack_len = 0;
}

void blake3_update(blake3_ctx* ctx, const uint8_t* data, size_t len) {
    while (len > 0) {
        // If the current chunk is complete, finalize it and reset the chunk state.
        // More input is coming, so this chunk is not ROOT.
        if (chunk_len(&ctx->chunk) == BLAKE3_CHUNK_SIZE) {
            output out = chunk_output(&ctx->chunk);
            uint32_t chunk_cv[8];
            output_cv(&out, chunk_cv);
            uint64_t total_chunks = ctx->chunk.counter + 1;
            add_chunk_cv(ctx, chunk_cv, total_chunks);
            chunk_init(&ctx->chunk, total_chunks);
        }

        // Compress input bytes into the current chunk state.
        size_t take = BLAKE3_CHUNK_SIZE - chunk_len(&ctx->chunk);
        if (take > len) {
            take = len;
        }
        chunk_update(&ctx->chunk, data, take);
        data += take;
        len -= take;
    }
}

void blake3_final(blake3_ctx* ctx, uint8_t hash[BLAKE3_DIGEST_SIZE]) {
    // Starting with the output from the current chunk, compute all the parent
    // chaining values along the right edge of the tree, until we have the root
    // output.
    output out = chunk_output(&ctx->chunk);
    for (size_t n = ctx->cv_stack_len; n > 0; n--) {
        uint32_t cv[8];
        output_cv(&out, cv);
        out = parent_output(ctx->cv_stack[n - 1], cv);
    }
    output_root(&out, hash);
}

void blake3_hash(const uint8_t* data, size_t len, uint8_t hash[BLAKE3_DIGEST_SIZE]) {
    blake3_ctx ctx;
    blake3_init(&ctx);
    blake3_update(&ctx, data, len);
    blake3_final(&ctx, hash);
}

#pragma endregion
