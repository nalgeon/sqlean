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

## [stats](https://github.com/nalgeon/sqlean/blob/main/docs/stats.md)

**Improvements and bug fixes**

All the aggregate functions now work as window functions: `stats_median(x) over (order by t rows 6 preceding)` gives a moving median.

`stats_seq` no longer hangs or overflows near the ends of the 64-bit range. `stats_seq(9223372036854775806, 9223372036854775807)` gives the two values and stops, and a step larger than the range gives just `start`.

`stats_seq` with `order by rowid desc` now returns the rows in the right order. The `step` hidden column now shows the step as given, so it is `-2` for `stats_seq(1, 10, -2)` (it was `2`).

The median and other percentiles no longer overflow when interpolating between huge values: `median` of `1e308` and `-1e308` is `0.0` (it was `+Inf`).

A variance too large to fit a real number is `+Inf`. Before, it was `-Inf`.

The documentation now gives the right default `stop` for `stats_seq`: 4294967295, not 9223372036854775807.

Fixed a crash in the variance functions when out of memory, and a buffer overflow in the percentile functions past 2^31 values.

The extension now reports a function registration failure when it loads, instead of ignoring it.

**Breaking changes**

Variance and standard deviation of no rows, or only `NULL`s, is now `NULL`, as for SQLite's built-in aggregates. Before, it was `0.0`. The sample variance and deviation of a single value is also `NULL` now, since it is undefined.

Invalid data is now skipped, as `NULL`s are:

- Text that is not a number and blobs. Before, the variance functions counted them as `0`, and the percentile functions raised an error (`1st argument to percentile() is not numeric`).
- In `stats_seq`, an argument that is not an integer (`1.5`, `'abc'` or a blob) gives no rows. Before, `1.5` was truncated to `1`, and `'abc'` and blobs were `0`. Whole reals are still accepted, so `stats_seq(1, 1e6)` works.

The percentile functions now accept numeric text (`'42'`) and infinite values. Before, both raised an error.

A `NULL` percentile in `stats_perc` skips the row. Before, it raised an error.

The error messages have changed: `percentile should be a number between 0 and 100`, `percentile should be the same for all rows`, and `table function stats_seq requires an argument`.

## [text](https://github.com/nalgeon/sqlean/blob/main/docs/text.md)

**Improvements and bug fixes**

Strings with zero bytes now work in every function. Before, most functions stopped at the first zero byte, and the `text_nocase` collation treated `'a\0b'` and `'a\0c'` as equal.

Positions, lengths and counts now accept any 64-bit integer. Before, they were truncated to 32 bits (`text_left('hello', 4294967297)` gave `h`), and some large negative values overflowed.

A result longer than SQLite's maximum string length now fails with `string or blob too big` before it is built. Before, `text_repeat('a', 2000000000)` allocated gigabytes first.

`text_like` now lets `%` match zero characters at the end of a match: `text_like('%a', 'a')` was 0 and is now 1.

`text_title` now uses Unicode title case for digraphs (`ǳ` becomes `ǲ`, not `Ǳ`). `text_upper` and `text_lower` now convert the title case digraphs `ǅ`, `ǈ`, `ǋ` and `ǲ`.

`text_index` and `text_contains` now agree on strings with invalid UTF-8.

An empty search string now gives consistent results, as in Go: `text_last_index(s, '')` is the length plus 1 (it was the length, and 0 for an empty `s`), and `text_count(s, '')` is the length plus 1 (it was 0).

`text_join('|')` and `text_concat()` now fail with `wrong number of arguments`, and the trim and pad functions with a wrong number of arguments now fail when the query is prepared, not when it runs.

Memory allocation failures are now reported as errors instead of giving `NULL` or an empty string.

The `text_casefold` function is now documented.

The extension now reports a function registration failure when it loads, instead of ignoring it.

**Breaking changes**

Invalid data now gives `NULL` instead of an error, as SQLite's built-in functions do:

- A `NULL` position, length or count gives `NULL`. Before, it was an error.
- A non-integer position, length or count gives `NULL`, including a real number such as `2.0`. Before, it was an error. Integer text such as `'2'` now works.
- A negative length in `text_substring` gives an empty string, and part 0 in `text_split` gives `NULL`. Before, both were errors.
- A blob in place of a string gives `NULL`. Before, blobs were treated as text.

Invalid UTF-8 is now kept as is. Before, character functions such as `text_reverse` replaced each malformed sequence with U+FFFD, and case functions returned a string with invalid UTF-8 unchanged. Now invalid bytes are copied unchanged, and case functions convert the valid characters around them.

The aliases no longer replace SQLite's built-in functions. `ltrim` and `rtrim` are removed (use `text_ltrim` and `text_rtrim`). `concat` and `concat_ws` are only registered on SQLite older than 3.44, and `octet_length` on SQLite older than 3.43.

## [time](https://github.com/nalgeon/sqlean/blob/main/docs/time.md)

**Improvements and bug fixes**

The `time` functions are now part of the `sqlean` bundle on 32-bit Windows. They were left out before.

The extension now reports a function registration failure when it loads, instead of ignoring it.

**Breaking changes**

