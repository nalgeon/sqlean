# fileio: Read and write files in SQLite

Read and write files directly from SQL. Adapted from [fileio.c](https://sqlite.org/src/file/ext/misc/fileio.c) by D. Richard Hipp.

### `writefile(path, data [, mode [, mtime]])`

Given the first two arguments, writes blob `data` to a file specified by `path`. If successful, returns the number of written bytes. If an error occurs, returns NULL.

```
sqlite> select writefile('hello.txt', 'hello world');
11
```

If optional `mode` argument is present, it expects an integer value that corresponds to a POSIX mode value (file type + permissions, as returned in the `stat.st_mode` field by the `stat()` system call). Three types of files may be written/created:

-   regular files: `(mode & 0170000) == 0100000`
-   symbolic links: `(mode & 0170000) == 0120000`
-   directories: `(mode & 0170000) == 0040000`

For a directory, the `data` is ignored. For a symbolic link, it is interpreted as text and used as the target of the link. For a regular file, it is interpreted as a blob and written into the
named file. Regardless of the type of file, its permissions are set to (`mode & 0777`) before returning.

If the optional `mtime` argument is present, it expects an integer - the number of seconds since the unix epoch. The modification-time of the target file is set to this value before returning.

### `readfile(path)`

Reads the file specified by `path` and returns the contents as `blob`.

```
sqlite> select writefile('hello.txt', 'hello world');
11

sqlite> select typeof(readfile('hello.txt'));
blob

sqlite> select length(readfile('hello.txt'));
11
```

### `fsdir(path [, dir])`

Not a function, but a virtual table. Lists a single file specified by `path`:

```
sqlite> select name, mode, mtime, length(data) from fsdir('hello.txt');
┌───────────┬───────┬────────────┬──────────────┐
│   name    │ mode  │   mtime    │ length(data) │
├───────────┼───────┼────────────┼──────────────┤
│ hello.txt │ 33188 │ 1639336214 │ 11           │
└───────────┴───────┴────────────┴──────────────┘
```

Or a whole directory:

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

```
sqlite> select name, mode, mtime, length(data) from fsdir('fileio.sql', 'test');
┌────────────┬───────┬────────────┬──────────────┐
│    name    │ mode  │   mtime    │ length(data) │
├────────────┼───────┼────────────┼──────────────┤
│ fileio.sql │ 33188 │ 1639336273 │ 349          │
└────────────┴───────┴────────────┴──────────────┘
```

Parameter `path` is an absolute or relative pathname. If the file that it refers to does not exist, it is an error. If the path refers to a regular file or symbolic link, it returns a single row. Or, if the path refers to a directory, it returns one row for the directory, and one row for each file within the hierarchy rooted at `path`.

Each row has the following columns:

-   `name`: Path to file or directory (text value).
-   `mode`: Value of `stat.st_mode` for directory entry (an integer).
-   `mtime`: Value of stat.st_mtime for directory entry (an integer).
-   `data`: For a regular file, a blob containing the file data. For a symlink, a text value containing the text of the link. For a directory, NULL.

If a non-NULL value is specified for the optional `dir` parameter and `path` is a relative path, then `path` is interpreted relative to `dir`. And the paths returned in the `name` column of the table are also relative to directory `dir`.

## Usage

```
sqlite> .load ./fileio
sqlite> select readfile('whatever.txt');
```

[Download](https://github.com/nalgeon/sqlean/releases/latest)
