// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Based on Google's uuid package, BSD 3-Clause License
// https://github.com/google/uuid

// Universally unique identifiers (UUIDs) as defined in RFC 9562.

#ifndef UUID_UUID_H
#define UUID_UUID_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// UUID_SIZE is the size of a UUID in bytes.
#define UUID_SIZE 16

// UUID_STR_SIZE is the buffer size that is always sufficient for uuid_fmt:
// 32 hex digits, 4 hyphens and the terminating NUL.
#define UUID_STR_SIZE 37

// UUID_V7_MAX_MS is the largest Unix time in milliseconds
// a version 7 UUID can hold (48 bits, about the year 10889).
#define UUID_V7_MAX_MS ((INT64_C(1) << 48) - 1)

// UUID is a 128-bit UUID in network byte order (big-endian),
// so comparing two UUIDs with memcmp orders them as their strings do.
typedef struct {
    uint8_t b[UUID_SIZE];
} UUID;

// Constructors.

// uuid_v4 returns a version 4 (random) UUID made of the bytes in rand,
// with the version and variant bits set.
UUID uuid_v4(const uint8_t rand[UUID_SIZE]);

// uuid_v7 sets out to a version 7 (time-ordered) UUID with the Unix time ms
// in milliseconds and the rest made of the bytes in rand.
// Returns false if ms is outside [0, UUID_V7_MAX_MS].
bool uuid_v7(int64_t ms, const uint8_t rand[UUID_SIZE], UUID* out);

// uuid_v7_time is like uuid_v7, but takes the Unix time as sec + nsec
// and stores the sub-millisecond fraction in the 12 bits after the version,
// so UUIDs made within the same millisecond sort by time.
// Returns false if the time in milliseconds is outside [0, UUID_V7_MAX_MS]
// or nsec is outside [0, 999999999].
bool uuid_v7_time(int64_t sec, int32_t nsec, const uint8_t rand[UUID_SIZE], UUID* out);

// uuid_v7_now is uuid_v7_time with the current time.
// Returns false if the system clock is outside the version 7 range.
bool uuid_v7_now(const uint8_t rand[UUID_SIZE], UUID* out);

// Fields.

// uuid_get_version returns the version of u (the 4 bits of the M digit).
int uuid_get_version(UUID u);

// uuid_get_unix_ms sets out to the Unix time in milliseconds stored in u.
// Returns false if u is not a version 7 UUID.
bool uuid_get_unix_ms(UUID u, int64_t* out);

// Formatting.

// uuid_fmt writes u in the canonical form xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
// (lowercase) into buf, with snprintf semantics. Returns 36.
size_t uuid_fmt(char* buf, size_t size, UUID u);

// uuid_parse parses the n bytes of s as a UUID.
// Returns false if s is not a UUID.
bool uuid_parse(const char* s, size_t n, UUID* out);

#endif /* UUID_UUID_H */
