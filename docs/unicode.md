# unicode: Unicode support

This extension implements true UNICODE functionality for SQLite in regards of case-insensitive comparison of unicode data and SQLite. It uses UNICODE mapping tables to provide the following to SQLite:

-   `upper()`, `lower()`, `title()`, `fold()` functions to normalize strings for comparison by case folding.
-   `unaccent()` function to normalize strings for comparison by removing accents.
-   `LIKE` operator that uses casefolding to provide case-independent matching.

Overrides the default NOCASE case-insensitive collation sequence to support UTF-8 characters.

Has no external dependencies (like libicu). Adapted from [sqlite3_unicode](https://github.com/Zensey/sqlite3_unicode) by Anton Litvinov.

[Download](https://github.com/nalgeon/sqlean/releases/latest)

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
