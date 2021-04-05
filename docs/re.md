# sqlite3-re: Regular Expressions

Regexp search and replace functions for SQLite.
Adapted from [regexp.old](https://github.com/garyhouston/regexp.old) by Henry Spencer.

Provides following functions:

### `REGEXP` statement

Checks if source string matches pattern.

```
sqlite> select true where 'the year is 2021' regexp '[0-9]+';
1
```

### `regexp_like(source, pattern)`

Checks if source string matches pattern.

```
sqlite> select regexp_like('the year is 2021', '[0-9]+');
1
sqlite> select regexp_like('the year is 2021', '2k21');
0
```

### `regexp_substr(source, pattern)`

Returns source substring matching pattern.

```
sqlite> select regexp_substr('the year is 2021', '[0-9]+');
2021
sqlite> select regexp_substr('the year is 2021', '2k21');

```

### `regexp_replace(source, pattern, replacement)`

Replaces matching substring with replacement string.

```
sqlite> select regexp_replace('the year is 2021', '[0-9]+', '2050');
the year is 2050
sqlite> select regexp_replace('the year is 2021', '2k21', '2050');
the year is 2021
```

Supports backreferences to captured groups `\1` trough `\9` in replacement string:

```
sqlite> select regexp_replace('the year is 2021', '([0-9]+)', '\1 or 2050');
the year is 2021 or 2050
```

## Supported syntax

The following regular expression syntax is supported:

```
X*      zero or more occurrences of X
X+      one or more occurrences of X
X?      zero or one occurrences of X
(X)     match X
X|Y     X or Y
^X      X occurring at the beginning of the string
X$      X occurring at the end of the string
.       Match any single character
\c      Character c where c is one of \{}()[]|*+?.
\c      C-language escapes for c in afnrtv. ex: \t or \n
[abc]   Any single character from the set abc
[^abc]  Any single character not in the set abc
[a-z]   Any single character in the range a-z
[^a-z]  Any single character not in the range a-z
```

## Usage

```
sqlite> .load ./sqlite3-re
sqlite> select regexp_like('abcdef', 'b.d');
```

[Download](https://github.com/nalgeon/sqlean/releases/latest)
