// Originally from the sha1 SQLite exension, Public Domain
// https://sqlite.org/src/file/ext/misc/sha1.c
// Modified by Anton Zhiyanov, https://github.com/nalgeon/sqlean/, MIT License

#include <stdint.h>
#include <string.h>

#include "crypto/sha1.h"

#define SHA_ROT(x, l, r) ((x) << (l) | (x) >> (r))
#define rol(x, k) SHA_ROT(x, k, 32 - (k))
#define ror(x, k) SHA_ROT(x, 32 - (k), k)

#define blk0(i) block[i]
#define blk(i)       \
    (block[i & 15] = \
         rol(block[(i + 13) & 15] ^ block[(i + 8) & 15] ^ block[(i + 2) & 15] ^ block[i & 15], 1))

/*
 * (R0+R1), R2, R3, R4 are the different operations (rounds) used in SHA1
 */
#define R0(v, w, x, y, z, i)                                     \
    z += ((w & (x ^ y)) ^ y) + blk0(i) + 0x5A827999 + rol(v, 5); \
    w = ror(w, 2);
#define R1(v, w, x, y, z, i)                                    \
    z += ((w & (x ^ y)) ^ y) + blk(i) + 0x5A827999 + rol(v, 5); \
    w = ror(w, 2);
#define R2(v, w, x, y, z, i)                            \
    z += (w ^ x ^ y) + blk(i) + 0x6ED9EBA1 + rol(v, 5); \
    w = ror(w, 2);
#define R3(v, w, x, y, z, i)                                          \
    z += (((w | x) & y) | (w & x)) + blk(i) + 0x8F1BBCDC + rol(v, 5); \
    w = ror(w, 2);
#define R4(v, w, x, y, z, i)                            \
    z += (w ^ x ^ y) + blk(i) + 0xCA62C1D6 + rol(v, 5); \
    w = ror(w, 2);

/*
 * Hash a single 512-bit block. This is the core of the algorithm.
 */
static void sha1_transform(uint32_t state[5], const uint8_t buffer[64]) {
    uint32_t qq[5]; /* a, b, c, d, e; */
    uint32_t block[16];
    /* SHA1 reads the message as big-endian words */
    for (size_t i = 0; i < 16; i++) {
        block[i] = ((uint32_t)buffer[4 * i] << 24) | ((uint32_t)buffer[4 * i + 1] << 16) |
                   ((uint32_t)buffer[4 * i + 2] << 8) | (uint32_t)buffer[4 * i + 3];
    }
    memcpy(qq, state, sizeof(qq));

#define a qq[0]
#define b qq[1]
#define c qq[2]
#define d qq[3]
#define e qq[4]

    /* 4 rounds of 20 operations each. Loop unrolled. */
    R0(a, b, c, d, e, 0);
    R0(e, a, b, c, d, 1);
    R0(d, e, a, b, c, 2);
    R0(c, d, e, a, b, 3);
    R0(b, c, d, e, a, 4);
    R0(a, b, c, d, e, 5);
    R0(e, a, b, c, d, 6);
    R0(d, e, a, b, c, 7);
    R0(c, d, e, a, b, 8);
    R0(b, c, d, e, a, 9);
    R0(a, b, c, d, e, 10);
    R0(e, a, b, c, d, 11);
    R0(d, e, a, b, c, 12);
    R0(c, d, e, a, b, 13);
    R0(b, c, d, e, a, 14);
    R0(a, b, c, d, e, 15);
    R1(e, a, b, c, d, 16);
    R1(d, e, a, b, c, 17);
    R1(c, d, e, a, b, 18);
    R1(b, c, d, e, a, 19);
    R2(a, b, c, d, e, 20);
    R2(e, a, b, c, d, 21);
    R2(d, e, a, b, c, 22);
    R2(c, d, e, a, b, 23);
    R2(b, c, d, e, a, 24);
    R2(a, b, c, d, e, 25);
    R2(e, a, b, c, d, 26);
    R2(d, e, a, b, c, 27);
    R2(c, d, e, a, b, 28);
    R2(b, c, d, e, a, 29);
    R2(a, b, c, d, e, 30);
    R2(e, a, b, c, d, 31);
    R2(d, e, a, b, c, 32);
    R2(c, d, e, a, b, 33);
    R2(b, c, d, e, a, 34);
    R2(a, b, c, d, e, 35);
    R2(e, a, b, c, d, 36);
    R2(d, e, a, b, c, 37);
    R2(c, d, e, a, b, 38);
    R2(b, c, d, e, a, 39);
    R3(a, b, c, d, e, 40);
    R3(e, a, b, c, d, 41);
    R3(d, e, a, b, c, 42);
    R3(c, d, e, a, b, 43);
    R3(b, c, d, e, a, 44);
    R3(a, b, c, d, e, 45);
    R3(e, a, b, c, d, 46);
    R3(d, e, a, b, c, 47);
    R3(c, d, e, a, b, 48);
    R3(b, c, d, e, a, 49);
    R3(a, b, c, d, e, 50);
    R3(e, a, b, c, d, 51);
    R3(d, e, a, b, c, 52);
    R3(c, d, e, a, b, 53);
    R3(b, c, d, e, a, 54);
    R3(a, b, c, d, e, 55);
    R3(e, a, b, c, d, 56);
    R3(d, e, a, b, c, 57);
    R3(c, d, e, a, b, 58);
    R3(b, c, d, e, a, 59);
    R4(a, b, c, d, e, 60);
    R4(e, a, b, c, d, 61);
    R4(d, e, a, b, c, 62);
    R4(c, d, e, a, b, 63);
    R4(b, c, d, e, a, 64);
    R4(a, b, c, d, e, 65);
    R4(e, a, b, c, d, 66);
    R4(d, e, a, b, c, 67);
    R4(c, d, e, a, b, 68);
    R4(b, c, d, e, a, 69);
    R4(a, b, c, d, e, 70);
    R4(e, a, b, c, d, 71);
    R4(d, e, a, b, c, 72);
    R4(c, d, e, a, b, 73);
    R4(b, c, d, e, a, 74);
    R4(a, b, c, d, e, 75);
    R4(e, a, b, c, d, 76);
    R4(d, e, a, b, c, 77);
    R4(c, d, e, a, b, 78);
    R4(b, c, d, e, a, 79);

    /* Add the working vars back into context.state[] */
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;

#undef a
#undef b
#undef c
#undef d
#undef e
}

