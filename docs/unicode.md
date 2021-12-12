# unicode: Unicode support for SQLite

This extension implements true UNICODE functionality for SQLite in regards of case-insensitive comparison of unicode data and SQLite. It uses UNICODE mapping tables to provide the following to SQLite:

-   `upper()`, `lower()`, `title()`, `fold()` functions to normalize strings for comparison by case folding.
-   `unaccent()` function to normalize strings for comparison by removing accents.
-   `LIKE` operator that uses casefolding to provide case-independent matching.

Overrides the default NOCASE case-insensitive collation sequence to support UTF-8 characters.

Has no external dependencies (like libicu). Adapted from [sqlite3_unicode](https://github.com/Zensey/sqlite3_unicode).

⚠️ This extension cannot be loaded via `load_extension()` SQL function due to SQLite restrictions. Quoting the [docs](https://sqlite.org/lang_corefunc.html#load_extension):

> The load_extension() function will fail if the extension attempts to modify or delete an SQL function or collating sequence. The extension can add new functions or collating sequences, but cannot modify or delete existing functions or collating sequences because those functions and/or collating sequences might be used elsewhere in the currently running SQL statement. To load an extension that changes or deletes functions or collating sequences, use the `sqlite3_load_extension()` C-language API.

It works via SQLite CLI `.load` command, though.

## Usage

Before:

```
sqlite> select upper('hello');
HELLO
sqlite> select upper('привет');
привет
```

After:

```
sqlite> .load ./unicode
sqlite> select upper('hello');
HELLO
sqlite> select upper('привет');
ПРИВЕТ
```

[Download](https://github.com/nalgeon/sqlean/releases/latest)
