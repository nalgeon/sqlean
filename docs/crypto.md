# crypto: Hashing, Encoding and Decoding in SQLite

Hashing, encoding and decoding functions.

### Hashing and message digest functions

-   `md5(data)`
-   `sha1(data)`
-   `sha256(data)`
-   `sha384(data)`
-   `sha512(data)`

Each hash function expects `data` to be `TEXT` or `BLOB`. Returns a `BLOB` hash. Use the `hex()` or `encode()` functions to encode it into a textual representation.

### Encoding and decoding functions

-   `encode(data, algo)` encodes binary data into a textual representation using the specified algorithm.
-   `decode(text, algo)` decodes binary data from a textual representation using the specified algorithm.

Supported algorithms:

-   `base32`
-   `base64`
-   `base85`
-   `hex`
-   `url`

[Base32](https://en.wikipedia.org/wiki/Base32):

```sql
select encode('hello', 'base32');
-- NBSWY3DP
select decode('NBSWY3DP', 'base32');
-- hello
```

[Base64](https://en.wikipedia.org/wiki/Base64):

```sql
select encode('hello', 'base64');
-- aGVsbG8=
select decode('aGVsbG8=', 'base64');
-- hello
```

[Base85](https://en.wikipedia.org/wiki/Ascii85) (aka Ascii85):

```sql
select encode('hello', 'base85');
-- BOu!rDZ
select decode('BOu!rDZ', 'base85');
-- hello
```

[Hexadecimal](https://en.wikipedia.org/wiki/Hexadecimal):

```sql
select encode('hello', 'hex');
-- 68656c6c6f
select decode('68656c6c6f', 'hex');
-- hello
```

[URL encoding](https://en.wikipedia.org/wiki/URL_encoding):

```sql
select encode('/hello?text=(ಠ_ಠ)', 'url');
-- %2Fhello%3Ft%3D%28%E0%B2%A0_%E0%B2%A0%29
select decode('%2Fhello%3Ft%3D%28%E0%B2%A0_%E0%B2%A0%29', 'url');
-- /hello?t=(ಠ_ಠ)
```

## Installation and Usage

SQLite command-line interface:

```
sqlite> .load ./crypto
sqlite> select hex(md5('abc'));
```

See [How to Install an Extension](install.md) for usage with IDE, Python, etc.

[⬇️ Download](https://github.com/nalgeon/sqlean/releases/latest) •
[✨ Explore](https://github.com/nalgeon/sqlean) •
[🚀 Follow](https://twitter.com/ohmypy)
