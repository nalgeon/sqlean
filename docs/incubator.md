## array

One-dimensional arrays for SQLite.

Supports integers, real numbers and strings (with limited max size). Uses 1-based indexing. Stores itself as a blob value.

```sql
sqlite> .load dist/array
sqlite> create table data(arr blob);
sqlite> insert into data(arr) values (array(11, 12, 13));

sqlite> select array_length(arr) from data;
3
sqlite> select array_at(arr, 2) from data;
12
sqlite> select value from data, unnest(data.arr);
11
12
13
```

Provides a lot of features you'd expect from arrays:

-   indexing and search,
-   adding and removing values,
-   slicing and concatenation,
-   aggregation (rows → array),
-   unnesting (array → rows),
-   and some more.

```
intarray()
-> Creates an empty 64-bit integer array.

realarray()
-> Creates an empty 64-bit real array.

textarray(width)
-> Creates an empty text array. Each string has a maximum size of `width` bytes.
   Shorter strings are fine, but longer ones will be truncated to `width` bytes.

array(value, ...)
-> Creates an array filled with provided values.
   Infers array type from the first value. If the value is a string,
   sets max width to the maximum size among provided strings.

array_length(arr)
-> Returns array elements count.

array_at(arr, index)
-> Returns the array element at the specified index (1-based).

array_index(arr, value)
-> Returns the index of the specified value, or 0 if there is no such value.

array_contains(arr, value)
-> Returns 1 if the value if found in the array, 0 otherwise.

array_append(arr, value)
-> Appends the value to the end of the array.

array_insert(arr, index, value)
-> Inserts the value at the specified index,
   shifting following elements to the right.

array_remove_at(arr, index)
-> Removes the element at the specified index,
   shifting following elements to the left.

array_remove(arr, value)
-> Removes the value from the array. If there are multiple such values,
   only removes the first one.

array_clear(arr)
-> Removes all elements from the array.

array_slice(arr, start[, end])
-> Returns a slice of the array from the `start` index inclusive
   to the `end` index non-inclusive (or to the end of the array).

array_concat(arr, other)
-> Concatenates two arrays.

array_agg(expr)
-> Aggregates a set of values into the array (reverse operation for unnest()).

unnest(arr)
-> Expands the array to a set of values (reverse operation for array_agg()).
   Implemented as a table-valued function.
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/array.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/array.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/array.dylib)

## besttype

Implements `ToBestType(x)` function:

-   NULL returns NULL
-   TEXT/BLOB that "looks like a number" returns the number as Integer or Real as appropriate
-   TEXT/BLOB that is zero-length returns NULL
-   Otherwise returns what was given

Created by [Keith Medcalf](http://www.dessus.com/files/sqlnumeric.c), Public Domain.

```sql
sqlite> .load dist/besttype
sqlite> select tobesttype('42.13');
42.13
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/besttype.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/besttype.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/besttype.dylib)

## bloom

Bloom filter — a fast index to tell if a value is probably in a table or certainly isn't.

Created by [Shawn Wagner](https://github.com/shawnw/useful_sqlite_extensions/blob/master/src/bloom_filter.c), MIT License.

```sql
sqlite> .load dist/bloom
sqlite> create virtual table plants using bloom_filter(20);
sqlite> insert into plants values ('apple'), ('asparagus'), ('cabbage'), ('grass');
sqlite> select count(*) from plants('apple');
1
sqlite> select count(*) from plants('lemon');
0
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/bloom.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/bloom.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/bloom.dylib)

## btreeinfo

Shows information about all btrees (tables and indexes) in an SQLite database file:

-   if the btree has a rowid,
-   estimated number of entries,
-   estimated number of pages,
-   depth of the btree.

Requires [sqlite_dbpage](https://sqlite.org/dbpage.html) virtual table.

Created by [D. Richard Hipp](https://sqlite.org/src/file/ext/misc/btreeinfo.c), Public Domain.

```sql
sqlite> .load dist/btreeinfo
sqlite> create table data as select * from generate_series(1, 9999);
sqlite> select type, name, hasrowid, nentry, npage, depth from sqlite_btreeinfo;
┌───────┬───────────────┬──────────┬────────┬───────┬───────┐
│ type  │     name      │ hasRowid │ nEntry │ nPage │ depth │
├───────┼───────────────┼──────────┼────────┼───────┼───────┤
│ table │ sqlite_schema │ 1        │ 2      │ 1     │ 1     │
│ table │ data          │ 1        │ 10010  │ 22    │ 2     │
└───────┴───────────────┴──────────┴────────┴───────┴───────┘
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/btreeinfo.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/btreeinfo.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/btreeinfo.dylib)

