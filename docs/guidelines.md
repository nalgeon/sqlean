# Extension guidelines

What a well-built Sqlean extension looks like. The reference implementation is `time` (`src/time/`, `test/time.sql`, `test/time/`, `docs/time.md`): when this guide doesn't cover something, do what `time` does.

The [core rules](#layout) apply to every module. The sections under [When the module has...](#when-the-module-has) apply only when they're relevant. The [review checklist](#review-checklist) at the end is the gate: a change isn't done until every question on it has the right answer.

## Layout

```text
src/sqlite3-<module>.c          entry point (boilerplate only)
src/<module>/extension.h        declares <module>_init(db), nothing else
src/<module>/extension.c        SQLite bindings: argument decoding, result encoding, registration
src/<module>/<name>.h           public domain API, fully documented (time: timex.h)
src/<module>/<concept>.c        domain logic, one file per concept (time: time.c, duration.c)
src/<module>/common.h           optional: static inline helpers shared by domain files
src/<module>/binding.h          optional: static inline helpers shared by binding files
test/<module>.sql               SQL tests
test/<module>/<concept>.test.c  C tests of the domain layer
docs/<module>.md                user documentation
```

Every file starts with the license header, an attribution block for code derived from elsewhere, and a one-line description. Header guards are `<MODULE>_<FILE>_H`, closed with `#endif /* ..._H */`.

```c
// Copyright (c) 2024 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Based on Go's time package, BSD 3-Clause License
// https://github.com/golang/go

// Duration methods.
```

The entry point `src/sqlite3-<module>.c` is copied from `src/sqlite3-time.c` with the names changed. It holds no logic, because the bundle (`src/sqlite3-sqlean.c`) bypasses it and calls `<module>_init` directly.

## Domain layer

Plain C with no SQLite dependency, so it can be tested and reasoned about on its own.

### API shape

- Model values as small value types passed by value (`Time` is `{int64_t sec; int32_t nsec;}`). Document the representation, unit and range where the type is declared.
- Use fixed-width integers for anything stored, serialized or potentially large. Plain `int` is for small bounded values, like calendar fields.
- Use an `enum` for a closed set and a named constant for a magic value (`enum Month`, `TIMEX_BLOB_SIZE`).
- Name functions `<prefix>_<verb>[_<object>]`, one prefix per type (`time_get_year`, `dur_round`). Constructors return the value; several outputs go through out-parameters.
- Split the public header into commented sections. Use the same sections as `#pragma region` blocks in the `.c` files, as groups in the C tests, and as headings in the docs.
- Give every public function a doc comment in the header that starts with its name and says what it returns ("`time_add returns the time t+d.`"). Repeat it above the definition and add the details: edge cases, rounding, clamping.

### Every input has defined behavior

SQL users can pass anything, and the binding layer only checks types. Each function picks one of these and documents it:

- **Normalize** out-of-range components: `time_date(2011, 10, 32, ...)` is November 1.
- **Saturate** on overflow: `time_add` clamps to the min/max `Time`.
- **Ignore** a meaningless parameter: `dur_truncate(d, m)` with `m <= 0` returns `d`.
- **Report failure** when there is no sensible result.

Crashes, garbage output and undefined behavior are never acceptable.

### Reporting failure

The return value says whether the function succeeded, and the result goes through an out-parameter. Never use an in-band sentinel that is also a valid result: if `time_parse` returned the zero time for garbage, it couldn't be told apart from `0001-01-01`.

- **One way to fail** (bad input): return `bool` (`bool time_parse(const char* s, Time* out)`).
- **More than one way** (bad input or out of memory): return a module status enum, `<Module>Status` with `OK` first (`FuzzyStatus`: `FUZZY_OK`, `FUZZY_INVALID`, `FUZZY_NOMEM`). The binding layer maps it in one helper.
- **Operating system calls**: return `0` or an `errno` value (`int fs_open(const char* path, FILE** out)`), and document the codes added beyond the OS's own (`EISDIR` for a directory).

Use one convention per module: if any function needs a status enum, every function that can fail returns it.

### Consistent families

Functions of the same kind (the distances, the phonetic codes) treat the same input the same way: case, empty input, input with nothing to process (no letters, no digits), invalid input. Any difference is deliberate, documented, and tested.

### No undefined behavior

Sqlean runs inside other people's processes, so UB counts as a security bug. The tests run under ASan and UBSan (`make check san=1 on=gcc`) and must pass.

- Do wrapping arithmetic in unsigned types, and convert back with a defined helper (`uint64_to_int64` in `time/common.h`), not a cast.
- Widen before negating or subtracting near the limits: `-(int64_t)year`, because `-INT_MIN` overflows.
- Detect overflow by comparison after the unsigned computation, then clamp (`dur_round`).
- Shift only unsigned values.
- Never pass NULL to `memcpy` and friends, even with a zero size.
- Comment each of these spots with why the obvious code would be UB, so nobody "simplifies" it back.

### Memory

- Prefer no heap allocation: return by value, or write into a caller's buffer with `snprintf` semantics (`size_t time_fmt_iso(char* buf, size_t size, ...)`).
- A function that writes into a fixed buffer exports a size constant that is always enough (`TIMEX_FMT_BUF_SIZE`), with its derivation in a comment and a C test of the worst case.
- When allocation is unavoidable, check it, and say in the header who owns the result and how to free it.

### Internal code

- Everything not in the public header is `static`: the bundle links all modules together, so a stray symbol can collide.
- No mutable global state: an extension runs in many connections on many threads. File and function `static` variables are `const`. Per-connection state goes in the user data, per-statement state in auxdata.
- Compile cleanly with `-Wall -Wextra -Werror` and format with `clang-format`.
- Comments match the code. When code changes, check every comment and doc that describes it.

## Binding layer

`extension.c` converts between `sqlite3_value`s and domain types, and nothing else. A wrapper reads "decode the arguments → call one domain function → encode the result". Real logic in a wrapper belongs in the domain layer.

### Wrappers

```c
// time_add(t, d)
static void fn_add(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    // decode each argument, call the domain function, set the result
}
```

- Name wrappers `fn_<name>`. The comment above lists the SQL signatures, with optional arguments in brackets: `// time_date(year, month, day[, hour, min, sec])`. `assert` the registered arities. Assign defaults for optional arguments next to their `argc` check.
- **Functions with the same shape share one wrapper**, with the domain function passed through `sqlite3_user_data` (`fn_extract`). The pointer type must match the function's real signature exactly (`enum Month`, `bool` and `int` are different types). If it doesn't, add small `static` adapters (`get_month`).
- Pass a constant as a pointer to a `static const` value, never as a value cast to `void*` (`fn_dur_const`).
- Put repeated decoding and encoding in helpers. Decoding helpers return `bool` with an out-parameter (`arg_time`). Helpers for spec arguments set the error themselves (`arg_field`).

### Arguments: NULL or error

Do what SQLite's built-in functions do. Bad **data** gives NULL, so one bad row doesn't abort a query. A bad **query** raises an error, so a typo doesn't silently turn into NULLs.

| Input                         | Example                                  | Result                              |
| ----------------------------- | ---------------------------------------- | ----------------------------------- |
| a NULL argument               | `upper(null)`                            | NULL                                |
| a convertible type            | `upper(123)`, `sqrt('4')`                | convert, then proceed               |
| an unconvertible type         | `sqrt('abc')`, `sqrt(x'01')`             | NULL                                |
| a malformed value             | `date('garbage')`, `unhex('zz')`         | NULL                                |
| out of domain                 | `sqrt(-1)`                               | NULL, unless the domain normalizes  |
| a malformed spec argument     | `'a' regexp '('`, `time_get(t, 'bogus')` | error naming it: `unknown field: x` |
| a failed side effect          | writing a file failed                    | error naming the target             |
| out of memory, result too big |                                          | `_error_nomem`, `_error_toobig`     |
| an unrepresentable result     | `abs(-9223372036854775808)`              | error, unless the domain saturates  |

A spec argument says _how_ to process the data: a pattern, a field or unit name, a format, an option. A file path is data, not spec: it often comes from a table, so a missing file gives NULL. Functions that only read give NULL on failure; functions that write raise an error, because a missed write is easy to overlook as NULL. The legacy functions that coerce garbage to zero (`abs('abc')` is `0.0`) are not the model.

Conversion:

- **Integer**: accept `SQLITE_INTEGER` after `sqlite3_value_numeric_type` (so `'42'` works). Read with `sqlite3_value_int64`, never `sqlite3_value_int`, which truncates. Range-check before narrowing.
- **Real**: accept `SQLITE_INTEGER` or `SQLITE_FLOAT` after `sqlite3_value_numeric_type`.
- **Text**: accept any non-BLOB value. Call `sqlite3_value_text`, then `sqlite3_value_bytes`, in that order. A domain function that takes NUL-terminated strings treats an embedded NUL as malformed. Functions that count or index text work in characters, as `length()` does; byte-oriented functions say so in the docs.
- **Blob of a fixed format**: accept only `SQLITE_BLOB` of the exact size, and let the domain decoder validate it.

### Results and memory

- Use `SQLITE_TRANSIENT` for stack buffers and `SQLITE_STATIC` for literals. Hand `sqlite3_malloc` results over with `sqlite3_free` as the destructor.
- Allocate with `sqlite3_malloc64`. Check every allocation; on failure, free what was allocated and call `sqlite3_result_error_nomem`.
- Cache an expensive spec argument (a compiled regex) per statement with `sqlite3_get_auxdata`/`sqlite3_set_auxdata`.

### Registration

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

- Flags are `SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC`. Drop `DETERMINISTIC` if the result depends on anything but the arguments (the clock, randomness). Functions that touch the file system or the environment are `SQLITE_UTF8 | SQLITE_DIRECTONLY`, so a malicious database can't call them from a trigger or view.
- Register each arity explicitly, never `-1`.
- Order the table by the header's sections. Aliases (such as Postgres names) come last under their own comment and reuse the regular wrappers.
- Function names are unique across all modules, because the bundle registers them in one connection.

## SQL API

- Prefix every function with the module or type (`time_*`, `dur_*`). Unprefixed names are only for well-known aliases from other databases (`now`, `date_trunc`), documented as aliases.
- Use consistent verbs: `<p>_get_<field>`, `<p>_to_<repr>` (out), `<p>_<repr>` (in), `<p>_fmt_<style>`, `<p>_parse`.
- Choose a storage format that sorts and compares correctly with plain SQL (`=`, `<`, `order by`, `min`/`max`).

## Tests

Every module has both layers, and each covers real behavior. New behavior and every fixed bug get a test where the logic lives: usually a C test for the domain logic and a SQL test for the binding. Every documented claim (an edge case, a NULL rule, a deliberate difference) has a test.

### SQL tests (`test/<module>.sql`)

```sql
.load dist/time

-- 2011-11-18 15:56:35 = 1321631795 sec

-- time_fmt_iso
-- negative sub-hour offsets keep their sign
select '61_09', time_fmt_iso(time_unix(0, 0), -1800) = '1969-12-31T23:30:00-00:30';
```

- One assertion per line, `select '<group>_<num>', <boolean>;`, with groups numbered by section and labels in order.
- Shared fixtures go at the top as comments. A non-obvious case gets a comment explaining the rule.
- Cover: every function, arity and alias; NULL in every argument position; bad data giving NULL; boundaries and normalization; a roundtrip through a real table if the type is meant to be stored.
- A spec-argument error can't be asserted line by line, so cover spec validation in the C tests.

### C tests (`test/<module>/<concept>.test.c`)

- Table-driven: a struct per test shape, a `static const` array of cases with golden values and a trailing comment on why each is there, and a loop of `assert`s.
- One `static void test_<function>(void)` per function, printing `test_<function>...` and `OK`, called from `main` in header order.
- Cover beyond the happy path: extreme values (`INT64_MIN`, `INT64_MAX`, empty and huge strings), roundtrips (format ↔ parse, encode ↔ decode), corrupt input reported as failure, and buffers of exactly the advertised size so the sanitizer catches overflows.
- Golden values come from an independent source (a spec, a reference implementation, a worked example), not from the code's own output.
- They exercise the domain layer without SQLite (`test/crypto/hex.test.c`). Only binding behavior that SQL can't assert (spec-argument errors, OS error messages, per-connection state, `SQLITE_LIMIT_LENGTH`) justifies running the extension in a connection (`test/define/define.test.c`).

`test/sqlean.sql` has a smoke test or two per module, to catch registration conflicts in the bundle.

## Documentation

`docs/<module>.md` follows `docs/time.md`:

1. `# <module>: <tagline>`, one sentence on what it provides, and a `•`-separated navigation line.
2. Concepts: the data model, its representation and range.
3. NULL and invalid input: which inputs give NULL and which spec arguments raise errors.
4. A `##` section per function group. Each function has a `###` heading, its signature in a `text` block, its behavior including edge cases, and an `sql` example with the result as a `-- comment`. Aliases are listed under their function.
5. Acknowledgements for ported code, then the standard installation footer.

Update the docs in the same commit as any observable change. `docs/release-notes.md` gets the major improvements, fixes and breaking changes, each with an example showing the old and new result.

## When the module has...

### Ported code

Before writing domain logic from scratch or rewriting hard-to-follow code, look for a proven Go implementation (the standard library or a well-known package). It's usually more readable and better tested than C found in the wild. The `crypto` hashes are ported this way.

- The license must be BSD or MIT. Keep the original copyright and add the source to `docs/third-party.md`.
- Port the portable Go code (`*_generic.go`), not the assembly.
- Go checks bounds and C doesn't: all the UB and memory rules apply to the port.
- Follow the original's semantics exactly. Comment deliberate parity ("as Go does") and every deviation.
- Verify the port against the reference over many inputs, sizes and split points, not just a few vectors.
- Simple and correct beats fast, unless there's a real performance need.

### Binary serialization

A binary format exposed to SQL (the time BLOB) has a version byte, an explicit byte order and a size constant. Decoding validates it: an unknown version or out-of-range field is a decode failure, never garbage.

### Platform-specific code

Keep platform differences in one place behind `#if` (`timespec_now` in `time.c`). Prefer C11, then POSIX, then Windows. On Windows, paths and other strings go through the wide-character API, converted from UTF-8 (`win_path` in `fileio/common.h`). Don't vendor POSIX emulation layers.

### Aggregate functions

Follow SQLite's aggregates: NULL inputs are skipped; no rows or only NULLs gives NULL (`0` for counting). Keep state in `sqlite3_aggregate_context`, requested with size `0` in the final callback so an empty group doesn't allocate. Register `xInverse` and `xValue` when the aggregate allows it.

### Table-valued functions

An eponymous virtual table with arguments as hidden columns, following the scalar argument rules with rows in place of a result. The reference is `src/fileio/scan.c`.

- Its own file, with a `static` `sqlite3_module` and an init function that `<module>_init` calls and checks. Aliases after the main name.
- `xConnect` declares the hidden columns after the regular ones, calls `sqlite3_vtab_config(db, SQLITE_VTAB_DIRECTONLY)` if it touches outside state, and keeps `argv[0]` (the name it was called by) for error messages.
- `xBestIndex` consumes only the `EQ` constraints on hidden columns (`argvIndex`, `omit = 1`) and leaves the rest, `LIMIT` and `ORDER BY` to SQLite. It returns `SQLITE_CONSTRAINT` for an unusable argument constraint. A missing required argument gets a plan that `xFilter` turns into `"table function <name> requires an argument"`.
- `xFilter` frees the previous scan's state first. A NULL or invalid data argument gives no rows; a bad spec argument sets `zErrMsg` and returns `SQLITE_ERROR`. Arguments returned as hidden columns are copied with `sqlite3_value_dup`.
- Resource failures return `SQLITE_NOMEM`, `SQLITE_TOOBIG` or `SQLITE_IOERR`. Buffers are capped at `SQLITE_LIMIT_LENGTH`. A row that can't be produced (a file that disappeared mid-listing) is skipped.

Virtual tables with their own schema (`vsv`, `define`) aren't covered yet. Add their conventions here when the first one is refactored.

## Review checklist

Answer each question against the code. For a refactor, first write tests for the current behavior (NULL propagation, extreme values), so every behavior change shows up in a diff and is either fixed or called out in the release notes.

**Layout**

- [ ] Does `extension.c` contain only decoding, a domain call and encoding per wrapper?
- [ ] Is everything outside the public header `static`, and every `static` variable `const`?

**Domain**

- [ ] Does every public function have a doc comment, starting with its name, that matches what the code does?
- [ ] Is there one failure convention across the module, and no in-band sentinels?
- [ ] Does every function have defined behavior for empty, huge, extreme and invalid input?
- [ ] Do functions of the same kind treat case, empty input, input with nothing to process and invalid input the same way?
- [ ] Is every UB-avoiding spot commented with why?

**Binding**

- [ ] Are there two wrappers with the same shape? Merge them through `sqlite3_user_data`.
- [ ] Does bad data give NULL, and only a bad spec argument, a failed write or a resource failure give an error?
- [ ] Is every allocation checked, with what was allocated freed on failure?
- [ ] Are flags, arities and registration return codes right?

**Tests and docs**

- [ ] Do the C tests cover the domain logic, not just one property of it, with independent golden values?
- [ ] Does every documented claim have a test, and does every observable change have a docs update and a release note?
- [ ] Do `make check san=1 on=gcc` and `make test only=sqlean` pass?
