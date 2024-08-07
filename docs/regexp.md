# regexp: Regular expressions in SQLite

The `sqlean-regexp` extension provides regexp search and replace functions. Supports all major regular expression features (see the section on syntax below).

Supports Unicode in character classes (like `\w`) and assertions (like `\b`).

[Reference](#reference) •
[Supported syntax](#supported-syntax) •
[Acknowledgements](#acknowledgements) •
[Installation and usage](#installation-and-usage)

## Reference

[regexp](#regexp-statement) •
[like](#regexp_like) •
[substr](#regexp_substr) •
[capture](#regexp_capture) •
[replace](#regexp_replace)

### REGEXP statement

Checks if the source string matches the pattern.

```sql
select true where 'the year is 2021' regexp '[0-9]+';
-- 1
```

### regexp_like

```text
regexp_like(source, pattern)
```

Checks if the source string matches the pattern.

```sql
select regexp_like('the year is 2021', '[0-9]+');
-- 1
select regexp_like('the year is 2021', '2k21');
-- 0
```

### regexp_substr

```text
regexp_substr(source, pattern)
```

Returns a substring of the source string that matches the pattern.

```sql
select regexp_substr('the year is 2021', '[0-9]+');
-- 2021
select regexp_substr('the year is 2021', '2k21');
-- (null)
```

### regexp_capture

```text
regexp_capture(source, pattern [, n])
```

Finds a substring of the source string that matches the pattern and returns the `n`th matching group within that substring. Group numbering starts at 1. `n = 0` (default) returns the entire substring.

```sql
select regexp_capture('years is 2021', '\d\d(\d\d)', 0);
-- 2021
select regexp_capture('years is 2021', '\d\d(\d\d)', 1);
-- 21
```

### regexp_replace

```text
regexp_replace(source, pattern, replacement)
```

Replaces all matching substrings with the replacement string.

```sql
select regexp_replace('the year is 2021', '[0-9]+', '2050');
-- the year is 2050
select regexp_replace('the year is 2021', '2k21', '2050');
-- the year is 2021
```

Supports backreferences to captured groups `$1` trough `$9` in the replacement string:

```sql
select regexp_replace('the year is 2021', '([0-9]+)', '$1 or 2050');
-- the year is 2021 or 2050
```

## Supported syntax

Basic expressions:

```
.        any character except newline
a        the character a
ab       the string ab
a|b      a or b
\        escapes a special character
```

Quantifiers:

```
*        0 or more
+        1 or more
?        0 or 1
{n}      exactly n
{n,m}    between n and m
{n,}     n or more
```

Quantifiers are greedy by default (i.e., match as much text as possible). Add `?` to make them lazy (i.e., match as little text as possible):

```
*?       0 or more (lazy)
+?       1 or more (lazy)
```

Groups:

```
(...)    capturing group
(?:...)  non-capturing group
(?>...)  atomic group
\N       match the Nth captured group
```

Character classes:

```
[ab-d]   one character of: a, b, c, d
[^ab-d]  one character except: a, b, c, d
\d       one digit
\D       one non-digit
\s       one whitespace
\S       one non-whitespace
\w       one word character
\W       one non-word character
```

Assertions:

```
^        start of string
$        end of string
\b       word boundary
\B       non-word boundary
(?=...)  positive lookahead
(?!...)  negative lookahead
```

Options:

```
(?i)    case-insensitive matching
```

```sql
select regexp_substr('the YEAR is 2021', 'year');
-- (null)
select regexp_substr('the YEAR is 2021', '(?i)year');
-- YEAR
```

## Acknowledgements

Based on the [PCRE2](https://github.com/pcre2project/pcre2) engine.

## Installation and usage

SQLite command-line interface:

```
sqlite> .load ./regexp
sqlite> select regexp_like('abcdef', 'b.d');
```

See [How to install an extension](install.md) for usage with IDE, Python, etc.

[⬇️ Download](https://github.com/nalgeon/sqlean/releases/latest) •
[✨ Explore](https://github.com/nalgeon/sqlean) •
[🚀 Follow](https://antonz.org/subscribe/)
