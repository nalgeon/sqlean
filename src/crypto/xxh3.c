// Copyright (c) 2019 Jeff Wendling, BSD 2-Clause License
// https://github.com/zeebo/xxh3

// Ported to C by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// XXH3 hash algorithm (64 and 128-bit) with seed 0 and the default secret,
// as described in the xxHash specification
// (https://github.com/Cyan4973/xxHash/blob/dev/doc/xxhash_spec.md).
// The buffering follows the Go implementation from github.com/zeebo/xxh3.
// Only the portable scalar code: no SIMD.

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "crypto/common.h"
#include "crypto/xxhash.h"

#define P32_1 0x9E3779B1U
#define P32_2 0x85EBCA77U
#define P32_3 0xC2B2AE3DU

#define P64_1 0x9E3779B185EBCA87ULL
#define P64_2 0xC2B2AE3D27D4EB4FULL
#define P64_3 0x165667B19E3779F9ULL
#define P64_4 0x85EBCA77C2B2AE63ULL
#define P64_5 0x27D4EB2F165667C5ULL

#define SECRET_SIZE 192
#define STRIPES_PER_BLOCK ((SECRET_SIZE - XXH3_STRIPE_SIZE) / 8)

// The default secret. Each part of the algorithm reads its own offsets of it.
static const uint8_t SECRET[SECRET_SIZE] = {
    0xb8, 0xfe, 0x6c, 0x39, 0x23, 0xa4, 0x4b, 0xbe, 0x7c, 0x01, 0x81, 0x2c, 0xf7, 0x21, 0xad, 0x1c,
    0xde, 0xd4, 0x6d, 0xe9, 0x83, 0x90, 0x97, 0xdb, 0x72, 0x40, 0xa4, 0xa4, 0xb7, 0xb3, 0x67, 0x1f,
    0xcb, 0x79, 0xe6, 0x4e, 0xcc, 0xc0, 0xe5, 0x78, 0x82, 0x5a, 0xd0, 0x7d, 0xcc, 0xff, 0x72, 0x21,
    0xb8, 0x08, 0x46, 0x74, 0xf7, 0x43, 0x24, 0x8e, 0xe0, 0x35, 0x90, 0xe6, 0x81, 0x3a, 0x26, 0x4c,
    0x3c, 0x28, 0x52, 0xbb, 0x91, 0xc3, 0x00, 0xcb, 0x88, 0xd0, 0x65, 0x8b, 0x1b, 0x53, 0x2e, 0xa3,
    0x71, 0x64, 0x48, 0x97, 0xa2, 0x0d, 0xf9, 0x4e, 0x38, 0x19, 0xef, 0x46, 0xa9, 0xde, 0xac, 0xd8,
    0xa8, 0xfa, 0x76, 0x3f, 0xe3, 0x9c, 0x34, 0x3f, 0xf9, 0xdc, 0xbb, 0xc7, 0xc7, 0x0b, 0x4f, 0x1d,
    0x8a, 0x51, 0xe0, 0x4b, 0xcd, 0xb4, 0x59, 0x31, 0xc8, 0x9f, 0x7e, 0xc9, 0xd9, 0x78, 0x73, 0x64,
    0xea, 0xc5, 0xac, 0x83, 0x34, 0xd3, 0xeb, 0xc3, 0xc5, 0x81, 0xa0, 0xff, 0xfa, 0x13, 0x63, 0xeb,
    0x17, 0x0d, 0xdd, 0x51, 0xb7, 0xf0, 0xda, 0x49, 0xd3, 0x16, 0x55, 0x26, 0x29, 0xd4, 0x68, 0x9e,
    0x2b, 0x16, 0xbe, 0x58, 0x7d, 0x47, 0xa1, 0xfc, 0x8f, 0xf8, 0xb8, 0xd1, 0x7a, 0xd0, 0x31, 0xce,
    0x45, 0xcb, 0x3a, 0x8f, 0x95, 0x16, 0x04, 0x28, 0xaf, 0xd7, 0xfb, 0xca, 0xbb, 0x4b, 0x40, 0x7e,
};

#pragma region Primitives

typedef struct {
    uint64_t lo, hi;
} u128;