Invalid data now gives `NULL` instead of an error or a made-up value, as SQLite's built-in date and math functions do. An error is still raised for a mistake in the query itself, such as an unknown field name.

## unicode

**Breaking changes**

The deprecated `unicode` extension is removed. Use [text](https://github.com/nalgeon/sqlean/blob/main/docs/text.md) instead: `text_upper`, `text_lower` and `text_casefold` change case, and the `text_nocase` collation compares strings without regard to case.

## [uuid](https://github.com/nalgeon/sqlean/blob/main/docs/uuid.md)

**Improvements and bug fixes**

`uuid7()` now stores the sub-millisecond part of the current time in the 12 bits after the version (RFC 9562, method 3), so UUIDs made within the same millisecond sort by time. Before, about half of them were out of order.

`uuid_str`, `uuid_blob` and `uuid7_timestamp_ms` now accept the URN form `urn:uuid:a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11`.

The extension now reports a function registration failure when it loads, instead of ignoring it.

**Breaking changes**

Invalid data now gives `NULL` instead of a made-up value, as SQLite's built-in functions do:

- `uuid7(seconds)` with a non-integer argument gives `NULL`. Before, the argument was ignored and the current time was used, so `uuid7(null)` and `uuid7(1.5)` returned a UUID for now. Integer text such as `'1000'` now works (before, it was ignored too).
- `uuid7(seconds)` outside the version 7 range (`0` to `281474976710`) gives `NULL`. Before, the time wrapped around: `uuid7(-1)` was `ffffffff-fc18-...` (the year 10889).

UUID strings are now parsed strictly. A string must be in one of the forms:

```text
a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11
{a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11}
urn:uuid:a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11
a0eebc999c0b4ef8bb6d6bb9bd380a11
```

Anything else gives `NULL`. Before, a hyphen was allowed before any pair of digits and braces didn't have to match, so the PostgreSQL forms `a0ee-bc99-9c0b-4ef8-bb6d-6bb9-bd38-0a11` and `{a0eebc99-9c0b4ef8-bb6d6bb9-bd380a11}` were accepted, as were `{a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11` and `a0-ee-bc-99-...`.

A UUID string followed by a NUL character and more text (`'a0eebc99-...-6bb9bd380a11' || char(0) || 'x'`) now gives `NULL`. Before, everything after the NUL was ignored.

## [vsv](https://github.com/nalgeon/sqlean/blob/main/docs/vsv.md)

The extension is rewritten as SQLite's `csv` extension plus a field separator, column types and error reporting. The CSV reader is now a port of Go's `encoding/csv`, and the rarely used options are removed.

**Improvements and bug fixes**

Columns now get their types from the declared types in `schema`, with SQLite's rules for a regular table: with `id integer`, `'22'` is the integer `22`, `'1.0'` is `1`, and `'1.5'` is the real `1.5`. Before, all values were text unless `affinity` was set, and then one affinity applied to every column, with its own rules (`affinity=integer` left `'1.0'` as text, and `numeric` depended on the platform).

`schema` no longer needs `columns`. Before, a schema without `columns` failed with `out of memory`, and a schema with more columns than `columns=N` crashed.

Malformed CSV is now an error with the line and column, as in Go: `parse error on line 1, column 4: bare " in non-quoted-field`. Before, an unterminated quoted field silently took the rest of the input, and text after a closing quote started a new row.

Blank lines are now skipped, as in Go. Before, each was a row of an empty string and `NULL`s.

An empty field is now always an empty string. Before, an empty last field at the end of the input was `NULL`, and an empty first field was a NUL character.

Fields are no longer cut at a NUL character, and a `0xff` byte at the start of a 1 KiB block no longer ends the input on platforms where `char` is signed (x86).

The BOM is now skipped on every scan. Before, the inner table of a join kept it in the first field after the first pass.

The field separator can now be any UTF-8 character (`fsep='→'`). An invalid separator is an error: before, `fsep='abc'` silently used a NUL character.

Parameter names are now case-insensitive, and boolean values can be quoted (`header='yes'`).

Errors now say what is wrong: `unknown parameter: x`, `invalid value for header: 'maybe'`, `cannot open file: 'people.csv'`. A missing file is always an error when the table is created. A field longer than SQLite's length limit is a `string or blob too big` error. Before, some failures were reported as `vtable constructor failed`, and a read error ended the table without an error.

On Windows, file names with non-ASCII characters now work.

The extension now reports a module registration failure when it loads, instead of ignoring it.

**Breaking changes**

The `rsep`, `dsep`, `nulls`, `skip`, `validatetext` and `affinity` parameters are removed, and using them is an error (`unsupported parameter: affinity`). Persistent `vsv` tables that use them fail with this error when used: they hold no data, so drop and recreate them. Instead of:

- `affinity`: declare the column types in `schema`.
- `nulls`: `nullif(c0, '')`.
- `skip=N`: `where rowid > N`.
- `dsep`: `cast(replace(c0, ',', '.') as real)`.
- `rsep`: records are always separated by newlines.

`columns` is accepted and ignored: the number of columns comes from the schema, the header, or the first record.

A bare `"` inside an unquoted field (`ab"c`) is now an error, as in Go and RFC 4180.
