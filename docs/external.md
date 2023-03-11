# Submit a third-party extension

Let's say you want to add an existing [eval](https://sqlite.org/src/file/ext/misc/eval.c) extension. Here is how to submit it to the incubator:

1. Fork the [incubator](https://github.com/nalgeon/sqlean/tree/incubator) branch.

2. Add some tests:

```sql
-- test/eval.sql

.load dist/eval

select '01', eval('select 42') = '42';
```

3. Add download step to the Makefile:

```Makefile
download-external:
	curl -L https://github.com/sqlite/sqlite/raw/branch-$(SQLITE_BRANCH)/ext/misc/eval.c --output src/eval.c
```

4. Add compilation and test steps to the Makefile:

```Makefile
compile-linux:
	gcc -fPIC -shared src/eval.c -o dist/eval.so

compile-windows:
	gcc -shared -I. src/eval.c -o dist/eval.dll

compile-macos:
	gcc -fPIC -dynamiclib -I src src/eval.c -o dist/eval.dylib

test-all:
	make test suite=eval
```

5. Compile for you OS and run tests to verify that everything is OK:

```shell
make prepare-dist
make download-external
make compile-linux
make test suite=eval
```

6. Submit a pull request with a short description:

> **eval**
>
> Dinamically runs arbitrary SQL statements.
>
> Created by [D. Richard Hipp](https://sqlite.org/src/file/ext/misc/eval.c), Public Domain.
