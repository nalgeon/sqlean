# Sqlean shell

`sqlean` shell is the SQLite command-line interface bundled with [essential extensions](../README.md) ranging from regular expressions and math statistics to file I/O and dynamic SQL.

```
  sqlean shell =
┌───────────────────────────┐
│ sqlite shell              │
├ + ────────────────────────┤
│ crypto   ipaddr   text    │
│ define   math     unicode │
│ fileio   regexp   uuid    │
│ fuzzy    stats    vsv     │
└───────────────────────────┘
```

## 1. Downloading

Builds are available for every OS:

-   `sqlean.exe` - for Windows
-   `sqlean-ubuntu` - for Ubuntu (and other Debian-based distributions)
-   `sqlean-macos` - for macOS (supports both Intel and Apple processors)

[⬇️ Download](https://github.com/nalgeon/sqlite/releases/latest)

## 2. Usage

The shell works like the standard SQLite shell, except that all `sqlean` extensions work out of the box without explicit loading:

```
SQLite version 3.42.0 2023-05-16 12:36:15
Enter ".help" for usage hints.
Connected to a transient in-memory database.
Use ".open FILENAME" to reopen on a persistent database.
sqlean> select median(value) from generate_series(1, 99);
50.0
```

**Note for macOS users**. macOS disables unsigned binaries and prevents the `sqlean` shell from running. To resolve this issue, remove the build from quarantine by running the following command in Terminal (replace `/path/to/folder` with an actual path to the folder containing the `sqlean-macos` binary):

```
chmod +x /path/to/folder/sqlean-macos
xattr -d com.apple.quarantine /path/to/folder/sqlean-macos
```

And then run the shell:

```
$ /path/to/folder/sqlean-macos
SQLite version 3.42.0 2023-05-16 12:36:15
Enter ".help" for usage hints.
Connected to a transient in-memory database.
Use ".open FILENAME" to reopen on a persistent database.
sqlean> █
```
