// Copyright (c) 2025 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define XXH_STATIC_LINKING_ONLY
#define XXH_IMPLEMENTATION
#include "crypto/xxhash.h"
#include "crypto/xxhash.impl.h"

#define XXH32_DIGEST_LENGTH 4
#define XXH64_DIGEST_LENGTH 8
#define XXH128_DIGEST_LENGTH 16

// XXH32.

void* xxh32_init() {
    XXH32_state_t* ctx = XXH32_createState();
    if (!ctx) {
        return NULL;
    }
    XXH32_reset(ctx, 0);
    return ctx;
}

void xxh32_update(XXH32_state_t* ctx, const void* data, size_t len) {
    XXH32_update(ctx, data, len);
}

int xxh32_final(XXH32_state_t* ctx, uint8_t* hash) {
    XXH32_hash_t digest = XXH32_digest(ctx);
    XXH32_canonical_t cano;
    XXH32_canonicalFromHash(&cano, digest);
    memcpy(hash, cano.digest, XXH32_DIGEST_LENGTH);
    XXH32_freeState(ctx);
    return XXH32_DIGEST_LENGTH;
}

// XXH64.

void* xxh64_init() {
    XXH64_state_t* ctx = XXH64_createState();
    if (!ctx) {
        return NULL;
    }
    XXH64_reset(ctx, 0);
    return ctx;
}

void xxh64_update(XXH64_state_t* ctx, const void* data, size_t len) {
    XXH64_update(ctx, data, len);
}

int xxh64_final(XXH64_state_t* ctx, uint8_t* hash) {
    XXH64_hash_t digest = XXH64_digest(ctx);
    XXH64_canonical_t cano;
    XXH64_canonicalFromHash(&cano, digest);
    memcpy(hash, cano.digest, XXH64_DIGEST_LENGTH);
    XXH64_freeState(ctx);
    return XXH64_DIGEST_LENGTH;
}

// XXH3 64-bit.

void* xxh3_64_init() {
    XXH3_state_t* ctx = XXH3_createState();
    if (!ctx) {
        return NULL;
    }
    XXH3_64bits_reset(ctx);
    return ctx;
}

void xxh3_64_update(XXH3_state_t* ctx, const void* data, size_t len) {
    XXH3_64bits_update(ctx, data, len);
}

int xxh3_64_final(XXH3_state_t* ctx, uint8_t* hash) {
    uint64_t digest = XXH3_64bits_digest(ctx);
    XXH64_canonical_t cano;
    XXH64_canonicalFromHash(&cano, digest);
    memcpy(hash, cano.digest, XXH64_DIGEST_LENGTH);
    XXH3_freeState(ctx);
    return XXH64_DIGEST_LENGTH;
}

// XXH3 128-bit.

void* xxh3_128_init() {
    XXH3_state_t* ctx = XXH3_createState();
    if (!ctx) {
        return NULL;
    }
    XXH3_128bits_reset(ctx);
    return ctx;
}

void xxh3_128_update(XXH3_state_t* ctx, const void* data, size_t len) {
    XXH3_128bits_update(ctx, data, len);
}

int xxh3_128_final(XXH3_state_t* ctx, uint8_t* hash) {
    XXH128_hash_t digest = XXH3_128bits_digest(ctx);
    XXH128_canonical_t cano;
    XXH128_canonicalFromHash(&cano, digest);
    memcpy(hash, cano.digest, XXH128_DIGEST_LENGTH);
    XXH3_freeState(ctx);
    return XXH128_DIGEST_LENGTH;
}
