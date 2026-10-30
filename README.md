# All the missing SQLite functions

SQLite has few functions compared to other database management systems. SQLite authors see this as a feature rather than a problem, because SQLite has an extension mechanism in place.

There are a lot of SQLite extensions out there, but they are incomplete, inconsistent and scattered across the internet. Sqlean brings them together, neatly packaged into domain modules, documented, tested, and built for Linux, Windows and macOS.

We do not try to gather all the existing extensions into one giant pile — that would not be very useful. The goal is to create a well-thought set of domain modules with a convenient API. A kind of standard library for SQLite.

To achieve it, we split extensions that are too broad, merge the ones that are too narrow, refactor, add missing features, test, document, and do a ton of other small things.

[Extensions](#extensions) •
[Downloading](#downloading) •
[Installation](#installation-and-usage) •
[Support](#support)

## Extensions

The functions are organized into the domain modules with clear API. Think of them as the extended standard library for SQLite:

- [crypto](docs/crypto.md): hashing, encoding and decoding data
- [define](docs/define.md): user-defined functions and dynamic sql
- [fileio](docs/fileio.md): read and write files
- [fuzzy](docs/fuzzy.md): fuzzy string matching and phonetics
- [ipaddr](docs/ipaddr.md): IP address manipulation
- [math](docs/math.md): math functions
- [regexp](docs/regexp.md): regular expressions
- [stats](docs/stats.md): math statistics
- [text](docs/text.md): string functions and Unicode
- [time](docs/time.md): high-precision date/time
- [uuid](docs/uuid.md): Universally Unique IDentifiers
- [vsv](docs/vsv.md): CSV files as virtual tables

The single-file `sqlean` bundle contains all listed extensions.

## Downloading

There are [precompiled binaries](https://github.com/nalgeon/sqlean/releases/latest) for every OS:

- `sqlean-win-x64.zip` - Windows (Intel/AMD x64 CPU)
- `sqlean-linux-x64.zip` - Linux (Intel/AMD x64 CPU)
- `sqlean-linux-arm64.zip` - Linux (ARM CPU)
- `sqlean-macos-x64.zip` - Intel-based macOS
- `sqlean-macos-arm64.zip` - Apple silicon (ARM-based) macOS

Binaries are 64-bit and require a 64-bit SQLite version.

## Installation and usage

The easiest way to try out Sqlean extensions is to load them in the SQLite command-line interface:

```
sqlite> .load ./sqlean
sqlite> select stats_median(value) from stats_seq(1, 99);
```

See [How to install an extension](docs/install.md) for usage with IDE, Python, JavaScript, etc.

## Building from source

Download the dependencies:

```
make prepare-dist
make download-sqlite
```

Then build for your OS:

```
make build
```

You'll find the compiled extensions in the `dist` folder.

## Contributing

If there's a feature you need that's missing, please open an issue to discuss it. Only open issues for real use cases you actually have, not for hypothetical situations or features that would just be nice to have.

If you find a bug, open an issue or send a PR. Please don't send PRs with new features — open an issue instead.

## License

Copyright 2021-2026 [Anton Zhiyanov](https://antonz.org/), [Contributors](https://github.com/nalgeon/sqlean/graphs/contributors) and [Third-party Authors](docs/third-party.md).

The software is available under the MIT License.

## Support

Sqlean is mostly a [one-man](https://antonz.org/) project, not backed by a VC fund or anything.

If you find Sqlean useful, please star it on GitHub and spread the word among your peers. It really helps to move the project forward.
