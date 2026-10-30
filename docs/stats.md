# stats: Mathematical statistics in SQLite

The `sqlean-stats` extension provides common statistical functions: percentiles, variance, standard deviation, and a sequence generator.

[Concepts](#concepts) •
[Percentiles](#percentiles) •
[Variance and deviation](#variance-and-deviation) •
[Sequences](#sequences) •
[Acknowledgements](#acknowledgements) •
[Installation and usage](#installation-and-usage)

## Concepts

The percentile and variance functions are aggregates, like `sum` or `avg`. They work with `group by`, `distinct`, and as window functions with any frame:

```sql
select value, stats_median(value) over (order by value rows 2 preceding)
from stats_seq(1, 5);
-- 1|1.0
-- 2|1.5
-- 3|2.0
-- 4|3.0
-- 5|4.0
```

The result is always a real number.

**Percentiles** interpolate linearly between the two closest values, as Excel's `PERCENTILE.INC` and NumPy's `percentile` (the default method) do. The p-th percentile of n sorted values is at the zero-based position `p/100 × (n-1)`: the 0th percentile is the minimum, the 100th is the maximum, and if the position falls between two values, the result is in between, proportionally.

**Sample and population**. The variance and deviation functions come in two flavors. The _sample_ ones (`stats_var`, `stats_stddev`) treat the values as a sample from a larger population and divide by n-1. The _population_ ones (`stats_var_pop`, `stats_stddev_pop`) treat the values as the whole population and divide by n. The sample variance of a single value is undefined, so it is `NULL`.

**Infinities**. Percentiles treat an infinite value like any other: the median of `1`, `+Inf` and `+Inf` is `+Inf`. An interpolation between `-Inf` and `+Inf` is undefined, so it is `NULL`. The variance of values that include an infinity is undefined, so it is `NULL`. A variance too large to fit a real number is `+Inf`.

### NULL and invalid input

Like SQLite's own aggregates, the functions skip `NULL` values. With no rows, or only `NULL`s, the result is `NULL`.

Text that looks like a number is converted: `'42'` is `42`. Other text and blobs are invalid data, and are skipped as `NULL`s are, so one bad row doesn't abort the whole query.

The percentile in `stats_perc` is part of the query, not the data, so an invalid one raises an error:

- A percentile that is not a number from 0 to 100: `stats_perc(x, 101)` fails with `percentile should be a number between 0 and 100`.
- A percentile that differs between rows: `stats_perc(x, x)` fails with `percentile should be the same for all rows`. Differences of 0.001 or less are allowed, and the first row's percentile is used.

A row with a `NULL` percentile is skipped.

## Percentiles

### stats_median

```text
stats_median(x)
```

Returns the median (the 50th percentile) of the values.

```sql
select stats_median(value) from stats_seq(1, 99);
-- 50.0

select stats_median(column1) from (values (1), (2), (3), (10));
-- 2.5
```

Aliased as `median`.

### stats_p25

```text
stats_p25(x)
stats_p75(x)
stats_p90(x)
stats_p95(x)
stats_p99(x)
```

Return the 25th, 75th, 90th, 95th and 99th percentiles of the values.

```sql
select stats_p25(value), stats_p75(value), stats_p90(value), stats_p95(value), stats_p99(value)
from stats_seq(1, 100);
-- 25.75|75.25|90.1|95.05|99.01
```

Aliased as `percentile_25`, `percentile_75`, `percentile_90`, `percentile_95` and `percentile_99`.

### stats_perc

```text
stats_perc(x, p)
```

Returns the p-th percentile of the values. `p` is a number from 0 to 100, the same for all rows.

```sql
select stats_perc(value, 10) from stats_seq(1, 100);
-- 10.9

select stats_perc(value, 0), stats_perc(value, 100) from stats_seq(1, 100);
-- 1.0|100.0
```

Aliased as `percentile`.

## Variance and deviation

### stats_var

```text
stats_var(x)
```

Returns the sample variance of the values. With fewer than two values, returns `NULL`.

```sql
select stats_var(column1) from (values (2), (4), (4), (4), (5), (5), (7), (9));
-- 4.57142857142857

select stats_var(5);
-- NULL
```

Aliased as `stats_var_samp`, `variance` and `var_samp`.

### stats_var_pop

```text
stats_var_pop(x)
```

Returns the population variance of the values.

```sql
select stats_var_pop(column1) from (values (2), (4), (4), (4), (5), (5), (7), (9));
-- 4.0

select stats_var_pop(5);
-- 0.0
```

Aliased as `var_pop`.

### stats_stddev

```text
stats_stddev(x)
```

Returns the sample standard deviation of the values: the square root of the sample variance. With fewer than two values, returns `NULL`.

```sql
select stats_stddev(column1) from (values (2), (4), (4), (4), (5), (5), (7), (9));
-- 2.1380899352994
```

Aliased as `stats_stddev_samp`, `stddev` and `stddev_samp`.

### stats_stddev_pop

```text
stats_stddev_pop(x)
```

Returns the population standard deviation of the values: the square root of the population variance.

```sql
select stats_stddev_pop(column1) from (values (2), (4), (4), (4), (5), (5), (7), (9));
-- 2.0
```

Aliased as `stddev_pop`.

## Sequences

### stats_seq

```text
stats_seq(start[, stop[, step]])
```

A table-valued function that generates the integers from `start` to `stop` (inclusive), `step` apart. `stop` defaults to 4294967295 (2³²-1), `step` defaults to 1.

```sql
select group_concat(value) from stats_seq(1, 5);
-- 1,2,3,4,5

select group_concat(value) from stats_seq(0, 20, 5);
-- 0,5,10,15,20

select random() from stats_seq(1, 20);
-- (20 random numbers)
```

The table has a single column, `value`. The arguments are also available as hidden columns `start`, `stop` and `step`, so `stats_seq(1, 5)` is the same as `stats_seq where start = 1 and stop = 5`. The `rowid` is the position in the sequence, starting from 1.

The sequence ends at the last value that is not past `stop`, so it never overflows: `stats_seq(9223372036854775806, 9223372036854775807)` is the two largest integers. If `start` is greater than `stop`, there are no rows.

A negative `step` gives the same values in decreasing order. A `step` of 0 is 1.

```sql
select group_concat(value) from stats_seq(1, 10, -3);
-- 10,7,4,1
```

The arguments are integers. Numeric text and whole reals are converted, so `stats_seq(1, 1e6)` works. If any argument is `NULL` or not an integer (`1.5`, `'abc'`, or a blob), there are no rows. Without `start`, the query fails with `table function stats_seq requires an argument`.

Aliased as `generate_series`.

## Acknowledgements

Adapted from [extension-functions.c](https://sqlite.org/contrib/) by Liam Healy, [percentile.c](https://sqlite.org/src/file/ext/misc/percentile.c) and [series.c](https://sqlite.org/src/file/ext/misc/series.c) by D. Richard Hipp.

## Installation and usage

SQLite command-line interface:

```
sqlite> .load ./stats
sqlite> select stats_median(value) from stats_seq(1, 99);
```

See [How to install an extension](install.md) for usage with IDE, Python, etc.

↓ [Download](https://github.com/nalgeon/sqlean/releases/latest) the extension.

⛱ [Explore](https://github.com/nalgeon/sqlean) other extensions.

★ [Subscribe](https://antonz.org/subscribe/) to stay on top of new features.
