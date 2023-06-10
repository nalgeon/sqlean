# All the Missing SQLite Functions

SQLite has few functions compared to other database management systems. SQLite authors see this as a feature rather than a problem, because SQLite has an extension mechanism in place.

There are a lot of SQLite extensions out there, but they are incomplete, inconsistent and scattered across the internet. `sqlean` brings them together, neatly packaged into domain modules, documented, tested, and built for Linux, Windows and macOS.

We do not try to gather all the existing extensions into one giant pile — that would not be very useful. The goal is to create a well-thought set of domain modules with a convenient API. A kind of standard library for SQLite.

To achieve it, we split extensions that are too broad, merge the ones that are too narrow, refactor, add missing features, test, document, and do a ton of other small things.

🌟 **Call for feedback**! Please [share your use cases](https://github.com/nalgeon/sqlean/issues/81) to help the project focus on important aspects.

## Main Set

These are the most popular functions. They are tested, documented and organized into the domain modules with clear API.

Think of them as the extended standard library for SQLite:

-   [crypto](docs/crypto.md): hashing, encoding and decoding data
-   [define](docs/define.md): user-defined functions and dynamic sql
-   [fileio](docs/fileio.md): read and write files
-   [fuzzy](docs/fuzzy.md): fuzzy string matching and phonetics
-   [ipaddr](docs/ipaddr.md): IP address manipulation
-   [json1](docs/json1.md): JSON functions
-   [math](docs/math.md): math functions
-   [regexp](docs/regexp.md): regular expressions
-   [stats](docs/stats.md): math statistics
-   [text](docs/text.md): string functions
-   [unicode](docs/unicode.md): Unicode support
-   [uuid](docs/uuid.md): Universally Unique IDentifiers
-   [vsv](docs/vsv.md): CSV files as virtual tables

The single-file `sqlean` bundle contains all extensions from the main set (except `ipaddr`).

## Incubator

These extensions haven't yet made their way to the main set. They may be untested, poorly documented, too broad, too narrow, or without a well-thought API.

Think of them as candidates for the standard library:

-   [array](https://github.com/nalgeon/sqlean/issues/27#issuecomment-1004109889): one-dimensional arrays
-   [besttype](https://github.com/nalgeon/sqlean/issues/27#issuecomment-999732640): convert string value to numeric
-   [bloom](https://github.com/nalgeon/sqlean/issues/27#issuecomment-1002267134): a fast way to tell if a value is already in a table
-   [btreeinfo](https://github.com/nalgeon/sqlean/issues/27#issuecomment-1004896027), [memstat](https://github.com/nalgeon/sqlean/issues/27#issuecomment-1007421989), [recsize](https://github.com/nalgeon/sqlean/issues/27#issuecomment-999732907) and [stmt](https://github.com/nalgeon/sqlean/issues/27#issuecomment-1007654407): various database introspection features
-   [math2](https://github.com/nalgeon/sqlean/issues/27#issuecomment-999128539): additional math functions and bit arithmetics
-   [classifier](https://github.com/nalgeon/sqlean/issues/27#issuecomment-1001239676): binary classifier via logistic regression
-   [closure](https://github.com/nalgeon/sqlean/issues/27#issuecomment-1004931771): navigate hierarchic tables with parent/child relationships
-   [compress](https://github.com/nalgeon/sqlean/issues/27#issuecomment-1000937999) and [sqlar](https://github.com/nalgeon/sqlean/issues/27#issuecomment-1000938046): compress / uncompress data
-   [cron](https://github.com/nalgeon/sqlean/issues/27#issuecomment-997427979): match dates against cron patterns
-   [dbdump](https://github.com/nalgeon/sqlean/issues/27#issuecomment-1006791300): export database as SQL
-   [decimal](https://github.com/nalgeon/sqlean/issues/27#issuecomment-1007348326), [fcmp](https://github.com/nalgeon/sqlean/issues/27#issuecomment-997482625) and [ieee754](https://github.com/nalgeon/sqlean/issues/27#issuecomment-1007375162): decimal and floating-point arithmetic
-   [envfuncs](https://github.com/nalgeon/sqlean/issues/27#issuecomment-997423609): read environment variables
-   [isodate](https://github.com/nalgeon/sqlean/issues/27#issuecomment-998138191): additional date and time functions
-   [path](https://github.com/nalgeon/sqlean/issues/27#issuecomment-1252243356): parsing and querying paths
-   [pearson](https://github.com/nalgeon/sqlean/issues/27#issuecomment-997417836): Pearson correlation coefficient between two data sets
-   [pivotvtab](https://github.com/nalgeon/sqlean/issues/27#issuecomment-997052157): pivot tables
-   [prefixes](https://github.com/nalgeon/sqlean/issues/27#issuecomment-1007464840): generate string prefixes
-   [rotate](https://github.com/nalgeon/sqlean/issues/27#issuecomment-1007500659): string obfuscation
-   [spellfix](https://github.com/nalgeon/sqlean/issues/27#issuecomment-1002297477): search a large vocabulary for close matches
-   [stats2](https://github.com/nalgeon/sqlean/issues/27#issuecomment-1000902666) and [stats3](https://github.com/nalgeon/sqlean/issues/27#issuecomment-1002703581): additional math statistics functions
-   [text2](https://github.com/nalgeon/sqlean/issues/27#issuecomment-1003105288): additional string functions
-   [uint](https://github.com/nalgeon/sqlean/issues/27#issuecomment-1001232670): natural string sorting and comparison
-   [unhex](https://github.com/nalgeon/sqlean/issues/27#issuecomment-997432989): reverse for `hex()`
-   [unionvtab](https://github.com/nalgeon/sqlean/issues/27#issuecomment-1007687162): union similar tables into one
-   [xmltojson](https://github.com/nalgeon/sqlean/issues/27#issuecomment-997018486): convert XML to JSON string
-   [zipfile](https://github.com/nalgeon/sqlean/issues/27#issuecomment-1001190336): read and write zip files
-   [zorder](https://github.com/nalgeon/sqlean/issues/27#issuecomment-1007733209): map multidimensional data to a single dimension

[Vote for your favorites](https://github.com/nalgeon/sqlean/issues/27)! We'll refactor and merge popular ones into the main set.

## Download

There are [precompiled binaries](https://github.com/nalgeon/sqlean/releases/latest) for every OS:

-   `sqlean-win-x64.zip` - for Windows
-   `sqlean-linux-x86.zip` - for Linux
-   `sqlean-macos-x86.zip` - for Intel-based macOS
-   `sqlean-macos-arm64.zip` - for Apple silicon (ARM-based) macOS

Binaries are 64-bit and require a 64-bit SQLite version. If you are using SQLite shell on Windows (`sqlite.exe`), its 64-bit version is available at https://github.com/nalgeon/sqlite.

Incubator extensions are [also available](https://github.com/nalgeon/sqlean/releases/tag/incubator).

## Installation and Usage

SQLite command-line interface:

```
sqlite> .load ./sqlean
sqlite> select median(value) from generate_series(1, 99);
```

See [How to Install an Extension](docs/install.md) for usage with IDE, Python, etc.

## Building from Source

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

Contributions are welcome! Submit your own or third-party extension to the incubator:

-   [How to submit your extension](https://github.com/nalgeon/sqlean/blob/incubator/docs/submit.md)
-   [How to submit a third-party extension](https://github.com/nalgeon/sqlean/blob/incubator/docs/external.md)

We want every extension to be self-contained. So we limit the project scope to extensions without external dependencies (other than the C standard library and SQLite itself).

Please note that we only accept extensions with permissive licenses (MIT License, Apache License etc) or public domain. Copyleft licenses like GPL won't do.

## License

Copyright 2021+ [Anton Zhiyanov](https://antonz.org/), [Contributors](https://github.com/nalgeon/sqlean/graphs/contributors) and [Third-party Authors](docs/third-party.md).

The software is available under the MIT License.

## Stay tuned

Follow [**@ohmypy**](https://twitter.com/ohmypy) on Twitter to keep up with new features 🚀
