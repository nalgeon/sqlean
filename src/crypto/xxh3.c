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

// The context from the header is storage for xxHash's own state.
_Static_assert(sizeof(xxh3_64_ctx) >= sizeof(XXH3_state_t), "xxh3_64_ctx too small");
_Static_assert(_Alignof(xxh3_64_ctx) >= _Alignof(XXH3_state_t), "xxh3_64_ctx misaligned");

#pragma region Digests

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

void xxh3_64_hash(const uint8_t* data, size_t len, uint8_t hash[XXH3_64_DIGEST_SIZE]) {
    digest64(XXH3_64bits(data, len), hash);
}

void xxh3_128_hash(const uint8_t* data, size_t len, uint8_t hash[XXH3_128_DIGEST_SIZE]) {
    digest128(XXH3_128bits(data, len), hash);
}

#pragma endregion
