# text: String Functions in SQLite

A rich set of string functions, from `slice`, `contains` and `count` to `split_part`, `trim` and `repeat`.

Provides 24 functions, many of which are postgres-compatible (i.e. they have the same alias and logic as in PostgreSQL). It can be useful when migrating from SQLite to PostgreSQL or vice versa.

Note that some unicode-related functions like `upper` and `lower` are in the separate [`unicode`](unicode.md) extension. Regular expression functions are in the separate [`regexp`](regexp.md) extension.

## Substrings and slicing

### `text_substring(str, start [,length])`

Extracts a substring of `length` characters starting at the `start` position (1-based). By default, extracts all characters from `start` to the end of the string.

```sql
select text_substring('hello world', 7);
-- world

select text_substring('hello world', 7, 5);
-- world
```

Postgres-compatible (`substr`), but not aliased as `substr` to avoid conflicts with the built-in `substr` SQLite function.

### `text_slice(str, start [,end])`

Extracts a substring from the `start` position inclusive to the `end` position non-inclusive (1-based). By default, `end` is the end of the string.

Both `start` and `end` can be negative, in which case they are counted from the end of the string toward the beginning of the string.

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

### `text_left(str, length)`

Extracts a substring of `length` characters from the beginning of the string. For negative `length`, extracts all but the last `|length|` characters.

```sql
select text_left('hello world', 5);
-- hello

select text_left('hello world', -6);
-- hello
```

Postgres-compatible, aliased as `left`.

### `text_right(str, length)`

Extracts a substring of `length` characters from the end of the string. For negative `length`, extracts all but the first `|length|` characters.

```sql
select text_right('hello world', 5);
-- world

select text_right('hello world', -6);
-- world
```

Postgres-compatible, aliased as `right`.

## Search and match

### `text_index(str, other)`

Returns the first index of the `other` substring in the original string.

```sql
select text_index('hello yellow', 'ello');
-- 2

select text_index('hello yellow', 'x');
-- 0
```

Postgres-compatible, aliased as `strpos`.

### `text_last_index(str, other)`

Returns the last index of the `other` substring in the original string.

```sql
select text_last_index('hello yellow', 'ello');
-- 8

select text_last_index('hello yellow', 'x');
-- 0
```

### `text_contains(str, other)`

Checks if the string contains the `other` substring.

```sql
select text_contains('hello yellow', 'ello');
-- 1

select text_contains('hello yellow', 'x');
-- 0
```

### `text_has_prefix(str, other)`

Checks if the string starts with the `other` substring.

```sql
select text_has_prefix('hello yellow', 'hello');
-- 1

select text_has_prefix('hello yellow', 'yellow');
-- 0
```

Postgres-compatible, aliased as `starts_with`.

### `text_has_suffix(str, other)`

Checks if the string ends with the `other` substring.

```sql
select text_has_suffix('hello yellow', 'hello');
-- 0

select text_has_suffix('hello yellow', 'yellow');
-- 1
```

### `text_count(str, other)`

Counts how many times the `other` substring is contained in the original string.

```sql
select text_count('hello yellow', 'ello');
-- 2

select text_count('hello yellow', 'x') = 0;
-- 0
```

## Split and join

### `text_split(str, sep, n)`

Splits a string by a separator and returns the n-th part (counting from one). When `n` is negative, returns the `|n|`th-from-last part.

```sql
select text_split('one|two|three', '|', 2);
-- two

select text_split('one|two|three', '|', -1);
-- three

text_split('one|two|three', ';', 2);
-- (empty string)
```

Postgres-compatible, aliased as `split_part`.

### `text_concat(str, ...)`

Concatenates strings and returns the resulting string. Ignores nulls.

```sql
select text_concat('one', 'two', 'three');
-- onetwothree

select text_concat('one', null, 'three');
-- onethree
```

Postgres-compatible, aliased as `concat`.

### `text_join(sep, str, ...)`

Joins strings using the separator and returns the resulting string. Ignores nulls.

```sql
select text_join('|', 'one', 'two');
-- one|two

select text_join('|', 'one', null, 'three');
-- one|three
```

