// Copyright 2009 The Go Authors, BSD 3-Clause License
// https://github.com/golang/go/blob/master/LICENSE

// Ported to C by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// MD5 hash algorithm (RFC 1321). The buffering follows Go's crypto/md5,
// and the block function is the plain loop from the RFC (Go's version
// of it is generated and fully unrolled).

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "crypto/common.h"
#include "crypto/md5.h"

// K[i] = floor(2^32 * abs(sin(i + 1)))
static const uint32_t K[64] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
};

// Per-round rotation amounts, four for each of the four rounds.
static const unsigned S[4][4] = {
    {7, 12, 17, 22},
    {5, 9, 14, 20},
    {4, 11, 16, 23},
    {6, 10, 15, 21},
};

// block hashes the full blocks of p (len is a multiple of the block size).
static void block(uint32_t h[4], const uint8_t* p, size_t len) {
    uint32_t m[16];
    for (; len >= MD5_BLOCK_SIZE; p += MD5_BLOCK_SIZE, len -= MD5_BLOCK_SIZE) {
        for (size_t i = 0; i < 16; i++) {
            m[i] = load_le32(p + i * 4);
        }

        uint32_t a = h[0], b = h[1], c = h[2], d = h[3];

        // Each of the four 16-step rounds differs only in the function f,
        // the order in which the message words are used, and the rotations.
        size_t i = 0;
        UNROLL
        for (; i < 16; i++) {
            uint32_t f = (b & c) | (~b & d);
            uint32_t t = d;
            d = c;
            c = b;
            b += rotl32(a + f + K[i] + m[i], S[0][i % 4]);
            a = t;
        }
        UNROLL
        for (; i < 32; i++) {
            uint32_t f = (d & b) | (~d & c);
            uint32_t t = d;
            d = c;
            c = b;
            b += rotl32(a + f + K[i] + m[(5 * i + 1) % 16], S[1][i % 4]);
            a = t;
        }
        UNROLL
        for (; i < 48; i++) {
            uint32_t f = b ^ c ^ d;
            uint32_t t = d;
            d = c;
            c = b;
            b += rotl32(a + f + K[i] + m[(3 * i + 5) % 16], S[2][i % 4]);
            a = t;
        }
        UNROLL
        for (; i < 64; i++) {
            uint32_t f = c ^ (b | ~d);
            uint32_t t = d;
            d = c;
            c = b;
            b += rotl32(a + f + K[i] + m[(7 * i) % 16], S[3][i % 4]);
            a = t;
        }

        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
    }
}

void md5_init(md5_ctx* ctx) {
    ctx->h[0] = 0x67452301;
    ctx->h[1] = 0xefcdab89;
    ctx->h[2] = 0x98badcfe;
    ctx->h[3] = 0x10325476;
    ctx->nx = 0;
    ctx->len = 0;
}

void md5_update(md5_ctx* ctx, const uint8_t* data, size_t len) {
    if (len == 0) {
        // The data may be NULL, which memcpy does not accept even for 0 bytes.
        return;
    }
    ctx->len += len;
    // Top up the buffered input first.
    if (ctx->nx > 0) {
        size_t n = MD5_BLOCK_SIZE - ctx->nx;
        if (n > len) {
            n = len;
        }
        memcpy(ctx->x + ctx->nx, data, n);
        ctx->nx += n;
        if (ctx->nx == MD5_BLOCK_SIZE) {
            block(ctx->h, ctx->x, MD5_BLOCK_SIZE);
            ctx->nx = 0;
        }
        data += n;
        len -= n;
    }
    // Then hash full blocks straight from the input.
    if (len >= MD5_BLOCK_SIZE) {
        size_t n = len - len % MD5_BLOCK_SIZE;
        block(ctx->h, data, n);
        data += n;
        len -= n;
    }
    // Then buffer the rest.
    if (len > 0) {
        memcpy(ctx->x, data, len);
        ctx->nx = len;
    }
}

void md5_final(md5_ctx* ctx, uint8_t hash[MD5_DIGEST_SIZE]) {
    // Padding: a 1 bit, then 0 bits until 56 bytes mod 64,
    // then the length in bits (little-endian).
    uint8_t tmp[MD5_BLOCK_SIZE + 8] = {0x80};
    uint64_t len = ctx->len;
    size_t t = len % 64 < 56 ? 56 - len % 64 : 64 + 56 - len % 64;
    store_le64(tmp + t, len << 3);
    md5_update(ctx, tmp, t + 8);

    for (size_t i = 0; i < 4; i++) {
        store_le32(hash + i * 4, ctx->h[i]);
    }
}

void md5_hash(const uint8_t* data, size_t len, uint8_t hash[MD5_DIGEST_SIZE]) {
    md5_ctx ctx;
    md5_init(&ctx);
    md5_update(&ctx, data, len);
    md5_final(&ctx, hash);
}
