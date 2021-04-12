# text: String Functions

Additional string functions for SQLite.
Adapted from [extension-functions.c](https://sqlite.org/contrib/) by Liam Healy. Currently available for Linux and macOS only.

Provides following functions:

### `reverse(source)`

Returns reversed string.

```
sqlite> select reverse('hello world');
dlrow olleh
```

### `split_part(source, sep, part)`

Splits `source` string on `sep` and returns the given `part` (counting from one).

```
sqlite> select split_part('one;two;three', ';', 2);
two
```

If `sep` is composed of multiple characters, each character is treated as separator. E.g.:

```
sqlite> select split_part('one/two\three', '/\', 2);
two
```

[Download](https://github.com/nalgeon/sqlean/releases/latest)

## Usage

```
sqlite> .load ./text
sqlite> select reverse('hello');
```
