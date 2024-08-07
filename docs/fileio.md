# fileio: Read and write files in SQLite

The `sqlean-fileio` extension provides means to access the file system directly from SQL.

[Reference](#reference) •
[Acknowledgements](#acknowledgements) •
[Installation and usage](#installation-and-usage)

## Reference

Main features:

-   [fileio_read](#fileio_read) - Read file contents as a blob.
-   [fileio_scan](#fileio_scan) - Read a file line by line.
-   [fileio_write](#fileio_write) - Write a blob to a file.
-   [fileio_append](#fileio_append) - Append a string to a file.
-   [fileio_mkdir](#fileio_mkdir) - Create a directory.
-   [fileio_symlink](#fileio_symlink) - Create a symlink.
-   [fileio_ls](#fileio_ls) - List files in a directory.

### fileio_read

```text
fileio_read(path [,offset [,limit]])
```

Reads the file specified by `path` and returns its contents as `blob`.

If `offset > 0`, seeks to that offset before reading. If `offset` is past the end of the file, returns `zeroblob(0)`.

If `limit > 0`, limits the number of bytes read.

```sql
select fileio_write('hello.txt', 'hello world');
-- 11

select typeof(fileio_read('hello.txt'));
-- blob

select length(fileio_read('hello.txt'));
-- 11

select fileio_read('hello.txt');
-- hello world

select fileio_read('hello.txt', 6);
-- world

select fileio_read('hello.txt', 0, 5);
-- hello
```

### fileio_scan

```text
fileio_scan(path)
```

Reads the file specified by `path` line by line, without loading the whole file into memory.

```sql
select rowid, value, name from fileio_scan('hello.txt');
```

```
┌───────┬───────┬───────────┐
│ rowid │ value │   name    │
├───────┼───────┼───────────┤
│ 1     │ one   │ hello.txt │
│ 2     │ two   │ hello.txt │
│ 3     │ three │ hello.txt │
└───────┴───────┴───────────┘
```

Treats `\n` as a line separator.

Each row has the following columns:

-   `rowid`: line number starting from 1.
-   `value`: a line from the file.
-   `name`: a path to the file.

Inspired by [sqlite-lines](https://github.com/asg017/sqlite-lines/) by Alex Garcia.

### fileio_write

```text
fileio_write(path, data [,perm [,mtime]])
```

Writes blob `data` to a file specified by `path`. Returns the number of written bytes. If an error occurs, returns NULL.

```sql
select fileio_write('hello.txt', 'hello world');
-- 11
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

```sql
select fileio_write('hello.txt', 'hello world', 436);
-- 11
```

If the optional `mtime` argument is present, it expects an integer — the number of seconds since the unix epoch. The modification-time of the target file is set to this value before returning.

### fileio_append

```text
fileio_append(path, str)
```

Appends `str` string to a file specified by `path`. Returns the number of written bytes. If an error occurs, returns NULL.

Useful for writing large datasets line by line, without loading the whole dataset into memory.

```sql
create table hello(value text);
insert into hello(value) values ('one'), ('two'), ('three');

select sum(fileio_append('hello.txt', value||char(10))) from hello;
-- 14
```

```
$ cat hello.txt
one
two
three
```

Using concatenation with `char(10)` adds `\n` to the end of the line.

### fileio_mkdir

```text
fileio_mkdir(path [,perm])
```

Creates a directory named `path` with permission bits `perm` (octal `777` by default).

```sql
select fileio_mkdir('hellodir');
```

### fileio_symlink

```text
fileio_symlink(src, dst)
```

Creates a symbolic link named `dst`, pointing to `src`.

```sql
select fileio_symlink('hello.txt', 'hello.lnk');
```

### fileio_ls

```text
fileio_ls(path [,recursive])
```

Lists files and directories as a virtual table.

List a single file specified by `path`:

```sql
select * from fileio_ls('hello.txt');
```

```
┌───────────┬───────┬────────────┬──────┐
│   name    │ mode  │   mtime    │ size │
├───────────┼───────┼────────────┼──────┤
│ hello.txt │ 33206 │ 1639516692 │ 11   │
└───────────┴───────┴────────────┴──────┘
```

List a whole directory. Lists only the direct children by default:

```sql
select * from fileio_ls('test') order by name;
```

```
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

List a whole directory recursively. When `recursive = true`, lists all the descendants:

```sql
select * from fileio_ls('src', true);
```

Each row has the following columns:

-   `name`: Path to file or directory (text value).
-   `mode`: File mode (`stat.st_mode`, integer value).
-   `mtime`: Last modification time (`stat.st_mtime`, integer number of seconds since the epoch).
-   `size`: Total size in bytes (`stat.st_size`, integer value).

Use `fileio_mode()` helper function to get a human-readable representation of the `mode`:

```sql
select name, fileio_mode(mode) as mode from fileio_ls('test');
```

```
┌─────────────────┬──────────────┐
│      name       │     mode     │
├─────────────────┼──────────────┤
│ test            │ drwxr-xr-x   │
│ test/crypto.sq  │ -rw-r--r--   │
│ test/fileio.sql │ -rw-r--r--   │
│ test/fuzzy.sql  │ -rw-r--r--   │
│ ...             │ ...          │
└─────────────────┴──────────────┘
```

Parameter `path` is an absolute or relative pathname:

-   If the path refers to a file that does not exist — `fileio_ls()` returns zero rows.
-   If the path refers to a regular file or symbolic link — it returns a single row.
-   If the path refers to a directory — it returns one row for the directory and one row for each direct child. Optionally returns a row for every descendant, if `recursive = true`.

### Backward Compatibilty

Some functions have aliases for backward compatibility:

```
readfile  = fileio_read
writefile = fileio_write
mkdir     = fileio_mkdir
symlink   = fileio_symlink
lsdir     = fileio_ls
lsmode    = fileio_mode
```

## Acknowledgements

Partly based on the [fileio.c](https://sqlite.org/src/file/ext/misc/fileio.c) by D. Richard Hipp.

## Installation and usage

SQLite command-line interface:

```
sqlite> .load ./fileio
sqlite> select fileio_read('whatever.txt');
```

See [How to install an extension](install.md) for usage with IDE, Python, etc.

↓ [Download](https://github.com/nalgeon/sqlean/releases/latest) the extension.

⛱ [Explore](https://github.com/nalgeon/sqlean) other extensions.

★ [Subscribe](https://antonz.org/subscribe/) to stay on top of new features.
