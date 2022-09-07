# define: User-defined functions in SQLite

Write arbitrary functions in SQL (as opposed to [application-defined functions](https://sqlite.org/appfunc.html), which require programming in C, Python, or another language).

Adapted from [statement_vtab.c](https://github.com/0x09/sqlite-statement-vtab/blob/master/statement_vtab.c) by 0x09.

### Scalar functions

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

To delete a scalar function, execute `undefine()`, then reconnect to the database:

```
sqlite> select undefine('sumn');
... reconnect
sqlite> select sumn(5);
Parse error: no such function: sumn
```

### Table-valued functions

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

## Usage

```
sqlite> .load ./define
sqlite> select define('sumn', ':n * (:n + 1) / 2');
sqlite> select sumn(5);
```

[Download](https://github.com/nalgeon/sqlean/releases/latest)
