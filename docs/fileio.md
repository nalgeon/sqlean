# fileio: Read and write files in SQLite

Access the file system directly from SQL. Adapted from [fileio.c](https://sqlite.org/src/file/ext/misc/fileio.c) by D. Richard Hipp.

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

### lsdir(path)

Lists files and directories as a virtual table.

List a single file specified by `path`:

```
sqlite> select * from lsdir('hello.txt');
┌───────────┬───────┬────────────┬──────┐
│   name    │ mode  │   mtime    │ size │
├───────────┼───────┼────────────┼──────┤
│ hello.txt │ 33206 │ 1639516692 │ 11   │
└───────────┴───────┴────────────┴──────┘
```

Or a whole directory (recursively with subdirectories):

```
sqlite> select * from lsdir('test') order by name;
┌─────────────────┬───────┬────────────┬──────┐
│      name       │ mode  │   mtime    │ size │
├─────────────────┼───────┼────────────┼──────┤
│ test            │ 16877 │ 1639514106 │ 384  │
│ test/crypto.sql │ 33188 │ 1639349274 │ 1426 │
│ test/fileio.sql │ 33188 │ 1639516282 │ 1606 │
│ test/fuzzy.sql  │ 33188 │ 1639349290 │ 2957 │
│ ...             │ ...   │ ...        │ ...  │
└─────────────────┴───────┴────────────┴──────┘
```

Each row has the following columns:

-   `name`: Path to file or directory (text value).
-   `mode`: File mode (`stat.st_mode`, integer value).
-   `mtime`: Last modification time (`stat.st_mtime`, integer number of seconds since the epoch).
-   `size`: Total size in bytes (`stat.st_size`, integer value).

Use `lsmode()` helper function to get a human-readable representation of the `mode`:

```
sqlite> select name, lsmode(mode) from fsdir('test');
┌─────────────────┬──────────────┐
│      name       │ lsmode(mode) │
├─────────────────┼──────────────┤
│ test            │ drwxr-xr-x   │
│ test/crypto.sq  │ -rw-r--r--   │
│ test/fileio.sql │ -rw-r--r--   │
│ test/fuzzy.sql  │ -rw-r--r--   │
│ ...             │ ...          │
└─────────────────┴──────────────┘
```

Parameter `path` is an absolute or relative pathname:

-   If the path refers to a file that does not exist — `lsdir()` returns zero rows.
-   If the path refers to a regular file or symbolic link — it returns a single row.
-   If the path refers to a directory — it returns one row for the directory, and one row for each file within the hierarchy rooted at `path`.

## Usage

```
sqlite> .load ./fileio
sqlite> select readfile('whatever.txt');
```

[Download](https://github.com/nalgeon/sqlean/releases/latest)
