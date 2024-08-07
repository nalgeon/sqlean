# crypto: Hashing, encoding and decoding in SQLite

The `sqlean-crypto` extension provides hashing, encoding and decoding functions.

[Hashing](#hashing-and-message-digest-functions) •
[Encoding/decoding](#encoding-and-decoding-functions) •
[Installation and usage](#installation-and-usage)

## Hashing and message digest functions

Each hash function expects `data` to be TEXT or BLOB. Returns a BLOB hash. Use the `hex()` or `crypto_encode()` functions to encode it into a textual representation.

[blake3](#crypto_blake3) •
[md5](#crypto_md5) •
[sha1](#crypto_sha1) •
[sha256](#crypto_sha256) •
[sha384](#crypto_sha384) •
[sha512](#crypto_sha512)

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

## Encoding and decoding functions

```text
crypto_encode(data, algo)
```

Encodes binary data into a textual representation using the specified algorithm.

```text
crypto_decode(text, algo)
```

Decodes binary data from a textual representation using the specified algorithm.

Supported algorithms: [base64](#base64) •
[base85](#base85) •
[hex](#hex) •
[url](#url)

### base32

[Base32](https://en.wikipedia.org/wiki/Base32) encoding.

```sql
select crypto_encode('hello', 'base32');
-- NBSWY3DP
select crypto_decode('NBSWY3DP', 'base32');
-- hello
```

### base64

[Base64](https://en.wikipedia.org/wiki/Base64) encoding.

```sql
select crypto_encode('hello', 'base64');
-- aGVsbG8=
select crypto_decode('aGVsbG8=', 'base64');
-- hello
```

### base85

[Base85](https://en.wikipedia.org/wiki/Ascii85) (aka Ascii85) encoding.

```sql
select crypto_encode('hello', 'base85');
-- BOu!rDZ
select crypto_decode('BOu!rDZ', 'base85');
-- hello
```

### hex

[Hexadecimal](https://en.wikipedia.org/wiki/Hexadecimal) encoding.

```sql
select crypto_encode('hello', 'hex');
-- 68656c6c6f
select crypto_decode('68656c6c6f', 'hex');
-- hello
```

### url

[URL encoding](https://en.wikipedia.org/wiki/URL_encoding)

```sql
select crypto_encode('/hello?text=(ಠ_ಠ)', 'url');
-- %2Fhello%3Ft%3D%28%E0%B2%A0_%E0%B2%A0%29
select crypto_decode('%2Fhello%3Ft%3D%28%E0%B2%A0_%E0%B2%A0%29', 'url');
-- /hello?t=(ಠ_ಠ)
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
