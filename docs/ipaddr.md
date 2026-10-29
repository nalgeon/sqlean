# ipaddr: IP address manipulation in SQLite

The `sqlean-ipaddr` extension provides functions for working with IPv4 and IPv6 addresses and subnets.

[Concepts](#concepts) •
[Addresses](#addresses) •
[Prefixes](#prefixes) •
[Acknowledgements](#acknowledgements) •
[Installation and usage](#installation-and-usage)

## Concepts

The functions work with addresses and prefixes stored as text.

An _address_ is IPv4 in dotted decimal (`192.168.16.12`) or IPv6 (`2001:db8::68`, `::ffff:192.168.16.12`). IPv6 is accepted in any case, with or without leading zeros and `::`.

A _prefix_ (also called a subnet or a CIDR block) is an address with a prefix length: the number of leading bits that identify the network (`192.168.16.0/24`, `2001:db8::/32`). The length is 0–32 for IPv4 and 0–128 for IPv6, in plain decimal (`/24`, not `/024` or `/+24`).

Every function accepts both forms. An address without a prefix length is a prefix of all its bits, so `192.168.16.12` is the same as `192.168.16.12/32`, and `2001:db8::68` is the same as `2001:db8::68/128`.

A prefix may have the bits after the prefix length (the host bits) set: `192.168.16.12/24` is an address `192.168.16.12` in the network `192.168.16.0/24`, like Postgres' `inet` type. The functions keep the host bits as is, except for `ip_network`, which clears them, and `ip_contains`, which ignores them.

**Families**. The family of an address is 4 or 6, as the IP version. An IPv4-mapped IPv6 address (`::ffff:192.168.16.12`) is IPv6, and the families never mix: an IPv4 prefix doesn't contain an IPv4-mapped address, and an IPv6 prefix doesn't contain an IPv4 address.

**Output**. The functions that return an address format it in the canonical form of [RFC 5952](https://www.rfc-editor.org/rfc/rfc5952): IPv6 in lowercase, without leading zeros, and with the longest run of two or more zero groups compressed to `::`. An IPv4-mapped address has the last 32 bits in dotted decimal (`::ffff:192.168.16.12`). Other IPv6 addresses are in hex, including the deprecated IPv4-compatible ones (`::c0a8:100c`).

**Not supported**: IPv6 zones (`fe80::1%eth0`), and the legacy IPv4 forms that some systems accept: fewer than 4 parts (`127.1`), octal or hex parts (`0x7f.0.0.1`), and parts with leading zeros (`010.0.0.1`), which are ambiguous because some parsers read them as octal.

**Sorting**. Addresses are text, so they sort as text, not as numbers: `'10.0.0.10' < '10.0.0.2'`.

### NULL and invalid input

If any function argument is `NULL`, the result will be `NULL`. This is checked before the arguments are parsed, so both `ip_contains(NULL, 'not an ip')` and `ip_contains('not an ip', NULL)` return `NULL`.

A `CHECK` constraint is satisfied if its expression is `NULL`, so `check (ip_family(addr) = 4)` won't reject a `NULL` value. Use `check (ip_family(addr) is not null)` to accept only valid addresses.

Like SQLite's own date and math functions, the extension returns `NULL` for invalid data instead of raising an error, so one bad row doesn't abort the whole query:

- A string that is not a valid address or prefix: `ip_family('192.168.16')`, `ip_masklen('192.168.16.0/33')` and `ip_family('fe80::1%eth0')` are `NULL`. There is no whitespace trimming: `ip_family(' 192.168.16.12')` is `NULL`.
- A BLOB: `ip_family(cast('192.168.16.12' as blob))` is `NULL`.
- A number is converted to text, which is not an address: `ip_family(3232239628)` is `NULL`.

## Addresses

### ip_family

```text
ip_family(ip)
```

Returns the family of an address or prefix: 4 for IPv4, 6 for IPv6.

```sql
select ip_family('192.168.16.12');
-- 4

select ip_family('2001:db8::/32');
-- 6

select ip_family('::ffff:192.168.16.12');
-- 6
```

Aliased as `ipfamily`.

### ip_host

```text
ip_host(ip)
```

Returns the address of a prefix, without the prefix length, as Postgres' `host()` does. An address is returned in the canonical form.

```sql
select ip_host('192.168.16.12/24');
-- 192.168.16.12

select ip_host('2001:DB8:0:0:0:0:0:123/64');
-- 2001:db8::123
```

Aliased as `iphost`.

## Prefixes

### ip_masklen

```text
ip_masklen(ip)
```

Returns the prefix length. For an address without one, that's all its bits: 32 for IPv4 and 128 for IPv6.

```sql
select ip_masklen('192.168.16.12/24');
-- 24

select ip_masklen('192.168.16.12');
-- 32

select ip_masklen('2001:db8::1');
-- 128
```

Aliased as `ipmasklen`.

### ip_network

```text
ip_network(ip)
```

Returns the network of a prefix: the address with the host bits cleared, and the prefix length.

```sql
select ip_network('192.168.16.12/24');
-- 192.168.16.0/24

select ip_network('2001:db8::123/64');
-- 2001:db8::/64

select ip_network('192.168.16.12');
-- 192.168.16.12/32
```

Aliased as `ipnetwork`.

### ip_contains

```text
ip_contains(subnet, ip)
```

Reports whether `subnet` contains `ip`, which is an address or another prefix. A prefix is contained if all its addresses are, so it can't be shorter than `subnet`. The host bits of `subnet` are ignored. A subnet contains itself, and an address without a prefix length contains only itself.

Returns 0 if `subnet` and `ip` are of different families (see [Concepts](#concepts)).

```sql
select ip_contains('192.168.16.0/24', '192.168.16.3');
-- 1

select ip_contains('192.168.16.0/24', '192.168.17.3');
-- 0

select ip_contains('192.168.0.0/16', '192.168.16.0/24');
-- 1

select ip_contains('192.168.16.0/24', '192.168.0.0/16');
-- 0

select ip_contains('0.0.0.0/0', '::ffff:192.168.16.3');
-- 0
```

Aliased as `ipcontains`.

## Acknowledgements

Originally contributed by [Vincent Bernat](https://github.com/vincentbernat). The parsing and formatting are based on Go's stdlib [net/netip](https://github.com/golang/go/tree/master/src/net/netip) package (BSD 3-Clause License).

## Installation and usage

SQLite command-line interface:

```
sqlite> .load ./ipaddr
sqlite> select ip_family('2001:db8::1');
```

See [How to install an extension](install.md) for usage with IDE, Python, etc.

↓ [Download](https://github.com/nalgeon/sqlean/releases/latest) the extension.

⛱ [Explore](https://github.com/nalgeon/sqlean) other extensions.

★ [Subscribe](https://antonz.org/subscribe/) to stay on top of new features.
