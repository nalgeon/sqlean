# uuid: Universally Unique IDentifiers (UUIDs) in SQLite

The `sqlean-uuid` extension provides limited support for [RFC 4122](https://www.ietf.org/rfc/rfc4122.txt) and [RFC 9562](https://datatracker.ietf.org/doc/rfc9562/) compliant UUIDs.

[Reference](#reference) •
[Acknowledgements](#acknowledgements) •
[Installation and usage](#installation-and-usage)

## Reference

Supported features:

-   Generate a version 4 (random) UUID.
-   Generate a version 7 (time-ordered, random) UUID.
-   Convert a 16-byte blob into a well-formed UUID string and vice versa.

[uuid4](#uuid4) •
[uuid7](#uuid7) •
[uuid7_timestamp_ms](#uuid7_timestamp_ms) •
[uuid_str](#uuid_str) •
[uuid_blob](#uuid_blob)

### uuid4

```text
uuid4()
```

Generates a version 4 (random) UUID as text. Aliased as `gen_random_uuid()` for PostgreSQL compatibility.

```sql
select uuid4();
-- c476b6e9-35f1-4afd-9552-704cd7edbe27

select gen_random_uuid();
-- 8d144638-3baf-4901-a554-b541142c152b
```

### uuid7

```text
uuid7()
```

Generates a version 7 (time-ordered, random) UUID as text.

```sql
select uuid7();
-- 018ff383-3e37-7615-b764-c241f544e573

select uuid7();
-- 018ff383-94fd-70fa-8da6-339180b8e15d
```

### uuid7_timestamp_ms

```text
uuid7_timestamp_ms(x)
```

Extracts unix timestamp in miliseconds from version 7 UUID `x` (text or blob). Returns `null` if the detected UUID version is not 7.

```sql
select uuid7();
-- 018ff38a-a5c9-712d-bc80-0550b3ad41a2

select uuid7_timestamp_ms('018ff38a-a5c9-712d-bc80-0550b3ad41a2');
-- 1717777901001

select datetime(uuid7_timestamp_ms('018ff38a-a5c9-712d-bc80-0550b3ad41a2') / 1000, 'unixepoch');
-- 2024-06-07 16:31:41

select uuid7_timestamp_ms(uuid4()) is null;
-- 1
```

### uuid_str

```text
uuid_str(x)
```

Converts a UUID `x` (text or blob) into a well-formed UUID string.

```sql
select uuid_str(randomblob(16));
-- fb6f9675-7509-d8b7-0891-00d4e6230894
```

### uuid_blob

```text
uuid_blob(x)
```

Converts a UUID `x` (text or blob) into a 16-byte blob.

```sql
select hex(uuid_blob(uuid4()));
-- 7192B1B452964E809500CF0364476CD3
```

## Acknowledgements

UUIDv4 adapted from [uuid.c](https://sqlite.org/src/file/ext/misc/uuid.c) by D. Richard Hipp. UUIDv7 contributed by [Nguyễn Hoàng Đức](https://github.com/nghduc97).

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
