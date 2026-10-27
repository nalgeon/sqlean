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

## [time](https://github.com/nalgeon/sqlean/blob/main/docs/time.md)

**Improvements and bug fixes**

The `time` functions are now part of the `sqlean` bundle on 32-bit Windows. They were left out before.

The extension now reports a function registration failure when it loads, instead of ignoring it.

**Breaking changes**

Invalid data now gives `NULL` instead of an error or a made-up value, as SQLite's built-in date and math functions do. An error is still raised for a mistake in the query itself, such as an unknown field name.
