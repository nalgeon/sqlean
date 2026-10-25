# Extension guidelines

What a well-built Sqlean extension looks like. Use this as the target when writing a new extension or refactoring an existing one.

The reference implementation is `time` (`src/time/`, `test/time.sql`, `test/time/`, `docs/time.md`). When this guide doesn't cover something, do what `time` does.

[Layout](#layout) •
[Domain layer](#domain-layer) •
[Binding layer](#binding-layer) •
[Entry point](#entry-point) •
[SQL API design](#sql-api-design) •
[Tests](#tests) •
[Documentation](#documentation) •
[Refactoring checklist](#refactoring-checklist)

## Layout

```text
src/sqlite3-<module>.c        loadable-extension entry point (boilerplate only)
src/<module>/extension.h      declares <module>_init(db), nothing else
src/<module>/extension.c      SQLite bindings: argument decoding, result encoding, registration
src/<module>/<name>.h         public domain API, fully documented (time: timex.h)
src/<module>/<concept>.c      domain logic, one file per concept (time: time.c, duration.c)
src/<module>/common.h         optional: internal helpers shared by domain files (static inline)
test/<module>.sql             SQL-level tests
test/<module>/<concept>.test.c  C-level tests of the domain layer
docs/<module>.md              user documentation
```

Every file starts with the license header, an attribution block if the code is derived from elsewhere, and a one-line description:

```c
// Copyright (c) 2024 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Based on Go's time package, BSD 3-Clause License
// https://github.com/golang/go

// Duration methods.
```

Header guards use `<MODULE>_<FILE>_H` (`TIME_COMMON_H`, `TIME_EXTENSION_H`) and close with `#endif /* ..._H */`.

## Domain layer

The domain layer is where the actual work happens. It's plain C with no SQLite dependency: no `sqlite3ext.h`, no `sqlite3_*` calls. That lets you test it directly from C, reason about it in isolation, and reuse it.

### Types and API shape

Model domain values as small value types passed by value: `Time` is `{int64_t sec; int32_t nsec;}`, and `Duration` is a `typedef int64_t`. Document each type's representation, unit, and range where it's declared.

Use fixed-width integer types (`int64_t`, `int32_t`, `uint8_t`) for anything that is stored, serialized, or can get large. Use plain `int` only for small, naturally bounded values, such as calendar fields.

Use `enum` for closed sets (`enum Month`, `enum Weekday`) and named constants for magic values (`TIMEX_UTC`, `TIMEX_BLOB_SIZE`, `Hour`).

Name functions `<prefix>_<verb>[_<object>]`, with one prefix per type: `time_get_year`, `time_to_unix`, `time_fmt_iso`, `dur_round`. Constructors return the value (`Time time_date(...)`). Accessors with several outputs take out-parameters (`time_get_date(t, &year, &month, &day)`).

Group the public header into commented sections (`// Constructors.`, `// Arithmetic.`, ...). Use the same sections as `#pragma region` blocks in the `.c` file, as section comments in the C test's `main`, and as headings in the docs.

Give every public function a doc comment in the header that starts with its name and says what it returns ("`time_add returns the time t+d.`"). Use the same comment above the definition, and add details there: edge cases, rounding behavior, clamping.

### Every input is valid input

A domain function must have defined, documented behavior for every possible argument value. SQL users can pass anything, and the binding layer only checks types, not ranges. Pick one of these strategies and document it:

Normalize out-of-range components: `time_date(2011, 10, 32, ...)` is November 1.

Saturate on overflow: `time_add` clamps to the min/max `Time`, and `dur_round` clamps to `MIN_DURATION`/`MAX_DURATION`.

Leave the input unchanged for meaningless parameters: `dur_truncate(d, m)` with `m <= 0` returns `d`.

Report failure explicitly when there is no sensible result, such as unparseable or corrupt input: return `bool` and pass the result through an out-parameter (`bool time_parse(const char* s, Time* out)`). The binding layer turns a failure into SQL NULL. Don't use an in-band sentinel that is also a valid result: if `time_parse` returned the zero time for garbage, that couldn't be told apart from parsing `0001-01-01`.

Crashes, garbage output, and undefined behavior are never acceptable, whatever the input.

### No undefined behavior

Sqlean runs inside other people's processes, so UB counts as a security bug. The test suites run under ASan and UBSan, and they must pass. Use `make sanitize-gcc`, which runs them on Linux with GCC and leak detection (`make ctest-all-sanitize` and `make test-all-sanitize` run them on the host). Patterns from `time`:

Wrapping arithmetic goes through unsigned types. Compute in `uint64_t`, then convert back with a fully defined helper (`uint64_to_int64` in `time/common.h`) instead of a cast.

Widen before negating or subtracting near the limits. `-(int64_t)year` instead of `-year`, because `-INT_MIN` overflows. `time_date` normalizes `int` fields in `int64_t` for the same reason.

Detect overflow by comparison after an unsigned computation, then clamp (see `dur_round`).

Use unsigned shifts for byte packing. Shift `uint64_t`/`uint32_t`, never a negative signed value.

Explain these spots in a comment. The comment should say why the obvious code would be UB, so nobody "simplifies" it back.

### Memory and buffers

Prefer no heap allocation. Return values by value, or write into caller-provided buffers with an explicit size (`size_t time_fmt_iso(char* buf, size_t size, ...)`, which has `snprintf` semantics and returns the length).

If a function writes into a fixed buffer, export a constant that is always big enough (`TIMEX_FMT_BUF_SIZE`). Next to the constant, write down how its value was derived, and add a C test that checks the worst case against it.

When allocation can't be avoided, the header must say who owns the result and how to free it.

### Serialization

Binary formats exposed to SQL (such as the time BLOB) need a version byte, an explicit byte order, and a fixed size constant. Validate them on decode: an unknown version or out-of-range fields are reported as a decode failure, never decoded to garbage.

### Internal code

Everything not declared in the public header is `static`. The bundle links all modules into one binary, so a stray non-static symbol can collide with a symbol from another module.

No mutable global state. An extension can be loaded into many connections on many threads at once, so file-level and function-level `static` variables must be `const`. Per-connection state goes in the `sqlite3_create_function` user data, and per-statement state goes in auxdata.

Code compiles cleanly with `-Wall -Wextra -Werror` (the `CTEST_FLAGS` in the `Makefile`) and is formatted with `clang-format` (`.clang-format`).

Helpers shared between domain files go in an internal header as `static inline` (`time/common.h`).

Put platform differences in one place behind `#if` and keep them out of the logic (`timespec_now` in `time.c`). Prefer the C11 standard API, and fall back to POSIX, then Windows.

### Ported code

When writing a new module, or replacing old code that is hard to follow, first look for a proven implementation in Go: the standard library, or a well-known third-party package. Go code is easy to port to C, and it's usually more readable and better tested than C code found in the wild. The `crypto` hashes are ported this way: MD5 and SHA from the Go standard library, XXH64 from `cespare/xxhash`, and XXH3 from `zeebo/xxh3`.

When porting:

- Check the license. BSD and MIT are fine; copyleft is not. Keep the original copyright in the file header and add the source to `docs/third-party.md`.
- Port the portable Go code (the `*_generic.go` files in the standard library), not the assembly versions.
- Go checks bounds and panics on bad indexes, C doesn't. Every rule from [No undefined behavior](#no-undefined-behavior) and [Memory and buffers](#memory-and-buffers) still applies to the ported code.
- Verify the port against a trusted reference implementation over many input sizes and split points, not just a few test vectors, and run it under the sanitizers.
- Accept that a portable port may be slower than hand-optimized (e.g. SIMD) C. Simple, correct code is worth more here, unless there's a real performance need.

Ported code follows the original's semantics exactly. Comment deliberate parity ("as Go does") and every intentional deviation, and credit the source in the module docs.

## Binding layer

`extension.c` converts between `sqlite3_value`s and domain types, and nothing else. A wrapper should read like "decode args → call one domain function → encode the result". If a wrapper contains real logic, move that logic into the domain layer.

### Wrapper shape

```c
// time_add(t, d)
static void fn_add(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    // decode and type-check each argument, in order
    // call the domain function
    // set the result
}
```

Name wrappers `fn_<name>` and mark them `static`. The comment above each one lists the SQL signature(s) it implements, with optional args in brackets: `// time_date(year, month, day[, hour, min, sec[, nsec[, offset_sec]]])`.

`assert` the arities the wrapper was registered with.

Default values for optional args are assigned explicitly, next to their `argc` check.

Function families with identical signatures share one wrapper. Pass the domain function through `sqlite3_user_data` (`fn_extract`, `fn_convert`, `fn_format`). The pointer's type must exactly match the function's real signature: calling a function through an incompatible pointer type is UB, even if the return types are all integers (`enum Month`, `bool` and `int` are different types). When they don't match, add small `static` adapters with the shared signature (`get_month`, `after` in `time/extension.c`).

Pass constants by pointer to a `static const` value, never by casting the value to `void*`: an `int64_t` doesn't fit into a pointer on 32-bit platforms (`fn_dur_const`).

Put repeated result encoding in a helper (`result_blob`), and repeated argument decoding as well. The decoding helpers return `bool` and pass the value through an out-parameter, so a wrapper reads `if (!arg_time(argv[0], &t)) { NULL }` (`arg_time`, `arg_int64`, `arg_int`). Spec-argument helpers set the error themselves (`arg_field`).

### Argument handling

Follow what SQLite's own built-in functions do. Bad _data_ gives NULL, never an error, so one bad row doesn't abort a whole query. A bad _query_ raises an error, so a typo doesn't silently turn into NULLs. This is what the date/time, math, `unhex`, JSON, and REGEXP functions do (checked against SQLite 3.47, the version pinned in the `Makefile`):

| Input                             | SQLite built-ins                                            | Sqlean rule                        |
| --------------------------------- | ----------------------------------------------------------- | ---------------------------------- |
| any argument is NULL              | `upper(null)`, `date(null)`, `sqrt(null)` → NULL            | NULL                               |
| convertible type                  | `upper(123)` → `'123'`, `sqrt('4')` → `2.0`                 | convert, then proceed              |
| unconvertible type                | `sqrt('abc')`, `sqrt(x'01')`, `date(x'01')` → NULL          | NULL                               |
| malformed value of the right type | `date('garbage')`, `date(d, 'bogus')`, `unhex('zz')` → NULL | NULL                               |
| out of domain                     | `sqrt(-1)`, `ln(0)` → NULL                                  | NULL, unless the domain normalizes |
| out-of-range fields               | `date('2011-02-30')` → `'2011-03-02'`                       | normalize if the domain defines it |

SQLite raises errors only in a few cases, and Sqlean does the same:

- Malformed spec arguments: a spec argument says _how_ to process the data (a pattern, a path, a field or unit name, a format, an option). It's almost always a literal in the query, so an invalid one is a bug in the query. `'a' regexp '('` raises "unmatched '('", `json_extract(j, 'bad')` raises "bad JSON path", and `like ... escape 'xy'` raises an error. Messages name the argument and the problem: `"unknown field: fortnight"`. (SQLite's date modifiers are the one exception: `date(d, 'bogus')` is NULL.)
- Resource failures: out of memory (`sqlite3_result_error_nomem`), or a result too big (`sqlite3_result_error_toobig`).
- A result that can't be represented: `abs(-9223372036854775808)` raises "integer overflow". Use this only when the domain doesn't define saturation (as `time_add` does) or normalization.
- Malformed input to a structure-validating function, when a NULL-returning validator exists alongside it: `json('{')` raises an error, but `json_valid('{')` returns `0`.

The legacy core functions that silently coerce garbage to zero (`abs('abc')` → `0.0`, `round('abc')` → `0.0`) are _not_ the model. Use the NULL-returning behavior of the newer functions.

In practice, each wrapper does this, in order:

1. If any argument is NULL, return NULL (`has_null`).
2. Decode each argument with the conversion rules below. If a data argument can't be converted, or has the wrong size, return NULL. If a spec argument is invalid, raise an error.
3. Call the domain function. If it reports failure, return NULL. Otherwise encode the result.

Conversion rules:

- Integer parameters: accept a value whose `sqlite3_value_numeric_type` is `SQLITE_INTEGER`. This applies numeric affinity, so `'42'` works and `'abc'` gives NULL. Read it with `sqlite3_value_int64`, never `sqlite3_value_int`, which silently truncates to 32 bits. Range-check before narrowing, or make the domain function accept the full range.
- Real parameters: accept `SQLITE_INTEGER` or `SQLITE_FLOAT` after `sqlite3_value_numeric_type`, as the math functions do.
- Text parameters: accept any non-BLOB value. Read the text with `sqlite3_value_text`, then its length with `sqlite3_value_bytes`, in that order. If the domain takes NUL-terminated strings, treat embedded NULs as malformed (`fn_parse` compares `strlen` with `bytes`). Text is UTF-8. When a function counts or indexes text, it works in characters, as `length()` and `substr()` do, and the docs say so. Byte-oriented functions say that explicitly.
- Blob parameters of a specific format (such as a time value): accept only `SQLITE_BLOB` of the exact size, and let the domain decoder validate the contents.

### Results and memory

Use `SQLITE_TRANSIENT` for stack buffers and `SQLITE_STATIC` for string literals. Hand heap results to SQLite with `sqlite3_free` as the destructor, so they aren't copied, if they were allocated with `sqlite3_malloc`.

Allocations in the binding layer use `sqlite3_malloc64`/`sqlite3_free`. Check every allocation, in both layers. On failure, call `sqlite3_result_error_nomem` and free what was allocated so far.

When a spec argument is expensive to prepare, such as compiling a regex pattern, cache the prepared form per statement with `sqlite3_get_auxdata`/`sqlite3_set_auxdata` instead of preparing it for every row.

### Registration

Describe the functions in a `static const` table, and register them in a loop that checks each return code:

```c
static const Function functions[] = {
    // constructors
    {"time_now", 0, FLAGS_ND, 0, fn_now},
    {"time_date", 3, FLAGS, 0, fn_date},
    ...
};

int time_init(sqlite3* db) {
    for (size_t i = 0; i < sizeof(functions) / sizeof(functions[0]); i++) {
        const Function* f = &functions[i];
        int rc = sqlite3_create_function(db, f->name, f->nargs, f->flags, f->user_data, f->fn, 0, 0);
        if (rc != SQLITE_OK) {
            return rc;
        }
    }
    return SQLITE_OK;
}
```

Every function gets `SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC` by default.

Drop `SQLITE_DETERMINISTIC` when the result depends on anything other than the arguments, such as the clock or randomness (`time_now`, `time_since`, `time_until`).

Drop `SQLITE_INNOCUOUS` when the function has side effects or reads outside state, such as the filesystem or the environment. Use `SQLITE_DIRECTONLY` when it can do harm if called from a trigger or view.

Register each supported arity explicitly instead of `-1`. SQLite then rejects wrong arities with a clear error, and the `assert` in the wrapper documents them.

Group registrations by section, in the same order as the header and the docs. Compatibility aliases (such as Postgres names) come last, under their own comment, and reuse the regular wrappers.

Function names must be unique across all modules, because the bundle registers them all in one connection.

### Aggregate functions

The rules above are for scalar functions. Aggregates follow SQLite's built-in aggregates instead:

NULL inputs are skipped, not propagated: `sum(x)` over `(NULL, 2, 4)` is `6`.

An aggregate over no rows, or over only NULLs, returns NULL (`sum`, `avg`, `max`), except for counting-style aggregates, which return `0`.

Keep the state in `sqlite3_aggregate_context`. Request it with size `0` in the final callback, so that an empty group doesn't allocate.

Register `xInverse` and `xValue` (making it usable as a window function) whenever the aggregate allows it.

Virtual tables (`vsv`, `fileio_ls`, `define`) aren't covered yet. Add their conventions here when the first of them is refactored.

## Entry point

`src/sqlite3-<module>.c` is pure boilerplate. Copy it from `src/sqlite3-time.c` and change only the names. It registers `sqlean_version()` and returns `<module>_init(db)`. Don't put any logic here, because the bundle (`src/sqlite3-sqlean.c`) bypasses this file and calls `<module>_init` directly.

## SQL API design

Prefix every function name with the module or type: `time_*`, `dur_*`. Unprefixed names are only for well-known aliases from other databases (`now`, `date_trunc`, `make_date`), and they are documented as aliases.

Use consistent verbs: `<p>_get_<field>` for accessors, `<p>_to_<repr>` for conversions out, `<p>_<repr>` for conversions in (`time_unix`), `<p>_fmt_<style>` for formatting, `<p>_parse` for parsing.

Choose a storage format users can index and compare with plain SQL (`=`, `<`, `order by`, `min`/`max`).

## Tests

Every module has both test layers. New behavior and every fixed bug get a test in the layer where the logic lives. Usually that's the C test for domain logic, plus a SQL test that covers the binding.

### SQL tests (`test/<module>.sql`)

```sql
-- Copyright ...

.load dist/time

-- 2011-11-18 15:56:35 = 1321631795 sec

-- time_add
select '41_07', time_add(time_unix(1321631795, 0), 5*dur_s()) = time_unix(1321631795, 5000000000);

-- time_fmt_iso
-- negative sub-hour offsets must keep their sign
select '61_09', time_fmt_iso(time_unix(0, 0), -1800) = '1969-12-31T23:30:00-00:30';
```

One assertion per line: `select '<group>_<num>', <boolean>;`. Number groups by section (`01_xx` for constructors, `11_xx` for getters, ...) so a failing label points straight at the area.

Put shared fixtures at the top as comments, so expected values can be checked by hand.

When a case isn't obvious (a boundary, a normalization rule, a format quirk), explain the rule in a comment above it.

Required sections for every module:

- Happy path for every function and every arity, including aliases;
- NULL propagation: every argument position of every function, `is null`;
- Bad data is NULL: unconvertible types, malformed values, wrong-size blobs, `is null`. A bad spec argument raises an error, which the line-based runner can't assert, so cover the validation of spec arguments in the C tests;
- Boundaries and normalization (leap days, week edges, extreme offsets);
- Storage roundtrip through a real table, including `min`/`max`/`=` if the type is meant to be stored.

### C tests (`test/<module>/<concept>.test.c`)

Make them table-driven: a `typedef struct` per test shape, a `static` array of cases with golden values, and a loop of `assert`s. Add a trailing comment on each case explaining why it's there.

Write one `static void test_<function>(void)` per function, printing `test_<function>...` and then `OK`. `main` calls them in header order, grouped by the same section comments.

Required cases beyond the happy path:

- Extreme values: `INT64_MIN`/`INT64_MAX`/`INT_MIN` for every parameter type, checked to not crash and to stay inside documented buffer bounds;
- Roundtrips: format ↔ parse, encode ↔ decode;
- Corrupt input: invalid serialized data or unparseable text is reported as a failure.

Register each test in the `Makefile`: add `<package>/<module>` to `CTESTS` and list the sources it compiles with in `CTEST_SRC_<package>_<module>`. Run it with `make ctest package=<package> module=<module>`. It must pass under `make ctest-sanitize` with the same arguments.

### Bundle test (`test/sqlean.sql`)

Each module has a smoke test or two in the bundle test. They catch registration conflicts and init-order problems, which the per-module suites can't see.

## Documentation

`docs/<module>.md` is user-facing and follows `docs/time.md`:

1. `# <module>: <tagline>`, then one sentence about what the extension provides.
2. A `•`-separated anchor navigation line.
3. Concepts\*\*: the data model, its representation and range, and anything users must know before calling functions.
4. NULL and invalid input: the propagation rule and its consequences (for example, `CHECK` constraints pass on NULL), which inputs give NULL, and which spec arguments raise errors.
5. One `##` section per function group. Each function has a `###` heading, its signature in a `text` block with optional arguments in brackets, the behavior including edge cases and normalization rules, and an `sql` example with the result as a `-- comment`. Aliases are listed under the function they alias.
6. Acknowledgements for ported code.
7. Installation and usage: the standard footer.

Whenever observable behavior changes, update the docs in the same commit.

## Refactoring checklist

When bringing an existing module up to these guidelines:

1. Write the missing tests against the current behavior first, especially NULL propagation and extreme values. Then refactor, and any behavior change will show up in the diff.
2. Split the module into domain, binding, and entry point if it isn't already. Move logic out of `extension.c`.
3. Go through the domain layer. If the code is hard to follow, consider replacing it with a port (see [Ported code](#ported-code)). Make sure every input has defined behavior, check for UB, use fixed-width types, and make private symbols `static`.
4. Go through the binding layer: check the argument order (NULL, type, shape), the split between data (NULL) and spec arguments (error), allocation checks, registration flags and return codes, and explicit arities.
5. Run `make sanitize-gcc`.
6. Update `docs/<module>.md`, and call out every user-visible behavior change in the commit message (`<module>: <what changed>`).