// mul128 returns the full 128-bit product of x and y.
static u128 mul128(uint64_t x, uint64_t y) {
#ifdef __SIZEOF_INT128__
    unsigned __int128 p = (unsigned __int128)x * y;
    return (u128){(uint64_t)p, (uint64_t)(p >> 64)};
#else
    // Schoolbook multiplication of the 32-bit halves.
    uint64_t lo_lo = (x & 0xFFFFFFFF) * (y & 0xFFFFFFFF);
    uint64_t hi_lo = (x >> 32) * (y & 0xFFFFFFFF);
    uint64_t lo_hi = (x & 0xFFFFFFFF) * (y >> 32);
    uint64_t hi_hi = (x >> 32) * (y >> 32);
    uint64_t cross = (lo_lo >> 32) + (hi_lo & 0xFFFFFFFF) + lo_hi;
    uint64_t hi = (hi_lo >> 32) + (cross >> 32) + hi_hi;
    uint64_t lo = (cross << 32) | (lo_lo & 0xFFFFFFFF);
    return (u128){lo, hi};
#endif
}

// mul_fold64 multiplies x and y and folds the 128-bit product into 64 bits.
static uint64_t mul_fold64(uint64_t x, uint64_t y) {
    u128 p = mul128(x, y);
    return p.lo ^ p.hi;
}

// The XXH64 avalanche.
static uint64_t xxh64_avalanche(uint64_t h) {
    h ^= h >> 33;
    h *= P64_2;
    h ^= h >> 29;
    h *= P64_3;
    h ^= h >> 32;
    return h;
}

static uint64_t xxh3_avalanche(uint64_t h) {
    h ^= h >> 37;
    h *= 0x165667919E3779F9ULL;
    h ^= h >> 32;
    return h;
}

// A stronger avalanche for 4-8 byte inputs.
static uint64_t rrmxmx(uint64_t h, uint64_t len) {
    h ^= rotl64(h, 49) ^ rotl64(h, 24);
    h *= 0x9FB21C651E98DF25ULL;
    h ^= (h >> 35) + len;
    h *= 0x9FB21C651E98DF25ULL;
    h ^= h >> 28;
    return h;
}

// mix16 mixes 16 bytes of input with 16 bytes of the secret.
static uint64_t mix16(const uint8_t* p, const uint8_t* s) {
    return mul_fold64(load_le64(p) ^ load_le64(s), load_le64(p + 8) ^ load_le64(s + 8));
}

// mix32 mixes two 16-byte inputs into a 128-bit accumulator.
static u128 mix32(u128 acc, const uint8_t* p1, const uint8_t* p2, const uint8_t* s) {
    acc.lo += mix16(p1, s);
    acc.lo ^= load_le64(p2) + load_le64(p2 + 8);
    acc.hi += mix16(p2, s + 16);
    acc.hi ^= load_le64(p1) + load_le64(p1 + 8);
    return acc;
}

#pragma endregion

#pragma region Short inputs

// Inputs of up to 240 bytes are hashed directly, with a separate
// algorithm for each length range.

static uint64_t hash64_short(const uint8_t* p, size_t len) {
    const uint8_t* s = SECRET;

    if (len == 0) {
        return xxh64_avalanche(load_le64(s + 56) ^ load_le64(s + 64));
    }

    if (len <= 3) {
        uint32_t combined = (uint32_t)p[0] << 16 | (uint32_t)p[len >> 1] << 24 |
                            (uint32_t)p[len - 1] | (uint32_t)len << 8;
        uint64_t bitflip = load_le32(s) ^ load_le32(s + 4);
        return xxh64_avalanche(combined ^ bitflip);
    }

    if (len <= 8) {
        uint64_t input = load_le32(p + len - 4) + ((uint64_t)load_le32(p) << 32);
        uint64_t bitflip = load_le64(s + 8) ^ load_le64(s + 16);
        return rrmxmx(input ^ bitflip, len);
    }

    if (len <= 16) {
        uint64_t lo = load_le64(p) ^ (load_le64(s + 24) ^ load_le64(s + 32));
        uint64_t hi = load_le64(p + len - 8) ^ (load_le64(s + 40) ^ load_le64(s + 48));
        return xxh3_avalanche(len + bswap64(lo) + hi + mul_fold64(lo, hi));
    }

    if (len <= 128) {
        // Mix 16-byte pairs from both ends.
        uint64_t acc = len * P64_1;
        for (size_t i = 0; i <= (len - 1) / 32; i++) {
            acc += mix16(p + 16 * i, s + 32 * i);
            acc += mix16(p + len - 16 * (i + 1), s + 32 * i + 16);
        }
        return xxh3_avalanche(acc);
    }

    // 129-240 bytes: mix the first 128 bytes, then the rest
    // with the secret shifted by 3 bytes, then the last 16 bytes.
    uint64_t acc = len * P64_1;
    for (size_t i = 0; i < 8; i++) {
        acc += mix16(p + 16 * i, s + 16 * i);
    }
    acc = xxh3_avalanche(acc);
    for (size_t i = 8; i < len / 16; i++) {
        acc += mix16(p + 16 * i, s + 16 * (i - 8) + 3);
    }
    acc += mix16(p + len - 16, s + 119);
    return xxh3_avalanche(acc);
}

