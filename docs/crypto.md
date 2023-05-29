# crypto: Hash Functions in SQLite

Hash functions and encode/decode functions.

### Secure hash and message digest functions

-   `md5(data)`
-   `sha1(data)`
-   `sha256(data)`
-   `sha384(data)`
-   `sha512(data)`

Each hash function expects `data` to be `TEXT` or `BLOB`. Returns a `BLOB` hash. Use the `hex()` or `encode()` functions to encode it into a textual representation.

### Encode/decode functions

-   `encode(data, algo)` encodes binary data into a textual representation using the specified algorithm.
-   `decode(text, algo)` decodes binary data from a textual representation using the specified algorithm.

For example:

```sql
select encode('hello', 'base64');
-- aGVsbG8=
select decode('aGVsbG8=', 'base64');
-- hello
```

Supported algorithms: `base32`, `base64`, `hex`

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
