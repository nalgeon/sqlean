# SQLite extensions incubator

The incubator contains SQLite extensions which haven't yet made their way to the main set. They may be untested, poorly documented, too broad, too narrow, or without a well-thought API. Think of them as candidates for the standard library.

See the [full extension list](https://github.com/nalgeon/sqlean/issues/27) and vote for your favorites! We'll refactor and merge popular ones into the main set.

Meanwhile, you can download compiled incubator extensions as they are and use them if you like.

## Download

There are [precompiled binaries](https://github.com/nalgeon/sqlean/releases/tag/incubator) for every OS:

-   `*.dll` - for Windows
-   `*.so` - for Linux
-   `*.dylib` - for macOS

Binaries are 64-bit and require a 64-bit SQLite version. If you are using SQLite shell on Windows (`sqlite.exe`), its 64-bit version is available at https://github.com/nalgeon/sqlite.

## Usage

CLI usage:

```
sqlite> .load ./cbrt
sqlite> select cbrt(27);
3.0
```

IDE usage:

```
select load_extension('/path/to/extension/cbrt');
select cbrt(27);
```

In-app usage:

```python
import sqlite3

connection = sqlite3.connect(":memory:")
connection.enable_load_extension(True)
connection.load_extension("./cbrt.so")
connection.execute("select cbrt(27)")
connection.close()
```

You can specify any other supported extension instead of `cbrt`.

## Submit your favorite extension

Contributions are welcome! Submit your own or third-party extension to the incubator:

- [How to submit your extension](docs/submit.md)
- [How to submit a third-party extension](docs/external.md)

Please note that we accept only extensions with permissive licenses (MIT License, Apache License etc) or public domain. Copyleft licenses like GPL won't do.

## License

Copyright (c) 2021 [Anton Zhiyanov](https://antonz.org/), [Contributors](https://github.com/nalgeon/sqlean/graphs/contributors) and [Third-party Authors](https://github.com/nalgeon/sqlean/issues/27).

The software is available under the MIT License.

## Stay tuned

Follow [**@ohmypy**](https://twitter.com/ohmypy) on Twitter to keep up with new features 🚀
