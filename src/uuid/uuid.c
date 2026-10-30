// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Based on Google's uuid package, BSD 3-Clause License
// https://github.com/google/uuid

// UUID functions.

#include "uuid/uuid.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

// Some platforms do not support timespec_get() from time.h.
#if defined(_WIN32)
#include <sys/timeb.h>
#elif !defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L || \
    (!defined(TIME_UTC) && (!defined(_POSIX_TIMERS) || _POSIX_TIMERS <= 0))
#include <sys/time.h>
#endif

#pragma region Private

static const int64_t nanos_per_milli = 1000000;

// timespec_now returns the current time with nanosecond precision.
// The same as in time/time.c: each module compiles on its own.
static struct timespec timespec_now(void) {
    struct timespec ts;
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && defined(TIME_UTC) && \
    !defined(__ANDROID__)
    // C11.
    timespec_get(&ts, TIME_UTC);
#elif defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)
    // POSIX.
    clock_gettime(CLOCK_REALTIME, &ts);
#elif defined(_WIN32)
    // Windows.
    struct __timeb64 tb;
    _ftime64(&tb);
    ts.tv_sec = (time_t)tb.time;
    ts.tv_nsec = tb.millitm * 1000000;
#else
    // Fallback for older systems.
    struct timeval tv;
    gettimeofday(&tv, NULL);
    ts.tv_sec = tv.tv_sec;
    ts.tv_nsec = tv.tv_usec * 1000;
#endif
    return ts;
}

// set_version sets the version bits of u and the variant bits to 10 (RFC 9562).
static void set_version(UUID* u, int version) {
    u->b[6] = (uint8_t)((u->b[6] & 0x0f) | (version << 4));
    u->b[8] = (uint8_t)((u->b[8] & 0x3f) | 0x80);
}

// make_v7 returns a version 7 UUID with the Unix time ms in milliseconds,
// the 12-bit rand_a, and rand_b made of the bytes in rand.
//
//  0                   1                   2                   3
//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                           unix_ts_ms                          |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |          unix_ts_ms           |  ver  |        rand_a         |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |var|                        rand_b                             |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                            rand_b                             |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
static UUID make_v7(int64_t ms, uint16_t rand_a, const uint8_t rand[UUID_SIZE]) {
    // ms is in [0, UUID_V7_MAX_MS], and shifting is defined only for unsigned values.
    uint64_t t = (uint64_t)ms;
    UUID u;
    u.b[0] = (uint8_t)(t >> 40);
    u.b[1] = (uint8_t)(t >> 32);
    u.b[2] = (uint8_t)(t >> 24);
    u.b[3] = (uint8_t)(t >> 16);
    u.b[4] = (uint8_t)(t >> 8);
    u.b[5] = (uint8_t)t;
    u.b[6] = (uint8_t)(rand_a >> 8);
    u.b[7] = (uint8_t)rand_a;
    for (int i = 8; i < UUID_SIZE; i++) {
        u.b[i] = rand[i];
    }
    set_version(&u, 7);
    return u;
}

