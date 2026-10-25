// Copyright (c) 2025 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "crypto/xxhash.h"

// Compile the vendored implementation into this file only, as static
// functions, so that its symbols don't leak out of the extension.
#define XXH_INLINE_ALL
#include "crypto/xxhash.impl.h"

// The contexts from the header are storage for xxHash's own states.
#define ASSERT_FITS(ctx, state)                                      \
    _Static_assert(sizeof(ctx) >= sizeof(state), #ctx " too small"); \
    _Static_assert(_Alignof(ctx) >= _Alignof(state), #ctx " misaligned")

ASSERT_FITS(xxh32_ctx, XXH32_state_t);
ASSERT_FITS(xxh64_ctx, XXH64_state_t);
ASSERT_FITS(xxh3_64_ctx, XXH3_state_t);

#pragma region Digests

static void digest32(XXH32_hash_t h, uint8_t hash[XXH32_DIGEST_SIZE]) {
    XXH32_canonical_t cano;
    XXH32_canonicalFromHash(&cano, h);
    memcpy(hash, cano.digest, XXH32_DIGEST_SIZE);
}

static void digest64(XXH64_hash_t h, uint8_t hash[XXH64_DIGEST_SIZE]) {
    XXH64_canonical_t cano;
    XXH64_canonicalFromHash(&cano, h);
    memcpy(hash, cano.digest, XXH64_DIGEST_SIZE);
}

static void digest128(XXH128_hash_t h, uint8_t hash[XXH3_128_DIGEST_SIZE]) {
    XXH128_canonical_t cano;
    XXH128_canonicalFromHash(&cano, h);
    memcpy(hash, cano.digest, XXH3_128_DIGEST_SIZE);
}

#pragma endregion

#pragma region Incremental

void xxh32_init(xxh32_ctx* ctx) {
    XXH32_reset((XXH32_state_t*)ctx, 0);
}

void xxh32_update(xxh32_ctx* ctx, const uint8_t* data, size_t len) {
    if (len == 0) {
        return;
    }
    XXH32_update((XXH32_state_t*)ctx, data, len);
}

void xxh32_final(xxh32_ctx* ctx, uint8_t hash[XXH32_DIGEST_SIZE]) {
    digest32(XXH32_digest((XXH32_state_t*)ctx), hash);
}

void xxh64_init(xxh64_ctx* ctx) {
    XXH64_reset((XXH64_state_t*)ctx, 0);
}

void xxh64_update(xxh64_ctx* ctx, const uint8_t* data, size_t len) {
    if (len == 0) {
        return;
    }
    XXH64_update((XXH64_state_t*)ctx, data, len);
}

void xxh64_final(xxh64_ctx* ctx, uint8_t hash[XXH64_DIGEST_SIZE]) {
    digest64(XXH64_digest((XXH64_state_t*)ctx), hash);
}

void xxh3_64_init(xxh3_64_ctx* ctx) {
    XXH3_64bits_reset((XXH3_state_t*)ctx);
}

void xxh3_64_update(xxh3_64_ctx* ctx, const uint8_t* data, size_t len) {
    if (len == 0) {
        return;
    }
    XXH3_64bits_update((XXH3_state_t*)ctx, data, len);
}

void xxh3_64_final(xxh3_64_ctx* ctx, uint8_t hash[XXH3_64_DIGEST_SIZE]) {
    digest64(XXH3_64bits_digest((XXH3_state_t*)ctx), hash);
}

void xxh3_128_init(xxh3_128_ctx* ctx) {
    XXH3_128bits_reset((XXH3_state_t*)ctx);
}

void xxh3_128_update(xxh3_128_ctx* ctx, const uint8_t* data, size_t len) {
    if (len == 0) {
        return;
    }
    XXH3_128bits_update((XXH3_state_t*)ctx, data, len);
}

void xxh3_128_final(xxh3_128_ctx* ctx, uint8_t hash[XXH3_128_DIGEST_SIZE]) {
    digest128(XXH3_128bits_digest((XXH3_state_t*)ctx), hash);
}

#pragma endregion

#pragma region One-shot

void xxh32_hash(const uint8_t* data, size_t len, uint8_t hash[XXH32_DIGEST_SIZE]) {
    digest32(XXH32(data, len, 0), hash);
}

void xxh64_hash(const uint8_t* data, size_t len, uint8_t hash[XXH64_DIGEST_SIZE]) {
    digest64(XXH64(data, len, 0), hash);
}

void xxh3_64_hash(const uint8_t* data, size_t len, uint8_t hash[XXH3_64_DIGEST_SIZE]) {
    digest64(XXH3_64bits(data, len), hash);
}

void xxh3_128_hash(const uint8_t* data, size_t len, uint8_t hash[XXH3_128_DIGEST_SIZE]) {
    digest128(XXH3_128bits(data, len), hash);
}

#pragma endregion
