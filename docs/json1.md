# json1: JSON Handling in SQLite

This is the 'native' SQLite [JSON1 extension](https://sqlite.org/json1.html).
It's often compiled into SQLite build, but in case your build doesn't include it - I've compiled it separately.

## Installation and Usage

SQLite command-line interface:

```
sqlite> .load ./json1
sqlite> select json_object("answer", 42);
```

See [How to Install an Extension](install.md) for usage with IDE, Python, etc.

[⬇️ Download](https://github.com/nalgeon/sqlean/releases/latest) •
[✨ Explore](https://github.com/nalgeon/sqlean) •
[🚀 Follow](https://twitter.com/ohmypy)
