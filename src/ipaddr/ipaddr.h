// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Based on Go's net/netip package, BSD 3-Clause License
// https://github.com/golang/go

// IP addresses and prefixes.

// The parsers take a string as a pointer and a length in bytes, so the
// string doesn't have to be NUL-terminated. A NUL byte is not valid
// anywhere in an address or a prefix.

#ifndef IPADDR_IPADDR_H
#define IPADDR_IPADDR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// IPFamily is the address family, numbered as the IP version.
typedef enum {
    IPADDR_V4 = 4,
    IPADDR_V6 = 6,
} IPFamily;

// IPAddr is an IPv4 or IPv6 address. bytes holds the 128-bit address in
// network byte order. As in Go, an IPv4 address a.b.c.d is stored as the
// IPv4-mapped ::ffff:a.b.c.d, so it differs from that IPv6 address only
// by the family.
typedef struct {
    uint8_t bytes[16];
    IPFamily family;
} IPAddr;

// IPPrefix is an IP address with a prefix length in bits: 0-32 for IPv4,
// 0-128 for IPv6 (including IPv4-mapped addresses). The bits of the address
// after the prefix (the host bits) are kept as is; ipprefix_masked clears them.
typedef struct {
    IPAddr addr;
    uint8_t bits;
} IPPrefix;

// IPADDR_FMT_BUF_SIZE is the buffer size that is always sufficient for
// ipaddr_fmt and ipprefix_fmt: the longest address takes 39 characters
// (8 groups of 4 hex digits and 7 colons; with "::" at least two groups
// are gone, and an IPv4-mapped address takes at most 22), the prefix
// length adds 4 ("/128"), and the NUL makes it 44.
#define IPADDR_FMT_BUF_SIZE 44

// Addresses.

// ipaddr_parse parses s as an IP address: IPv4 in dotted decimal
// ("192.0.2.1") or IPv6 ("2001:db8::68", "::ffff:192.0.2.1").
// Returns false if s is not a valid address.
bool ipaddr_parse(const char* s, size_t n, IPAddr* out);

// ipaddr_fmt writes the text form of a into buf, as snprintf does,
// and returns its length.
size_t ipaddr_fmt(char* buf, size_t size, IPAddr a);

// Prefixes.

// ipprefix_parse parses s as an IP prefix in CIDR notation
// ("192.168.1.0/24", "2001:db8::/32"), or as an address, which is
// a prefix of all its bits. Returns false if s is not a valid prefix.
bool ipprefix_parse(const char* s, size_t n, IPPrefix* out);

// ipprefix_masked returns p with the host bits of its address cleared.
IPPrefix ipprefix_masked(IPPrefix p);

// ipprefix_contains reports whether every address in q is in p.
bool ipprefix_contains(IPPrefix p, IPPrefix q);

// ipprefix_fmt writes the text form of p into buf, as snprintf does,
// and returns its length.
size_t ipprefix_fmt(char* buf, size_t size, IPPrefix p);

#endif /* IPADDR_IPADDR_H */
