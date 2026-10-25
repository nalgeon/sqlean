// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Base32 encoding/decoding (RFC 4648)

#ifndef _BASE32_H_
#define _BASE32_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// base32_encoded_size returns the maximum number of bytes base32_encode
// writes for len source bytes, or SIZE_MAX if it does not fit into size_t.
size_t base32_encoded_size(size_t len);

// base32_encode encodes len bytes of src into dst, which must hold at least
// base32_encoded_size(len) bytes. Returns the number of bytes written.
// The result is not NUL-terminated. src may be NULL if len is 0.
size_t base32_encode(const uint8_t* src, size_t len, uint8_t* dst);

// base32_decoded_size returns the maximum number of bytes base32_decode
// writes for the given source, or SIZE_MAX if it does not fit into size_t.
size_t base32_decoded_size(const uint8_t* src, size_t len);

// base32_decode decodes len bytes of src into dst, which must hold at least
// base32_decoded_size(src, len) bytes, and sets out_len to the number of bytes
// written. Returns false if src is not valid base32, in which case the contents
// of dst and out_len are unspecified. src may be NULL if len is 0.
bool base32_decode(const uint8_t* src, size_t len, uint8_t* dst, size_t* out_len);

#endif /* _BASE32_H_ */
