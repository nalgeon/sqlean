# SQLite Extensions Incubator

The incubator contains SQLite extensions which haven't yet made their way to the main set. They may be untested, poorly documented, too broad, too narrow, or without a well-thought API. Think of them as candidates for the standard library.

See the [full extension list](https://github.com/nalgeon/sqlean/issues/27) and vote for your favorites! We'll refactor and merge popular ones into [the main set](https://github.com/nalgeon/sqlean).

Meanwhile, you can download compiled incubator extensions as they are and use them if you like.

## Downloading

There are [precompiled binaries](https://github.com/nalgeon/sqlean/releases/tag/incubator) for every OS:

-   `*.dll` - for Windows
-   `*.so` - for Linux
-   `*.dylib` - for macOS

Binaries are 64-bit and require a 64-bit SQLite version. If you are using SQLite shell on Windows (`sqlite.exe`), its 64-bit version is available at https://github.com/nalgeon/sqlite.

## Installation and usage

Examples below use the `cbrt` extension; you can specify any other supported extension.

SQLite command-line interface (CLI, aka 'sqlite3.exe' on Windows):

```
sqlite> .load ./cbrt
sqlite> select cbrt(27);
```

IDE, e.g. SQLiteStudio, SQLiteSpy or DBeaver:

```
select load_extension('/path/to/extension/cbrt');
select cbrt(27);
```

Python:

```python
import sqlite3

conn = sqlite3.connect(":memory:")
conn.enable_load_extension(True)
conn.load_extension("./cbrt.so")
conn.execute("select cbrt(27)")
conn.close()
```

Node.js, using [better-sqlite3](https://github.com/WiseLibs/better-sqlite3):

```js
const sqlite3 = require("better-sqlite3");
const db = new sqlite3(":memory:");
db.loadExtension("./cbrt");
db.exec("select cbrt(27)");
db.close();
```

## Building from source

Download the dependencies:

```
make prepare-dist
make download-sqlite
make download-external
```

Then build for your OS (choose one of the following):

```
make compile-linux
make compile-windows
make compile-macos
```

You'll find the compiled extensions in the `dist` folder.

## Submitting your favorite extension

Contributions are welcome! Submit your own or third-party extension to the incubator:

-   [How to submit your extension](docs/submit.md)
-   [How to submit a third-party extension](docs/external.md)

We want every extension to be self-contained. So we limit the project scope to extensions without external dependencies (other than the C standard library and SQLite itself).

Please note that we accept only extensions with permissive licenses (MIT License, Apache License etc) or public domain. Copyleft licenses like GPL won't do.

## License

Copyright (c) 2021+ [Anton Zhiyanov](https://antonz.org/), [Contributors](https://github.com/nalgeon/sqlean/graphs/contributors) and [Third-party Authors](https://github.com/nalgeon/sqlean/issues/27).

The software is available under the MIT License.

## Stay tuned

Follow [**@ohmypy**](https://twitter.com/ohmypy) on Twitter to keep up with new features 🚀
