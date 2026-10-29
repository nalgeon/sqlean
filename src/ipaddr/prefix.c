// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Based on Go's net/netip package, BSD 3-Clause License
// https://github.com/golang/go

// IP prefix parsing, masking and formatting.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ipaddr/ipaddr.h"

#pragma region Private

// max_bits returns the number of bits in an address of the family.
static int max_bits(IPFamily family) {
    return family == IPADDR_V4 ? 32 : 128;
}

// mask_bytes clears all but the first bits bits of the 16 bytes of b.
// For an IPv4 address, the prefix starts after the 96 bits of the
// IPv4-mapped form.
static void mask_bytes(uint8_t* b, IPFamily family, int bits) {
    if (family == IPADDR_V4) {
        bits += 96;
    }
    for (int i = 0; i < 16; i++) {
        int keep = bits - 8 * i;  // the number of bits to keep in b[i]
        if (keep >= 8) {
            continue;
        }
        if (keep <= 0) {
            b[i] = 0;
        } else {
            // keep is 1-7, so the shift is less than the width
            b[i] &= (uint8_t)(0xffu << (8 - keep));
        }
    }
}

#pragma endregion

#pragma region Prefixes

// ipprefix_parse parses s as an IP prefix in CIDR notation
// ("192.168.1.0/24", "2001:db8::/32"), or as an address, which is
// a prefix of all its bits. Returns false if s is not a valid prefix.
// The prefix length is decimal without a sign or leading zeros, as in Go.
// Unlike Go, the prefix length is optional. Host bits are kept.
bool ipprefix_parse(const char* s, size_t n, IPPrefix* out) {
    size_t slash = n;
    while (slash > 0 && s[slash - 1] != '/') {
        slash--;
    }

    IPAddr addr;
    if (slash == 0) {
        // no prefix length
        if (!ipaddr_parse(s, n, &addr)) {
            return false;
        }
        *out = (IPPrefix){.addr = addr, .bits = (uint8_t)max_bits(addr.family)};
        return true;
    }

    if (!ipaddr_parse(s, slash - 1, &addr)) {
        return false;
    }
    const char* bstr = s + slash;
    size_t blen = n - slash;
    // at most 3 digits: a longer length is out of range anyway,
    // and the limit keeps the value from overflowing
    if (blen == 0 || blen > 3 || (blen > 1 && bstr[0] == '0')) {
        return false;
    }
    int bits = 0;
    for (size_t i = 0; i < blen; i++) {
        if (bstr[i] < '0' || bstr[i] > '9') {
            return false;
        }
        bits = bits * 10 + (bstr[i] - '0');
    }
    if (bits > max_bits(addr.family)) {
        return false;
    }
    *out = (IPPrefix){.addr = addr, .bits = (uint8_t)bits};
    return true;
}

// ipprefix_masked returns p with the host bits of its address cleared.
IPPrefix ipprefix_masked(IPPrefix p) {
    mask_bytes(p.addr.bytes, p.addr.family, p.bits);
    return p;
}

// ipprefix_contains reports whether every address in q is in p: they
// are of the same family, q is not shorter than p, and the addresses
// match in the first p.bits bits. The host bits of p are ignored.
// As in Go, an IPv4-mapped address is not in an IPv4 prefix,
// and an IPv4 address is not in an IPv6 prefix.
bool ipprefix_contains(IPPrefix p, IPPrefix q) {
    if (p.addr.family != q.addr.family || p.bits > q.bits) {
        return false;
    }
    mask_bytes(p.addr.bytes, p.addr.family, p.bits);
    mask_bytes(q.addr.bytes, q.addr.family, p.bits);
    return memcmp(p.addr.bytes, q.addr.bytes, sizeof(p.addr.bytes)) == 0;
}

// ipprefix_fmt writes the text form of p into buf, as snprintf does,
// and returns its length. The address is formatted as in ipaddr_fmt,
// host bits included, followed by the prefix length: 192.168.1.7/24.
size_t ipprefix_fmt(char* buf, size_t size, IPPrefix p) {
    char addr[IPADDR_FMT_BUF_SIZE];
    ipaddr_fmt(addr, sizeof(addr), p.addr);
    return (size_t)snprintf(buf, size, "%s/%u", addr, (unsigned)p.bits);
}

#pragma endregion
