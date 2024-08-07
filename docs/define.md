# define: User-defined functions in SQLite

The `sqlean-define` extension allows writing arbitrary functions in SQL (as opposed to [application-defined functions](https://sqlite.org/appfunc.html), which require programming in C, Python, or another language). Or even execute arbitrary SQL from a string.

[Scalar Functions](#scalar-functions) •
[Table-valued functions](#table-valued-functions) •
[Arbitrary SQL statements](#arbitrary-sql-statements) •
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

Scalar functions are compiled into prepared statements. SQLite requires these statements to be freed before the connection is closed. Unfortunately, there is no way to free them automatically. Therefore, always execute `define_free()` before disconnecting:

```
sqlite> .load dist/define
sqlite> select define('subxy', '? - ?');
...
sqlite> select define_free();
sqlite> .exit
```

To delete a scalar function, execute `undefine()`, then reconnect to the database:

```
sqlite> select undefine('sumn');
sqlite> select define_free();
... reconnect
sqlite> select sumn(5);
Parse error: no such function: sumn
```

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

To delete a table-valued function, execute `undefine()`:

```
sqlite> select undefine('strcut');
sqlite> select * from strcut('one;two', ';');
Parse error: no such table: strcut
```

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

## Performance

User-defined functions are compiled into prepared statements, so they are pretty fast even on large datasets.

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

Defines a scalar function and stores it in the `sqlean_define` table.

`create virtual table NAME using define((BODY))`

Defines a table-valued function and stores it in the `sqlean_define` table.

`define_free()`

Frees up occupied resources (compiled statements cache). Should always be called before disconnecting.

`eval(SQL[, SEPARATOR])`

Executes arbitrary SQL and returns the result as string (if any).

`undefine(NAME)`

Deletes a previously defined function (scalar or table-valued).

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
