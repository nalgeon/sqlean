# crypto: Hashing, encoding and decoding in SQLite

The `sqlean-crypto` extension provides hashing, encoding and decoding functions.

[Concepts](#concepts) •
[Hashing](#hashing-and-message-digest-functions) •
[Hash aggregates](#hash-aggregates) •
[Encoding/decoding](#encoding-and-decoding-functions) •
[Installation and usage](#installation-and-usage)

## Concepts

The hash and encode functions work on bytes. A BLOB argument is used as is. Any other value is converted to text first (UTF-8), so `crypto_md5(42)` is the same as `crypto_md5('42')`.

Hash functions and `crypto_decode` return a BLOB. Use `hex()` or `crypto_encode()` to get a textual representation of it, or `cast(... as text)` if the bytes are a string. `crypto_encode` returns TEXT.

### NULL and invalid input

If any function argument is `NULL`, the result will be `NULL`, including a `NULL` encoding name: `crypto_encode('hello', NULL)` is `NULL`.

Like SQLite's own functions, `crypto_decode` returns `NULL` for data it can't decode, instead of raising an error, so one bad row doesn't abort the whole query:

```sql
select crypto_decode('xyz', 'hex');
-- NULL
```

Decoding an empty string gives an empty BLOB, not `NULL`.

An unknown encoding name is an error (`unknown encoding: base63`), because it's a mistake in the query rather than in the data. Encoding names are case-sensitive and must be TEXT.

## Hashing and message digest functions

Each hash function returns a BLOB hash of `data`. The xxHash functions use a seed of 0 and return the hash in big-endian byte order (the canonical xxHash representation).

[blake3](#crypto_blake3) •
[md5](#crypto_md5) •
[sha1](#crypto_sha1) •
[sha256](#crypto_sha256) •
[sha384](#crypto_sha384) •
[sha512](#crypto_sha512) •
[xxh32](#crypto_xxh32) •
[xxh64](#crypto_xxh64) •
[xxh3_64](#crypto_xxh3_64) •
[xxh3_128](#crypto_xxh3_128)

### crypto_blake3

```text
crypto_blake3(data)
```

Returns a BLAKE3 hash of the data as a blob.

```sql
select hex(crypto_blake3('abc'));
-- 6437B3AC38465133FFB63B75273A8DB548C558465D79DB03FD359C6CD5BD9D85
```

### crypto_md5

```text
crypto_md5(data)
```

Returns an MD5 hash of the data as a blob.

```sql
select hex(crypto_md5('abc'));
-- 900150983CD24FB0D6963F7D28E17F72
```

### crypto_sha1

```text
crypto_sha1(data)
```

Returns a SHA-1 hash of the data as a blob.

```sql
select hex(crypto_sha1('abc'));
-- A9993E364706816ABA3E25717850C26C9CD0D89D
```

### crypto_sha256

```text
crypto_sha256(data)
```

Returns a SHA2-256 hash of the data as a blob.

```sql
select hex(crypto_sha256('abc'));
-- BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD
```

### crypto_sha384

```text
crypto_sha384(data)
```

Returns a SHA2-384 hash of the data as a blob.

```sql
select hex(crypto_sha384('abc'));
-- CB00753F45A35E8BB5A03D699AC65007272C32AB0EDED1631A8B605A43FF5BED8086072BA1E7CC2358BAECA134C825A7
```

### crypto_sha512

```text
crypto_sha512(data)
```

Returns a SHA2-512 hash of the data as a blob.

```sql
select hex(crypto_sha512('abc'));
-- DDAF35A193617ABACC417349AE20413112E6FA4E89A97EA20A9EEEE64B55D39A2192992A274FC1A836BA3C23A3FEEBBD454D4423643CE80E2A9AC94FA54CA49F
```

### crypto_xxh32

```text
crypto_xxh32(data)
```

Returns a XXH32 hash of the data as a blob.

```sql
select hex(crypto_xxh32('abc'));
-- 32D153FF
```

### crypto_xxh64

```text
crypto_xxh64(data)
```

Returns a XXH64 hash of the data as a blob.

```sql
select hex(crypto_xxh64('abc'));
-- 44BC2CF5AD770999
```

### crypto_xxh3_64

```text
crypto_xxh3_64(data)
```

Returns a XXH3_64 hash of the data as a blob.

```sql
select hex(crypto_xxh3_64('abc'));
-- 78AF5F94892F3950
```

### crypto_xxh3_128

```text
crypto_xxh3_128(data)
```

Returns a XXH3_128 hash of the data as a blob.

```sql
select hex(crypto_xxh3_128('abc'));
-- 06B05AB6733A618578AF5F94892F3950
```

## Hash aggregates

```text
crypto_blake3_agg(data)
crypto_md5_agg(data)
crypto_sha1_agg(data)
crypto_sha256_agg(data)
crypto_sha384_agg(data)
crypto_sha512_agg(data)
crypto_xxh32_agg(data)
crypto_xxh64_agg(data)
crypto_xxh3_64_agg(data)
crypto_xxh3_128_agg(data)
```

Aggregate versions of the hash functions. Each one returns the hash of all the `data` values in the group, concatenated: `crypto_md5_agg` over `'a'`, `'b'` and `'c'` is the same as `crypto_md5('abc')`. The values are hashed as they come, without building the concatenation in memory, so the aggregates work for tables of any size.

The hash depends on the order of the values, which SQL doesn't define. To get a stable result, order the values inside the call (SQLite 3.44+):

```sql
with t(id, data) as (values (1, 'a'), (2, 'b'), (3, 'c'))
select hex(crypto_md5_agg(data order by id)) from t;
-- 900150983CD24FB0D6963F7D28E17F72
```

The values are converted to bytes the same way as in the scalar functions. `NULL` values are skipped. With no rows, or only `NULL` values, the result is `NULL`.

The values are concatenated without separators, so `'ab', 'c'` and `'a', 'bc'` give the same hash. If that matters, add a separator or the length to each value yourself, as in `crypto_sha256_agg(length(data) || ':' || data order by id)`.

The aggregates can't be used as window functions: hashing has no way to remove a value from the hash when it leaves the window.

## Encoding and decoding functions

```text
crypto_encode(data, encoding)
```

Encodes binary data into a textual representation using the specified encoding.

```text
crypto_decode(text, encoding)
```

Decodes binary data from a textual representation using the specified encoding. Returns `NULL` if the text is not valid for the encoding.

Supported encodings: [base32](#base32) •
[base64](#base64) •
[base85](#base85) •
[hex](#hex) •
[url](#url)

### base32

[Base32](https://en.wikipedia.org/wiki/Base32) encoding (RFC 4648 alphabet).

Decoding expects uppercase letters `A-Z` and digits `2-7`. Trailing `=` padding is optional. Lowercase letters, whitespace and other characters are invalid.

```sql
select crypto_encode('hello', 'base32');
-- NBSWY3DP
select crypto_decode('NBSWY3DP', 'base32');
-- hello
```

### base64

[Base64](https://en.wikipedia.org/wiki/Base64) encoding (RFC 4648 standard alphabet).

Decoding expects the length to be a multiple of 4, with up to two `=` padding characters at the end. Whitespace, `=` elsewhere and characters outside the alphabet are invalid.

```sql
select crypto_encode('hello', 'base64');
-- aGVsbG8=
select crypto_decode('aGVsbG8=', 'base64');
-- hello
```

### base85

[Base85](https://en.wikipedia.org/wiki/Ascii85) (aka Ascii85) encoding, without the `<~` and `~>` delimiters.

An all-zero group of 4 bytes is encoded as `z`.

```sql
select crypto_encode('hello', 'base85');
-- BOu!rDZ
select crypto_decode('BOu!rDZ', 'base85');
-- hello
```

### hex

[Hexadecimal](https://en.wikipedia.org/wiki/Hexadecimal) encoding.

Encoding gives lowercase digits. Decoding accepts both cases, and expects an even number of digits.

```sql
select crypto_encode('hello', 'hex');
-- 68656c6c6f
select crypto_decode('68656c6c6f', 'hex');
-- hello
```

### url

[URL encoding](https://en.wikipedia.org/wiki/URL_encoding) (percent-encoding).

Encoding keeps the RFC 3986 unreserved characters (`A-Z`, `a-z`, `0-9`, `-`, `_`, `.`, `~`) as is, and escapes every other byte as `%XX`.

Decoding expects `%` to be followed by two hex digits. A `+` decodes into a space, as in HTML form data. Other characters are kept as is.

```sql
select crypto_encode('/hello?text=(ಠ_ಠ)', 'url');
-- %2Fhello%3Ftext%3D%28%E0%B2%A0_%E0%B2%A0%29
select crypto_decode('%2Fhello%3Ftext%3D%28%E0%B2%A0_%E0%B2%A0%29', 'url');
-- /hello?text=(ಠ_ಠ)
```

## Installation and usage

SQLite command-line interface:

```
sqlite> .load ./crypto
sqlite> select hex(crypto_md5('abc'));
```

See [How to install an extension](install.md) for usage with IDE, Python, etc.

↓ [Download](https://github.com/nalgeon/sqlean/releases/latest) the extension.

⛱ [Explore](https://github.com/nalgeon/sqlean) other extensions.

★ [Subscribe](https://antonz.org/subscribe/) to stay on top of new features.
