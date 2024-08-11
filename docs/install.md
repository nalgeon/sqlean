# How to install an extension

The easiest way to try out Sqlean extensions is to use the [pre-bundled shell](shell.md). But if you prefer to use the standard SQLite shell, IDEs or software APIs — read on.

Examples below use the `stats` extension; you can specify any other supported extension. To load all extensions at once, use the single-file `sqlean` bundle.

-   [Download: Manually](#download-manually)
-   [Download: Package manager](#download-package-manager)
-   [Install: Command-line interface](#install-command-line-interface)
-   [Install: GUI database browser](#install-gui-database-browser)
-   [Install: rqlite](#install-rqlite)
-   [Install: Python](#install-python)
-   [Install: Node.js](#install-nodejs)
-   [Install: Browser JavaScript](#install-browser-javascript)
-   [Install: Go](#install-go)

## Download: Manually

There are [precompiled binaries](https://github.com/nalgeon/sqlean/releases/latest) for every OS:

-   `sqlean-win-x64.zip` - for Windows
-   `sqlean-linux-x86.zip` - for Linux (x86 CPU)
-   `sqlean-linux-arm64.zip` - for Linux (ARM CPU)
-   `sqlean-macos-x86.zip` - for Intel-based macOS
-   `sqlean-macos-arm64.zip` - for Apple silicon (ARM-based) macOS

Binaries are 64-bit and require a 64-bit SQLite version. If you are using SQLite shell on Windows (`sqlite.exe`), its 64-bit version is available at https://github.com/nalgeon/sqlite.

Other extensions are available for download from [sqlpkg.org](https://sqlpkg.org/).

## Download: Package manager

Personally, I'm not a fan of managing the extensions manually. I always tend to put them in different places and can't find them later. So I created [`sqlpkg`](https://github.com/nalgeon/sqlpkg-cli) — a package manager for SQLite extensions.

Use the `install` command to download the extension with `sqlpkg`:

```
sqlpkg install nalgeon/stats
```

`nalgeon/stats` is the ID of the extension, you can find it in the [package registry](https://sqlpkg.org/). Check it out — it has plenty of extensions!

`sqlpkg` installs all extensions in the special folder:

-   `%USERPROFILE%\.sqlpkg` on Windows
-   `~/.sqlpkg` on Linux/macOS

So for our `nalgeon/stats` extension it will be:

-   `C:\Users\anton\.sqlpkg\nalgeon\stats\stats.dll` on Windows
-   `/home/anton/.sqlpkg/nalgeon/stats/stats.so` on Linux
-   `/Users/anton/.sqlpkg/nalgeon/stats/stats.dylib` on macOS

Be sure to change the path accordingly in the examples below.

## Install: Command-line interface

SQLite CLI, also known as SQLite shell, is a console interface (`sqlite3.exe` on Windows, `sqlite3` on Linux/macOS).

Start it and load the extension with the `.load` command:

Windows:

```
.load c:/Users/anton/Downloads/stats
```

Linux/macOS:

```
.load /Users/anton/Downloads/stats
```

Now you can use the extension! For example, the `stats` extension adds the `median` and `generate_series` functions:

```sql
select median(value) from generate_series(1, 99);
```

**Note for macOS users**. macOS may disable unsigned binaries and prevent the extension from loading. To resolve this issue, remove the extension from quarantine by running the following command in Terminal:

```
xattr -d com.apple.quarantine /Users/anton/Downloads/stats.dylib
```

Also note that the "stock" SQLite CLI on macOS does not support extensions. Use the [custom build](https://github.com/nalgeon/sqlite).

## Install: GUI database browser

To load extension in SQLiteStudio, SQLiteSpy, DBeaver and other similar tools, use the `load_extension` function.

Windows:

```sql
select load_extension('c:\Users\anton\sqlite\stats');
```

Linux/macOS:

```sql
select load_extension('/Users/anton/Downloads/stats');
```

## Install: rqlite

To load extensions in [rqlite](https://www.rqlite.io), first download a suitable release (x86 for Linux is demonstrated below):

```bash
curl -L https://github.com/nalgeon/sqlean/releases/download/0.27.0/sqlean-linux-x86.zip -o sqlean.zip
```

Then pass the zipfile to rqlite when you start the rqlite node:

```bash
rqlited -extensions-path=sqlean.zip data
```

See full details on [rqlite.io](https://rqlite.io/docs/guides/extensions/).

## Install: Python

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

## Install: Node.js

Use the [`better-sqlite3`](https://github.com/WiseLibs/better-sqlite3) package.

Windows:

```js
const sqlite3 = require("better-sqlite3");
const db = new sqlite3(":memory:");
db.loadExtension(`c:\Users\anton\sqlite\stats`);
db.exec("select median(value) from generate_series(1, 99)");
db.close();
```

Linux/macOS:

```js
const sqlite3 = require("better-sqlite3");
const db = new sqlite3(":memory:");
db.loadExtension("/Users/anton/Downloads/stats");
db.exec("select median(value) from generate_series(1, 99)");
db.close();
```

## Install: Browser JavaScript

Use the [sqlean.js](https://github.com/nalgeon/sqlean.js) package:

```js
import sqlite3Init from "./sqlean.mjs";

async function init() {
    return await sqlite3Init({
        print: console.log,
        printErr: console.error,
    });
}

init().then((sqlite3) => {
    const db = new sqlite3.oo1.DB();
    db.exec("select median(value) from generate_series(1, 99)");
});
```

## Install: Go

Use the [`mattn/go-sqlite3`](https://github.com/mattn/go-sqlite3) package.

Windows:

```go
package main

import (
    "database/sql"
    "fmt"

    sqlite3 "github.com/mattn/go-sqlite3"
)

func main() {
    sql.Register("sqlite_ext",
        &sqlite3.SQLiteDriver{
            Extensions: []string{
                `c:\Users\anton\sqlite\stats`,
            },
        })

    db, err := sql.Open("sqlite_ext", ":memory:")
    db.Query("select median(value) from generate_series(1, 99)")
    db.Close()
}
```

Linux/macOS:

```go
package main

import (
    "database/sql"
    "fmt"

    sqlite3 "github.com/mattn/go-sqlite3"
)

func main() {
    sql.Register("sqlite_ext",
        &sqlite3.SQLiteDriver{
            Extensions: []string{
                "/Users/anton/Downloads/stats",
            },
        })

    db, err := sql.Open("sqlite_ext", ":memory:")
    db.Query("select median(value) from generate_series(1, 99)")
    db.Close()
}
```

Note that we use the same identifier `sqlite_ext` in the `sql.Register` and `sql.Open`.
