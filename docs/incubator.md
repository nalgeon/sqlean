## cbrt

Cube root function.

Created by [Anton Zhiyanov](https://github.com/nalgeon/sqlean/blob/incubator/src/cbrt.c), MIT License.

```sql
sqlite> .load dist/cbrt
sqlite> select cbrt(27);
3.0
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/cbrt.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/cbrt.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/cbrt.dylib)

## eval

Dinamically runs arbitrary SQL statements.

Created by [D. Richard Hipp](https://sqlite.org/src/file/ext/misc/eval.c), Public Domain.

```sql
sqlite> .load dist/eval
sqlite> select eval('select 42');
42
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/eval.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/eval.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/eval.dylib)

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

## xmltojson

Converts an XML string to the corresponding JSON string.

Created by [jakethaw](https://github.com/jakethaw/xml_to_json), MIT License.

```sql
sqlite> .load dist/xmltojson
sqlite> select xml_to_json('<answer>42</answer>');
{"answer":"42"}
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/xmltojson.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/xmltojson.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/xmltojson.dylib)