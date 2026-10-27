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

## [time](https://github.com/nalgeon/sqlean/blob/main/docs/time.md)

**Improvements and bug fixes**

The `time` functions are now part of the `sqlean` bundle on 32-bit Windows. They were left out before.

The extension now reports a function registration failure when it loads, instead of ignoring it.

**Breaking changes**

Invalid data now gives `NULL` instead of an error or a made-up value, as SQLite's built-in date and math functions do. An error is still raised for a mistake in the query itself, such as an unknown field name.