// mid128_final turns the accumulator for 17-240 byte inputs into the hash.
static u128 mid128_final(u128 acc, size_t len) {
    u128 h;
    h.lo = xxh3_avalanche(acc.lo + acc.hi);
    h.hi = 0 - xxh3_avalanche(acc.lo * P64_1 + acc.hi * P64_4 + len * P64_2);
    return h;
}

static u128 hash128_short(const uint8_t* p, size_t len) {
    const uint8_t* s = SECRET;

    if (len == 0) {
        u128 h;
        h.lo = xxh64_avalanche(load_le64(s + 64) ^ load_le64(s + 72));
        h.hi = xxh64_avalanche(load_le64(s + 80) ^ load_le64(s + 88));
        return h;
    }

    if (len <= 3) {
        uint32_t combined_lo = (uint32_t)p[0] << 16 | (uint32_t)p[len >> 1] << 24 |
                               (uint32_t)p[len - 1] | (uint32_t)len << 8;
        uint32_t combined_hi = rotl32(bswap32(combined_lo), 13);
        uint64_t bitflip_lo = load_le32(s) ^ load_le32(s + 4);
        uint64_t bitflip_hi = load_le32(s + 8) ^ load_le32(s + 12);
        u128 h;
        h.lo = xxh64_avalanche(combined_lo ^ bitflip_lo);
        h.hi = xxh64_avalanche(combined_hi ^ bitflip_hi);
        return h;
    }

    if (len <= 8) {
        uint64_t input = load_le32(p) + ((uint64_t)load_le32(p + len - 4) << 32);
        uint64_t bitflip = load_le64(s + 16) ^ load_le64(s + 24);
        u128 m = mul128(input ^ bitflip, P64_1 + (len << 2));
        m.hi += m.lo << 1;
        m.lo ^= m.hi >> 3;
        m.lo ^= m.lo >> 35;
        m.lo *= 0x9FB21C651E98DF25ULL;
        m.lo ^= m.lo >> 28;
        m.hi = xxh3_avalanche(m.hi);
        return m;
    }

    if (len <= 16) {
        uint64_t bitflip_lo = load_le64(s + 32) ^ load_le64(s + 40);
        uint64_t bitflip_hi = load_le64(s + 48) ^ load_le64(s + 56);
        uint64_t input_lo = load_le64(p);
        uint64_t input_hi = load_le64(p + len - 8);
        u128 m = mul128(input_lo ^ input_hi ^ bitflip_lo, P64_1);
        m.lo += (uint64_t)(len - 1) << 54;
        input_hi ^= bitflip_hi;
        m.hi += input_hi + (uint64_t)(uint32_t)input_hi * (P32_2 - 1);
        m.lo ^= bswap64(m.hi);
        u128 h = mul128(m.lo, P64_2);
        h.hi += m.hi * P64_2;
        h.lo = xxh3_avalanche(h.lo);
        h.hi = xxh3_avalanche(h.hi);
        return h;
    }

    if (len <= 128) {
        // Mix 16-byte pairs from both ends, from the middle outwards.
        u128 acc = {len * P64_1, 0};
        for (size_t n = (len - 1) / 32 + 1; n > 0; n--) {
            size_t i = n - 1;
            acc = mix32(acc, p + 16 * i, p + len - 16 * (i + 1), s + 32 * i);
        }
        return mid128_final(acc, len);
    }

    // 129-240 bytes: mix the first 128 bytes, then the rest
    // with the secret shifted by 3 bytes, then the last 32 bytes.
    u128 acc = {len * P64_1, 0};
    for (size_t i = 0; i < 4; i++) {
        acc = mix32(acc, p + 32 * i, p + 32 * i + 16, s + 32 * i);
    }
    acc.lo = xxh3_avalanche(acc.lo);
    acc.hi = xxh3_avalanche(acc.hi);
    for (size_t i = 4; i < len / 32; i++) {
        acc = mix32(acc, p + 32 * i, p + 32 * i + 16, s + 32 * (i - 4) + 3);
    }
    acc = mix32(acc, p + len - 16, p + len - 32, s + 103);
    return mid128_final(acc, len);
}

