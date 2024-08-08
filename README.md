# All the missing SQLite functions

SQLite has few functions compared to other database management systems. SQLite authors see this as a feature rather than a problem, because SQLite has an extension mechanism in place.

There are a lot of SQLite extensions out there, but they are incomplete, inconsistent and scattered across the internet. Sqlean brings them together, neatly packaged into domain modules, documented, tested, and built for Linux, Windows and macOS.

We do not try to gather all the existing extensions into one giant pile — that would not be very useful. The goal is to create a well-thought set of domain modules with a convenient API. A kind of standard library for SQLite.

To achieve it, we split extensions that are too broad, merge the ones that are too narrow, refactor, add missing features, test, document, and do a ton of other small things.

[Main set](#main-set) •
[Other extensions](#other-extensions) •
[Downloading](#downloading) •
[Installation](#installation-and-usage) •
[Sister projects](#sister-projects) •
[Funding](#funding)

## Main set

These are the most popular functions. They are tested, documented and organized into the domain modules with clear API.

Think of them as the extended standard library for SQLite:

-   [crypto](docs/crypto.md): hashing, encoding and decoding data
-   [define](docs/define.md): user-defined functions and dynamic sql
-   [fileio](docs/fileio.md): read and write files
-   [fuzzy](docs/fuzzy.md): fuzzy string matching and phonetics
-   [ipaddr](docs/ipaddr.md): IP address manipulation
-   [math](docs/math.md): math functions
-   [regexp](docs/regexp.md): regular expressions
-   [stats](docs/stats.md): math statistics
-   [text](docs/text.md): string functions and Unicode
-   [time](docs/time.md): high-precision date/time
-   [uuid](docs/uuid.md): Universally Unique IDentifiers
-   [vsv](docs/vsv.md): CSV files as virtual tables

The single-file `sqlean` bundle contains all extensions from the main set.

## Other extensions

The scope of this project is limited to extensions without external dependencies (other than the C standard library and SQLite itself).

There are a lot of useful extensions that do not quite fit the scope for various reasons. To learn more about them, visit the [SQLite extension hub](https://sqlpkg.org/).

## Downloading

There are [precompiled binaries](https://github.com/nalgeon/sqlean/releases/latest) for every OS:

-   `sqlean-win-x64.zip` - for Windows
-   `sqlean-linux-x86.zip` - for Linux (x86 CPU)
-   `sqlean-linux-arm64.zip` - for Linux (ARM CPU)
-   `sqlean-macos-x86.zip` - for Intel-based macOS
-   `sqlean-macos-arm64.zip` - for Apple silicon (ARM-based) macOS

Binaries are 64-bit and require a 64-bit SQLite version. If you are using SQLite shell on Windows (`sqlite.exe`), its 64-bit version is available at https://github.com/nalgeon/sqlite.

Other extensions are available for download from [sqlpkg.org](https://sqlpkg.org/).

## Installation and usage

The easiest way to try out Sqlean extensions is to use the [pre-bundled shell](docs/shell.md). But you can also load them individually.

For example, using the SQLite command-line interface:

```
sqlite> .load ./sqlean
sqlite> select median(value) from generate_series(1, 99);
```

See [How to install an extension](docs/install.md) for usage with IDE, Python, JavaScript, etc.

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

## Contributing

This project only accepts bug fixes and minor improvements. If you'd like to contribute a new extension, please submit it to the [extension hub repo](https://github.com/nalgeon/sqlpkg/).

## Sister projects

Projects related to Sqlean:

-   [sqlpkg.org](https://sqlpkg.org/): a catalog of all SQLite extensions, with search and manual downloads.
-   [sqlime.org](https://sqlime.org/): an online SQLite playground for debugging and sharing SQL snippets.
-   [sqlpkg](https://github.com/nalgeon/sqlpkg-cli): an automated packager manager for SQLite (like `npm` for JS or `pip` for Python).
-   [sqlean.py](https://github.com/nalgeon/sqlean.py): a drop-in replacement for the default `sqlite3` module.
-   [sqlean.js](https://github.com/nalgeon/sqlean.js): browser-based SQLite with extensions.
-   [shell](https://github.com/nalgeon/sqlite): custom builds for SQLite and Sqlean shells.

## License

Copyright 2021-2024 [Anton Zhiyanov](https://antonz.org/), [Contributors](https://github.com/nalgeon/sqlean/graphs/contributors) and [Third-party Authors](docs/third-party.md).

The software is available under the MIT License.

## Funding

Sqlean is kindly sponsored by [Paul Harrington](https://github.com/phrrngtn). Thank you, Paul!

If you find Sqlean useful, consider sponsoring me on GitHub. It really helps to move the project forward.

♥ [Become a sponsor](https://github.com/sponsors/nalgeon) to support Sqlean.

★ [Subscribe](https://antonz.org/subscribe/) to stay on top of new features.