/* Initialize a SHA1 context */
void sha1_init(sha1_ctx* ctx) {
    /* SHA1 initialization constants */
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;
    ctx->count = 0;
}

/* Add new content to the SHA1 hash */
void sha1_update(sha1_ctx* ctx, const uint8_t* data, size_t len) {
    if (len == 0) {
        return;
    }
    size_t used = (size_t)(ctx->count & 63);
    ctx->count += len;

    size_t i = 0;
    if (used + len >= 64) {
        i = 64 - used;
        memcpy(&ctx->buffer[used], data, i);
        sha1_transform(ctx->state, ctx->buffer);
        for (; i + 64 <= len; i += 64) {
            sha1_transform(ctx->state, &data[i]);
        }
        used = 0;
    }
    memcpy(&ctx->buffer[used], &data[i], len - i);
}

/* Pad the message, append its length in bits, and write the digest */
void sha1_final(sha1_ctx* ctx, uint8_t hash[SHA1_DIGEST_SIZE]) {
    uint8_t finalcount[8];
    uint64_t bitlen = ctx->count << 3;
    for (size_t i = 0; i < 8; i++) {
        finalcount[i] = (uint8_t)(bitlen >> (56 - 8 * i));
    }

    static const uint8_t pad[64] = {0x80};
    size_t used = (size_t)(ctx->count & 63);
    size_t pad_len = used < 56 ? 56 - used : 120 - used;
    sha1_update(ctx, pad, pad_len);
    sha1_update(ctx, finalcount, 8); /* Should cause a sha1_transform() */

    for (size_t i = 0; i < SHA1_DIGEST_SIZE; i++) {
        hash[i] = (uint8_t)(ctx->state[i >> 2] >> ((3 - (i & 3)) * 8));
    }
}

void sha1_hash(const uint8_t* data, size_t len, uint8_t hash[SHA1_DIGEST_SIZE]) {
    sha1_ctx ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, data, len);
    sha1_final(&ctx, hash);
}
