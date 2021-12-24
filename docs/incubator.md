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

## cbrt

Cube root function.

Created by [Anton Zhiyanov](https://github.com/nalgeon/sqlean/blob/incubator/src/cbrt.c), MIT License.

```sql
sqlite> .load dist/cbrt
sqlite> select cbrt(27);
3.0
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/cbrt.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/cbrt.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/cbrt.dylib)

## cron

Compares dates against cron patterns, whether they match or not.

Created by [David Schramm ](https://github.com/daschr/sqlite3_extensions/blob/master/cron.c), MIT License.

```sql
sqlite> .load dist/cron
sqlite> select cron_match('2006-01-02 15:04:05','4 15 * * *');
1
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/cron.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/cron.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/cron.dylib)

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

Floating point numbers comparison and rounding.

Created by [Keith Medcalf](http://www.dessus.com/files/sqlfcmp.c), Public Domain.

```sql
sqlite> select 0.1*3 = 0.3;
0
sqlite> .load dist/fcmp
sqlite> select feq(0.1*3, 0.3);
1
```

Floating point numbers comparison:

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
