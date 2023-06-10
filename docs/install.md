# How to Install an Extension

## 1. Download

There are [precompiled binaries](https://github.com/nalgeon/sqlean/releases/latest) for every OS:

-   `sqlean-win-x64.zip` - for Windows
-   `sqlean-linux-x86.zip` - for Linux
-   `sqlean-macos-x86.zip` - for Intel-based macOS
-   `sqlean-macos-arm64.zip` - for Apple silicon (ARM-based) macOS

Binaries are 64-bit and require a 64-bit SQLite version. If you are using SQLite shell on Windows (`sqlite.exe`), its 64-bit version is available at https://github.com/nalgeon/sqlite.

Incubator extensions are [also available](https://github.com/nalgeon/sqlean/releases/tag/incubator).

## 2. Load and Use

Examples below use the `stats` extension; you can specify any other supported extension. To load all extensions at once, use the single-file `sqlean` bundle.

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

Python:

```python
import sqlite3

connection = sqlite3.connect(":memory:")
connection.enable_load_extension(True)
connection.load_extension("./stats")
connection.execute("select median(value) from generate_series(1, 99)")
connection.close()
```

Node.js, using [better-sqlite3](https://github.com/WiseLibs/better-sqlite3):

```js
const sqlite3 = require("better-sqlite3");
const db = new sqlite3(":memory:");
db.loadExtension("./stats");
db.exec("select median(value) from generate_series(1, 99)");
db.close();
```