## cbrt

Cube root function.

Created by [Anton Zhiyanov](https://github.com/nalgeon/sqlean/blob/incubator/src/cbrt.c), MIT License.

```sql
sqlite> .load dist/cbrt
sqlite> select cbrt(27);
3.0
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/cbrt.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/cbrt.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/cbrt.dylib)

## classifier

Binary classifier via logistic regression.

Created by [Alex Wilson](https://github.com/mrwilson/squib/blob/master/classifier.c), MIT License.

```sql
sqlite> .load dist/classifier
sqlite> select train(feature1, feature2, feature3, label) from data;
sqlite> select classify(1, 1, 0);
0.763584749816848
sqlite> select classify(0, 0, 1);
0.225364243341812
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/classifier.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/classifier.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/classifier.dylib)

## closure

Navigate hierarchic tables with parent/child relationship.

Created by [D. Richard Hipp](https://sqlite.org/src/file/ext/misc/closure.c), Public Domain.

```sql
.load dist/closure

-- create a parent/child table
create table employees (
    id integer primary key,
    parent_id integer,
    name varchar(50)
);
create index employees_parent_idx on employees(parent_id);
insert into employees
(id, parent_id, name)
values
(11, null, 'Diane'),
(12, 11, 'Bob'),
(21, 11, 'Emma'),
(22, 21, 'Grace'),
(23, 21, 'Henry'),
(24, 21, 'Irene'),
(25, 21, 'Frank'),
(31, 11, 'Cindy'),
(32, 31, 'Dave'),
(33, 31, 'Alice');
```

```
Diane
└ Bob
└ Emma
  ├ Grace
  ├ Henry
  ├ Irene
  └ Frank
└ Cindy
  ├ Dave
  └ Alice
```

```sql
-- create a virtual hierarchy table
create virtual table hierarchy using transitive_closure(
    tablename = "employees",
    idcolumn = "id",
    parentcolumn = "parent_id"
);

-- select hierarchy branch rooted at Cindy
select employees.id, name from employees, hierarchy
where employees.id = hierarchy.id and hierarchy.root = 31;
┌────┬───────┐
│ id │ name  │
├────┼───────┤
│ 31 │ Cindy │
│ 32 │ Dave  │
│ 33 │ Alice │
└────┴───────┘
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/closure.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/closure.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/closure.dylib)

## compress

Compress / uncompress data using zlib. Doesn't work on Windows.

Created by [D. Richard Hipp](https://sqlite.org/src/file/ext/misc/compress.c), Public Domain.

```sql
sqlite> .load dist/compress
sqlite> select hex(compress('hello world'));
8B789CCB48CDC9C95728CF2FCA4901001A0B045D
sqlite> select uncompress(compress('hello world'));
hello world
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/compress.so) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/compress.dylib)

## cron

Compares dates against cron patterns, whether they match or not.

Created by [David Schramm](https://github.com/daschr/sqlite3_extensions/blob/master/cron.c), MIT License.

```sql
sqlite> .load dist/cron
sqlite> select cron_match('2006-01-02 15:04:05','4 15 * * *');
1
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/cron.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/cron.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/cron.dylib)

## dbdump

Export database or table structure and contents into a single UTF-8 string.

Created by [D. Richard Hipp](https://sqlite.org/src/file/ext/misc/dbdump.c), Public Domain.

`dbdump([schema[, table]])`

-   `schema` is the schema to be dumped. Default is "main" but can also be "temp" or any attached database.
-   `table` is the table to be dumped. If `table` is omitted, then all tables are dumped.

```sql
.load dist/dbdump

create table employees (id integer primary key, name text);
insert into employees (name) values ('Diane'), ('Bob');

create table expenses (year integer, month integer, expense integer);
insert into expenses values (2020, 1, 82), (2020, 2, 75), (2020, 3, 104);

select dbdump('main', 'employees');
```

```sql
PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE employees (id integer primary key, name text);
INSERT INTO employees VALUES(1,'Diane');
INSERT INTO employees VALUES(2,'Bob');
COMMIT;
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/dbdump.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/dbdump.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/dbdump.dylib)

