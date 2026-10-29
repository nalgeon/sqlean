// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// IP prefix tests. Most cases come from Go's net/netip tests.

// Each formatting check gets a heap buffer of exactly the needed size,
// so a sanitizer catches an overflow.

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ipaddr/ipaddr.h"

// S makes a string argument with an explicit length, so it can contain NUL.
#define S(s) s, sizeof(s) - 1

#define LEN(a) (sizeof(a) / sizeof(a[0]))

// parse parses a NUL-terminated s, which must be valid.
static IPPrefix parse(const char* s) {
    IPPrefix p;
    bool ok = ipprefix_parse(s, strlen(s), &p);
    assert(ok);
    return p;
}

// check_fmt formats p into a buffer of exactly the needed size
// and compares it to want.
static void check_fmt(IPPrefix p, const char* want) {
    size_t size = strlen(want) + 1;
    char* buf = malloc(size);
    assert(buf != NULL);
    size_t len = ipprefix_fmt(buf, size, p);
    assert(len == strlen(want));
    assert(strcmp(buf, want) == 0);
    free(buf);
}

static void test_ipprefix_parse(void) {
    printf("test_ipprefix_parse...");
    static const struct {
        const char* in;
        IPFamily family;
        int bits;
        const char* want;  // the formatted prefix
    } tests[] = {
        {"192.168.0.0/24", IPADDR_V4, 24, "192.168.0.0/24"},
        {"0.0.0.0/0", IPADDR_V4, 0, "0.0.0.0/0"},
        {"1.2.3.4/32", IPADDR_V4, 32, "1.2.3.4/32"},
        {"2001:db8::/32", IPADDR_V6, 32, "2001:db8::/32"},
        {"::/0", IPADDR_V6, 0, "::/0"},
        {"::1/128", IPADDR_V6, 128, "::1/128"},
        {"::ffff:1.2.3.4/96", IPADDR_V6, 96, "::ffff:1.2.3.4/96"},
        {"::ffff:1.2.3.4/128", IPADDR_V6, 128, "::ffff:1.2.3.4/128"},
        // host bits are kept
        {"192.168.16.12/8", IPADDR_V4, 8, "192.168.16.12/8"},
        {"2001:db8::1/32", IPADDR_V6, 32, "2001:db8::1/32"},
        // an address is a prefix of all its bits
        {"1.2.3.4", IPADDR_V4, 32, "1.2.3.4/32"},
        {"2001:db8::1", IPADDR_V6, 128, "2001:db8::1/128"},
        {"::ffff:1.2.3.4", IPADDR_V6, 128, "::ffff:1.2.3.4/128"},
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        IPPrefix p = parse(tests[i].in);
        assert(p.addr.family == tests[i].family);
        assert(p.bits == tests[i].bits);
        check_fmt(p, tests[i].want);
    }
    printf("OK\n");
}

static void test_ipprefix_parse_invalid(void) {
    printf("test_ipprefix_parse_invalid...");
    static const char* tests[] = {
        "",
        "/",
        "/24",
        "1.257.1.1/24",
        "1.1.1.0/",
        "1.1.1.0/q",
        "1.1.1.0/-1",
        "1.1.1.0/33",
        "2001::/129",
        "1.1.1.0/1000",
        "1.1.1.0/99999999999999999999",
        // only canonical decimal
        "1.1.1.0/+32",
        "1.1.1.0/-32",
        "1.1.1.0/032",
        "1.1.1.0/00",
        "1.1.1.0/ 24",
        "1.1.1.0/24 ",
        "1.1.1.0/0x18",
        // a single slash
        "1.1.1.0/24/24",
        "1.1.1.0//24",
        // zones are not supported
        "1.1.1.0%a/24",
        "2001:db8::%a/32",
        "fe80::1%eth0",
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        IPPrefix p;
        assert(!ipprefix_parse(tests[i], strlen(tests[i]), &p));
    }
    printf("OK\n");
}

static void test_ipprefix_parse_length(void) {
    printf("test_ipprefix_parse_length...");
    IPPrefix p;
    // the parser reads only n bytes, so the string doesn't have to end there
    assert(ipprefix_parse("1.2.3.4/245", 10, &p));
    assert(p.bits == 24);
    assert(ipprefix_parse("1.2.3.4/24", 7, &p));
    assert(p.bits == 32);
    // a NUL is not valid
    assert(!ipprefix_parse(S("1.2.3.4/24\0"), &p));
    assert(!ipprefix_parse(S("1.2.3.4/2\0"), &p));
    assert(!ipprefix_parse(S("1.2.3.4\0/24"), &p));
    printf("OK\n");
}

static void test_ipprefix_masked(void) {
    printf("test_ipprefix_masked...");
    static const char* tests[][2] = {
        {"2001:db8::1/32", "2001:db8::/32"},
        {"fe80::dead:beef:dead:beef/96", "fe80::dead:beef:0:0/96"},
        {"aaaa::/4", "a000::/4"},
        {"::/63", "::/63"},
        {"2001:db8::1/128", "2001:db8::1/128"},
        {"ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/0", "::/0"},
        {"ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/65", "ffff:ffff:ffff:ffff:8000::/65"},
        {"ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/127",
         "ffff:ffff:ffff:ffff:ffff:ffff:ffff:fffe/127"},
        {"192.0.2.0/16", "192.0.0.0/16"},
        {"255.255.255.255/20", "255.255.240.0/20"},
        {"255.255.255.255/0", "0.0.0.0/0"},
        {"255.255.255.255/1", "128.0.0.0/1"},
        {"255.255.255.255/31", "255.255.255.254/31"},
        {"255.255.255.255/32", "255.255.255.255/32"},
        {"192.168.16.12/24", "192.168.16.0/24"},
        // an IPv4-mapped prefix counts all 128 bits
        {"::ffff:255.255.255.255/120", "::ffff:255.255.255.0/120"},
        {"::ffff:255.255.255.255/96", "::ffff:0.0.0.0/96"},
        {"::ffff:255.255.255.255/80", "::/80"},
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        check_fmt(ipprefix_masked(parse(tests[i][0])), tests[i][1]);
    }
    printf("OK\n");
}

