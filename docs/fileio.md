# fileio: Read and write files in SQLite

Read and write files directly from SQL. Adapted from [fileio.c](https://sqlite.org/src/file/ext/misc/fileio.c) by D. Richard Hipp.

### writefile(path, data [,perm [,mtime]])

Writes blob `data` to a file specified by `path`. Returns the number of written bytes. If an error occurs, returns NULL.

```
sqlite> select writefile('hello.txt', 'hello world');
11
```

The `perm` argument specifies permission bits for the file (octal `666` by default). Expects _decimal_ value, not octal. Here are some popular values:

| Octal | Decimal | Description |
| ----- | ------- | ----------- |
| 600   | 384     | `rw-------` |
| 644   | 420     | `rw-r--r--` |
| 664   | 436     | `rw-rw-r--` |
| 666   | 438     | `rw-rw-rw-` |
| 755   | 493     | `rwxr-xr-x` |
| 777   | 511     | `rwxrwxrwx` |

```
sqlite> select writefile('hello.txt', 'hello world', 436);
11
```

If the optional `mtime` argument is present, it expects an integer — the number of seconds since the unix epoch. The modification-time of the target file is set to this value before returning.

### readfile(path)

Reads the file specified by `path` and returns its contents as `blob`.

```
sqlite> select writefile('hello.txt', 'hello world');
11

sqlite> select typeof(readfile('hello.txt'));
blob

sqlite> select length(readfile('hello.txt'));
11
```

### mkdir(path[, perm])

Creates a directory named `path` with permission bits `perm` (octal `777` by default).

```
sqlite> mkdir('hellodir')
```

### symlink(src, dst)

Creates a symbolic link named `dst`, pointing to `src`.

```
select symlink('hello.txt', 'hello.lnk');
```

### fsdir(path[, dir])

Lists files and directories. Not a function, but a virtual table.

List a single file specified by `path`:

```
sqlite> select name, mode, mtime, length(data) from fsdir('hello.txt');
┌───────────┬───────┬────────────┬──────────────┐
│   name    │ mode  │   mtime    │ length(data) │
├───────────┼───────┼────────────┼──────────────┤
│ hello.txt │ 33188 │ 1639336214 │ 11           │
└───────────┴───────┴────────────┴──────────────┘
```

Or a whole directory (with subdirectories):

```
sqlite> select name, mode, mtime, length(data) from fsdir('test');
┌───────────────────┬───────┬────────────┬──────────────┐
│       name        │ mode  │   mtime    │ length(data) │
├───────────────────┼───────┼────────────┼──────────────┤
│ test              │ 16877 │ 1639338455 │              │
│ test/stats.sql    │ 33188 │ 1618771137 │ 939          │
│ test/math.sql     │ 33188 │ 1617645238 │ 1112         │
│ test/ipaddr.sql   │ 33188 │ 1633001481 │ 1704         │
│ test/fileio.sql   │ 33188 │ 1639337954 │ 392          │
│ test/text.sql     │ 33188 │ 1637171984 │ 990          │
│ test/crypto.sql   │ 33188 │ 1618769096 │ 1338         │
│ test/fuzzy.sql    │ 33188 │ 1638018849 │ 2869         │
│ test/spellfix.sql │ 33188 │ 1637516200 │ 230          │
│ test/re.sql       │ 33188 │ 1633001328 │ 836          │
│ test/uuid.sql     │ 33188 │ 1636919312 │ 1122         │
└───────────────────┴───────┴────────────┴──────────────┘
```

Each row has the following columns:

-   `name`: Path to file or directory (text value).
-   `mode`: Value of `stat.st_mode` for directory entry (an integer).
-   `mtime`: Value of `stat.st_mtime` for directory entry (an integer).
-   `data`: For a regular file, a blob containing the file data. For a symlink, a text value containing the text of the link. For a directory, NULL.

Use `lsmode()` helper function to get a human-readable representation of the `mode`:

```
sqlite> select name, lsmode(mode) from fsdir('test');
┌───────────────────┬──────────────┐
│       name        │ lsmode(mode) │
├───────────────────┼──────────────┤
│ test              │ drwxr-xr-x   │
│ test/stats.sql    │ -rw-r--r--   │
│ test/ipaddr.sql   │ -rw-r--r--   │
│ test/fileio.sql   │ -rw-r--r--   │
│ ...               │ ...          │
└───────────────────┴──────────────┘
```

Parameter `path` is an absolute or relative pathname. If the file that it refers to does not exist, it is an error. If the path refers to a regular file or symbolic link, it returns a single row. Or, if the path refers to a directory, it returns one row for the directory, and one row for each file within the hierarchy rooted at `path`.

If a non-NULL value is specified for the optional `dir` parameter and `path` is a relative path, then `path` is interpreted relative to `dir`. And the paths returned in the `name` column of the table are also relative to directory `dir`.

```
sqlite> select name, mode, mtime, length(data) from fsdir('fileio.sql', 'test');
┌────────────┬───────┬────────────┬──────────────┐
│    name    │ mode  │   mtime    │ length(data) │
├────────────┼───────┼────────────┼──────────────┤
│ fileio.sql │ 33188 │ 1639336273 │ 349          │
└────────────┴───────┴────────────┴──────────────┘
```

## Usage

```
sqlite> .load ./fileio
sqlite> select readfile('whatever.txt');
```

[Download](https://github.com/nalgeon/sqlean/releases/latest)