## decimal

Arbitrary-precision decimal arithmetic on numbers stored as text strings. Because the numbers are stored to arbitrary precision and as text, no approximations are needed. Computations can be done exactly.

Created by [D. Richard Hipp](https://sqlite.org/src/file/ext/misc/decimal.c), Public Domain.

There are three math functions available:

-   `decimal_add(A,B)`
-   `decimal_sub(A,B)`
-   `decimal_mul(A,B)`

These functions respectively add, subtract, and multiply their arguments and return a new text string that is the decimal representation of the result. There is no division operator at this time.

Use the `decimal_cmp(A,B)` to compare two decimal values. The result will be negative, zero, or positive if A is less than, equal to, or greater than B, respectively.

The `decimal_sum(X)` function is an aggregate, like the built-in `sum()` aggregate function, except that `decimal_sum()` computes its result to arbitrary precision and is therefore precise.

```sql
sqlite> .load dist/decimal
sqlite> select 0.1 + 0.2 = 0.3;
0
sqlite> select decimal_add(decimal('0.1'), decimal('0.2')) = decimal('0.3');
1
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/decimal.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/decimal.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/decimal.dylib)

## define

Create scalar and table-valued functions from SQL.

Created by [0x09](https://github.com/0x09/sqlite-statement-vtab/blob/master/statement_vtab.c), Public Domain.

Define a scalar function to calculate the sum of numbers 1..n:

```sql
sqlite> .load dist/define
sqlite> select create_function("sumn", "?1 * (?1 + 1) / 2");
sqlite> select sumn(5);
15
```

Define a table-valued function to split the string into two by the separator:

```sql
create virtual table split using statement((
  select
    substr(:str, 1, instr(:str, :sep) - 1) as left_part,
    :sep as separator,
    substr(:str, instr(:str, :sep) + 1) as right_part
));

select * from split('one;two', ';');
select * from split('one;two', ';');
┌───────────┬───────────┬────────────┐
│ left_part │ separator │ right_part │
├───────────┼───────────┼────────────┤
│ one       │ ;         │ two        │
└───────────┴───────────┴────────────┘
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/define.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/define.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/define.dylib)

## envfuncs

Returns the value of the environment variable.

Created by [John Howie](https://github.com/jhowie/sqlite3-ext/blob/main/envfuncs.c), BSD-3-Clause License.

```sql
sqlite> .load dist/envfuncs
sqlite> select getenv('USER');
antonz
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/envfuncs.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/envfuncs.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/envfuncs.dylib)

## eval

Dinamically runs arbitrary SQL statements.

Created by [D. Richard Hipp](https://sqlite.org/src/file/ext/misc/eval.c), Public Domain.

```sql
sqlite> .load dist/eval
sqlite> select eval('select 42');
42
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/eval.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/eval.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/eval.dylib)

## fcmp

Floating-point numbers comparison and rounding.

Created by [Keith Medcalf](http://www.dessus.com/files/sqlfcmp.c), Public Domain.

```sql
sqlite> select 0.1*3 = 0.3;
0
sqlite> .load dist/fcmp
sqlite> select feq(0.1*3, 0.3);
1
```

Floating-point numbers comparison:

```
flt(x[, y[, u]])   -> x less than y
fle(x[, y[, u]])   -> x less or equal y
feq(x[, y[, u]])   -> x equal y
fge(x[, y[, u]])   -> x greater or equal y
fgt(x[, y[, u]])   -> x greater than y
fne(x[, y[, u]])   -> x not equal y
```

Rounding:

```
roundho(x)   -> Half to Odd
roundhe(x)   -> Half to Even
roundhu(x)   -> Half Up
roundhd(x)   -> Half Down
roundha(x)   -> Half Away from 0
roundht(x)   -> Half Towards 0
money(x)     -> Money (Half to Even, 4 digits)

rounddu(x)   -> Directed Up
rounddd(x)   -> Directed Down
roundda(x)   -> Directed Away from 0
rounddt(x)   -> Directed Towards 0
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/fcmp.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/fcmp.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/fcmp.dylib)

## ieee754

Converts a floating-point number F between its binary64 representation and the M×2^E format (`F = M × 2^E`).

Created by [D. Richard Hipp](https://sqlite.org/src/file/ext/misc/ieee754.c), Public Domain.

-   `ieee754(F)`. Converts a floating-point number to the mantissa/exponent string representation.
-   `ieee754(M,E)`. Converts a mantissa/exponent combination to the floating-point number.
-   `ieee754_mantissa(F)`. Given a floating-point number, extracts the mantissa component.
-   `ieee754_exponent(F)`. Given a floating-point number, extracts the exponent component.
-   `ieee754_to_blob(F)`. Converts a floating-point number into an 8-byte blob that is the big-endian binary64 encoding of that number.
-   `ieee754_from_blob(B)`. Converts an 8-byte blob into the floating-point value that the binary64 encoding represents.

```sql
sqlite> .load dist/ieee754
sqlite> select ieee754(45.25);
ieee754(181,-2)
sqlite> select ieee754_mantissa(45.25);
181
sqlite> select ieee754_exponent(45.25);
-2
sqlite> select ieee754(181,-2);
45.25
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/ieee754.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/ieee754.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/ieee754.dylib)

