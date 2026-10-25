// Copyright 2009 The Go Authors, BSD 3-Clause License
// https://github.com/golang/go/blob/master/LICENSE

// Ported to C by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// SHA-1 hash algorithm (RFC 3174), following Go's crypto/sha1.

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "crypto/common.h"
#include "crypto/sha1.h"

#define K0 0x5A827999
#define K1 0x6ED9EBA1
#define K2 0x8F1BBCDC
#define K3 0xCA62C1D6

// block hashes the full blocks of p (len is a multiple of the block size).
static void block(uint32_t h[5], const uint8_t* p, size_t len) {
    uint32_t w[16];
    for (; len >= SHA1_BLOCK_SIZE; p += SHA1_BLOCK_SIZE, len -= SHA1_BLOCK_SIZE) {
        for (size_t i = 0; i < 16; i++) {
            w[i] = load_be32(p + i * 4);
        }

        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];

        // Each of the four 20-step rounds differs only in the function f
        // and the constant K. The message schedule is kept in a 16-word
        // circular buffer, expanded as the steps go.
        size_t i = 0;
        UNROLL
        for (; i < 16; i++) {
            uint32_t f = (b & c) | (~b & d);
            uint32_t t = rotl32(a, 5) + f + e + w[i & 0xf] + K0;
            e = d;
            d = c;
            c = rotl32(b, 30);
            b = a;
            a = t;
        }
        UNROLL
        for (; i < 20; i++) {
            uint32_t tmp = w[(i - 3) & 0xf] ^ w[(i - 8) & 0xf] ^ w[(i - 14) & 0xf] ^ w[i & 0xf];
            w[i & 0xf] = rotl32(tmp, 1);
            uint32_t f = (b & c) | (~b & d);
            uint32_t t = rotl32(a, 5) + f + e + w[i & 0xf] + K0;
            e = d;
            d = c;
            c = rotl32(b, 30);
            b = a;
            a = t;
        }
        UNROLL
        for (; i < 40; i++) {
            uint32_t tmp = w[(i - 3) & 0xf] ^ w[(i - 8) & 0xf] ^ w[(i - 14) & 0xf] ^ w[i & 0xf];
            w[i & 0xf] = rotl32(tmp, 1);
            uint32_t f = b ^ c ^ d;
            uint32_t t = rotl32(a, 5) + f + e + w[i & 0xf] + K1;
            e = d;
            d = c;
            c = rotl32(b, 30);
            b = a;
            a = t;
        }
        UNROLL
        for (; i < 60; i++) {
            uint32_t tmp = w[(i - 3) & 0xf] ^ w[(i - 8) & 0xf] ^ w[(i - 14) & 0xf] ^ w[i & 0xf];
            w[i & 0xf] = rotl32(tmp, 1);
            uint32_t f = ((b | c) & d) | (b & c);
            uint32_t t = rotl32(a, 5) + f + e + w[i & 0xf] + K2;
            e = d;
            d = c;
            c = rotl32(b, 30);
            b = a;
            a = t;
        }
        UNROLL
        for (; i < 80; i++) {
            uint32_t tmp = w[(i - 3) & 0xf] ^ w[(i - 8) & 0xf] ^ w[(i - 14) & 0xf] ^ w[i & 0xf];
            w[i & 0xf] = rotl32(tmp, 1);
            uint32_t f = b ^ c ^ d;
            uint32_t t = rotl32(a, 5) + f + e + w[i & 0xf] + K3;
            e = d;
            d = c;
            c = rotl32(b, 30);
            b = a;
            a = t;
        }

        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
    }
}

void sha1_init(sha1_ctx* ctx) {
    ctx->h[0] = 0x67452301;
    ctx->h[1] = 0xEFCDAB89;
    ctx->h[2] = 0x98BADCFE;
    ctx->h[3] = 0x10325476;
    ctx->h[4] = 0xC3D2E1F0;
    ctx->nx = 0;
    ctx->len = 0;
}

void sha1_update(sha1_ctx* ctx, const uint8_t* data, size_t len) {
    if (len == 0) {
        // The data may be NULL, which memcpy does not accept even for 0 bytes.
        return;
    }
    ctx->len += len;
    // Top up the buffered input first.
    if (ctx->nx > 0) {
        size_t n = SHA1_BLOCK_SIZE - ctx->nx;
        if (n > len) {
            n = len;
        }
        memcpy(ctx->x + ctx->nx, data, n);
        ctx->nx += n;
        if (ctx->nx == SHA1_BLOCK_SIZE) {
            block(ctx->h, ctx->x, SHA1_BLOCK_SIZE);
            ctx->nx = 0;
        }
        data += n;
        len -= n;
    }
    // Then hash full blocks straight from the input.
    if (len >= SHA1_BLOCK_SIZE) {
        size_t n = len - len % SHA1_BLOCK_SIZE;
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

void sha1_final(sha1_ctx* ctx, uint8_t hash[SHA1_DIGEST_SIZE]) {
    // Padding: a 1 bit, then 0 bits until 56 bytes mod 64,
    // then the length in bits (big-endian).
    uint8_t tmp[SHA1_BLOCK_SIZE + 8] = {0x80};
    uint64_t len = ctx->len;
    size_t t = len % 64 < 56 ? 56 - len % 64 : 64 + 56 - len % 64;
    store_be64(tmp + t, len << 3);
    sha1_update(ctx, tmp, t + 8);

    for (size_t i = 0; i < 5; i++) {
        store_be32(hash + i * 4, ctx->h[i]);
    }
}

void sha1_hash(const uint8_t* data, size_t len, uint8_t hash[SHA1_DIGEST_SIZE]) {
    sha1_ctx ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, data, len);
    sha1_final(&ctx, hash);
}
