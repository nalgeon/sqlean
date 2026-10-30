# Upcoming release notes

Improvements, bug fixes, and breaking changes introduced by the extension refactoring, to be included in the release notes.

## [crypto](https://github.com/nalgeon/sqlean/blob/main/docs/crypto.md)

**Improvements and bug fixes**

SHA-1 now gives correct hashes for inputs of 512 MiB and longer. Before, its bit counter overflowed.

Base64 decoding no longer reads out of bounds on empty or malformed input.

The extension now reports a function registration failure when it loads, instead of ignoring it.

**Breaking changes**

Invalid data now gives `NULL` instead of an error, as SQLite's built-in functions do. An error is still raised for a mistake in the query itself, such as an unknown encoding name.

## [define](https://github.com/nalgeon/sqlean/blob/main/docs/define.md)

**Improvements and bug fixes**

You no longer need to call `define_free()` before closing a connection. `define_free()` is kept for compatibility and does nothing.

Calling `define()` again on an existing function now replaces its body. The new body takes effect in the current connection right away, even for queries that are already prepared. Before, it failed with `database is locked`.

A function can now call itself (directly or through other functions).

`undefine()` now removes a scalar function from the current connection right away. Before, the function kept working until you reconnected.

Errors now carry the actual message, such as `no such column: x`, `integer overflow`, or `function already exists: abs`. Before, most errors were reported as a bare `SQL logic error`.

Fixed a memory leak and a possible crash when a table-valued function's body fails to compile (for example, when a table it uses was dropped).

The documentation now has a Security section that explains how defined functions can bypass SQLite's protection against malicious schemas, and how to use `trusted_schema` to stay safe.

**Breaking changes**

A function body must now be a single statement. `define('f', '1; select 2')` and `create virtual table ... using define((select 1; select 2))` fail with `function body must be a single statement`. Before, everything after the first statement was silently ignored. Trailing semicolons and comments are still allowed, and functions with several statements saved by older versions still load (only the first statement is used).

The undocumented `define_cache()` debugging function is removed.

## [fileio](https://github.com/nalgeon/sqlean/blob/main/docs/fileio.md)

**Improvements and bug fixes**

Fixed crashes and memory errors with `NULL` arguments, such as `fileio_append(path, null)`.

`fileio_scan` now works with `where`, `limit` and `rowid` filters, keeps NUL characters in lines, and removes only one line ending from each line (before, a line like `a\r\r\n` lost both `\r`s).

`fileio_ls` no longer doubles the slash in names (`dir//a.txt`), and lists the rest of the tree when a directory can't be read instead of failing the whole listing.

`fileio_read` now supports offsets and limits beyond 2 GiB.

`fileio_append` writes blobs as is, instead of cutting them at the first NUL byte.

`fileio_mode` now shows FIFOs, devices and sockets, and can be used in indexes and in views with an untrusted schema.

On Windows, all file operations now use the Unicode API, so non-ASCII paths are handled the same way everywhere.

The extension now reports a function registration failure when it loads, instead of ignoring it.

**Breaking changes**

Invalid data now gives `NULL` (or no rows) instead of an error or a made-up value, as SQLite's built-in functions do. This includes a `NULL` in any argument (a `NULL` `mtime` or `perm` used to mean 0), a negative `offset` or `limit`, an out-of-range `perm` or `mode`, a non-integer number, and a path that is not text.

A failure to change the file system is now always an error, and the error message includes the path. Before, `fileio_write` with two arguments returned `NULL`, and `fileio_symlink` silently ignored failures.

`fileio_scan` returns no rows for a file that doesn't exist or can't be read, instead of an error. `fileio_read` returns `NULL` for a directory.

Extra arguments are now an error: `fileio_read` used to ignore them.

## [fuzzy](https://github.com/nalgeon/sqlean/blob/main/docs/fuzzy.md)

**Improvements and bug fixes**

`fuzzy_damlev`, `fuzzy_hamming`, `fuzzy_jarowin`, `fuzzy_leven` and `fuzzy_osadist` now support Unicode strings and count characters instead of bytes: `fuzzy_leven('café', 'cafe')` is `1`. Before, they raised an error for non-ASCII strings.

`fuzzy_osadist` now counts a transposition of two adjacent characters as one edit, as the Optimal String Alignment distance should. Before, it cost two edits: `fuzzy_osadist('ab', 'ba')` was `2`, now it's `1`.

`fuzzy_jarowin` is now symmetric. Before, it used the wrong match window when the first string was longer, so `fuzzy_jarowin('dicksonx', 'dixon')` and `fuzzy_jarowin('dixon', 'dicksonx')` gave different results.

`fuzzy_soundex` and `fuzzy_rsoundex` now skip leading non-letters: `fuzzy_soundex('1abc')` is `A120` (was `1120`). `fuzzy_soundex` now treats an uppercase `H` or `W` between two letters with the same code the same way as a lowercase one.

`fuzzy_phonetic` now ignores case in all its rules, as it does for single letters: `fuzzy_phonetic('KNIGHT')` is `MACD` (was `CMACD`), the same as `fuzzy_phonetic('knight')`. Before, the rules for an initial `KN`/`GN`, `WR`, `TCH` and `DGE` only applied to lowercase letters.

A NUL character inside a string is now an ordinary character. Before, most functions ignored everything after it.

Fixed crashes when out of memory: most functions didn't check their allocations.

The extension now reports a function registration failure when it loads, instead of ignoring it.

**Breaking changes**

Invalid data now gives `NULL` instead of an error or a made-up value, as SQLite's built-in functions do:

