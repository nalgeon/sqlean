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

For example:

```sql
select encode('hello', 'base64');
-- aGVsbG8=
select decode('aGVsbG8=', 'base64');
-- hello
```

Supported algorithms:

-   `base32` (`hello` ⇆`NBSWY3DP`)
-   `base64` (`hello` ⇆ `aGVsbG8=`)
-   `base85` (`hello` ⇆ `BOu!rDZ`)
-   `hex` (`hello` ⇆ `68656c6c6f`)
-   `url` (`hello, world!` ⇆ `hello%2C%20world%21`)

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