#pragma endregion

#pragma region Long inputs

// Inputs longer than 240 bytes are mixed into 8 accumulators, one 64-byte
// stripe at a time. A block of 16 stripes uses the secret at offsets 0, 8,
// ..., 120, and is followed by a scramble of the accumulators.

static void init_acc(uint64_t acc[8]) {
    acc[0] = P32_3;
    acc[1] = P64_1;
    acc[2] = P64_2;
    acc[3] = P64_3;
    acc[4] = P64_4;
    acc[5] = P32_2;
    acc[6] = P64_5;
    acc[7] = P32_1;
}

static void accumulate_stripe(uint64_t acc[8], const uint8_t* p, const uint8_t* s) {
    UNROLL
    for (size_t i = 0; i < 8; i++) {
        uint64_t v = load_le64(p + 8 * i);
        uint64_t k = v ^ load_le64(s + 8 * i);
        acc[i ^ 1] += v;
        acc[i] += (uint64_t)(uint32_t)k * (uint32_t)(k >> 32);
    }
}

static void accumulate(uint64_t acc[8], const uint8_t* p, size_t n_stripes) {
    // Work on a local copy: as far as the compiler knows, the input may alias
    // acc, so it would otherwise reload and store acc after every lane.
    uint64_t a[8];
    memcpy(a, acc, sizeof(a));
    for (size_t i = 0; i < n_stripes; i++) {
        accumulate_stripe(a, p + XXH3_STRIPE_SIZE * i, SECRET + 8 * i);
    }
    memcpy(acc, a, sizeof(a));
}

static void scramble(uint64_t acc[8]) {
    const uint8_t* s = SECRET + SECRET_SIZE - XXH3_STRIPE_SIZE;
    for (size_t i = 0; i < 8; i++) {
        acc[i] ^= acc[i] >> 47;
        acc[i] ^= load_le64(s + 8 * i);
        acc[i] *= P32_1;
    }
}

static void block(uint64_t acc[8], const uint8_t* p) {
    accumulate(acc, p, STRIPES_PER_BLOCK);
    scramble(acc);
}

// accumulate_long mixes len > 0 bytes into the accumulators: full blocks
// first, then the stripes of the last block, which may be partial, then
// the last 64 bytes of the input as a separate stripe. The last block is
// never empty, and the last stripe may start in the block before it, so the
// 64 bytes before p + len must be readable.
static void accumulate_long(uint64_t acc[8], const uint8_t* p, size_t len) {
    size_t n_blocks = (len - 1) / XXH3_BLOCK_SIZE;
    for (size_t i = 0; i < n_blocks; i++) {
        block(acc, p + XXH3_BLOCK_SIZE * i);
    }
    p += XXH3_BLOCK_SIZE * n_blocks;
    len -= XXH3_BLOCK_SIZE * n_blocks;
    accumulate(acc, p, (len - 1) / XXH3_STRIPE_SIZE);
    accumulate_stripe(acc, p + len - XXH3_STRIPE_SIZE, SECRET + SECRET_SIZE - XXH3_STRIPE_SIZE - 7);
}

// merge_accs mixes the accumulators into a single 64-bit hash.
static uint64_t merge_accs(const uint64_t acc[8], const uint8_t* s, uint64_t start) {
    uint64_t h = start;
    for (size_t i = 0; i < 4; i++) {
        h += mul_fold64(acc[2 * i] ^ load_le64(s + 16 * i),
                        acc[2 * i + 1] ^ load_le64(s + 16 * i + 8));
    }
    return xxh3_avalanche(h);
}