- A `NULL` argument to a distance function gives `NULL` (was an error). `fuzzy_translit(NULL)` and `fuzzy_script(NULL)` give `NULL` (were `''` and `999`).
- A non-ASCII string passed to `fuzzy_editdist` or a phonetic code function gives `NULL` (was an error, or a garbled code for `fuzzy_phonetic`).
- A BLOB argument gives `NULL`. Before, it was treated as text.
- `fuzzy_hamming` of strings with different lengths gives `NULL` (was `-1`).

`fuzzy_caver`, `fuzzy_soundex` and `fuzzy_rsoundex` of a string without letters give an empty string: `fuzzy_caver('123')` is `''` (was `1111111111`), `fuzzy_soundex('123')` is `''` (was `1000`).

`fuzzy_script` no longer counts punctuation and symbols as Latin: `fuzzy_script('«»')` is `999` (was `215`), and `fuzzy_script('«привет»')` is `220` (was `998`).

`fuzzy_translit` replaces invalid UTF-8 bytes with `?`. Before, it cut the result at the first one.

## [ipaddr](https://github.com/nalgeon/sqlean/blob/main/docs/ipaddr.md)

**Improvements and bug fixes**

The functions are renamed to `ip_family`, `ip_host`, `ip_masklen`, `ip_network` and `ip_contains`. The old names (`ipfamily`, `iphost`, `ipmasklen`, `ipnetwork`, `ipcontains`) are kept as aliases.

`ipaddr` is now available on Windows, including in the `sqlean` bundle. The extension no longer depends on the system's socket library.

Addresses are parsed and formatted the same way on every platform. Before, the results depended on the OS: macOS accepted IPv4 parts with leading zeros (`01.2.3.4`) and IPv6 zones (`fe80::1%eth0`), while Linux and Windows rejected them.

Fixed a crash when out of memory.

The extension now reports a function registration failure when it loads, instead of ignoring it.

**Breaking changes**

Invalid input is now `NULL` in more cases:

- A prefix length must be plain decimal: `/+24`, `/-0`, `/ 24` and `/024` are invalid. Before, they were read as 24, 0, 24 and 24.
- IPv4 parts with leading zeros (`010.0.0.1`) and IPv6 zones (`fe80::1%eth0`) are invalid on every platform, including macOS.
- A NUL character makes the value invalid. Before, the value was cut at the NUL: `ipfamily('192.168.16.12' || char(0) || 'x')` was `4`.
- A BLOB argument is `NULL`. Before, it was read as text.

IPv6 addresses with an IPv4 part are formatted as dotted decimal only if they are IPv4-mapped (`::ffff:192.168.16.12`), as RFC 5952 recommends. Others are in hex: `ip_host('::192.168.16.12')` is now `::c0a8:100c`, not `::192.168.16.12`.

## [regexp](https://github.com/nalgeon/sqlean/blob/main/docs/regexp.md)

**Improvements and bug fixes**

`regexp_replace` no longer returns the source unchanged when the result is more than 1 KiB longer than the source. `regexp_replace('a', 'a', printf('%.*c', 2000, 'b'))` now gives the 2000 `b`s.

A NUL character is matched like any other. Before, the source and the pattern were cut at the first NUL: `regexp_like('a' || char(0) || 'b', 'b')` was `0`, and `regexp_replace` lost everything after the NUL.

In `regexp_replace`, a group that didn't take part in the match is replaced with an empty string. Before, the whole call returned the source unchanged: `regexp_replace('ac', 'a(b)?c', '[$1]')` was `ac`, now it's `[]`.

Fixed undefined behavior in `regexp_capture` when the group didn't take part in the match but a later group did, as in `regexp_capture('ac', 'a(b)?(c)', 1)`. It now returns `NULL`.

A query with a constant pattern compiles it once, even when most rows don't match. Before, the pattern was recompiled after every row that didn't match.

Fixed crashes when out of memory.

The functions can now be used in views and triggers with an untrusted schema.

The extension now reports a function registration failure when it loads, instead of ignoring it.

**Breaking changes**

`NULL` in any argument gives `NULL`, as SQLite's built-in functions do. Before, `null regexp 'a'` and `regexp_like(null, 'a')` were `0`, and `regexp_replace('abc', 'b', null)` returned the source.

Invalid data now gives `NULL` instead of being treated as no match:

- A BLOB argument is `NULL`. Before, it was read as text. Cast it to text to keep the old behavior.
- Text that is not valid UTF-8, in the source or in the replacement, is `NULL`. Before, `regexp_like` gave `0` and `regexp_replace` returned the source.

A mistake in the query now raises an error instead of being treated as no match:

- A group number in `regexp_capture` that is negative, not an integer, or larger than the number of groups in the pattern: `regexp_capture('abc', 'b', 1)` fails with `invalid group: 1`. Before, it was `NULL`.
- An invalid replacement, or one that refers to a group the pattern doesn't have: `regexp_replace('abc', 'b', '$1')` fails with `invalid replacement: unknown substring at offset 2`. Before, it returned the source.

A match that hits PCRE2's backtracking limit raises an error (`regexp: match limit exceeded`). Before, it was treated as no match, so the result silently depended on the input size.

`regexp_replace` always returns text. Before, it returned the source as is when nothing matched, so `regexp_replace(123, 'z', 'x')` was the integer `123`, and is now the text `'123'`.

## [time](https://github.com/nalgeon/sqlean/blob/main/docs/time.md)

**Improvements and bug fixes**

The `time` functions are now part of the `sqlean` bundle on 32-bit Windows. They were left out before.

The extension now reports a function registration failure when it loads, instead of ignoring it.

**Breaking changes**

Invalid data now gives `NULL` instead of an error or a made-up value, as SQLite's built-in date and math functions do. An error is still raised for a mistake in the query itself, such as an unknown field name.
