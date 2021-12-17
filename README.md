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

## Submit your own extension

Let's say, you have written an extension to find the cube root of a number. Here is how to submit it to the incubator:

1. Fork the [incubator](https://github.com/nalgeon/sqlean/tree/incubator) branch.

2. Add the extension source:

```c
// src/cbrt.c
// Cube root function.

#include <assert.h>
#include <math.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

// cbrt(X)
// Calculates cube root of X for integer or real X.
// Returns NULL for other types.
static void sqlite3_cbrt(sqlite3_context* context, int argc, sqlite3_value** argv) {
    // implementation
}

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_cbrt_init(sqlite3* db, char** pzErrMsg, const sqlite3_api_routines* pApi) {
    // initialization
}
```

3. Add some tests

```sql
-- test/cbrt.sql

.load dist/cbrt

select '01', cbrt(27) = 3.0;
select '02', cbrt(-27) = -3.0;
select '03', cbrt(8.0) = 2.0;
select '04', cbrt(1) = 1;
select '05', cbrt(0) = 0;
select '06', cbrt('whatever') is null;
```

4. Add compilation steps to the Makefile:

```Makefile
compile-linux:
	gcc -fPIC -shared src/cbrt.c -o dist/cbrt.so -lm

compile-windows:
	gcc -shared -I. src/cbrt.c -o dist/cbrt.dll -lm

compile-macos:
	gcc -fPIC -dynamiclib -I src src/cbrt.c -o dist/cbrt.dylib -lm
```

5. Submit a pull request with a short description:

> **cbrt**
>
> Cube root function.
>
> Created by [Anton Zhiyanov](https://github.com/nalgeon), MIT License.

## License

Copyright (c) 2021 [Anton Zhiyanov](https://antonz.org/), [Contributors](https://github.com/nalgeon/sqlean/graphs/contributors) and [Third-party Authors](<[docs/third-party.md](https://github.com/nalgeon/sqlean/releases/tag/incubator)>).

The software is available under the MIT License.

## Stay tuned

Follow [**@ohmypy**](https://twitter.com/ohmypy) on Twitter to keep up with new features 🚀
