# How to Install an Extension

The easiest way to try out `sqlean` extensions is to use the [pre-bundled shell](shell.md). But if you prefer to use the standard SQLite shell, IDEs or software APIs — read on.

## 1. Downloading

There are [precompiled binaries](https://github.com/nalgeon/sqlean/releases/latest) for every OS:

-   `sqlean-win-x64.zip` - for Windows
-   `sqlean-linux-x86.zip` - for Linux
-   `sqlean-macos-x86.zip` - for Intel-based macOS
-   `sqlean-macos-arm64.zip` - for Apple silicon (ARM-based) macOS

Binaries are 64-bit and require a 64-bit SQLite version. If you are using SQLite shell on Windows (`sqlite.exe`), its 64-bit version is available at https://github.com/nalgeon/sqlite.

Incubator extensions are [also available](https://github.com/nalgeon/sqlean/releases/tag/incubator).

## 2. Usage

Examples below use the `stats` extension; you can specify any other supported extension. To load all extensions at once, use the single-file `sqlean` bundle.

### Command-line or IDE

SQLite command-line interface (CLI, aka 'sqlite3.exe' on Windows):

```
sqlite> .load ./stats
sqlite> select median(value) from generate_series(1, 99);
```

IDE, e.g. SQLiteStudio, SQLiteSpy or DBeaver:

```
select load_extension('c:\Users\anton\sqlite\stats.dll');
select median(value) from generate_series(1, 99);
```

**Note for macOS users**. macOS may disable unsigned binaries and prevent the extension from loading. To resolve this issue, remove the extension from quarantine by running the following command in Terminal (replace `/path/to/folder` with an actual path to the folder containing the extension):

```
xattr -d com.apple.quarantine /path/to/folder/stats.dylib
```

Also note that the "stock" SQLite CLI on macOS does not support extensions. Use the [custom build](https://github.com/nalgeon/sqlite).

### Python

Install the `sqlean.py` package, which is a drop-in replacement for the default `sqlite3` module:

```
pip install sqlean.py
```

All extensions from the main set are already enabled:

```python
import sqlean as sqlite3

conn = sqlite3.connect(":memory:")
conn.execute("select median(value) from generate_series(1, 99)")
conn.close()
```

You can also use the default `sqlite3` module and load extensions manually:

```python
import sqlite3

conn = sqlite3.connect(":memory:")
conn.enable_load_extension(True)
conn.load_extension("./stats")
conn.execute("select median(value) from generate_series(1, 99)")
conn.close()
```

**Note for macOS users**. "Stock" SQLite on macOS does not support extensions, so the default `sqlite3` module won't work. Use the `sqlean.py` package.

### Node.js

Using [better-sqlite3](https://github.com/WiseLibs/better-sqlite3):

```js
const sqlite3 = require("better-sqlite3");
const db = new sqlite3(":memory:");
db.loadExtension("./stats");
db.exec("select median(value) from generate_series(1, 99)");
db.close();
```