// hex_value returns the value of the hex digit c, or -1 if c is not one.
static int hex_value(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

// xtob converts the hex digits x1 and x2 into a byte.
// Returns false if either of them is not a hex digit.
static bool xtob(char x1, char x2, uint8_t* out) {
    int hi = hex_value(x1);
    int lo = hex_value(x2);
    if (hi < 0 || lo < 0) {
        return false;
    }
    *out = (uint8_t)(hi << 4 | lo);
    return true;
}

// has_prefix_fold reports whether s starts with the lowercase ASCII prefix,
// ignoring case. Doesn't use tolower, which depends on the locale.
static bool has_prefix_fold(const char* s, const char* prefix) {
    for (; *prefix; s++, prefix++) {
        char c = (*s >= 'A' && *s <= 'Z') ? (char)(*s - 'A' + 'a') : *s;
        if (c != *prefix) {
            return false;
        }
    }
    return true;
}

#pragma endregion

#pragma region Constructors

// uuid_v4 returns a version 4 (random) UUID made of the bytes in rand,
// with the version and variant bits set.
UUID uuid_v4(const uint8_t rand[UUID_SIZE]) {
    UUID u;
    for (int i = 0; i < UUID_SIZE; i++) {
        u.b[i] = rand[i];
    }
    set_version(&u, 4);
    return u;
}

// uuid_v7 sets out to a version 7 (time-ordered) UUID with the Unix time ms
// in milliseconds and the rest made of the bytes in rand.
// Returns false if ms is outside [0, UUID_V7_MAX_MS].
//
// rand_a is random, so UUIDs made within the same millisecond
// are in random order.
bool uuid_v7(int64_t ms, const uint8_t rand[UUID_SIZE], UUID* out) {
    if (ms < 0 || ms > UUID_V7_MAX_MS) {
        return false;
    }
    uint16_t rand_a = (uint16_t)(rand[6] << 8 | rand[7]);
    *out = make_v7(ms, rand_a, rand);
    return true;
}

// uuid_v7_time is like uuid_v7, but takes the Unix time as sec + nsec
// and stores the sub-millisecond fraction in the 12 bits after the version,
// so UUIDs made within the same millisecond sort by time.
// Returns false if the time in milliseconds is outside [0, UUID_V7_MAX_MS]
// or nsec is outside [0, 999999999].
//
// This is method 3 of RFC 9562, section 6.2: the fraction is scaled to
// 4096 steps of about 244 nanoseconds. UUIDs made within the same step,
// or after the clock goes back, are not ordered.
bool uuid_v7_time(int64_t sec, int32_t nsec, const uint8_t rand[UUID_SIZE], UUID* out) {
    if (nsec < 0 || nsec > 999999999) {
        return false;
    }
    // Check sec before multiplying, so sec * 1000 can't overflow.
    if (sec < 0 || sec > UUID_V7_MAX_MS / 1000) {
        return false;
    }
    int64_t ms = sec * 1000 + nsec / nanos_per_milli;
    if (ms > UUID_V7_MAX_MS) {
        return false;
    }
    // At most 999999 * 4096, which fits in 32 bits.
    uint32_t frac = (uint32_t)(nsec % nanos_per_milli);
    uint16_t rand_a = (uint16_t)(frac * 4096 / nanos_per_milli);
    *out = make_v7(ms, rand_a, rand);
    return true;
}

// uuid_v7_now is uuid_v7_time with the current time.
// Returns false if the system clock is outside the version 7 range.
bool uuid_v7_now(const uint8_t rand[UUID_SIZE], UUID* out) {
    struct timespec ts = timespec_now();
    return uuid_v7_time((int64_t)ts.tv_sec, (int32_t)ts.tv_nsec, rand, out);
}

#pragma endregion

#pragma region Fields

// uuid_get_version returns the version of u (the 4 bits of the M digit).
// The variant is not checked.
int uuid_get_version(UUID u) {
    return u.b[6] >> 4;
}

// uuid_get_unix_ms sets out to the Unix time in milliseconds stored in u.
// Returns false if u is not a version 7 UUID. The variant is not checked.
bool uuid_get_unix_ms(UUID u, int64_t* out) {
    if (uuid_get_version(u) != 7) {
        return false;
    }
    // 48 bits, so the result always fits in int64_t.
    uint64_t ms = 0;
    for (int i = 0; i < 6; i++) {
        ms = ms << 8 | u.b[i];
    }
    *out = (int64_t)ms;
    return true;
}

#pragma endregion

#pragma region Formatting

// uuid_fmt writes u in the canonical form xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
// (lowercase) into buf, with snprintf semantics. Returns 36.
size_t uuid_fmt(char* buf, size_t size, UUID u) {
    static const char digits[] = "0123456789abcdef";
    char str[UUID_STR_SIZE];
    char* p = str;
    for (int i = 0; i < UUID_SIZE; i++) {
        if (i == 4 || i == 6 || i == 8 || i == 10) {
            *p++ = '-';
        }
        *p++ = digits[u.b[i] >> 4];
        *p++ = digits[u.b[i] & 0x0f];
    }
    *p = '\0';
    return (size_t)snprintf(buf, size, "%s", str);
}

// uuid_parse parses the n bytes of s as a UUID.
// Returns false if s is not a UUID.
//
// Accepts the following forms, with hex digits in any case:
//
//  xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
//  {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
//  urn:uuid:xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
//  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//
// Any other byte, including NUL, makes s invalid.
bool uuid_parse(const char* s, size_t n, UUID* out) {
    UUID u;
    switch (n) {
        // xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
        case 36:
            break;

        // urn:uuid:xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
        case 36 + 9:
            if (!has_prefix_fold(s, "urn:uuid:")) {
                return false;
            }
            s += 9;
            break;

        // {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
        // Go skips the first byte without checking either brace; we check both.
        case 36 + 2:
            if (s[0] != '{' || s[37] != '}') {
                return false;
            }
            s += 1;
            break;

        // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
        case 32:
            for (int i = 0; i < UUID_SIZE; i++) {
                if (!xtob(s[i * 2], s[i * 2 + 1], &u.b[i])) {
                    return false;
                }
            }
            *out = u;
            return true;

        default:
            return false;
    }

    // s is now at least 36 bytes long,
    // it must be of the form xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    if (s[8] != '-' || s[13] != '-' || s[18] != '-' || s[23] != '-') {
        return false;
    }
    static const int offsets[UUID_SIZE] = {0,  2,  4,  6,  9,  11, 14, 16,
                                           19, 21, 24, 26, 28, 30, 32, 34};
    for (int i = 0; i < UUID_SIZE; i++) {
        if (!xtob(s[offsets[i]], s[offsets[i] + 1], &u.b[i])) {
            return false;
        }
    }
    *out = u;
    return true;
}

#pragma endregion
