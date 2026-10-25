// Copyright (c) 2016 Caleb Spare, MIT License
// https://github.com/cespare/xxhash

// Ported to C by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// XXH32 and XXH64 hash algorithms with seed 0, as described in the xxHash
// specification (https://github.com/Cyan4973/xxHash/blob/dev/doc/xxhash_spec.md).
// XXH64 follows the Go implementation from github.com/cespare/xxhash.
// XXH32 is the same algorithm with 32-bit lanes and different constants.

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "crypto/common.h"
#include "crypto/xxhash.h"

#pragma region XXH32

#define P32_1 0x9E3779B1U
#define P32_2 0x85EBCA77U
#define P32_3 0xC2B2AE3DU
#define P32_4 0x27D4EB2FU
#define P32_5 0x165667B1U

static uint32_t round32(uint32_t acc, uint32_t lane) {
    acc += lane * P32_2;
    return rotl32(acc, 13) * P32_1;
}

// stripes32 mixes len bytes into the accumulators, 16 bytes at a time.
// The len must be a multiple of XXH32_STRIPE_SIZE.
static void stripes32(uint32_t v[4], const uint8_t* p, size_t len) {
    for (; len > 0; p += XXH32_STRIPE_SIZE, len -= XXH32_STRIPE_SIZE) {
        v[0] = round32(v[0], load_le32(p));
        v[1] = round32(v[1], load_le32(p + 4));
        v[2] = round32(v[2], load_le32(p + 8));
        v[3] = round32(v[3], load_le32(p + 12));
    }
}

void xxh32_init(xxh32_ctx* ctx) {
    ctx->v[0] = P32_1 + P32_2;
    ctx->v[1] = P32_2;
    ctx->v[2] = 0;
    ctx->v[3] = 0 - P32_1;
    ctx->nx = 0;
    ctx->len = 0;
}

void xxh32_update(xxh32_ctx* ctx, const uint8_t* data, size_t len) {
    if (len == 0) {
        // The data may be NULL, which memcpy does not accept even for 0 bytes.
        return;
    }
    ctx->len += len;
    // Top up the buffered input first.
    if (ctx->nx > 0) {
        size_t n = XXH32_STRIPE_SIZE - ctx->nx;
        if (n > len) {
            n = len;
        }
        memcpy(ctx->x + ctx->nx, data, n);
        ctx->nx += n;
        if (ctx->nx == XXH32_STRIPE_SIZE) {
            stripes32(ctx->v, ctx->x, XXH32_STRIPE_SIZE);
            ctx->nx = 0;
        }
        data += n;
        len -= n;
    }
    // Then mix full stripes straight from the input.
    if (len >= XXH32_STRIPE_SIZE) {
        size_t n = len - len % XXH32_STRIPE_SIZE;
        stripes32(ctx->v, data, n);
        data += n;
        len -= n;
    }
    // Then buffer the rest.
    if (len > 0) {
        memcpy(ctx->x, data, len);
        ctx->nx = len;
    }
}

void xxh32_final(xxh32_ctx* ctx, uint8_t hash[XXH32_DIGEST_SIZE]) {
    const uint32_t* v = ctx->v;
    uint32_t h;
    if (ctx->len >= XXH32_STRIPE_SIZE) {
        h = rotl32(v[0], 1) + rotl32(v[1], 7) + rotl32(v[2], 12) + rotl32(v[3], 18);
    } else {
        h = v[2] + P32_5;
    }
    // The length is added modulo 2^32.
    h += (uint32_t)ctx->len;

    // Mix in the buffered input that did not fill a stripe.
    const uint8_t* p = ctx->x;
    size_t n = ctx->nx;
    for (; n >= 4; p += 4, n -= 4) {
        h += load_le32(p) * P32_3;
        h = rotl32(h, 17) * P32_4;
    }
    for (; n > 0; p++, n--) {
        h += *p * P32_5;
        h = rotl32(h, 11) * P32_1;
    }

    // Avalanche.
    h ^= h >> 15;
    h *= P32_2;
    h ^= h >> 13;
    h *= P32_3;
    h ^= h >> 16;

    store_be32(hash, h);
}

void xxh32_hash(const uint8_t* data, size_t len, uint8_t hash[XXH32_DIGEST_SIZE]) {
    xxh32_ctx ctx;
    xxh32_init(&ctx);
    xxh32_update(&ctx, data, len);
    xxh32_final(&ctx, hash);
}

#pragma endregion

#pragma region XXH64

