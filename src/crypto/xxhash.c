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

void xxh32_hash(const uint8_t* data, size_t len, uint8_t hash[XXH32_DIGEST_SIZE]) {
    XXH32_canonical_t cano;
    XXH32_canonicalFromHash(&cano, XXH32(data, len, 0));
    memcpy(hash, cano.digest, XXH32_DIGEST_SIZE);
}

void xxh64_hash(const uint8_t* data, size_t len, uint8_t hash[XXH64_DIGEST_SIZE]) {
    XXH64_canonical_t cano;
    XXH64_canonicalFromHash(&cano, XXH64(data, len, 0));
    memcpy(hash, cano.digest, XXH64_DIGEST_SIZE);
}

void xxh3_64_hash(const uint8_t* data, size_t len, uint8_t hash[XXH3_64_DIGEST_SIZE]) {
    XXH64_canonical_t cano;
    XXH64_canonicalFromHash(&cano, XXH3_64bits(data, len));
    memcpy(hash, cano.digest, XXH3_64_DIGEST_SIZE);
}

void xxh3_128_hash(const uint8_t* data, size_t len, uint8_t hash[XXH3_128_DIGEST_SIZE]) {
    XXH128_canonical_t cano;
    XXH128_canonicalFromHash(&cano, XXH3_128bits(data, len));
    memcpy(hash, cano.digest, XXH3_128_DIGEST_SIZE);
}
