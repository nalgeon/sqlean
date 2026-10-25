// Copyright (c) 2025 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// xxHash family of non-cryptographic hash functions (seed 0).
// Digests are written in canonical (big-endian) byte order.

#ifndef __XXHASH_H__
#define __XXHASH_H__

#include <stddef.h>
#include <stdint.h>

#define XXH32_DIGEST_SIZE 4
#define XXH64_DIGEST_SIZE 8
#define XXH3_64_DIGEST_SIZE 8
#define XXH3_128_DIGEST_SIZE 16

// One-shot hashing of len bytes. The data may be NULL if len is 0.
void xxh32_hash(const uint8_t* data, size_t len, uint8_t hash[XXH32_DIGEST_SIZE]);
void xxh64_hash(const uint8_t* data, size_t len, uint8_t hash[XXH64_DIGEST_SIZE]);
void xxh3_64_hash(const uint8_t* data, size_t len, uint8_t hash[XXH3_64_DIGEST_SIZE]);
void xxh3_128_hash(const uint8_t* data, size_t len, uint8_t hash[XXH3_128_DIGEST_SIZE]);

#endif
