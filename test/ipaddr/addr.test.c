// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// IP address tests. Most cases come from Go's net/netip tests.

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

typedef struct {
    const char* in;
    IPFamily family;
    const char* want;  // the formatted address
} ParseTest;

// parse parses a NUL-terminated s, which must be valid.
static IPAddr parse(const char* s) {
    IPAddr a;
    bool ok = ipaddr_parse(s, strlen(s), &a);
    assert(ok);
    return a;
}

// check_fmt formats a into a buffer of exactly the needed size
// and compares it to want.
static void check_fmt(IPAddr a, const char* want) {
    size_t size = strlen(want) + 1;
    char* buf = malloc(size);
    assert(buf != NULL);
    size_t len = ipaddr_fmt(buf, size, a);
    assert(len == strlen(want));
    assert(strcmp(buf, want) == 0);
    free(buf);
}

static void test_ipaddr_parse(void) {
    printf("test_ipaddr_parse...");
    static const ParseTest tests[] = {
        // IPv4
        {"0.0.0.0", IPADDR_V4, "0.0.0.0"},
        {"192.168.140.255", IPADDR_V4, "192.168.140.255"},
        {"255.255.255.255", IPADDR_V4, "255.255.255.255"},
        // IPv6
        {"::", IPADDR_V6, "::"},
        {"::1", IPADDR_V6, "::1"},
        {"fd7a:115c:a1e0:ab12:4843:cd96:626b:430b", IPADDR_V6,
         "fd7a:115c:a1e0:ab12:4843:cd96:626b:430b"},
        {"fd7a:115c::626b:430b", IPADDR_V6, "fd7a:115c::626b:430b"},
        {"fd7a:115c:a1e0:ab12:4843:cd96::", IPADDR_V6, "fd7a:115c:a1e0:ab12:4843:cd96::"},
        {"fd7a:115c:a1e0:ab12:4843:cd96:626b::", IPADDR_V6, "fd7a:115c:a1e0:ab12:4843:cd96:626b:0"},
        {"fd7a:115c:a1e0::4843:cd96:626b:430b", IPADDR_V6, "fd7a:115c:a1e0:0:4843:cd96:626b:430b"},
        {"FD9E:1A04:F01D::1", IPADDR_V6, "fd9e:1a04:f01d::1"},
        {"0:0:0:0:0:0:0:0", IPADDR_V6, "::"},
        {"0000:0000::0001", IPADDR_V6, "::1"},
        // embedded IPv4
        {"::ffff:192.168.140.255", IPADDR_V6, "::ffff:192.168.140.255"},
        {"::ffff:c0a8:8cff", IPADDR_V6, "::ffff:192.168.140.255"},
        {"1:2::ffff:192.168.140.255", IPADDR_V6, "1:2::ffff:c0a8:8cff"},
        {"1:2:3:4:5:6:1.2.3.4", IPADDR_V6, "1:2:3:4:5:6:102:304"},
        {"::1.2.3.4", IPADDR_V6, "::102:304"},
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        IPAddr a = parse(tests[i].in);
        assert(a.family == tests[i].family);
        check_fmt(a, tests[i].want);
    }
    printf("OK\n");
}

static void test_ipaddr_parse_bytes(void) {
    printf("test_ipaddr_parse_bytes...");
    // IPv4 is stored as IPv4-mapped, and differs from it only by the family
    static const uint8_t mapped[16] = {[10] = 0xff, [11] = 0xff, 0xc0, 0xa8, 0x8c, 0xff};
    IPAddr v4 = parse("192.168.140.255");
    IPAddr v6 = parse("::ffff:192.168.140.255");
    assert(v4.family == IPADDR_V4 && v6.family == IPADDR_V6);
    assert(memcmp(v4.bytes, mapped, 16) == 0);
    assert(memcmp(v6.bytes, mapped, 16) == 0);

    static const uint8_t want[16] = {0xfd, 0x7a, 0x11, 0x5c, [12] = 0x62, 0x6b, 0x43, 0x0b};
    IPAddr a = parse("fd7a:115c::626b:430b");
    assert(memcmp(a.bytes, want, 16) == 0);
    printf("OK\n");
}

