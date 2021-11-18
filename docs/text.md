# text: Text manipulation in SQLite

Additional string functions.
Adapted from [extension-functions.c](https://sqlite.org/contrib/) by Liam Healy.

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
sqlite> select split_part('one;;three', ';', 2);

```

If `sep` is composed of multiple characters, each character is treated as separator. E.g.:

```
sqlite> select split_part('one/two\three', '/\', 2);
two
```

Only ASCII (1-byte) symbols are supported as separators.

[Download](https://github.com/nalgeon/sqlean/releases/latest)

## Usage

```
sqlite> .load ./text
sqlite> select reverse('hello');
```
