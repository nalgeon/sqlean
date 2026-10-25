/*
 * FIPS 180-2 SHA-224/256/384/512 implementation
 *
 * Copyright (C) 2005-2023 Olivier Gay <olivier.gay@a3.epfl.ch>
 * https://github.com/ogay/sha2, BSD 3-Clause License
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __SHA2_H__
#define __SHA2_H__

#include <stddef.h>
#include <stdint.h>

#define SHA256_DIGEST_SIZE (256 / 8)
#define SHA384_DIGEST_SIZE (384 / 8)
#define SHA512_DIGEST_SIZE (512 / 8)

#define SHA256_BLOCK_SIZE (512 / 8)
#define SHA512_BLOCK_SIZE (1024 / 8)
#define SHA384_BLOCK_SIZE SHA512_BLOCK_SIZE

typedef struct {
    uint64_t tot_len;
    uint64_t len;
    uint8_t block[2 * SHA256_BLOCK_SIZE];
    uint32_t h[8];
} sha256_ctx;

typedef struct {
    uint64_t tot_len;
    uint64_t len;
    uint8_t block[2 * SHA512_BLOCK_SIZE];
    uint64_t h[8];
} sha512_ctx;

typedef sha512_ctx sha384_ctx;

// Incremental hashing: init, then update any number of times, then final.
// The context is owned by the caller. The message may be NULL if len is 0.

void sha256_init(sha256_ctx* ctx);
void sha256_update(sha256_ctx* ctx, const uint8_t* message, size_t len);
void sha256_final(sha256_ctx* ctx, uint8_t digest[SHA256_DIGEST_SIZE]);

void sha384_init(sha384_ctx* ctx);
void sha384_update(sha384_ctx* ctx, const uint8_t* message, size_t len);
void sha384_final(sha384_ctx* ctx, uint8_t digest[SHA384_DIGEST_SIZE]);

void sha512_init(sha512_ctx* ctx);
void sha512_update(sha512_ctx* ctx, const uint8_t* message, size_t len);
void sha512_final(sha512_ctx* ctx, uint8_t digest[SHA512_DIGEST_SIZE]);

// One-shot hashing of len bytes. The data may be NULL if len is 0.

void sha256_hash(const uint8_t* data, size_t len, uint8_t digest[SHA256_DIGEST_SIZE]);
void sha384_hash(const uint8_t* data, size_t len, uint8_t digest[SHA384_DIGEST_SIZE]);
void sha512_hash(const uint8_t* data, size_t len, uint8_t digest[SHA512_DIGEST_SIZE]);

#endif