static uint64_t long64_final(const uint64_t acc[8], uint64_t len) {
    return merge_accs(acc, SECRET + 11, len * P64_1);
}

static u128 long128_final(const uint64_t acc[8], uint64_t len) {
    u128 h;
    h.lo = merge_accs(acc, SECRET + 11, len * P64_1);
    h.hi = merge_accs(acc, SECRET + SECRET_SIZE - XXH3_STRIPE_SIZE - 11, ~(len * P64_2));
    return h;
}

static uint64_t hash64(const uint8_t* p, size_t len) {
    if (len <= 240) {
        return hash64_short(p, len);
    }
    uint64_t acc[8];
    init_acc(acc);
    accumulate_long(acc, p, len);
    return long64_final(acc, len);
}

static u128 hash128(const uint8_t* p, size_t len) {
    if (len <= 240) {
        return hash128_short(p, len);
    }
    uint64_t acc[8];
    init_acc(acc);
    accumulate_long(acc, p, len);
    return long128_final(acc, len);
}

#pragma endregion

#pragma region Public API

static void store_hash128(uint8_t hash[XXH3_128_DIGEST_SIZE], u128 h) {
    store_be64(hash, h.hi);
    store_be64(hash + 8, h.lo);
}

void xxh3_64_init(xxh3_64_ctx* ctx) {
    init_acc(ctx->acc);
    ctx->nx = 0;
    ctx->blocks = 0;
}

void xxh3_64_update(xxh3_64_ctx* ctx, const uint8_t* data, size_t len) {
    while (len > 0) {
        // If the buffer is full, mix its first block: more input is coming,
        // so it's not the last one. Keep the stripe after the block buffered.
        if (ctx->nx == sizeof(ctx->x)) {
            block(ctx->acc, ctx->x);
            ctx->blocks++;
            memcpy(ctx->x, ctx->x + XXH3_BLOCK_SIZE, XXH3_STRIPE_SIZE);
            ctx->nx = XXH3_STRIPE_SIZE;
        }

        size_t n = sizeof(ctx->x) - ctx->nx;
        if (n > len) {
            n = len;
        }
        memcpy(ctx->x + ctx->nx, data, n);
        ctx->nx += n;
        data += n;
        len -= n;
    }
}

void xxh3_64_final(xxh3_64_ctx* ctx, uint8_t hash[XXH3_64_DIGEST_SIZE]) {
    if (ctx->blocks == 0) {
        store_be64(hash, hash64(ctx->x, ctx->nx));
        return;
    }
    // After the first block, the buffer always holds at least a stripe,
    // which is what accumulate_long needs to read before the end.
    uint64_t acc[8];
    memcpy(acc, ctx->acc, sizeof(acc));
    accumulate_long(acc, ctx->x, ctx->nx);
    store_be64(hash, long64_final(acc, ctx->blocks * XXH3_BLOCK_SIZE + ctx->nx));
}

void xxh3_128_init(xxh3_128_ctx* ctx) {
    xxh3_64_init(ctx);
}

void xxh3_128_update(xxh3_128_ctx* ctx, const uint8_t* data, size_t len) {
    xxh3_64_update(ctx, data, len);
}

void xxh3_128_final(xxh3_128_ctx* ctx, uint8_t hash[XXH3_128_DIGEST_SIZE]) {
    if (ctx->blocks == 0) {
        store_hash128(hash, hash128(ctx->x, ctx->nx));
        return;
    }
    // After the first block, the buffer always holds at least a stripe,
    // which is what accumulate_long needs to read before the end.
    uint64_t acc[8];
    memcpy(acc, ctx->acc, sizeof(acc));
    accumulate_long(acc, ctx->x, ctx->nx);
    store_hash128(hash, long128_final(acc, ctx->blocks * XXH3_BLOCK_SIZE + ctx->nx));
}

void xxh3_64_hash(const uint8_t* data, size_t len, uint8_t hash[XXH3_64_DIGEST_SIZE]) {
    store_be64(hash, hash64(data, len));
}

void xxh3_128_hash(const uint8_t* data, size_t len, uint8_t hash[XXH3_128_DIGEST_SIZE]) {
    store_hash128(hash, hash128(data, len));
}

#pragma endregion
