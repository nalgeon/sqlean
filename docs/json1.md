# json1: JSON Functions

This is the 'native' SQLite [JSON1 extension](https://sqlite.org/json1.html).
It's often compiled into SQLite build, but in case your build doesn't include it - I've compiled it separately.

Download: [Download](https://github.com/nalgeon/sqlean/releases/latest)

## Usage

```
sqlite> .load ./json1
sqlite> select json_object("answer", 42);
```
