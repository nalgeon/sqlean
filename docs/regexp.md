# regexp: Regular Expressions in SQLite

Regexp search and replace functions. Based on the [PCRE2](https://github.com/pcre2project/pcre2) engine, this extension supports all major regular expression features (see the section on syntax below).

Supports Unicode in character classes (like `\w`) and assertions (like `\b`).

Provides the following functions:

### `REGEXP` statement

Checks if the source string matches the pattern.

```
sqlite> select true where 'the year is 2021' regexp '[0-9]+';
1
```

### `regexp_like(source, pattern)`

Checks if the source string matches the pattern.

```
sqlite> select regexp_like('the year is 2021', '[0-9]+');
1
sqlite> select regexp_like('the year is 2021', '2k21');
0
```

### `regexp_substr(source, pattern)`

Returns a substring of the source string that matches the pattern.

```
sqlite> select regexp_substr('the year is 2021', '[0-9]+');
2021
sqlite> select regexp_substr('the year is 2021', '2k21');
(null)
```

### `regexp_replace(source, pattern, replacement)`

Replaces all matching substrings with the replacement string.

```
sqlite> select regexp_replace('the year is 2021', '[0-9]+', '2050');
the year is 2050
sqlite> select regexp_replace('the year is 2021', '2k21', '2050');
the year is 2021
```

Supports backreferences to captured groups `$1` trough `$9` in the replacement string:

```
sqlite> select regexp_replace('the year is 2021', '([0-9]+)', '$1 or 2050');
the year is 2021 or 2050
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

## Usage

```
sqlite> .load ./regexp
sqlite> select regexp_like('abcdef', 'b.d');
```

[⬇️ Download](https://github.com/nalgeon/sqlean/releases/latest) •
[✨ Explore](https://github.com/nalgeon/sqlean) •
[🚀 Follow](https://twitter.com/ohmypy)
