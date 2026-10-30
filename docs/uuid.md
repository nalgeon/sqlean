# uuid: Universally Unique IDentifiers (UUIDs) in SQLite

The `sqlean-uuid` extension generates and converts [RFC 9562](https://datatracker.ietf.org/doc/rfc9562/) UUIDs: random version 4 and time-ordered version 7.

[Concepts](#concepts) •
[Generating UUIDs](#generating-uuids) •
[Extracting fields](#extracting-fields) •
[Conversion](#conversion) •
[Acknowledgements](#acknowledgements) •
[Installation and usage](#installation-and-usage)

## Concepts

A UUID is a 128-bit value, written as 32 hex digits in five groups:

```text
xxxxxxxx-xxxx-Mxxx-Nxxx-xxxxxxxxxxxx
```

`M` is the version, and the top bits of `N` are the variant (`8`, `9`, `a` or `b` for RFC 9562 UUIDs). The generators set both.

You can store a UUID as text (36 characters, the form the generators return) or as a 16-byte BLOB (`uuid_blob`), which takes less than half the space. Both forms compare and sort the same way with plain SQL operators (`=`, `<`, `order by`, `min`, `max`), as long as the text is in canonical lowercase form. Use `uuid_str` to bring text from elsewhere to that form.

A version 4 UUID is 122 random bits. A version 7 UUID starts with a 48-bit Unix time in milliseconds, so UUIDs made later sort later (until the year 10889):

```text
  Version 7

┌────────────┬─────┬─────────────────┬─────┬─────────┐
│ unix_ts_ms │ ver │ sub-millisecond │ var │ random  │
└────────────┴─────┴─────────────────┴─────┴─────────┘
  48 bit       4     12 bit            2     62 bit
```

`uuid7()` stores the sub-millisecond part of the current time in the 12 bits after the version, so UUIDs made within the same millisecond also sort by time, with a resolution of about 244 nanoseconds. This is method 3 of RFC 9562, section 6.2. The order is not guaranteed for UUIDs made within the same 244 ns or when the system clock goes back, and the resolution depends on the system clock.

### NULL and invalid input

If the argument is `NULL`, the result is `NULL`. Functions that don't take any arguments never return `NULL`.

Like SQLite's own functions, the extension returns `NULL` for invalid data instead of raising an error, so one bad row doesn't abort the whole query:

- A UUID argument that is neither a valid UUID string nor a 16-byte BLOB: `uuid_str('hello')` and `uuid_str(randomblob(15))` are `NULL`. A BLOB is never parsed as text.
- A UUID string with any extra characters, including whitespace and NUL characters: `uuid_str(' a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11')` is `NULL`.
- A version 7 timestamp that is not an integer, or is outside the version 7 range: `uuid7(1.5)` and `uuid7(-1)` are `NULL`. Numeric text is converted, so `uuid7('1000')` works.

The accepted string forms are listed under [uuid_str](#uuid_str).

## Generating UUIDs

### uuid4

```text
uuid4()
```

Generates a version 4 (random) UUID as text.

```sql
select uuid4();
-- c476b6e9-35f1-4afd-9552-704cd7edbe27
```

Aliased as Postgres-like `gen_random_uuid()`.

### uuid7

```text
uuid7([seconds])
```

Generates a version 7 (time-ordered) UUID as text.

Without arguments, uses the current time, including its sub-millisecond part (see [Concepts](#concepts)).

```sql
select uuid7();
-- 019a3c6d-3e37-7615-b764-c241f544e573

select uuid7();
-- 019a3c6d-3e37-7a1c-8da6-339180b8e15d
```

With `seconds`, uses that Unix time in whole seconds, from `0` (1970-01-01) to `281474976710` (10889-08-02). The rest of the UUID is random, so UUIDs made with the same `seconds` are in random order. Returns `NULL` if `seconds` is not an integer or is outside the range.

```sql
select uuid7(0);
-- 00000000-0000-7558-a13b-e7014913d8ad

select uuid7(1700000000);
-- 018bcfe5-6800-7b8a-8a96-e315717d31bb

select uuid7(-1);
-- NULL
```

## Extracting fields

### uuid7_timestamp_ms

```text
uuid7_timestamp_ms(uuid)
```

Extracts the Unix time in milliseconds from a version 7 UUID (text or blob). Returns `NULL` if the UUID is invalid or its version is not 7. The variant is not checked.

```sql
select uuid7_timestamp_ms('018ff38a-a5c9-712d-bc80-0550b3ad41a2');
-- 1717777901001

select datetime(uuid7_timestamp_ms('018ff38a-a5c9-712d-bc80-0550b3ad41a2') / 1000, 'unixepoch');
-- 2024-06-07 16:31:41

select uuid7_timestamp_ms(uuid4());
-- NULL
```

## Conversion

### uuid_str

```text
uuid_str(uuid)
```

Converts a UUID (text or blob) into the canonical lowercase string form.

A string can be in any of the following forms, with hex digits in any case:

```text
a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11
{a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11}
urn:uuid:a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11
a0eebc999c0b4ef8bb6d6bb9bd380a11
```

A blob must be exactly 16 bytes. The version and variant are not checked, so any 16 bytes convert.

Returns `NULL` for anything else.

```sql
select uuid_str('{A0EEBC99-9C0B-4EF8-BB6D-6BB9BD380A11}');
-- a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11

select uuid_str(x'a0eebc999c0b4ef8bb6d6bb9bd380a11');
-- a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11

select uuid_str('a0eebc99-9c0b-4ef8-bb6d');
-- NULL
```

### uuid_blob

```text
uuid_blob(uuid)
```

Converts a UUID (text or blob) into a 16-byte blob, in network byte order (big-endian). Accepts the same input as `uuid_str`, and returns a blob unchanged.

```sql
select hex(uuid_blob('a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11'));
-- A0EEBC999C0B4EF8BB6D6BB9BD380A11
```

## Acknowledgements

Parsing and version 7 generation are based on Google's [uuid](https://github.com/google/uuid) package for Go. The original extension was adapted from [uuid.c](https://sqlite.org/src/file/ext/misc/uuid.c) by D. Richard Hipp. UUIDv7 was first contributed by [Nguyễn Hoàng Đức](https://github.com/nghduc97).

## Installation and usage

SQLite command-line interface:

```
sqlite> .load ./uuid
sqlite> select uuid4();
```

See [How to install an extension](install.md) for usage with IDE, Python, etc.

↓ [Download](https://github.com/nalgeon/sqlean/releases/latest) the extension.

⛱ [Explore](https://github.com/nalgeon/sqlean) other extensions.

★ [Subscribe](https://antonz.org/subscribe/) to stay on top of new features.