## isodate

Additional date and time functions:

-   Extract date parts according to ISO 8601: week day, week of a year, year
-   Convert ISO 8601 datetime to unix timestamp

Created by [Harald Hanche-Olsen](https://sqlite.org/forum/forumpost/cb8923cb4db72f2e?t=h) and [Richard Hipp](https://sqlite.org/src/artifact/d0f09f7924a27e0d?ln=968-984), Public Domain.

```sql
sqlite> .load dist/isodate
sqlite> select iso_weekday('2021-12-22');
3
sqlite> select iso_week('2021-12-22');
51
sqlite> select iso_year('2021-12-22');
2021
sqlite> select unixepoch('2021-12-22 12:34:45');
1640176485
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/isodate.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/isodate.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/isodate.dylib)

## json2

Even more JSON functions.

Created by [Shawn Wagner](https://github.com/shawnw/useful_sqlite_extensions/blob/master/src/json_funcs.c), MIT License.

```sql
sqlite> .load dist/json2
sqlite> select json_keys('{"a":1, "b":2, "c":3}');
["a","b","c"]
```

```
json_pretty(json)
-> pretty-print json for readability

json_equal(json, json)
-> check if two JSONs are equal

json_length(json)
json_length(json, path)
-> number of array elements or object keys

json_keys(json)
json_keys(json, path)
-> keys of the given object

json_contains_path(json, path)
-> check if the object has data at the given path
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/json2.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/json2.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/json2.dylib)

## math2

Even more math functions and bit arithmetics.

Created by [Keith Medcalf](http://www.dessus.com/files/sqlmath.c), Public Domain.

```sql
sqlite> select round(m_e(), 3)
2.718
```

Constants:

```
m_e()           -> Euler's number (e)
m_log2e()       -> log2(e)
m_log10e()      -> log10(e)
m_ln2()         -> ln(2)
m_ln10()        -> ln(10)
m_pi()          -> Pi number
m_pi_2()        -> pi/2
m_pi_4()        -> pi/4
m_1_pi()        -> 1/pi
m_2_pi()        -> 2/pi
m_2_sqrtpi()    -> 2/sqrt(pi)
m_sqrt2()       -> sqrt(2)
m_sqrt1_2()     -> sqrt(0.5)
m_deg2rad()     -> radians(1)
m_rad2deg()     -> degrees(1)
```

Bit arithmetics:

```
isset(value, bit, bit, bit ...)
-> true if all bits are set in value

isclr(value, bit, bit, bit ...)
-> true if all bits are clr in value

ismaskset(value, mask)
-> true if all set bits in mask set in value

ismaskclr(value, mask)
-> true if all set bits set in mask are clr in value

bitmask(bit, bit, bit ...)
-> value of bitmask with bits set

setbits(value, bit, bit, ...)
-> value with bits set

clrbits(value, bit, bit, ...)
-> value with bits cleared

bitmask(bit)
-> aggregate function, set bits and return resulting mask
```

Other functions:

```
fabs(x)
-> abs value

ldexp(x, y)
-> x * 2^y

mantissa(x), exponent(x)
-> x = mantissa * 2^exponent

trunc(x), frac(x)
-> integer and fractional parts

fromhex(hex_str)
-> hexadecimal to decimal
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/math2.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/math2.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/math2.dylib)

## pearson

Returns Pearson correlation coefficient between two data sets.

Created by [Alex Wilson](https://github.com/mrwilson/squib/blob/master/pearson.c), MIT License.

```sql
sqlite> .load dist/pearson
sqlite> create table data as select value as x, value*2 as y from generate_series(1, 8);
sqlite> select pearson(x, y) from data;
1.0
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/pearson.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/pearson.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/pearson.dylib)

