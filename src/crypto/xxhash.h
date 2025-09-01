// Copyright (c) 2025 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

#ifndef __XXHASH_H__
#define __XXHASH_H__

#include <stddef.h>
#include <stdint.h>

#include "crypto/xxhash.impl.h"

void* xxh32_init();
void xxh32_update(XXH32_state_t* ctx, const void* data, size_t len);
int xxh32_final(XXH32_state_t* ctx, uint8_t* hash);

void* xxh64_init();
void xxh64_update(XXH64_state_t* ctx, const void* data, size_t len);
int xxh64_final(XXH64_state_t* ctx, uint8_t* hash);

void* xxh3_64_init();
void xxh3_64_update(XXH3_state_t* ctx, const void* data, size_t len);
int xxh3_64_final(XXH3_state_t* ctx, uint8_t* hash);

void* xxh3_128_init();
void xxh3_128_update(XXH3_state_t* ctx, const void* data, size_t len);
int xxh3_128_final(XXH3_state_t* ctx, uint8_t* hash);

#endif
