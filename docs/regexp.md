# regexp: Regular expressions in SQLite

The `sqlean-regexp` extension provides regexp search and replace functions. Supports all major regular expression features (see the section on syntax below).

Supports Unicode in character classes (like `\w`) and assertions (like `\b`).

[Concepts](#concepts) •
[Reference](#reference) •
[Supported syntax](#supported-syntax) •
[Acknowledgements](#acknowledgements) •
[Installation and usage](#installation-and-usage)

## Concepts

Patterns use the [PCRE2](https://www.pcre.org/current/doc/html/pcre2pattern.html) syntax, which is the same as in Perl and very close to the one in most other languages. The common parts are described in [Supported syntax](#supported-syntax).

The extension works with characters, not bytes: `.` matches one character, even if it takes several bytes in UTF-8 (like `п` or `😀`). Character classes (`\d`, `\w`, `\s`), word boundaries (`\b`) and case-insensitive matching follow Unicode, so `\w` matches `п`, and `(?i)привет` matches `ПРИВЕТ`.

A pattern matches anywhere in the source string. Use `^` and `$` to match the whole string:

```sql
select regexp_like('abc', 'b');
-- 1
select regexp_like('abc', '^b$');
-- 0
```

Each function compiles its pattern once per query when the pattern is a constant, so a query that matches many rows against the same pattern doesn't recompile it for every row. A pattern taken from a column is compiled for each row.

### NULL and invalid input

If any function argument is `NULL`, the result is `NULL`, including for the `regexp` operator: `null regexp 'a'` is `NULL`, not `0`.

A `CHECK` constraint is satisfied if its expression is `NULL`, so `check (code regexp '^[A-Z]{3}$')` won't reject a `NULL` value.

Numbers are matched as text: `regexp_like(123, '^\d+$')` is `1`. The functions return `NULL` for data they can't match, instead of raising an error, so one bad row doesn't abort the whole query:

- A BLOB argument: `regexp_like(x'616263', 'b')` is `NULL`. Cast it to text to match it as text.
- Text that is not valid UTF-8, in the source or in the replacement: `regexp_like(cast(x'ff' as text), 'a')` is `NULL`.

The NUL character is matched like any other, so `regexp_replace('a' || char(0) || 'b', 'b', 'x')` keeps the NUL and gives `a<NUL>x`.

A mistake in the query itself raises an error:

- An invalid pattern: `regexp_like('abc', '(')` fails with `invalid pattern: missing closing parenthesis at offset 1`.
- An invalid replacement, or one that refers to a group the pattern doesn't have: `regexp_replace('abc', 'b', '$1')` fails with `invalid replacement: unknown substring at offset 2`.
- A group number that is negative, not an integer, or larger than the number of groups in the pattern: `regexp_capture('abc', 'b', 1)` fails with `invalid group: 1`.

A match that takes too long also raises an error (see [Match limits](#match-limits)).

## Reference

[regexp](#regexp-statement) •
[like](#regexp_like) •
[substr](#regexp_substr) •
[capture](#regexp_capture) •
[replace](#regexp_replace)

### REGEXP statement

Checks if the source string matches the pattern. `source regexp pattern` is the same as `regexp_like(source, pattern)`.

```sql
select true where 'the year is 2021' regexp '[0-9]+';
-- 1
```

The extension replaces the `regexp` function, so the operator uses the PCRE2 syntax even in the SQLite shell, which has a simpler built-in one.

### regexp_like

```text
regexp_like(source, pattern)
```

Checks if the source string matches the pattern. Returns `1` or `0`.

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

Returns the first substring of the source string that matches the pattern, or `NULL` if there is no match.

```sql
select regexp_substr('the year is 2021', '[0-9]+');
-- 2021
select regexp_substr('the year is 2021', '2k21');
-- (null)
```

A pattern can match an empty string, so the result can be empty without being `NULL`:

```sql
select regexp_substr('abc', 'x*') = '';
-- 1
```

### regexp_capture

```text
regexp_capture(source, pattern [, n])
```

Finds the first substring of the source string that matches the pattern and returns the `n`th capturing group within that substring. Group numbering starts at 1. `n = 0` (default) returns the entire substring.

```sql
select regexp_capture('years is 2021', '\d\d(\d\d)', 0);
-- 2021
select regexp_capture('years is 2021', '\d\d(\d\d)', 1);
-- 21
```

Groups are numbered by their opening parenthesis, from left to right. Named groups `(?<name>...)` are numbered too, non-capturing groups `(?:...)` are not:

```sql
select regexp_capture('abcd', '((a)(b))(c)', 3);
-- b
select regexp_capture('abc', '(?:a)(b)', 1);
-- b
```

Returns `NULL` if there is no match, or if the group didn't take part in the match:

```sql
select regexp_capture('ac', 'a(b)?c', 1);
-- (null)
select regexp_capture('ac', 'a(b*)c', 1) = '';
-- 1
```

A group number larger than the number of groups in the pattern is an error, even if there is no match.

### regexp_replace

```text
regexp_replace(source, pattern, replacement)
```

Replaces all matching substrings with the replacement string. Always returns text.

```sql
select regexp_replace('the year is 2021', '[0-9]+', '2050');
-- the year is 2050
select regexp_replace('the year is 2021', '2k21', '2050');
-- the year is 2021
```

The replacement string can refer to the match and the captured groups:

```text
$n, ${n}    the nth captured group (any number, not only 1 to 9)
${name}     the named group
$0          the entire match
$$          a dollar sign
```

```sql
select regexp_replace('the year is 2021', '([0-9]+)', '$1 or 2050');
-- the year is 2021 or 2050
select regexp_replace('John Smith', '(?<first>\w+) (?<last>\w+)', '${last}, ${first}');
-- Smith, John
```

A group that didn't take part in the match is replaced with an empty string. A group the pattern doesn't have is an error.

The replacement string can change the case of what follows, and use escape sequences as in a pattern:

```text
\U          uppercase until \E or the end
\L          lowercase until \E or the end
\u          uppercase the next character
\l          lowercase the next character
\E          stop changing the case
\$, \\      a dollar sign, a backslash
\t, \n      a tab, a newline
```

```sql
select regexp_replace('hello world', '\b(\w)', '\u$1');
-- Hello World
select regexp_replace('hello world', 'world', '\U$0');
-- hello WORLD
```

An empty pattern matches between every character:

```sql
select regexp_replace('abc', '', '-');
-- -a-b-c-
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
(...)          capturing group
(?<name>...)   named capturing group
(?:...)        non-capturing group
(?>...)        atomic group
\N             match the Nth captured group
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

The classes follow Unicode: `\d` matches any decimal digit (like the Arabic-Indic `٣`), and `\w` any letter or digit (like `п`). Use `[0-9]` and `[a-zA-Z0-9_]` for ASCII only.

Assertions:

```
^         start of string
$         end of string, or before a newline at the end
\b        word boundary
\B        non-word boundary
(?=...)   positive lookahead
(?!...)   negative lookahead
(?<=...)  positive lookbehind
(?<!...)  negative lookbehind
```

A lookbehind must have a fixed length: `(?<=\$)\d+` works, `(?<=\$+)\d+` doesn't.

Options:

```
(?i)    case-insensitive matching
(?m)    ^ and $ match at the start and end of each line
(?s)    . matches a newline too
(?x)    ignore whitespace and # comments in the pattern
```

```sql
select regexp_substr('the YEAR is 2021', 'year');
-- (null)
select regexp_substr('the YEAR is 2021', '(?i)year');
-- YEAR
```

See the [PCRE2 documentation](https://www.pcre.org/current/doc/html/pcre2pattern.html) for the full syntax.

### Match limits

Some patterns take exponential time on some inputs because of backtracking. A classic example is `(a+)+$` with a long string of `a`s followed by a `b`. To keep such a query from running forever, the match stops after 10 million backtracking steps and raises an error:

```sql
select regexp_like('aaaaaaaaaaaaaaaaaaaaaaaaaaaaaab', '(a+)+$');
-- Runtime error: regexp: match limit exceeded
```

To lower the limit, start the pattern with `(*LIMIT_MATCH=n)`. To avoid hitting it, rewrite the pattern without nested quantifiers (`a+$` here), or use an atomic group or a possessive quantifier (`(?>a+)+$`, `(a++)+$`).

## Acknowledgements

Based on the [PCRE2](https://github.com/pcre2project/pcre2) engine (version 10.42, BSD 3-Clause License) by Philip Hazel.

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
