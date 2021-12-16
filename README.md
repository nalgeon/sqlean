# SQLite extensions incubator

The incubator contains SQLite extensions which haven't yet made their way to the main extension set. They may be untested, poorly documented, and without a well-thought API.

See the full extension list and vote for your favorites! We'll merge the most popular ones into the main extension set.

Meanwhile, you can use download compiled incubator extensions as they are and use them if you like.

## Download

There are [precompiled binaries](https://github.com/nalgeon/sqlean/releases/tag/incubator) for every OS:

-   `*.dll` - for Windows
-   `*.so` - for Linux
-   `*.dylib` - for macOS

Binaries are 64-bit and require a 64-bit SQLite version. If you are using SQLite shell on Windows (`sqlite.exe`), its 64-bit version is available at https://github.com/nalgeon/sqlite.

## Usage

CLI usage:

```
sqlite> .load ./hello
sqlite> select hello('world');
```

IDE usage:

```
select load_extension('/path/to/extension/hello');
select median(value) from generate_series(1, 99);
```

In-app usage:

```python
import sqlite3

connection = sqlite3.connect(":memory:")
connection.enable_load_extension(True)
connection.load_extension("./hello.so")
connection.execute("select hello('world')")
connection.close()
```

You can specify any other supported extension instead of `hello`.

## License

Copyright (c) 2021 [Anton Zhiyanov](https://antonz.org/), [Contributors](https://github.com/nalgeon/sqlean/graphs/contributors) and [Third-party Authors]([docs/third-party.md](https://github.com/nalgeon/sqlean/releases/tag/incubator)).

The software is available under the MIT License.

## Stay tuned

Follow [**@ohmypy**](https://twitter.com/ohmypy) on Twitter to keep up with new features 🚀