static void test_ipprefix_masked_bits(void) {
    printf("test_ipprefix_masked_bits...");
    // every prefix length keeps exactly its bits of an all-ones address
    IPPrefix v4 = parse("255.255.255.255");
    IPPrefix v6 = parse("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
    for (int bits = 0; bits <= 128; bits++) {
        IPPrefix p = v6;
        p.bits = (uint8_t)bits;
        IPPrefix m = ipprefix_masked(p);
        assert(m.bits == bits);
        int ones = 0;
        for (int i = 0; i < 16; i++) {
            for (int b = 7; b >= 0; b--) {
                int bit = (m.addr.bytes[i] >> b) & 1;
                // the ones come first
                assert(bit == (ones < bits ? 1 : 0));
                ones += bit;
            }
        }
        if (bits <= 32) {
            p = v4;
            p.bits = (uint8_t)bits;
            m = ipprefix_masked(p);
            uint32_t got = (uint32_t)m.addr.bytes[12] << 24 | (uint32_t)m.addr.bytes[13] << 16 |
                           (uint32_t)m.addr.bytes[14] << 8 | m.addr.bytes[15];
            uint32_t want = bits == 0 ? 0 : 0xffffffffu << (32 - bits);
            assert(got == want);
            // the IPv4-mapped part stays
            assert(m.addr.bytes[10] == 0xff && m.addr.bytes[11] == 0xff);
        }
    }
    printf("OK\n");
}

static void test_ipprefix_contains(void) {
    printf("test_ipprefix_contains...");
    static const struct {
        const char* p;
        const char* q;
        bool want;
    } tests[] = {
        {"192.168.0.0/24", "192.168.0.1", true},
        {"192.168.0.0/24", "192.168.0.55", true},
        {"192.168.0.0/24", "192.168.1.1", false},
        {"192.168.0.0/24", "1.1.1.1", false},
        {"192.168.1.1/32", "192.168.1.1", true},
        {"192.168.1.1/32", "192.168.1.2", false},
        {"100.64.0.0/10", "100.64.0.0", true},
        {"100.64.0.0/10", "100.81.251.94", true},
        {"100.64.0.0/10", "100.127.255.255", true},
        {"100.64.0.0/10", "100.63.255.255", false},
        {"100.64.0.0/10", "100.128.0.0", false},
        {"2001:db8::/96", "2001:db8::aaaa:bbbb", true},
        {"2001:db8::/96", "2001:db8::1", true},
        {"2001:db8::/96", "2001:db8::1:aaaa:bbbb", false},
        {"2001:db8::/96", "2001:db9::", false},
        {"0.0.0.0/0", "192.168.0.1", true},
        {"0.0.0.0/0", "1.1.1.1", true},
        {"::/0", "::1", true},
        {"::/0", "2001:db8::1", true},
        {"2000::/3", "2001:db8::1", true},
        {"2000::/3", "fe80::1", false},
        // the host bits of p are ignored
        {"192.168.16.77/24", "192.168.16.3", true},
        // subnets
        {"192.168.0.0/16", "192.168.16.0/24", true},
        {"192.168.16.0/24", "192.168.16.0/24", true},
        {"192.168.16.0/24", "192.168.0.0/16", false},
        {"192.168.16.0/24", "192.168.17.0/24", false},
        // different families
        {"0.0.0.0/0", "2001:db8::1", false},
        {"::/0", "192.0.2.1", false},
        {"192.168.0.0/24", "::ffff:192.168.0.1", false},
        {"::ffff:192.168.0.0/120", "192.168.0.1", false},
        {"::ffff:192.168.0.0/120", "::ffff:192.168.0.1", true},
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        assert(ipprefix_contains(parse(tests[i].p), parse(tests[i].q)) == tests[i].want);
    }
    printf("OK\n");
}

static void test_ipprefix_fmt_worst(void) {
    printf("test_ipprefix_fmt_worst...");
    // the longest prefix takes exactly IPADDR_FMT_BUF_SIZE
    static const char* want = "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/128";
    assert(strlen(want) + 1 == IPADDR_FMT_BUF_SIZE);
    check_fmt(parse(want), want);
    check_fmt(parse("::ffff:255.255.255.255/128"), "::ffff:255.255.255.255/128");
    check_fmt(parse("255.255.255.255/32"), "255.255.255.255/32");
    printf("OK\n");
}

static void test_ipprefix_fmt_truncate(void) {
    printf("test_ipprefix_fmt_truncate...");
    IPPrefix p = parse("192.168.140.255/24");
    // as snprintf: the full length, the output truncated and terminated
    char buf[17];
    assert(ipprefix_fmt(buf, sizeof(buf), p) == 18);
    assert(strcmp(buf, "192.168.140.255/") == 0);
    assert(ipprefix_fmt(NULL, 0, p) == 18);
    printf("OK\n");
}

int main(void) {
    test_ipprefix_parse();
    test_ipprefix_parse_invalid();
    test_ipprefix_parse_length();
    test_ipprefix_masked();
    test_ipprefix_masked_bits();
    test_ipprefix_contains();
    test_ipprefix_fmt_worst();
    test_ipprefix_fmt_truncate();
    return 0;
}
