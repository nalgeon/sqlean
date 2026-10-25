# Breaking changes

Changes in observable behavior introduced by the extension refactoring, to be included in the release notes.

## time

Invalid data now gives `NULL` instead of an error or a made-up value, as SQLite's built-in date and math functions do. An error is still raised for a mistake in the query itself, such as an unknown field name.

1. **Wrong-type and wrong-size time arguments return `NULL`.** Before, they raised errors such as `1st parameter: should be a time blob`. Now `time_get_year('2011-11-18')`, `time_to_unix(x'00')` and similar calls return `NULL`.

2. **Integer arguments follow SQLite's conversion rules.** Numeric text is converted, so `time_date('2011', '11', '18')` and `time_add(t, '1000000000')` work. Reals and non-numeric text (`time_add(t, 1.5)`, `time_unix('x')`) return `NULL`. Before, anything other than an integer raised an error.

3. **32-bit integer arguments out of range return `NULL`.** This covers the `time_date` parts, the years, months and days of `time_add_date`, and the `offset_sec` of the `time_fmt_*` functions. Before, they were silently truncated to 32 bits, so `time_date(4294969307, 11, 18)` returned a time in year 2011.

4. **`time_parse` returns `NULL` for strings it can't parse.** Before, it returned the zero time (`0001-01-01 00:00:00 UTC`), which couldn't be told apart from a valid result.

5. **`time_parse` returns `NULL` for a BLOB argument.** Before, the BLOB's bytes were parsed as text.

6. **Corrupt time BLOBs return `NULL`.** A 13-byte BLOB with an unknown version byte or a nanosecond field over 999999999 used to decode as the zero time.

7. **Field-name errors changed.** An unknown field in `time_get`, `time_trunc`, `date_part` or `date_trunc` now raises `unknown field: <name>`. A BLOB field name raises `field name should be text`. A number used as a field name (`date_part(42, t)`) raises `unknown field: 42`. Applications that match the old error messages need updating.

8. **`time_trunc` treats numeric text as a duration.** `time_trunc(t, '3600000000000')` now truncates to an hour. Before, the string was treated as a field name and raised an error. A real or BLOB as the second argument returns `NULL`.

### Not breaking

- The `time` functions are now part of the `sqlean` bundle on 32-bit Windows. They were left out before.
- The extension now reports a function registration failure when it loads, instead of ignoring it.

## crypto

Invalid data now gives `NULL` instead of an error, as SQLite's built-in functions do. An error is still raised for a mistake in the query itself, such as an unknown encoding name.

1. **`crypto_decode` returns `NULL` for invalid data.** Before, it raised `invalid input string`. Now `crypto_decode('xyz', 'hex')` and similar calls return `NULL`.

2. **Encoding names must match exactly.** Before, the name was matched as a prefix of `base32`, so `'b'`, `'base'` and even `''` selected base32. Now any name other than `base32`, `base64`, `base85`, `hex` or `url` raises `unknown encoding: <name>`. Names are case-sensitive.

3. **A BLOB encoding name is an error.** `crypto_encode('hello', x'626173653634')` raises `encoding name should be text`. Before, the BLOB's bytes were read as the name.

4. **A `NULL` encoding name returns `NULL`.** Before, it was undefined behavior.

5. **Base32 decoding is strict.** Lowercase letters and characters outside the alphabet are invalid, so `crypto_decode('mzxw6===', 'base32')` and `crypto_decode('MZ XW6===', 'base32')` return `NULL`. Before, such characters were silently skipped.

6. **Base64 decoding rejects misplaced padding.** `=` is only valid as one or two trailing characters, so `'YQ=A'`, `'===='` and `'YQ==YQ=='` return `NULL`. Before, they decoded into made-up bytes.

7. **URL encoding escapes NUL bytes.** `crypto_encode(x'610062', 'url')` returns `'a%00b'`. Before, the result was truncated at the NUL byte (`'a'`).

### Not breaking

- SHA-1 now gives correct hashes for inputs of 512 MiB and longer. Before, its bit counter overflowed.
- Base64 decoding no longer reads out of bounds on empty or malformed input.
- The extension now reports a function registration failure when it loads, instead of ignoring it.
- The internal xxHash functions are no longer exported from the extension library, so they can't clash with another copy of xxHash loaded into the same process.
