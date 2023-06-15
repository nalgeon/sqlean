# Submit your extension

Let's say you have written an extension to find the cube root of a number. Here is how to submit it to the incubator:

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

3. Add some tests:

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

4. Add compilation and test steps to the Makefile:

```Makefile
compile-linux:
	make compile-linux-extension name=cbrt

compile-windows:
	make compile-windows-extension name=cbrt

compile-macos:
	make compile-macos-extension name=cbrt

test-all:
	make test suite=cbrt
```

5. Compile for you OS and run tests to verify that everything is OK:

```shell
make prepare-dist
make download-sqlite
make compile-linux-extension name=cbrt
make test suite=cbrt
```

6. Submit a pull request with a short description:

> **cbrt**
>
> Cube root function.
>
> Created by [Anton Zhiyanov](https://github.com/nalgeon), MIT License.

## Code style

Please follow these rules:

-   Auto-format the code according to the provided [clang-format specification](../.clang-format).
-   Use descriptive names. No `z`, `cc`, `pSvRt` and other cryptic variables, please.
-   Use `snake_case` for variable and function names.
