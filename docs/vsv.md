# vsv: CSV files as virtual tables in SQLite

The `sqlean-vsv` extension provides a virtual table for querying CSV files (and other separated values) directly, without importing them into the database.

[Concepts](#concepts) •
[Parameters](#parameters) •
[Acknowledgements](#acknowledgements) •
[Installation and usage](#installation-and-usage)

## Concepts

For the `people.csv` file with the following data:

```csv
11,Diane,London
22,Grace,Berlin
33,Alice,Paris
```

A `vsv` table could look like this:

```sql
create virtual table temp.people using vsv(
    filename=people.csv,
    schema="create table people(id integer, name text, city text)"
);

select * from people;
```

```text
┌────┬───────┬────────┐
│ id │ name  │  city  │
├────┼───────┼────────┤
│ 11 │ Diane │ London │
│ 22 │ Grace │ Berlin │
│ 33 │ Alice │ Paris  │
└────┴───────┴────────┘
```

### Records and fields

The input is read as [RFC 4180](https://www.rfc-editor.org/rfc/rfc4180) CSV, the same way Go's `encoding/csv` reads it:

- Each line is a record, and fields are separated by commas (or another [separator](#fsep)).
- A field can be quoted with `"`. A quoted field may contain separators, newlines and quotes, which are doubled: `"say ""hi"""` is `say "hi"`.
- `\r\n` is read as `\n`, also inside quoted fields. A `\r` not followed by `\n` is a regular character.
- Blank lines are skipped. A line with only spaces is not blank: it's a record with one field.
- Spaces are part of the field: `a, b` has the fields `a` and ` b`.
- A UTF-8 byte order mark at the start of the input is skipped.
- Fields are passed to SQLite as text, without checking that they are valid UTF-8. A NUL character is kept in the field.

An empty field is empty text (`''`), unless [nulls](#nulls) is on. A record shorter than the table gives `NULL` for the missing columns, and the extra fields of a longer record are ignored.

### Rows

Each record is a row, numbered from 1 in `rowid` (the header doesn't count). The table is read-only.

The table doesn't store the data: every query reads the input from the start, so it sees the current contents of the file. The columns are fixed when the table is created. To keep the data, copy it into a regular table:

```sql
create table people_copy as select * from people;
```

### Columns and types

Without a [schema](#schema), the columns are named after the header fields if there is a [header](#header), or `c0`, `c1`, ... otherwise, and counted from the first record. All values are text. Empty input has one column, `c0`.

With a schema, the columns and their types come from it. A field is converted as SQLite converts text inserted into a regular table with the same column types ([type affinity](https://sqlite.org/datatype3.html#type_affinity)):

```sql
create virtual table temp.t using vsv(
    data='1.0,1.0,1.0,abc',
    schema='create table x(a integer, b real, c text, d integer)'
);
select a, typeof(a), b, typeof(b), c, typeof(c), d, typeof(d) from t;
-- 1, integer, 1.0, real, 1.0, text, abc, text
```

- `INTEGER` and `NUMERIC` columns turn a number into an integer if it has no fractional part (`'1.0'` → `1`, `'1e3'` → `1000`) and into a real otherwise (`'1.5'` → `1.5`).
- `REAL` columns turn a number into a real (`'1'` → `1.0`).
- `TEXT` columns and columns without a type keep the field as is.
- A field that doesn't look like a number stays text in any column, as in SQLite.

### Errors

The table reports problems with the parameters or the input as errors, instead of returning partial or made-up data:

- A bad parameter or schema is an error when the table is created: `unknown parameter: x`, `invalid value for fsep: 'ab'`, `bad schema: 'create table x(' - incomplete input`.
- An input file that can't be opened is an error when the table is created and when it's queried: `cannot open file: 'people.csv'`.
- Malformed CSV is an error with the line and column, as in Go: `parse error on line 1, column 4: bare " in non-quoted-field` for `a,b"c`, or `extraneous or missing " in quoted-field` for an unterminated or badly closed quoted field. For a field that spans several lines, the message also gives the line the record starts on: `record on line 1; parse error on line 3, column 2: ...`. A query that reaches the malformed record fails, even if it has already returned some rows.
- A field or a record longer than the [length limit](https://sqlite.org/limits.html#max_length) is a `string or blob too big` error.

### Security

A `vsv` table reads files. If a database you open from an untrusted source could define one in its schema and query it from its own triggers and views, it could read files on your system. To prevent this, `vsv` tables can't be used from triggers and views stored in a database file: `unsafe use of virtual table "people"`. Create the tables in the `temp` schema (`create virtual table temp.people ...`), so they don't become part of the database file. Triggers and views in `temp` can use them.

## Parameters

```text
create virtual table temp.<name> using vsv(
    filename=STRING | data=STRING,
    [schema=STRING,]
    [header[=BOOL],]
    [nulls[=BOOL],]
    [fsep=STRING]
)
```

Parameter names are case-insensitive. A `STRING` value can be quoted with `'` or `"` (doubled inside to escape it), and must be quoted if it contains a comma. A `BOOL` value is `yes`, `on`, `true` or `1`, or `no`, `off`, `false` or `0`, in any case, quoted or not. A parameter can be given only once.

The `columns` parameter of older versions is accepted and ignored. The removed `rsep`, `dsep`, `skip`, `validatetext` and `affinity` parameters are errors (`unsupported parameter: affinity`).

### filename

```text
filename=STRING
```

The path of the input file. Relative paths are resolved against the current directory of the process.

```sql
create virtual table temp.t using vsv(filename='people.csv');
select * from t;
-- 11, Diane, London
-- 22, Grace, Berlin
-- 33, Alice, Paris
```

Exactly one of `filename` and `data` is required: `must specify either filename or data, but not both`.

### data

```text
data=STRING
```

The input itself, instead of a file.

```sql
create virtual table temp.t using vsv(data='a,b
c,d');
select c0, c1 from t;
-- a, b
-- c, d
```

### schema

```text
schema=STRING
```

A `create table` statement that defines the columns and their [types](#columns-and-types). The table name in it doesn't matter. The fields map to the columns by position, including hidden columns.

```sql
create virtual table temp.t using vsv(
    data='22,Grace,Berlin',
    schema='create table x(id integer, name text, city text)'
);
select id + 1, typeof(id) from t;
-- 23, integer
```

With a schema, the header (if there is one) is skipped and its names are not used. Without a schema, all columns are text.

### header

```text
header[=BOOL]
```

Whether the first record is a header with column names. Defaults to no; the name alone means yes.

```sql
create virtual table temp.t using vsv(data='id,name
1,one
2,two', header);
select id, name from t;
-- 1, one
-- 2, two
```

The names are used as is, even if they need quoting in SQL. Duplicate names are an error: `bad schema: 'create table x("a","b","a")' - duplicate column name: a`.

### nulls

```text
nulls[=BOOL]
```

Whether an empty field is `NULL`. Defaults to no; the name alone means yes. Only an unquoted empty field is `NULL`: a quoted one (`""`) stays an empty string, so a file can tell a missing value from an empty one.

```sql
create virtual table temp.t using vsv(data='1,,""', nulls);
select c0, c1 is null, c2 = '' from t;
-- 1, 1, 1
```

With a [schema](#schema), the empty fields of numeric columns are `NULL` instead of `''`, so aggregates skip them as they skip missing values. Without `nulls`, `avg` counts an empty field as `0`, and `max` returns it, since text sorts after numbers.

```sql
create virtual table temp.t using vsv(
    data='10,
,
20,',
    schema='create table x(price real)',
    nulls
);
select count(price), avg(price) from t;
-- 2, 15.0
```

The header is not affected: an empty name stays empty.

### fsep

```text
fsep=STRING
```

The field separator. Defaults to a comma. It's a single character, which can be any UTF-8 character except `"`, `\r` and `\n`. Since some characters are hard to type in SQL, `fsep` also accepts the escapes `\t` (a tab) and `\xHH` (an ASCII character in hex).

```sql
create virtual table temp.t using vsv(data='a;b,c', fsep=';');
select c0, c1 from t;
-- a, b,c

create virtual table temp.t using vsv(data='a|b', fsep='\x7c');
select c0, c1 from t;
-- a, b
```

Records are always separated by newlines.

## Acknowledgements

Based on [vsv.c](https://github.com/ncruces/kmedcalf-sqlite/blob/main/vsv.c) by Keith Medcalf, which is based on SQLite's [csv.c](https://sqlite.org/src/file/ext/misc/csv.c) by D. Richard Hipp. The CSV reader is ported from Go's [encoding/csv](https://github.com/golang/go/tree/master/src/encoding/csv) package (BSD 3-Clause License), and the type conversion follows SQLite's.

## Installation and usage

SQLite command-line interface:

```
sqlite> .load ./vsv
sqlite> create virtual table temp.people using vsv(filename=people.csv);
sqlite> select * from people;
```

See [How to install an extension](install.md) for usage with IDE, Python, etc.

↓ [Download](https://github.com/nalgeon/sqlean/releases/latest) the extension.

⛱ [Explore](https://github.com/nalgeon/sqlean) other extensions.

★ [Subscribe](https://antonz.org/subscribe/) to stay on top of new features.