Postgres-compatible, aliased as `concat_ws`.

### `text_repeat(str, count)`

Concatenates the string to itself a given number of times and returns the resulting string.

```sql
select text_repeat('one', 3);
-- oneoneone
```

Postgres-compatible, aliased as `repeat`.

## Trim and pad

### `text_ltrim(str [,chars])`

Trims certain characters (spaces by default) from the beginning of the string.

```sql
select text_ltrim('  hello');
-- hello

select text_ltrim('273hello', '123456789');
-- hello
```

Postgres-compatible, aliased as `ltrim`.

### `text_rtrim(str [,chars])`

Trims certain characters (spaces by default) from the end of the string.

```sql
select text_rtrim('hello  ');
-- hello

select text_rtrim('hello273', '123456789');
-- hello
```

Postgres-compatible, aliased as `rtrim`.

### `text_trim(str [,chars])`

Trims certain characters (spaces by default) from the beginning and end of the string.

```sql
select text_trim('  hello  ');
-- hello

select text_trim('273hello273', '123456789');
-- hello
```

Postgres-compatible, aliased as `btrim`.

## `text_lpad(str, length [,fill])`

Pads the string to the specified length by prepending certain characters (spaces by default).

```sql
select text_lpad('hello', 7);
--   hello

select text_lpad('hello', 7, '*');
-- **hello
```

Postgres-compatible, aliased as `lpad`.

⚠️ PostgreSQL does not support unicode strings in `lpad`, while this function does.

## `text_rpad(str, length [,fill])`

Pads the string to the specified length by appending certain characters (spaces by default).

```sql
select text_rpad('hello', 7);
-- hello

select text_rpad('hello', 7, '*');
-- hello**
```

Postgres-compatible, aliased as `rpad`.

⚠️ PostgreSQL does not support unicode strings in `rpad`, while this function does.

## Other modifications

### `text_replace(str, old, new [,count])`

Replaces `old` substrings with `new` substrings in the original string, but not more than `count` times. By default, replaces all `old` substrings.

```sql
select text_replace('hello', 'l', '*');
-- he**o

select text_replace('hello', 'l', '*', 1);
-- he*lo
```

Postgres-compatible (`replace`), but not aliased as `replace` to avoid conflicts with the built-in `replace` SQLite function.

### `text_translate(str, from, to)`

Replaces each string character that matches a character in the `from` set with the corresponding character in the `to` set. If `from` is longer than `to`, occurrences of the extra characters in `from` are deleted.

```sql
select text_translate('hello', 'ol', '01');
-- he110

select text_translate('hello', 'ol', '0');
-- he0
```

Postgres-compatible, aliased as `translate`.

⚠️ PostgreSQL does not support unicode strings in `translate`, while this function does.

### `text_reverse(str)`

Reverses the order of the characters in the string.

```sql
select text_reverse('hello');
-- olleh
```

Postgres-compatible, aliased as `reverse`.

⚠️ PostgreSQL does not support unicode strings in `reverse`, while this function does.

## String properties

### `text_length(str)`

Returns the number of characters in the string.

```sql
select text_length('𐌀𐌁𐌂');
-- 3
```

Postgres-compatible, aliased as `char_length` and `character_length`.

### `text_size(str)`

Returns the number of bytes in the string.

```sql
select text_size('𐌀𐌁𐌂');
-- 12
```

Postgres-compatible, aliased as `octet_length`.

### `text_bitsize(str)`

Returns the number of bits in the string.

```sql
select text_bitsize('one');
-- 24
```

Postgres-compatible, aliased as `bit_length`.

## Installation and Usage

SQLite command-line interface:

```
sqlite> .load ./text
sqlite> select reverse('hello');
```

See [How to Install an Extension](install.md) for usage with IDE, Python, etc.

[⬇️ Download](https://github.com/nalgeon/sqlean/releases/latest) •
[✨ Explore](https://github.com/nalgeon/sqlean) •
[🚀 Follow](https://twitter.com/ohmypy)
