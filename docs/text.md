# text: String functions in SQLite

The `sqlean-text` extension provides a rich set of functions for working with text.

Also provides Unicode-aware functions for changing text case (upper, lower, title, casefold), plus a custom nocase collation.

Many of the functions are Postgres-compatible (i.e. they have the same alias and logic as in PostgreSQL). It can be useful when migrating from SQLite to PostgreSQL or vice versa.

Regular expression functions are in the separate [regexp](regexp.md) extension.

[Concepts](#concepts) •
[Substrings and slicing](#substrings-and-slicing) •
[Search and match](#search-and-match) •
[Split and join](#split-and-join) •
[Trim and pad](#trim-and-pad) •
[Change case](#change-case) •
[Other modifications](#other-modifications) •
[String properties](#string-properties) •
[Invalid UTF-8](#invalid-utf-8) •
[Acknowledgements](#acknowledgements) •
[Installation and usage](#installation-and-usage)

## Concepts

**Characters**. Positions and lengths count characters (Unicode code points), not bytes, and positions start at 1. `text_length('ж')` is 1, while `text_size('ж')` is 2. A zero byte (`char(0)`) is a character like any other: it doesn't end the string.

**Negative positions** count from the end of the string in `text_slice` and `text_split` (-1 is the last character or part). In `text_left` and `text_right`, a negative length drops that many characters from the other end. Positions beyond the string are clamped to it, so out-of-range arguments give a shorter (or empty) result rather than an error.

**Empty search strings** follow Go's `strings` package. An empty substring is found at the start of the string (`text_index` gives 1) and after its end (`text_last_index` gives the length plus 1), between every two characters (`text_count` gives the length plus 1), and is contained in any string, including an empty one. `text_replace` with an empty `old` returns the string unchanged.

**Case** mapping uses the simple (one-to-one) Unicode mappings, so the result has the same number of characters as the input: `ß` becomes `ẞ` in upper case, not `SS`. The mappings cover the Basic Multilingual Plane (U+0000 to U+FFFF). Letters outside it, such as Deseret (`𐐀`), are left unchanged.

### NULL and invalid input

If any function argument is `NULL`, the result is `NULL`. The exceptions are `text_concat` and `text_join`, which skip `NULL` strings (but `text_join` with a `NULL` separator is `NULL`).

Like SQLite's own string functions, the extension returns `NULL` for invalid data instead of raising an error, so one bad row doesn't abort the whole query:

- A blob in place of a string: `text_upper(x'61')` and `text_length(x'414243')` are `NULL`. Numbers are converted to text, so `text_length(12345)` is 5.
- A position, length or count that is not an integer: `text_left('hello', 'x')` and `text_left('hello', 2.5)` are `NULL`, and so is `text_left('hello', 2.0)`. Integer text is converted, so `text_left('hello', '2')` is `he`.
- Part 0 in `text_split`, since parts are counted from 1.

Errors are raised for a mistake in the query, and when a result is too large:

- `text_join` without strings and `text_concat` without arguments fail with `wrong number of arguments`, as do other functions called with the wrong number of arguments.
- A result longer than SQLite's maximum string length fails with `string or blob too big`, before the string is built: `text_repeat('a', 9e18)`.

### Aliases

Postgres-compatible functions have aliases with Postgres names: `left`, `right`, `strpos`, `starts_with`, `split_part`, `repeat`, `btrim`, `lpad`, `rpad`, `translate`, `reverse`, `char_length`, `character_length` and `bit_length`.

The extension never replaces SQLite's built-in functions. `concat` and `concat_ws` are registered only on SQLite older than 3.44, and `octet_length` only on SQLite older than 3.43, where they are not built in. SQLite has its own `ltrim`, `rtrim`, `trim`, `replace`, `substr`, `upper`, `lower` and `like`, which work differently (for example, `upper` and `lower` only change ASCII letters), so use the `text_` functions for Unicode-aware behavior.

## Substrings and slicing

### text_substring

```text
text_substring(str, start [,length])
```

Extracts a substring of `length` characters starting at the `start` position (1-based). By default, extracts all characters from `start` to the end of the string.

As in Postgres, a `start` before the beginning of the string shortens the substring: `text_substring('hello', 0, 3)` is `he`, since the characters at positions 0, 1 and 2 are taken. A negative `length` gives an empty string.

```sql
select text_substring('hello world', 7);
-- world

select text_substring('hello world', 7, 5);
-- world
```

Postgres-compatible (`substr`), but not aliased as `substr` to avoid conflicts with the built-in `substr` SQLite function.

### text_slice

```
text_slice(str, start [,end])
```

Extracts a substring from the `start` position inclusive to the `end` position non-inclusive (1-based). By default, `end` is the end of the string.

Both `start` and `end` can be negative, in which case they are counted from the end of the string toward the beginning of the string. A position of 0 is the beginning of the string, like 1.

```sql
select text_slice('hello world', 7);
-- world

select text_slice('hello world', 7, 12);
-- world

select text_slice('hello world', -5);
-- world

select text_slice('hello world', -5, -2);
-- wor
```

### text_left

```text
text_left(str, length)
```

Extracts a substring of `length` characters from the beginning of the string. For negative `length`, extracts all but the last `|length|` characters.

```sql
select text_left('hello world', 5);
-- hello

select text_left('hello world', -6);
-- hello
```

Postgres-compatible, aliased as `left`.

### text_right

```text
text_right(str, length)
```

Extracts a substring of `length` characters from the end of the string. For negative `length`, extracts all but the first `|length|` characters.

```sql
select text_right('hello world', 5);
-- world

select text_right('hello world', -6);
-- world
```

Postgres-compatible, aliased as `right`.

## Search and match

### text_index

```text
text_index(str, other)
```

Returns the position of the first occurrence of the `other` substring in the original string, or 0 if there is none. The search is case-sensitive.

```sql
select text_index('hello yellow', 'ello');
-- 2

select text_index('hello yellow', 'x');
-- 0
```

Postgres-compatible, aliased as `strpos`.

### text_last_index

```text
text_last_index(str, other)
```

Returns the position of the last occurrence of the `other` substring in the original string, or 0 if there is none.

```sql
select text_last_index('hello yellow', 'ello');
-- 8

select text_last_index('hello yellow', 'x');
-- 0
```

### text_contains

```text
text_contains(str, other)
```

Checks if the string contains the `other` substring.

```sql
select text_contains('hello yellow', 'ello');
-- 1

select text_contains('hello yellow', 'x');
-- 0
```

### text_has_prefix

```text
text_has_prefix(str, other)
```

Checks if the string starts with the `other` substring.

```sql
select text_has_prefix('hello yellow', 'hello');
-- 1

select text_has_prefix('hello yellow', 'yellow');
-- 0
```

Postgres-compatible, aliased as `starts_with`.

### text_has_suffix

```text
text_has_suffix(str, other)
```

Checks if the string ends with the `other` substring.

```sql
select text_has_suffix('hello yellow', 'hello');
-- 0

select text_has_suffix('hello yellow', 'yellow');
-- 1
```

### text_count

```text
text_count(str, other)
```

Counts how many times the `other` substring is contained in the original string. Occurrences don't overlap: `text_count('aaa', 'aa')` is 1.

```sql
select text_count('hello yellow', 'ello');
-- 2

select text_count('hello yellow', 'x');
-- 0
```

### text_like

```text
text_like(pattern, str)
```

Reports whether a string matches a pattern using the LIKE syntax: `%` matches any sequence of characters (including none), and `_` matches exactly one character. The arguments are in the same order as in SQLite's `like()` function: the pattern first.

Unlike SQLite's `like`, which only ignores the case of ASCII letters, `text_like` ignores case for all of Unicode (using case folding). It has no escape character, so `%` and `_` can't be matched literally.

```sql
select text_like('cóm_ está_', 'CÓMO ESTÁS');
-- 1

select text_like('ça%', 'Ça roule');
-- 1
```

Not aliased as `like` to avoid conflicts with the built-in `like` SQLite function.

## Split and join

### text_split

```text
text_split(str, sep, n)
```

Splits a string by a separator and returns the n-th part (counting from one). When `n` is negative, returns the `|n|`th-from-last part. When there is no such part, returns an empty string. Part 0 is `NULL`. An empty separator doesn't split the string, so the whole string is the only part.

```sql
select text_split('one|two|three', '|', 2);
-- two

select text_split('one|two|three', '|', -1);
-- three

select text_split('one|two|three', ';', 2);
-- (empty string)
```

Postgres-compatible, aliased as `split_part`.

### text_concat

```text
text_concat(str, ...)
```

Concatenates strings and returns the resulting string. Ignores nulls.

On SQLite older than 3.44, aliased as `concat` (newer versions have it built in).

```sql
select text_concat('one', 'two', 'three');
-- onetwothree

select text_concat('one', null, 'three');
-- onethree
```

### text_join

```text
text_join(sep, str, ...)
```

Joins strings using the separator and returns the resulting string. Ignores nulls.

On SQLite older than 3.44, aliased as `concat_ws` (newer versions have it built in).

```sql
select text_join('|', 'one', 'two');
-- one|two

select text_join('|', 'one', null, 'three');
-- one|three
```

### text_repeat

```text
text_repeat(str, count)
```

Concatenates the string to itself a given number of times and returns the resulting string. A count of 0 or less gives an empty string.

```sql
select text_repeat('one', 3);
-- oneoneone
```

Postgres-compatible, aliased as `repeat`.

## Trim and pad

### text_ltrim

```text
text_ltrim(str [,chars])
```

Trims certain characters (spaces by default) from the beginning of the string. Only spaces are trimmed by default, not tabs or newlines (as in Postgres).

```sql
select text_ltrim('  hello');
-- hello

select text_ltrim('273hello', '123456789');
-- hello
```

Postgres-compatible, but not aliased as `ltrim` to avoid conflicts with the built-in `ltrim` SQLite function.

### text_rtrim

```text
text_rtrim(str [,chars])
```

Trims certain characters (spaces by default) from the end of the string.

```sql
select text_rtrim('hello  ');
-- hello

select text_rtrim('hello273', '123456789');
-- hello
```

Postgres-compatible, but not aliased as `rtrim` to avoid conflicts with the built-in `rtrim` SQLite function.

### text_trim

```text
text_trim(str [,chars])
```

Trims certain characters (spaces by default) from the beginning and end of the string.

```sql
select text_trim('  hello  ');
-- hello

select text_trim('273hello273', '123456789');
-- hello
```

Postgres-compatible, aliased as `btrim`.

### text_lpad

```text
text_lpad(str, length [,fill])
```

Pads the string to the specified length by prepending certain characters (spaces by default). If the string is longer than `length`, it is truncated to `length` characters (keeping the beginning). An empty `fill` doesn't pad.

```sql
select text_lpad('hello', 7);
--   hello

select text_lpad('hello', 7, '*');
-- **hello
```

Postgres-compatible, aliased as `lpad`.

### text_rpad

```text
text_rpad(str, length [,fill])
```

Pads the string to the specified length by appending certain characters (spaces by default). If the string is longer than `length`, it is truncated to `length` characters. An empty `fill` doesn't pad.

```sql
select text_rpad('hello', 7);
-- hello

select text_rpad('hello', 7, '*');
-- hello**
```

Postgres-compatible, aliased as `rpad`.

## Change case

### text_upper

```text
text_upper(str)
```

Transforms a string to upper case.

```sql
select text_upper('cómo estás');
-- CÓMO ESTÁS
```

Not aliased as `upper` to avoid conflicts with the built-in `upper` SQLite function.

### text_lower

```text
text_lower(str)
```

Transforms a string to lower case.

```sql
select text_lower('CÓMO ESTÁS');
-- cómo estás
```

Not aliased as `lower` to avoid conflicts with the built-in `lower` SQLite function.

### text_title

```text
text_title(str)
```

Transforms a string to title case: the first letter of each word in title case, the rest in lower case. A word is a sequence of letters, digits and underscores, so an apostrophe starts a new word (`don't` becomes `Don'T`). Digraphs that have a title case form use it: `ǳ` becomes `ǲ`, not `Ǳ`.

```sql
select text_title('cómo estás');
-- Cómo Estás
```

### text_casefold

```text
text_casefold(str)
```

Folds the case of a string, for case-insensitive comparisons. It's similar to lower case, but more consistent: for example, all forms of the Greek sigma (`Σ`, `σ`, `ς`) fold to `σ`. Uses the simple case folding, so `ß` stays `ß`.

```sql
select text_casefold('ΣΑΣ');
-- σασ

select text_casefold('Straße');
-- straße
```

### text_nocase

The `text_nocase` collating sequence compares strings without regard to case: it compares their case-folded forms (see `text_casefold`).

```sql
select 1 where 'cómo estás' = 'CÓMO ESTÁS';
-- (null)

select 1 where 'cómo estás' = 'CÓMO ESTÁS' collate text_nocase;
-- 1
```

## Other modifications

### text_replace

```text
text_replace(str, old, new [,count])
```

Replaces `old` substrings with `new` substrings in the original string, but not more than `count` times. By default, replaces all `old` substrings. A negative `count` replaces nothing, and an empty `old` returns the string unchanged.

```sql
select text_replace('hello', 'l', '*');
-- he**o

select text_replace('hello', 'l', '*', 1);
-- he*lo
```

Postgres-compatible (`replace`), but not aliased as `replace` to avoid conflicts with the built-in `replace` SQLite function.

### text_translate

```text
text_translate(str, from, to)
```

Replaces each string character that matches a character in the `from` set with the corresponding character in the `to` set. If `from` is longer than `to`, occurrences of the extra characters in `from` are deleted. Extra characters in `to` are ignored, and if a character appears in `from` more than once, its first occurrence is used (as in Postgres).

```sql
select text_translate('hello', 'ol', '01');
-- he110

select text_translate('hello', 'ol', '0');
-- he0
```

Postgres-compatible, aliased as `translate`.

### text_reverse

```text
text_reverse(str)
```

Reverses the order of the characters in the string.

```sql
select text_reverse('hello');
-- olleh
```

Postgres-compatible, aliased as `reverse`.

## String properties

### text_length

```text
text_length(str)
```

Returns the number of characters in the string.

```sql
select text_length('𐌀𐌁𐌂');
-- 3
```

Postgres-compatible, aliased as `char_length` and `character_length`.

### text_size

```text
text_size(str)
```

Returns the number of bytes in the string. Unlike SQLite's `octet_length`, gives `NULL` for a blob.

```sql
select text_size('𐌀𐌁𐌂');
-- 12
```

Postgres-compatible. On SQLite older than 3.43, aliased as `octet_length` (newer versions have it built in).

### text_bitsize

```text
text_bitsize(str)
```

Returns the number of bits in the string.

```sql
select text_bitsize('one');
-- 24
```

Postgres-compatible, aliased as `bit_length`.

## Invalid UTF-8

SQLite does not check that a text value is valid UTF-8, so a string may contain arbitrary bytes (for example, after `cast(x'ff' as text)`). Text functions keep such bytes as they are: they are counted, searched, compared and copied, but never changed or replaced.

Each malformed byte sequence (a maximal ill-formed subpart, as Unicode calls it: 1 to 3 bytes) counts as one character. The valid characters around it are left alone:

```sql
select text_length(cast(x'41ff42' as text));
-- 3

select hex(text_reverse(cast(x'41ff42' as text)));
-- 42FF41
```

Case functions convert the valid characters and copy the invalid bytes:

```sql
select hex(text_upper(cast(x'61ff62' as text)));
-- 41FF42
```

Search functions look for the exact bytes, so an invalid sequence only matches itself. The `text_nocase` collation sorts invalid sequences after all valid characters, and different ones compare as different.

## Acknowledgements

Case mapping tables are adapted from [stc](https://github.com/stclib/stc) by Tyge Løvset (MIT License). Unicode category and script ranges are extracted from [RE2](https://github.com/google/re2) by Google (BSD 3-Clause License). The functions follow Go's [strings](https://pkg.go.dev/strings) package (BSD 3-Clause License) and PostgreSQL's string functions.

## Installation and usage

SQLite command-line interface:

```
sqlite> .load ./text
sqlite> select text_reverse('hello');
```

See [How to install an extension](install.md) for usage with IDE, Python, etc.

↓ [Download](https://github.com/nalgeon/sqlean/releases/latest) the extension.

⛱ [Explore](https://github.com/nalgeon/sqlean) other extensions.

★ [Subscribe](https://antonz.org/subscribe/) to stay on top of new features.