## pivotvtab

Creates a pivot table from a regular one.

Created by [jakethaw](https://github.com/jakethaw/pivot_vtab), MIT License.

Suppose we have a table with quarterly sales for years 2018-2021:

```sql
select * from sales;
┌──────┬─────────┬─────────┐
│ year │ quarter │ revenue │
├──────┼─────────┼─────────┤
│ 2018 │ 1       │ 12000   │
│ 2018 │ 2       │ 39600   │
│ 2018 │ 3       │ 24000   │
│ 2018 │ 4       │ 18000   │
│ 2019 │ 1       │ 26400   │
│ 2019 │ 2       │ 32400   │
│ ...  │ ...     │ ...     │
│ 2021 │ 4       │ 39000   │
└──────┴─────────┴─────────┘
```

And we want to transform it into the 2D table with years as rows and columns as quarters:

```
┌──────┬───────┬───────┬───────┬───────┐
│ year │  Q1   │  Q2   │  Q3   │  Q4   │
├──────┼───────┼───────┼───────┼───────┤
│ 2018 │ 12000 │ 39600 │ 24000 │ 18000 │
│ 2019 │ 26400 │ 32400 │ 26400 │ 26400 │
│ 2020 │ 15000 │ 25200 │ 29700 │ 26400 │
│ 2021 │ 27000 │ 61200 │ 42000 │ 39000 │
└──────┴───────┴───────┴───────┴───────┘
```

This looks like a job for `pivotvtab`!

First we create the 'rows' (years) table:

```sql
create table years as
select value as year from generate_series(2018, 2021);
```

Then the 'columns' (quarters) table:

```sql
create table quarters as
select value as quarter, 'Q'||value as name from generate_series(1, 4);
```

And finally the pivot table:

```sql
.load dist/pivotvtab

create virtual table sales_by_year using pivot_vtab (
  -- rows
  (select year from years),
  -- columns
  (select quarter, name from quarters),
  -- data
  (select revenue from sales where year = ?1 and quarter = ?2)
);
```

Voilà:

```sql
select * from sales_by_year;
┌──────┬───────┬───────┬───────┬───────┐
│ year │  Q1   │  Q2   │  Q3   │  Q4   │
├──────┼───────┼───────┼───────┼───────┤
│ 2018 │ 12000 │ 39600 │ 24000 │ 18000 │
│ 2019 │ 26400 │ 32400 │ 26400 │ 26400 │
│ 2020 │ 15000 │ 25200 │ 29700 │ 26400 │
│ 2021 │ 27000 │ 61200 │ 42000 │ 39000 │
└──────┴───────┴───────┴───────┴───────┘
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/pivotvtab.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/pivotvtab.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/pivotvtab.dylib)

## recsize

Еstimates total record size.

Created by [Keith Medcalf](http://www.dessus.com/files/sqlsize.c), Public Domain.

```sql
sqlite> .load dist/recsize
sqlite> select recsize(10);
3
sqlite> select recsize(10, 20, 30);
7
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/recsize.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/recsize.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/recsize.dylib)

## spellfix

Provides a mechanism to search a large vocabulary for close matches.

Created by [D. Richard Hipp](https://sqlite.org/src/file/ext/misc/spellfix.c), Public Domain.

[Documentation](https://sqlite.org/spellfix1.html)

```sql
sqlite> .load dist/spellfix
sqlite> create virtual table dictionary using spellfix1;
sqlite> insert into dictionary(word)
values ('similarity'), ('search'), ('is'), ('awesome');
sqlite> select word from dictionary where word match 'awesoem';
awesome
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/spellfix.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/spellfix.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/spellfix.dylib)

## sqlar

Compress / uncompress data with zlib using the SQL Archive approach:

-   Only compress the value if it yields a smaller blob.
-   Uncompress the value if needed given the orizinal value size.

Doesn't work on Windows.

Created by [D. Richard Hipp](https://sqlite.org/src/file/ext/misc/sqlar.c), Public Domain.

```sql
sqlite> .load dist/sqlar
sqlite> select length(sqlar_compress(zeroblob(1024)));
17
sqlite> select sqlar_uncompress( sqlar_compress(zeroblob(1024)), 1024 ) = zeroblob(1024);
1
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/sqlar.so) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/sqlar.dylib)

