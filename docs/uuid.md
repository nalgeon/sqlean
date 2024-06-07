# uuid: Universally Unique IDentifiers (UUIDs) in SQLite

Limited support for [RFC 4122](https://www.ietf.org/rfc/rfc4122.txt) compliant UUIDs:

-   Generate a version 4 (random) UUID.
-   Convert a 16-byte blob into a well-formed UUID string and vice versa.

Adapted from [uuid.c](https://sqlite.org/src/file/ext/misc/uuid.c) by D. Richard Hipp.

Provides following functions:

<h3 name="uuid4"><code>uuid4()</code></h3>

Generates a version 4 (random) UUID as a string. Aliased as `gen_random_uuid()` for PostgreSQL compatibility.

```
sqlite> select uuid4();
c476b6e9-35f1-4afd-9552-704cd7edbe27

sqlite> select gen_random_uuid();
8d144638-3baf-4901-a554-b541142c152b
```

<h3 name="uuid7"><code>uuid7()</code></h3>

Generates a version 7 (random) UUID as a string. Version 7 UUIDs are time ordered.

```
sqlite> select uuid7();
018ff383-3e37-7615-b764-c241f544e573

sqlite> select uuid7();
018ff383-94fd-70fa-8da6-339180b8e15d
```

<h3 name="uuid_str"><code>uuid_str(X)</code></h3>

Converts a UUID `X` into a well-formed UUID string. `X` can be either a string or a blob.

```
sqlite> select uuid_str(randomblob(16));
fb6f9675-7509-d8b7-0891-00d4e6230894
```

<h3 name="uuid_blob"><code>uuid_blob(X)</code></h3>

Converts a UUID `X` into a 16-byte blob. X can be either a string or a blob.

```
sqlite> select hex(uuid_blob(uuid4()));
7192B1B452964E809500CF0364476CD3
```

<h3 name="uuid7_timestamp_ms"><code>uuid7_timestamp_ms(X)</code></h3>

Extract unix timestamp in miliseconds from version 7 UUID `X`. It will output random time if used on UUIDs not on version 7 format.

```
sqlite> SELECT uuid7();
018ff38a-a5c9-712d-bc80-0550b3ad41a2

sqlite> SELECT uuid7_timestamp_ms('018ff38a-a5c9-712d-bc80-0550b3ad41a2');
1717777901001

sqlite> SELECT datetime(uuid7_timestamp_ms('018ff38a-a5c9-712d-bc80-0550b3ad41a2') / 1000, 'unixepoch');
2024-06-07 16:31:41
```

## Installation and Usage

SQLite command-line interface:

```
sqlite> .load ./uuid
sqlite> select uuid4();
sqlite> select uuid7();
```

See [How to Install an Extension](install.md) for usage with IDE, Python, etc.

[⬇️ Download](https://github.com/nalgeon/sqlean/releases/latest) •
[✨ Explore](https://github.com/nalgeon/sqlean) •
[🚀 Follow](https://antonz.org/subscribe/)
