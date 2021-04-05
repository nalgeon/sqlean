# `sqlean`: all the missing SQLite functions

SQLite has very few functions compared to other DBMS. SQLite authors see this as a feature rather than a bug, because SQLite has extension mechanism in place.

There are a lot of SQLite extensions out there, but they are incomplete, inconsistent and scattered across the internet.

`sqlean` brings them all together, neatly packaged by domain modules and built for Linux, Windows and macOS.

Here is what we've got right now:

-   [re](docs/re.md): regular expressions
-   [stats](docs/stats.md): common statistical functions
-   [vsv](docs/vsv.md): CSV files as virtual tables
-   [unicode](docs/unicode.md): Unicode string functions

## Download

There are [precompiled binaries](https://github.com/nalgeon/sqlean/releases/latest) for every OS:

-   `*.dll` - for Windows
-   `*.so` - for Linux
-   `*.dylib` - for macOS

## Usage

CLI usage:

```
sqlite> .load ./sqlite3-stats
sqlite> select median(value) from generate_series(1, 100);
```

IDE usage:

```
select load_extension('/path/to/extension/sqlite3-stats');
select median(value) from generate_series(1, 100);
```

In-app usage:

```python
import sqlite3

connection = sqlite3.connect(":memory:")
connection.enable_load_extension(True)
connection.load_extension("./sqlite3-stats.so")
connection.execute("select median(value) from generate_series(1, 100)")
connection.close()
```

You can specify any other supported extension instead of `sqlite3-stats`.
