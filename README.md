# All the missing SQLite functions

SQLite has few functions compared to other database management systems. SQLite authors see this as a feature rather than a problem, because SQLite has an extension mechanism in place.

There are a lot of SQLite extensions out there, but they are incomplete, inconsistent and scattered across the internet.

`sqlean` brings them all together, neatly packaged into domain modules, documented, tested, and built for Linux, Windows and macOS.

Here is what we've got right now:

-   [crypto](docs/crypto.md): secure hashes
-   [fileio](docs/fileio.md): read and write files
-   [fuzzy](docs/fuzzy.md): fuzzy string matching and phonetics
-   [ipaddr](docs/ipaddr.md): IP address manipulation
-   [json1](docs/json1.md): JSON functions
-   [math](docs/math.md): math functions
-   [re](docs/re.md): regular expressions
-   [spellfix](docs/spellfix.md): similarity search for large tables
-   [stats](docs/stats.md): math statistics
-   [text](docs/text.md): string functions
-   [unicode](docs/unicode.md): Unicode support
-   [uuid](docs/uuid.md): Universally Unique IDentifiers
-   [vsv](docs/vsv.md): CSV files as virtual tables

## Download

There are [precompiled binaries](https://github.com/nalgeon/sqlean/releases/latest) for every OS:

-   `*.dll` - for Windows
-   `*.so` - for Linux
-   `*.dylib` - for macOS

Binaries are 64-bit and require a 64-bit SQLite version. If you are using SQLite shell on Windows (`sqlite.exe`), its 64-bit version is available at https://github.com/nalgeon/sqlite.

## Usage

CLI usage:

```
sqlite> .load ./stats
sqlite> select median(value) from generate_series(1, 99);
```

IDE usage:

```
select load_extension('/path/to/extension/stats');
select median(value) from generate_series(1, 99);
```

In-app usage:

```python
import sqlite3

connection = sqlite3.connect(":memory:")
connection.enable_load_extension(True)
connection.load_extension("./stats.so")
connection.execute("select median(value) from generate_series(1, 99)")
connection.close()
```

You can specify any other supported extension instead of `stats`.

## License

Copyright (c) 2021 [Anton Zhiyanov](https://antonz.org/), [Contributors](https://github.com/nalgeon/sqlean/graphs/contributors) and [Third-party Authors](docs/third-party.md).

The software is available under the MIT License.

## Stay tuned

Follow [**@ohmypy**](https://twitter.com/ohmypy) on Twitter to keep up with new features 🚀