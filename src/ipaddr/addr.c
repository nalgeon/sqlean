// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Based on Go's net/netip package, BSD 3-Clause License
// https://github.com/golang/go

// IP address parsing and formatting.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ipaddr/ipaddr.h"

#pragma region Private

// hex_digit returns the value of the hex digit c, or -1 if it's not one.
static int hex_digit(char c) {
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

// parse_ipv4_fields parses s as a dotted decimal IPv4 address into the
// 4 bytes of fields. Returns false if s is not a valid IPv4 address.
// Like Go, rejects octets with leading zeros, since some parsers read
// them as octal.
static bool parse_ipv4_fields(const char* s, size_t n, uint8_t* fields) {
    int val = 0;
    int pos = 0;
    int digits = 0;  // number of digits in the current octet
    for (size_t i = 0; i < n; i++) {
        char c = s[i];
        if (c >= '0' && c <= '9') {
            if (digits == 1 && val == 0) {
                // leading zero
                return false;
            }
            val = val * 10 + (c - '0');
            digits++;
            if (val > 255) {
                return false;
            }
        } else if (c == '.') {
            // .1.2.3, 1.2.3., 1..2.3
            if (i == 0 || i == n - 1 || s[i - 1] == '.') {
                return false;
            }
            // 1.2.3.4.5
            if (pos == 3) {
                return false;
            }
            fields[pos++] = (uint8_t)val;
            val = 0;
            digits = 0;
        } else {
            return false;
        }
    }
    if (pos < 3) {
        return false;
    }
    fields[3] = (uint8_t)val;
    return true;
}

// parse_ipv4 parses s as an IPv4 address ("192.168.0.1").
static bool parse_ipv4(const char* s, size_t n, IPAddr* out) {
    IPAddr a = {.bytes = {[10] = 0xff, [11] = 0xff}, .family = IPADDR_V4};
    if (!parse_ipv4_fields(s, n, a.bytes + 12)) {
        return false;
    }
    *out = a;
    return true;
}

// parse_ipv6 parses s as an IPv6 address ("2001:db8::68").
// Unlike Go, doesn't accept a zone ("fe80::1%eth0"): '%' is
// an unexpected character.
static bool parse_ipv6(const char* s, size_t n, IPAddr* out) {
    IPAddr a = {.family = IPADDR_V6};
    uint8_t* ip = a.bytes;
    int ellipsis = -1;  // position of the ellipsis in ip

    // might have a leading ellipsis
    if (n >= 2 && s[0] == ':' && s[1] == ':') {
        ellipsis = 0;
        s += 2;
        n -= 2;
        // might be only the ellipsis
        if (n == 0) {
            *out = a;
            return true;
        }
    }

    // loop, parsing hex numbers followed by a colon
    int i = 0;
    while (i < 16) {
        size_t off = 0;
        uint32_t acc = 0;
        for (; off < n; off++) {
            int d = hex_digit(s[off]);
            if (d < 0) {
                break;
            }
            // more than 4 digits in the group; at most 4 digits
            // also means the value fits in 16 bits
            if (off > 3) {
                return false;
            }
            acc = (acc << 4) + (uint32_t)d;
        }
        if (off == 0) {
            // no digits
            return false;
        }

        // if followed by a dot, might be a trailing IPv4
        if (off < n && s[off] == '.') {
            if (ellipsis < 0 && i != 12) {
                // not the right place
                return false;
            }
            if (i + 4 > 16) {
                // not enough room
                return false;
            }
            // the IPv4 starts at the digits just read and takes the rest
            if (!parse_ipv4_fields(s, n, ip + i)) {
                return false;
            }
            n = 0;
            i += 4;
            break;
        }

        // save this 16-bit chunk
        ip[i] = (uint8_t)(acc >> 8);
        ip[i + 1] = (uint8_t)acc;
        i += 2;

        // stop at the end of the string
        s += off;
        n -= off;
        if (n == 0) {
            break;
        }

        // otherwise must be followed by a colon and more
        if (s[0] != ':' || n == 1) {
            return false;
        }
        s++;
        n--;

        // look for an ellipsis
        if (s[0] == ':') {
            if (ellipsis >= 0) {
                // already have one
                return false;
            }
            ellipsis = i;
            s++;
            n--;
            if (n == 0) {
                // can be at the end
                break;
            }
        }
    }

    // must have used the entire string
    if (n != 0) {
        return false;
    }

    // if didn't parse enough, expand the ellipsis
    if (i < 16) {
        if (ellipsis < 0) {
            return false;
        }
        int shift = 16 - i;
        for (int j = i - 1; j >= ellipsis; j--) {
            ip[j + shift] = ip[j];
        }
        memset(ip + ellipsis, 0, (size_t)shift);
    } else if (ellipsis >= 0) {
        // the ellipsis must represent at least one zero group
        return false;
    }
    *out = a;
    return true;
}

// Text is a buffer that any formatted address fits in.
typedef struct {
    char s[IPADDR_FMT_BUF_SIZE];
    size_t len;
} Text;

static void put_char(Text* t, char c) {
    t->s[t->len++] = c;
}

static void put_dec(Text* t, uint8_t x) {
    t->len += (size_t)snprintf(t->s + t->len, sizeof(t->s) - t->len, "%u", (unsigned)x);
}

static void put_hex(Text* t, uint16_t x) {
    t->len += (size_t)snprintf(t->s + t->len, sizeof(t->s) - t->len, "%x", (unsigned)x);
}

// group returns the i-th 16-bit group of a.
static uint16_t group(IPAddr a, int i) {
    return (uint16_t)((unsigned)a.bytes[2 * i] << 8 | a.bytes[2 * i + 1]);
}

// is_4in6 reports whether a is an IPv4-mapped IPv6 address.
static bool is_4in6(IPAddr a) {
    static const uint8_t mapped[12] = {[10] = 0xff, [11] = 0xff};
    return a.family == IPADDR_V6 && memcmp(a.bytes, mapped, sizeof(mapped)) == 0;
}

// put_ipv4 writes the last 4 bytes of a in dotted decimal.
static void put_ipv4(Text* t, IPAddr a) {
    for (int i = 12; i < 16; i++) {
        if (i > 12) {
            put_char(t, '.');
        }
        put_dec(t, a.bytes[i]);
    }
}

// put_ipv6 writes a following section 4 of RFC 5952: no leading zeros,
// :: for the longest run of zero groups (the first of equal ones),
// and no :: for a single zero group.
static void put_ipv6(Text* t, IPAddr a) {
    int zero_start = -1;
    int zero_end = -1;
    for (int i = 0; i < 8; i++) {
        int j = i;
        while (j < 8 && group(a, j) == 0) {
            j++;
        }
        if (j - i >= 2 && j - i > zero_end - zero_start) {
            zero_start = i;
            zero_end = j;
        }
    }

    for (int i = 0; i < 8; i++) {
        if (i == zero_start) {
            put_char(t, ':');
            put_char(t, ':');
            i = zero_end;
            if (i >= 8) {
                break;
            }
        } else if (i > 0) {
            put_char(t, ':');
        }
        put_hex(t, group(a, i));
    }
}

#pragma endregion

#pragma region Addresses

// ipaddr_parse parses s as an IP address: IPv4 in dotted decimal
// ("192.0.2.1") or IPv6 ("2001:db8::68", "::ffff:192.0.2.1").
// Returns false if s is not a valid address.
// As in Go, the first '.' or ':' decides the family, and an IPv4-mapped
// address is IPv6. Unlike Go, IPv6 zones are not valid.
bool ipaddr_parse(const char* s, size_t n, IPAddr* out) {
    for (size_t i = 0; i < n; i++) {
        switch (s[i]) {
            case '.':
                return parse_ipv4(s, n, out);
            case ':':
                return parse_ipv6(s, n, out);
            case '%':
                return false;
        }
    }
    return false;
}

// ipaddr_fmt writes the text form of a into buf, as snprintf does,
// and returns its length. IPv4 is in dotted decimal. IPv6 follows
// RFC 5952: lowercase, without leading zeros, with the longest run of
// zero groups compressed to ::, and with IPv4-mapped addresses as
// ::ffff:a.b.c.d. Other IPv6 addresses with an IPv4 part, such as the
// deprecated IPv4-compatible ones, are in hex: ::c000:201.
size_t ipaddr_fmt(char* buf, size_t size, IPAddr a) {
    Text t = {.len = 0};
    if (a.family == IPADDR_V4) {
        put_ipv4(&t, a);
    } else if (is_4in6(a)) {
        memcpy(t.s, "::ffff:", 7);
        t.len = 7;
        put_ipv4(&t, a);
    } else {
        put_ipv6(&t, a);
    }
    t.s[t.len] = 0;
    return (size_t)snprintf(buf, size, "%s", t.s);
}

#pragma endregion
