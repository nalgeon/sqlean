# define: User-defined functions in SQLite

The `sqlean-define` extension allows writing arbitrary functions in SQL (as opposed to [application-defined functions](https://sqlite.org/appfunc.html), which require programming in C, Python, or another language). Or even execute arbitrary SQL from a string.

[Scalar Functions](#scalar-functions) •
[Table-valued functions](#table-valued-functions) •
[Arbitrary SQL statements](#arbitrary-sql-statements) •
[Security](#security) •
[Performance](#performance) •
[Reference](#reference) •
[Acknowledgements](#acknowledgements) •
[Installation and usage](#installation-and-usage)

## Scalar functions

`select define(NAME, BODY)`

Defines a scalar function. For example, a function for summing the numbers `1..n` can be defined as:

```sql
sqlite> select define('sumn', ':n * (:n + 1) / 2');
sqlite> select sumn(5);
15
```

Functions can take multiple parameters - anonymous, positional, or named:

```sql
-- anonymous
select define('subxy1', '? - ?');
select subxy1(5, 1);
4

-- positional
select define('subxy2', '?1 - ?2');
select subxy2(5, 1);
4

select define('subxy3', '?2 - ?1');
select subxy3(5, 1);
-4

-- named
select define('subxy4', ':x - :y');
select subxy4(5, 1);
4
```

When calling a function with anonymous or named parameters, one should pass the arguments in the order of appearance in the function definition. For example, if the function is defined as `:x - :y`, there is no way to pass `y` value first.

Functions can use other scalar functions - both built-in and user-defined. For example, the function that returns a random integer `N` such that `a <= N <= b` can be defined as:

```sql
select define('randint', ':a + abs(random()) % (:b - :a + 1)');
select randint(10, 99);
40
select randint(10, 99);
67
select randint(10, 99);
76
```

To change a function, define it again:

```sql
sqlite> select define('sumn', '(:n * :n + :n) / 2');
sqlite> select sumn(5);
15
```

The new body takes effect in the current connection right away, even for queries that are already prepared or running. Other open connections keep the old body until they reconnect. If the new definition has a different number of parameters, the old one stops working (`wrong number of arguments to function sumn()`).

You can't define a function with the name of an existing function that wasn't created with `define()`, such as a built-in one (`function already exists: abs`), or with the name of a table-valued function.

If the body returns no rows, the function returns NULL, just like a scalar subquery:

```sql
sqlite> select define('find_user', 'name from users where id = ?');
sqlite> select find_user(42) is null;
1
```

To list defined scalar functions, select them from the `sqlean_define` table:

```sql
select name, body from sqlean_define where type = 'scalar';
┌───────┬───────────────────┐
│ name  │       body        │
├───────┼───────────────────┤
│ sumn  │ ?1 * (?1 + 1) / 2 │
│ subxy │ ?1 - ?2           │
└───────┴───────────────────┘
```

To delete a scalar function, execute `undefine()`:

```
sqlite> select undefine('sumn');
sqlite> select sumn(5);
Runtime error: no such function: sumn
```

The function is deleted from `sqlean_define`, and stops working in the current connection right away. Other open connections keep the function until they reconnect.

## Table-valued functions

`create virtual table NAME using define((BODY))`

Defines a function capable of returning mutiple values, or even mutiple rows of values. For example, a function to split a string around a separator can be defined as:

```sql
create virtual table strcut using define((
  select
    substr(:str, 1, instr(:str, :sep) - 1) as left,
    :sep as separator,
    substr(:str, instr(:str, :sep) + 1) as right
));

select * from strcut('one;two', ';');
one|;|two

select left, right from strcut('one;two', ';');
one|two

select left from strcut('one;two', ';');
one
```

Applying a function to multiple values yields multiple result rows:

```sql
create table data(id integer primary key, value text);
insert into data(value) values ('one;two'), ('three;four'), ('five;six');

select * from data;
┌────┬────────────┐
│ id │   value    │
├────┼────────────┤
│ 1  │ one;two    │
│ 2  │ three;four │
│ 3  │ five;six   │
└────┴────────────┘

select id, left, right from data, strcut(data.value, ';');
┌────┬───────┬───────┐
│ id │ left  │ right │
├────┼───────┼───────┤
│ 1  │ one   │ two   │
│ 2  │ three │ four  │
│ 3  │ five  │ six   │
└────┴───────┴───────┘
```

The function body can be any select statement [supported by SQLite](https://www.sqlite.org/lang_select.html).

To list defined table-valued functions, select them from the `sqlean_define` table:

```
sqlite> select name, body from sqlean_define where type = 'table';
┌────────┬────────┐
│  name  │  body  │
├────────┼────────┤
│ strcut │  ...   │
└────────┴────────┘
```

To delete a table-valued function, execute `undefine()` or drop its table:

```
sqlite> select undefine('strcut');
sqlite> select * from strcut('one;two', ';');
Parse error: no such table: strcut
```

Either way, the function is deleted from `sqlean_define` and from all connections.

## Arbitrary SQL statements

`eval(SQL[, SEPARATOR])`

Executes arbitrary SQL and returns the result as string (if any):

```sql
select eval('select 42');
42
select eval('select 10 + 32');
42
select eval('select abs(-42)');
42
select eval('select ''hello''');
hello
```

Joins multiple result values via space or custom separator:

```sql
select eval('select 1, 2, 3');
1 2 3
select eval('select 1, 2, 3', '|');
1|2|3
```

Joins multiple result rows into a single string:

```sql
select eval('select 1; select 2; select 3;');
1 2 3
select eval('select 1, 2, 3; select 4, 5, 6; select 7, 8, 9;');
1 2 3 4 5 6 7 8 9
```

Supports DDL and DML statements:

```sql
select eval('create table tmp(value int)');
select eval('insert into tmp(value) values (1), (2), (3)');
select count(*) from tmp;
3
select eval('select value from tmp');
1 2 3
select eval('drop table tmp');
```

## Security

Defined functions run their body with the full rights of the connection, and they are part of the database: scalar functions are loaded from the `sqlean_define` table when the extension is loaded, and table-valued functions are virtual tables in the schema. So anyone who can modify the database file can change what they do.

SQLite protects against malicious schemas by not allowing dangerous functions, such as `eval`, `define`, or `undefine`, in views and triggers (they are [direct-only](https://sqlite.org/c3ref/c_deterministic.html#sqlitedirectonly)). But a defined function that calls such a function is not blocked, so it can be used to bypass this protection:

```sql
select define('evalf', 'eval(?)');
create view v as select evalf('delete from data');

-- runs the delete
select * from v;
```

The same goes for table-valued functions, and for direct-only functions and virtual tables from other extensions, such as `fileio_write` or `vsv`.

SQLite blocks defined functions in views, triggers, `CHECK` constraints, and `DEFAULT` clauses when the [trusted_schema](https://sqlite.org/pragma.html#pragma_trusted_schema) setting is off. It's on by default, so when opening or attaching a database you don't trust, turn it off before querying it:

```sql
pragma trusted_schema = 0;
```

With `trusted_schema` off, defined functions still work in regular queries, but not in schema objects. Generated columns and indexes don't accept defined functions in any case, because they are not deterministic.

SQLite versions before 3.50.0 don't apply this check to `CHECK` constraints, not even for direct-only functions like `eval`. With these versions, `trusted_schema` doesn't fully protect you, so don't load the extension into a connection that opens untrusted databases.

## Performance

User-defined functions are compiled into prepared statements, so they are pretty fast even on large datasets. A scalar function compiles its body once per query, not once per call.

Given 1M rows table with random data:

```sql
create table data as
select random() as x
from generate_series(1, 1000000);
```

Regular SQL query:

```sql
select max(x+1) from data;
Run Time: real 0.130 user 0.123171 sys 0.006865
```

Scalar function is 2x slower:

```sql
select define('plus', ':x + 1');
select max(plus(x)) from data;
Run Time: real 0.249 user 0.243840 sys 0.005304
```

Table-valued function is 2.5x slower:

```sql
create virtual table plus using define((select :x + 1 as value));
select max(value) from data, plus(data.x);
Run Time: real 0.336 user 0.330145 sys 0.005352
```

## Reference

`define(NAME, BODY)`

Defines a scalar function and stores it in the `sqlean_define` table. Defining an existing function again replaces its body. Errors if the body doesn't compile, or if a function with this name exists but wasn't created with `define()`.

`create virtual table NAME using define((BODY))`

Defines a table-valued function and stores it in the `sqlean_define` table.

`define_free()`

Does nothing. Kept for compatibility: older versions required calling it before disconnecting.

`eval(SQL[, SEPARATOR])`

Executes arbitrary SQL and returns the result as string (if any).

`undefine(NAME)`

Deletes a previously defined function (scalar or table-valued). Does nothing if there is no such function in `sqlean_define`, so it never drops a table it didn't create. Names are case-insensitive.

## Acknowledgements

Adapted from [statement_vtab.c](https://github.com/0x09/sqlite-statement-vtab/blob/master/statement_vtab.c) by 0x09 and [eval.c](https://www.sqlite.org/src/file/ext/misc/eval.c) by D. Richard Hipp.

## Installation and usage

SQLite command-line interface:

```
sqlite> .load ./define
sqlite> select define('sumn', ':n * (:n + 1) / 2');
sqlite> select sumn(5);
```

See [How to install an extension](install.md) for usage with IDE, Python, etc.

↓ [Download](https://github.com/nalgeon/sqlean/releases/latest) the extension.

⛱ [Explore](https://github.com/nalgeon/sqlean) other extensions.

★ [Subscribe](https://antonz.org/subscribe/) to stay on top of new features.