#define P64_1 0x9E3779B185EBCA87ULL
#define P64_2 0xC2B2AE3D27D4EB4FULL
#define P64_3 0x165667B19E3779F9ULL
#define P64_4 0x85EBCA77C2B2AE63ULL
#define P64_5 0x27D4EB2F165667C5ULL

static uint64_t round64(uint64_t acc, uint64_t lane) {
    acc += lane * P64_2;
    return rotl64(acc, 31) * P64_1;
}

static uint64_t merge_round64(uint64_t acc, uint64_t v) {
    acc ^= round64(0, v);
    return acc * P64_1 + P64_4;
}

// stripes64 mixes len bytes into the accumulators, 32 bytes at a time.
// The len must be a multiple of XXH64_STRIPE_SIZE.
static void stripes64(uint64_t v[4], const uint8_t* p, size_t len) {
    for (; len > 0; p += XXH64_STRIPE_SIZE, len -= XXH64_STRIPE_SIZE) {
        v[0] = round64(v[0], load_le64(p));
        v[1] = round64(v[1], load_le64(p + 8));
        v[2] = round64(v[2], load_le64(p + 16));
        v[3] = round64(v[3], load_le64(p + 24));
    }
}

void xxh64_init(xxh64_ctx* ctx) {
    ctx->v[0] = P64_1 + P64_2;
    ctx->v[1] = P64_2;
    ctx->v[2] = 0;
    ctx->v[3] = 0 - P64_1;
    ctx->nx = 0;
    ctx->len = 0;
}

void xxh64_update(xxh64_ctx* ctx, const uint8_t* data, size_t len) {
    if (len == 0) {
        // The data may be NULL, which memcpy does not accept even for 0 bytes.
        return;
    }
    ctx->len += len;
    // Top up the buffered input first.
    if (ctx->nx > 0) {
        size_t n = XXH64_STRIPE_SIZE - ctx->nx;
        if (n > len) {
            n = len;
        }
        memcpy(ctx->x + ctx->nx, data, n);
        ctx->nx += n;
        if (ctx->nx == XXH64_STRIPE_SIZE) {
            stripes64(ctx->v, ctx->x, XXH64_STRIPE_SIZE);
            ctx->nx = 0;
        }
        data += n;
        len -= n;
    }
    // Then mix full stripes straight from the input.
    if (len >= XXH64_STRIPE_SIZE) {
        size_t n = len - len % XXH64_STRIPE_SIZE;
        stripes64(ctx->v, data, n);
        data += n;
        len -= n;
    }
    // Then buffer the rest.
    if (len > 0) {
        memcpy(ctx->x, data, len);
        ctx->nx = len;
    }
}

void xxh64_final(xxh64_ctx* ctx, uint8_t hash[XXH64_DIGEST_SIZE]) {
    const uint64_t* v = ctx->v;
    uint64_t h;
    if (ctx->len >= XXH64_STRIPE_SIZE) {
        h = rotl64(v[0], 1) + rotl64(v[1], 7) + rotl64(v[2], 12) + rotl64(v[3], 18);
        h = merge_round64(h, v[0]);
        h = merge_round64(h, v[1]);
        h = merge_round64(h, v[2]);
        h = merge_round64(h, v[3]);
    } else {
        h = v[2] + P64_5;
    }
    h += ctx->len;

    // Mix in the buffered input that did not fill a stripe.
    const uint8_t* p = ctx->x;
    size_t n = ctx->nx;
    for (; n >= 8; p += 8, n -= 8) {
        h ^= round64(0, load_le64(p));
        h = rotl64(h, 27) * P64_1 + P64_4;
    }
    if (n >= 4) {
        h ^= load_le32(p) * P64_1;
        h = rotl64(h, 23) * P64_2 + P64_3;
        p += 4;
        n -= 4;
    }
    for (; n > 0; p++, n--) {
        h ^= *p * P64_5;
        h = rotl64(h, 11) * P64_1;
    }

    // Avalanche.
    h ^= h >> 33;
    h *= P64_2;
    h ^= h >> 29;
    h *= P64_3;
    h ^= h >> 32;

    store_be64(hash, h);
}

void xxh64_hash(const uint8_t* data, size_t len, uint8_t hash[XXH64_DIGEST_SIZE]) {
    xxh64_ctx ctx;
    xxh64_init(&ctx);
    xxh64_update(&ctx, data, len);
    xxh64_final(&ctx, hash);
}

#pragma endregion
