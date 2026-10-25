// Copyright 2009 The Go Authors, BSD 3-Clause License
// https://github.com/golang/go/blob/master/LICENSE

// Ported to C by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// SHA-256, SHA-384 and SHA-512 hash algorithms (FIPS 180-4),
// following Go's crypto/sha256 and crypto/sha512.

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "crypto/common.h"
#include "crypto/sha2.h"

#pragma region SHA-256

static const uint32_t K256[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

// block256 hashes the full blocks of p (len is a multiple of the block size).
static void block256(uint32_t hs[8], const uint8_t* p, size_t len) {
    uint32_t w[64];
    for (; len >= SHA256_BLOCK_SIZE; p += SHA256_BLOCK_SIZE, len -= SHA256_BLOCK_SIZE) {
        UNROLL
        for (size_t i = 0; i < 16; i++) {
            w[i] = load_be32(p + i * 4);
        }
        UNROLL
        for (size_t i = 16; i < 64; i++) {
            uint32_t v1 = w[i - 2];
            uint32_t t1 = rotr32(v1, 17) ^ rotr32(v1, 19) ^ (v1 >> 10);
            uint32_t v2 = w[i - 15];
            uint32_t t2 = rotr32(v2, 7) ^ rotr32(v2, 18) ^ (v2 >> 3);
            w[i] = t1 + w[i - 7] + t2 + w[i - 16];
        }

        uint32_t a = hs[0], b = hs[1], c = hs[2], d = hs[3];
        uint32_t e = hs[4], f = hs[5], g = hs[6], h = hs[7];
        UNROLL
        for (size_t i = 0; i < 64; i++) {
            // Ch and Maj in their cheaper equivalent forms:
            // ch = (e & f) ^ (~e & g), maj = (a & b) ^ (a & c) ^ (b & c)
            uint32_t s1 = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
            uint32_t ch = g ^ (e & (f ^ g));
            uint32_t t1 = h + s1 + ch + K256[i] + w[i];
            uint32_t s0 = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
            uint32_t maj = (a & b) | (c & (a | b));
            uint32_t t2 = s0 + maj;
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }

        hs[0] += a;
        hs[1] += b;
        hs[2] += c;
        hs[3] += d;
        hs[4] += e;
        hs[5] += f;
        hs[6] += g;
        hs[7] += h;
    }
}

void sha256_init(sha256_ctx* ctx) {
    static const uint32_t iv[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
    };
    memcpy(ctx->h, iv, sizeof(iv));
    ctx->nx = 0;
    ctx->len = 0;
}

void sha256_update(sha256_ctx* ctx, const uint8_t* data, size_t len) {
    if (len == 0) {
        // The data may be NULL, which memcpy does not accept even for 0 bytes.
        return;
    }
    ctx->len += len;
    // Top up the buffered input first.
    if (ctx->nx > 0) {
        size_t n = SHA256_BLOCK_SIZE - ctx->nx;
        if (n > len) {
            n = len;
        }
        memcpy(ctx->x + ctx->nx, data, n);
        ctx->nx += n;
        if (ctx->nx == SHA256_BLOCK_SIZE) {
            block256(ctx->h, ctx->x, SHA256_BLOCK_SIZE);
            ctx->nx = 0;
        }
        data += n;
        len -= n;
    }
    // Then hash full blocks straight from the input.
    if (len >= SHA256_BLOCK_SIZE) {
        size_t n = len - len % SHA256_BLOCK_SIZE;
        block256(ctx->h, data, n);
        data += n;
        len -= n;
    }
    // Then buffer the rest.
    if (len > 0) {
        memcpy(ctx->x, data, len);
        ctx->nx = len;
    }
}

void sha256_final(sha256_ctx* ctx, uint8_t digest[SHA256_DIGEST_SIZE]) {
    // Padding: a 1 bit, then 0 bits until 56 bytes mod 64,
    // then the length in bits (big-endian).
    uint8_t tmp[SHA256_BLOCK_SIZE + 8] = {0x80};
    uint64_t len = ctx->len;
    size_t t = len % 64 < 56 ? 56 - len % 64 : 64 + 56 - len % 64;
    store_be64(tmp + t, len << 3);
    sha256_update(ctx, tmp, t + 8);

    for (size_t i = 0; i < 8; i++) {
        store_be32(digest + i * 4, ctx->h[i]);
    }
}

#pragma endregion

#pragma region SHA-512

static const uint64_t K512[80] = {
    0x428a2f98d728ae22, 0x7137449123ef65cd, 0xb5c0fbcfec4d3b2f, 0xe9b5dba58189dbbc,
    0x3956c25bf348b538, 0x59f111f1b605d019, 0x923f82a4af194f9b, 0xab1c5ed5da6d8118,
    0xd807aa98a3030242, 0x12835b0145706fbe, 0x243185be4ee4b28c, 0x550c7dc3d5ffb4e2,
    0x72be5d74f27b896f, 0x80deb1fe3b1696b1, 0x9bdc06a725c71235, 0xc19bf174cf692694,
    0xe49b69c19ef14ad2, 0xefbe4786384f25e3, 0x0fc19dc68b8cd5b5, 0x240ca1cc77ac9c65,
    0x2de92c6f592b0275, 0x4a7484aa6ea6e483, 0x5cb0a9dcbd41fbd4, 0x76f988da831153b5,
    0x983e5152ee66dfab, 0xa831c66d2db43210, 0xb00327c898fb213f, 0xbf597fc7beef0ee4,
    0xc6e00bf33da88fc2, 0xd5a79147930aa725, 0x06ca6351e003826f, 0x142929670a0e6e70,
    0x27b70a8546d22ffc, 0x2e1b21385c26c926, 0x4d2c6dfc5ac42aed, 0x53380d139d95b3df,
    0x650a73548baf63de, 0x766a0abb3c77b2a8, 0x81c2c92e47edaee6, 0x92722c851482353b,
    0xa2bfe8a14cf10364, 0xa81a664bbc423001, 0xc24b8b70d0f89791, 0xc76c51a30654be30,
    0xd192e819d6ef5218, 0xd69906245565a910, 0xf40e35855771202a, 0x106aa07032bbd1b8,
    0x19a4c116b8d2d0c8, 0x1e376c085141ab53, 0x2748774cdf8eeb99, 0x34b0bcb5e19b48a8,
    0x391c0cb3c5c95a63, 0x4ed8aa4ae3418acb, 0x5b9cca4f7763e373, 0x682e6ff3d6b2b8a3,
    0x748f82ee5defb2fc, 0x78a5636f43172f60, 0x84c87814a1f0ab72, 0x8cc702081a6439ec,
    0x90befffa23631e28, 0xa4506cebde82bde9, 0xbef9a3f7b2c67915, 0xc67178f2e372532b,
    0xca273eceea26619c, 0xd186b8c721c0c207, 0xeada7dd6cde0eb1e, 0xf57d4f7fee6ed178,
    0x06f067aa72176fba, 0x0a637dc5a2c898a6, 0x113f9804bef90dae, 0x1b710b35131c471b,
    0x28db77f523047d84, 0x32caab7b40c72493, 0x3c9ebe0a15c9bebc, 0x431d67c49c100d4c,
    0x4cc5d4becb3e42b6, 0x597f299cfc657e2a, 0x5fcb6fab3ad6faec, 0x6c44198c4a475817,
};

// block512 hashes the full blocks of p (len is a multiple of the block size).
static void block512(uint64_t hs[8], const uint8_t* p, size_t len) {
    uint64_t w[80];
    for (; len >= SHA512_BLOCK_SIZE; p += SHA512_BLOCK_SIZE, len -= SHA512_BLOCK_SIZE) {
        UNROLL
        for (size_t i = 0; i < 16; i++) {
            w[i] = load_be64(p + i * 8);
        }
        UNROLL
        for (size_t i = 16; i < 80; i++) {
            uint64_t v1 = w[i - 2];
            uint64_t t1 = rotr64(v1, 19) ^ rotr64(v1, 61) ^ (v1 >> 6);
            uint64_t v2 = w[i - 15];
            uint64_t t2 = rotr64(v2, 1) ^ rotr64(v2, 8) ^ (v2 >> 7);
            w[i] = t1 + w[i - 7] + t2 + w[i - 16];
        }

        uint64_t a = hs[0], b = hs[1], c = hs[2], d = hs[3];
        uint64_t e = hs[4], f = hs[5], g = hs[6], h = hs[7];
        UNROLL
        for (size_t i = 0; i < 80; i++) {
            // Ch and Maj in their cheaper equivalent forms:
            // ch = (e & f) ^ (~e & g), maj = (a & b) ^ (a & c) ^ (b & c)
            uint64_t s1 = rotr64(e, 14) ^ rotr64(e, 18) ^ rotr64(e, 41);
            uint64_t ch = g ^ (e & (f ^ g));
            uint64_t t1 = h + s1 + ch + K512[i] + w[i];
            uint64_t s0 = rotr64(a, 28) ^ rotr64(a, 34) ^ rotr64(a, 39);
            uint64_t maj = (a & b) | (c & (a | b));
            uint64_t t2 = s0 + maj;
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }

        hs[0] += a;
        hs[1] += b;
        hs[2] += c;
        hs[3] += d;
        hs[4] += e;
        hs[5] += f;
        hs[6] += g;
        hs[7] += h;
    }
}

void sha512_init(sha512_ctx* ctx) {
    static const uint64_t iv[8] = {
        0x6a09e667f3bcc908, 0xbb67ae8584caa73b, 0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1,
        0x510e527fade682d1, 0x9b05688c2b3e6c1f, 0x1f83d9abfb41bd6b, 0x5be0cd19137e2179,
    };
    memcpy(ctx->h, iv, sizeof(iv));
    ctx->nx = 0;
    ctx->len = 0;
}

void sha512_update(sha512_ctx* ctx, const uint8_t* data, size_t len) {
    if (len == 0) {
        // The data may be NULL, which memcpy does not accept even for 0 bytes.
        return;
    }
    ctx->len += len;
    // Top up the buffered input first.
    if (ctx->nx > 0) {
        size_t n = SHA512_BLOCK_SIZE - ctx->nx;
        if (n > len) {
            n = len;
        }
        memcpy(ctx->x + ctx->nx, data, n);
        ctx->nx += n;
        if (ctx->nx == SHA512_BLOCK_SIZE) {
            block512(ctx->h, ctx->x, SHA512_BLOCK_SIZE);
            ctx->nx = 0;
        }
        data += n;
        len -= n;
    }
    // Then hash full blocks straight from the input.
    if (len >= SHA512_BLOCK_SIZE) {
        size_t n = len - len % SHA512_BLOCK_SIZE;
        block512(ctx->h, data, n);
        data += n;
        len -= n;
    }
    // Then buffer the rest.
    if (len > 0) {
        memcpy(ctx->x, data, len);
        ctx->nx = len;
    }
}

// finish512 pads the input and writes the first size bytes of the state
// into digest (SHA-512 and SHA-384 differ only in the size).
static void finish512(sha512_ctx* ctx, uint8_t* digest, size_t size) {
    // Padding: a 1 bit, then 0 bits until 112 bytes mod 128,
    // then the length in bits as a 128-bit big-endian number.
    uint8_t tmp[SHA512_BLOCK_SIZE + 16] = {0x80};
    uint64_t len = ctx->len;
    size_t t = len % 128 < 112 ? 112 - len % 128 : 128 + 112 - len % 128;
    store_be64(tmp + t, len >> 61);
    store_be64(tmp + t + 8, len << 3);
    sha512_update(ctx, tmp, t + 16);

    for (size_t i = 0; i < size / 8; i++) {
        store_be64(digest + i * 8, ctx->h[i]);
    }
}

void sha512_final(sha512_ctx* ctx, uint8_t digest[SHA512_DIGEST_SIZE]) {
    finish512(ctx, digest, SHA512_DIGEST_SIZE);
}

#pragma endregion

#pragma region SHA-384

void sha384_init(sha384_ctx* ctx) {
    static const uint64_t iv[8] = {
        0xcbbb9d5dc1059ed8, 0x629a292a367cd507, 0x9159015a3070dd17, 0x152fecd8f70e5939,
        0x67332667ffc00b31, 0x8eb44a8768581511, 0xdb0c2e0d64f98fa7, 0x47b5481dbefa4fa4,
    };
    memcpy(ctx->h, iv, sizeof(iv));
    ctx->nx = 0;
    ctx->len = 0;
}

void sha384_update(sha384_ctx* ctx, const uint8_t* data, size_t len) {
    sha512_update(ctx, data, len);
}

void sha384_final(sha384_ctx* ctx, uint8_t digest[SHA384_DIGEST_SIZE]) {
    finish512(ctx, digest, SHA384_DIGEST_SIZE);
}

#pragma endregion

#pragma region One-shot

void sha256_hash(const uint8_t* data, size_t len, uint8_t digest[SHA256_DIGEST_SIZE]) {
    sha256_ctx ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, digest);
}

void sha384_hash(const uint8_t* data, size_t len, uint8_t digest[SHA384_DIGEST_SIZE]) {
    sha384_ctx ctx;
    sha384_init(&ctx);
    sha384_update(&ctx, data, len);
    sha384_final(&ctx, digest);
}

void sha512_hash(const uint8_t* data, size_t len, uint8_t digest[SHA512_DIGEST_SIZE]) {
    sha512_ctx ctx;
    sha512_init(&ctx);
    sha512_update(&ctx, data, len);
    sha512_final(&ctx, digest);
}

#pragma endregion
