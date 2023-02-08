# unicode: Unicode Support for SQLite

Implements case-insensitive string comparison for Unicode strings. Has no external dependencies (like libicu). Adapted from [sqlite3_unicode](https://github.com/Zensey/sqlite3_unicode).

Provides the following unicode features:

-   `upper()` and `lower()` functions to normalize case.
-   `like()` function and `LIKE` operator with case-independent matching.
-   `unaccent()` function to normalize strings by removing accents.

Tries to override the default NOCASE case-insensitive collation sequence to support UTF-8 characters (available in SQLite CLI and C API only).

### Upper and Lower

```
sqlite> select upper('привет');
ПРИВЕТ
sqlite> select nupper('привет');
ПРИВЕТ
```

`nupper()` is an alias for `upper()` in case the latter is already overridden by some other extension.

```
sqlite> select lower('ПРИВЕТ');
привет
sqlite> select nlower('ПРИВЕТ');
привет
```

`nlower()` is an alias for `lower()` in case the latter is already overridden by some other extension.

### Case-insensitive LIKE

The pattern in `like()` function goes first:

```
sqlite> select like('пРиВ_Т', 'привет');
1
```

The pattern in `LIKE` operator goes second:

```
sqlite> select 'привет' like 'пРиВ_Т';
1
```

### Unaccent

```
sqlite> select unaccent('hôtel');
hotel
```

## Installation and Usage

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

See [How to Install an Extension](install.md) for usage with IDE, Python, etc.

[⬇️ Download](https://github.com/nalgeon/sqlean/releases/latest) •
[✨ Explore](https://github.com/nalgeon/sqlean) •
[🚀 Follow](https://twitter.com/ohmypy)