static void test_ipaddr_parse_invalid(void) {
    printf("test_ipaddr_parse_invalid...");
    static const char* tests[] = {
        "",
        "bad",
        "1234",
        ".1.2.3",
        "1.2.3.",
        "1..2.3",
        "1.2.3.4.5",
        // leading zeros, octal and hex
        "010.000.015.001",
        "000001.00000002.00000003.000000004",
        "0300.0250.0214.0377",
        "0xc0.0xa8.0x8c.0xff",
        "::ffff:1.2.03.4",
        // class A and B forms
        "192.168.12345",
        "127.0.1",
        "192.1234567",
        "127.1",
        "192.168.300.1",
        "192.168.0.1.5.6",
        "1:2:3:4:5:6:7",
        "1:2:3:4:5:6:7:8:9",
        "1:2:3:4::5:6:7:8",
        "fe801::1",
        "fe80:tail:scal:e::",
        "ffff:ffff:ffff:ffff:ffff:ffff:ffff:192.168.140.255",
        "ffff::ffff:ffff:ffff:ffff:ffff:ffff:192.168.140.255",
        "::ffff:192.168.140.bad",
        "::ffff:1.2.3.z",
        "1:2:3:4:1.2.3.4",
        "fe80::1::1",
        "fe80:1?:1",
        "fe80:",
        ":",
        ":::",
        "1:",
        ":1",
        "0:0:0:0:0:ffff:0:00000",
        "0:0:0:0:00000:ffff:127.1.2.3",
        " 1.2.3.4",
        "1.2.3.4 ",
        // zones are not supported
        "1.2.3.4%eth0",
        "fe80::1%",
        "fe80::1%eth0",
        "fe80::1%1",
        "::ffff:192.168.140.255%eth1",
        // a prefix is not an address
        "192.168.0.0/24",
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        IPAddr a;
        assert(!ipaddr_parse(tests[i], strlen(tests[i]), &a));
    }
    printf("OK\n");
}

static void test_ipaddr_parse_length(void) {
    printf("test_ipaddr_parse_length...");
    IPAddr a;
    // the parser reads only n bytes, so the string doesn't have to end there
    assert(ipaddr_parse("1.2.3.4/8", 7, &a));
    check_fmt(a, "1.2.3.4");
    assert(ipaddr_parse("::1:2", 3, &a));
    check_fmt(a, "::1");
    assert(!ipaddr_parse("1.2.3.4", 0, &a));
    // a NUL is not valid
    assert(!ipaddr_parse(S("1.2.3.4\0"), &a));
    assert(!ipaddr_parse(S("1.2\0.3.4"), &a));
    assert(!ipaddr_parse(S("::1\0"), &a));
    assert(!ipaddr_parse(S("\0::1"), &a));
    printf("OK\n");
}

static void test_ipaddr_fmt(void) {
    printf("test_ipaddr_fmt...");
    // RFC 5952 section 4
    static const char* tests[][2] = {
        // lowercase, no leading zeros
        {"2001:0DB8:0000:0000:0000:0000:0000:0001", "2001:db8::1"},
        {"2001:db8:aaaa:bbbb:cccc:dddd:eeee:0001", "2001:db8:aaaa:bbbb:cccc:dddd:eeee:1"},
        // the longest run of zero groups
        {"2001:db8:0:0:1:0:0:0", "2001:db8:0:0:1::"},
        {"2001:0:0:1:0:0:0:1", "2001:0:0:1::1"},
        // the first of equal runs
        {"2001:db8:0:0:1:0:0:1", "2001:db8::1:0:0:1"},
        // a single zero group is not compressed
        {"2001:db8:0:1:1:1:1:1", "2001:db8:0:1:1:1:1:1"},
        {"0:1:2:3:4:5:6:7", "0:1:2:3:4:5:6:7"},
        {"1:2:3:4:5:6:7:0", "1:2:3:4:5:6:7:0"},
        // runs at the ends
        {"0:0:1:2:3:4:5:6", "::1:2:3:4:5:6"},
        {"1:2:3:4:5:6:0:0", "1:2:3:4:5:6::"},
        // only IPv4-mapped addresses are dotted
        {"::ffff:0.0.0.0", "::ffff:0.0.0.0"},
        {"::192.0.2.1", "::c000:201"},
        {"::fffe:192.0.2.1", "::fffe:c000:201"},
        {"0:0:0:0:1:ffff:192.0.2.1", "::1:ffff:c000:201"},
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        check_fmt(parse(tests[i][0]), tests[i][1]);
    }
    printf("OK\n");
}

static void test_ipaddr_fmt_worst(void) {
    printf("test_ipaddr_fmt_worst...");
    // the longest address of each kind fits in IPADDR_FMT_BUF_SIZE
    // with room for "/128"
    static const char* tests[] = {
        "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",
        "::ffff:255.255.255.255",
        "255.255.255.255",
    };
    for (size_t i = 0; i < LEN(tests); i++) {
        assert(strlen(tests[i]) + 4 + 1 <= IPADDR_FMT_BUF_SIZE);
        check_fmt(parse(tests[i]), tests[i]);
    }
    assert(strlen(tests[0]) + 4 + 1 == IPADDR_FMT_BUF_SIZE);
    printf("OK\n");
}

static void test_ipaddr_fmt_truncate(void) {
    printf("test_ipaddr_fmt_truncate...");
    IPAddr a = parse("192.168.140.255");
    // as snprintf: the full length, the output truncated and terminated
    char buf[8];
    assert(ipaddr_fmt(buf, sizeof(buf), a) == 15);
    assert(strcmp(buf, "192.168") == 0);
    assert(ipaddr_fmt(NULL, 0, a) == 15);
    printf("OK\n");
}

int main(void) {
    test_ipaddr_parse();
    test_ipaddr_parse_bytes();
    test_ipaddr_parse_invalid();
    test_ipaddr_parse_length();
    test_ipaddr_fmt();
    test_ipaddr_fmt_worst();
    test_ipaddr_fmt_truncate();
    return 0;
}
