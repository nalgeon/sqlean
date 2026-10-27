# Upcoming release notes

Improvements, bug fixes, and breaking changes introduced by the extension refactoring, to be included in the release notes.

## crypto

Improvements and bug fixes:

- SHA-1 now gives correct hashes for inputs of 512 MiB and longer. Before, its bit counter overflowed.
- Base64 decoding no longer reads out of bounds on empty or malformed input.
- The extension now reports a function registration failure when it loads, instead of ignoring it.

Breaking changes:

Invalid data now gives `NULL` instead of an error, as SQLite's built-in functions do. An error is still raised for a mistake in the query itself, such as an unknown encoding name.

## time

Improvements and bug fixes:

- The `time` functions are now part of the `sqlean` bundle on 32-bit Windows. They were left out before.
- The extension now reports a function registration failure when it loads, instead of ignoring it.

Breaking changes:

Invalid data now gives `NULL` instead of an error or a made-up value, as SQLite's built-in date and math functions do. An error is still raised for a mistake in the query itself, such as an unknown field name.
