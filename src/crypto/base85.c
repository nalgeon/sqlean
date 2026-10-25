// Originally by Fränz Friederes, MIT License
// https://github.com/cryptii/cryptii/blob/main/src/Encoder/Ascii85.js

// Modified by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean/

// Base85 (Ascii85) encoding/decoding

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "crypto/base85.h"
#include "crypto/common.h"

// Maximum value of a tuple (85^5 - 1 does not fit into 32 bits).
#define MAX_TUPLE UINT32_C(0xFFFFFFFF)

size_t base85_encoded_size(size_t len) {
    // Each group of 4 bytes takes at most 5 characters.
    return groups_size(len, 4, 5);
}

size_t base85_encode(const uint8_t* src, size_t len, uint8_t* dst) {
    // Encode each group of 4 bytes
    uint32_t digits[5], tuple;
    size_t pos = 0;
    for (size_t i = 0; i < len; i += 4) {
        // The last group may be partial
        size_t group_len = (len - i < 4) ? len - i : 4;

        // Read 32-bit unsigned integer from bytes following the
        // big-endian convention (most significant byte first),
        // padding a partial group with zero bytes
        tuple = 0;
        for (size_t j = 0; j < group_len; j++) {
            tuple |= (uint32_t)src[i + j] << (24 - 8 * j);
        }

        if (tuple == 0 && group_len == 4) {
            // An all-zero tuple is encoded as a single character.
            // Only full groups qualify: for a partial group the shorthand
            // would decode into more bytes than the source has.
            dst[pos++] = 'z';
            continue;
        }

        // Calculate 5 digits by repeatedly dividing
        // by 85 and taking the remainder
        for (size_t j = 0; j < 5; j++) {
            digits[4 - j] = tuple % 85;
            tuple = tuple / 85;
        }

        // A group of n bytes is encoded as n+1 characters,
        // so omit the characters added due to zero padding
        for (size_t j = 0; j < group_len + 1; j++) {
            dst[pos++] = digits[j] + 33;
        }
    }

    return pos;
}

size_t base85_decoded_size(const uint8_t* src, size_t len) {
    // Every 'z' expands into 4 bytes, while the other characters decode
    // in groups of 5 to 4 bytes. Since 'z' is only valid at a group
    // boundary (it is rejected as a digit otherwise), the remaining
    // characters form full groups plus at most one trailing group of
    // n characters, which decodes into n-1 bytes. Both cases are covered
    // by rounding down: (5k + n) * 4 / 5 = 4k + n - 1 for 2 <= n <= 4.
    size_t n_shorthand = 0;
    for (size_t i = 0; i < len; i++) {
        if (src[i] == 'z') {
            n_shorthand++;
        }
    }
    // Saturate instead of overflowing (only possible for len > SIZE_MAX / 4).
    if (n_shorthand > SIZE_MAX / 4) {
        return SIZE_MAX;
    }
    size_t rest = (len - n_shorthand) / 5 * 4 + (len - n_shorthand) % 5 * 4 / 5;
    if (rest > SIZE_MAX - n_shorthand * 4) {
        return SIZE_MAX;
    }
    return rest + n_shorthand * 4;
}

bool base85_decode(const uint8_t* src, size_t len, uint8_t* dst, size_t* out_len) {
    uint8_t digits[5], tuple_bytes[4];
    uint64_t tuple;
    size_t pos = 0;
    for (size_t i = 0; i < len;) {
        if (src[i] == 'z') {
            // A single character encodes an all-zero tuple
            dst[pos++] = 0;
            dst[pos++] = 0;
            dst[pos++] = 0;
            dst[pos++] = 0;
            i++;
            continue;
        }

        // The last group may be partial, but a single character
        // encodes no bytes at all, so it is invalid
        size_t group_len = (len - i < 5) ? len - i : 5;
        if (group_len == 1) {
            return false;
        }

        // Retrieve radix-85 digits of tuple,
        // padding a partial group with the largest digit ('u')
        for (size_t k = 0; k < 5; k++) {
            if (k >= group_len) {
                digits[k] = 84;
                continue;
            }
            uint8_t digit = src[i + k] - 33;
            if (digit > 84) {
                return false;
            }
            digits[k] = digit;
        }

        // Create 32-bit binary number from digits and handle padding
        // tuple = a * 85^4 + b * 85^3 + c * 85^2 + d * 85 + e
        // (calculated as 64-bit, since the digits may overflow 32 bits)
        tuple = (uint64_t)digits[0] * 52200625 + (uint64_t)digits[1] * 614125 +
                (uint64_t)digits[2] * 7225 + (uint64_t)digits[3] * 85 + digits[4];
        if (tuple > MAX_TUPLE) {
            return false;
        }

        // Get bytes from tuple
        tuple_bytes[0] = (tuple >> 24) & 0xff;
        tuple_bytes[1] = (tuple >> 16) & 0xff;
        tuple_bytes[2] = (tuple >> 8) & 0xff;
        tuple_bytes[3] = tuple & 0xff;

        // Append bytes to result, dropping the ones
        // that came from the padding digits
        for (size_t k = 0; k + 1 < group_len; k++) {
            dst[pos++] = tuple_bytes[k];
        }
        i += 5;
    }

    *out_len = pos;
    return true;
}