## stats2

Even more math statistics functions.

Created by [Keith Medcalf](http://www.dessus.com/files/sqlfunc.c), Public Domain.

```sql
sqlite> .load dist/stats2
sqlite> select sem(value) from generate_series(1, 99);
2.88675134594813
```

Aggregate functions (also available as window aggregates):

```
avg(v)
aavg(v)
gavg(v)
rms(v)
stdev(v)
stdevp(v)
var(v)
varp(v)
sem(v)
ci(v)
skew(v)
skewp(v)
kurt(v)
kurtp(v)
```

Weighted aggregate functions (also available as weighted window aggregates):

```
avg(v, w)
stdev(v, w)
stdevp(v, w)
var(v, w)
varp(v, w)
sem(v, w)
ci(v, w)
```

Other aggregate window functions:

```
FirstNotNull(v)
LastNotNull(v)
```

Other aggregate non-window functions:

```
range(v)
median(v)
covar(x, y)
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/stats2.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/stats2.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/stats2.dylib)

## stats3

And even more math statistics functions.

Created by [Shawn Wagner](https://github.com/shawnw/useful_sqlite_extensions/blob/master/src/math_funcs.c), MIT License.

```sql
sqlite> .load dist/stats3
sqlite> select geo_mean(value) from generate_series(1, 99);
37.6231004740974
```

```
corr(x, y)
-> correlation coefficient

covar_samp(x, y)
covar_pop(x, y)
-> sample and population covariance

geo_mean(v)
harm_mean(v)
median(v)
mode(v)
-> mean values

q1(v)
q3(v)
-> 1st and 3rd quartile values

iqr(v)
-> interquartile range

product(v)
-> product of values
```

All functions are also available as window aggregates.

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/stats3.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/stats3.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/stats3.dylib)

## text2

Even more string functions.

Created by [Shawn Wagner](https://github.com/shawnw/useful_sqlite_extensions/blob/master/src/more_str_funcs.c), MIT License.

-   `repeat(string, count)` - repeat the `string` `count` times
-   `concat(string, ...)` - concatenate strings
-   `concat_ws(sep, string, ...)` - concatenate strings using `sep` as a separator

```sql
sqlite> .load dist/text2
sqlite> select repeat('*', 3);
***
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/text2.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/text2.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/text2.dylib)

## uint

[Natural](https://en.wikipedia.org/wiki/Natural_sort_order) string sorting and comparison.

Created by [D. Richard Hipp](https://sqlite.org/src/file/ext/misc/uint.c), Public Domain.

```sql
sqlite> .load dist/uint
sqlite> select '2' < '10' collate uint;
1
sqlite> select '01' = '1' collate uint;
1
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/uint.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/uint.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/uint.dylib)

## unhex

Reverse function for `hex()`. Decodes the previously hex-encoded blob and returns it.

Created by [Keith Medcalf](http://www.dessus.com/files/sqlunhex.c), Public Domain.

```sql
sqlite> .load dist/unhex
sqlite> select unhex(hex('hello'));
hello
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/unhex.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/unhex.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/unhex.dylib)

## xmltojson

Converts an XML string to the corresponding JSON string.

Created by [jakethaw](https://github.com/jakethaw/xml_to_json), MIT License.

```sql
sqlite> .load dist/xmltojson
sqlite> select xml_to_json('<answer>42</answer>');
{"answer":"42"}
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/xmltojson.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/xmltojson.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/xmltojson.dylib)

## zipfile

Read and write zip files, both in memory and on disk. Doesn't work on Windows.

Created by [D. Richard Hipp](https://sqlite.org/src/file/ext/misc/zipfile.c), Public Domain.

[Documentation](https://sqlite.org/zipfile.html)

```sql
sqlite> .load dist/zipfile

sqlite> create virtual table temp.zip using zipfile('test.zip');
sqlite> insert into temp.zip(name, data) values('readme.txt', 'a glorious zip file');

sqlite> select name, data from zipfile('test.zip');
readme.txt|a glorious zip file
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/zipfile.so) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/zipfile.dylib)
