# stats: Mathematical statistics in SQLite

Common statistical functions. Adapted from [extension-functions.c](https://sqlite.org/contrib/) by Liam Healy, [percentile.c](https://sqlite.org/src/file/ext/misc/percentile.c) and [series.c](https://sqlite.org/src/file/ext/misc/series.c) by D. Richard Hipp.

### Aggregate functions

-   `median(x)` — median (50th percentile),
-   `percentile_25(x)` — 25th percentile,
-   `percentile_75(x)` — 75th percentile,
-   `percentile_90(x)` — 90th percentile,
-   `percentile_95(x)` — 95th percentile,
-   `percentile_99(x)` — 99th percentile,
-   `percentile(x, perc)` — custom percentile (`perc` between 0 and 100),
-   `stddev(x)` or `stddev_samp(x)` — sample standard deviation,
-   `stddev_pop(x)` — population standard deviation,
-   `variance(x)` or `var_samp(x)` — sample variance,
-   `var_pop(x)` — population variance.

### generate_series(start[, stop[, step]])

This table-valued function generates a sequence of integer values starting with `start`, ending with `stop` (inclusive) with an optional `step`.

Generate all integers from 1 to 99:

```sql
select * from generate_series(1, 99);
```

Generate all multiples of 5 less than or equal to 100:

```sql
select * from generate_series(5, 100, 5);
```

Generate 20 random integer values:

```sql
select random() from generate_series(1, 20);
```

The `generate_series()` table has a single result column named `value` holding integer values, and a number of rows determined by the parameters `start`, `stop`, and `step`. The first row of the table has a value of `start`. Subsequent rows increase by `step` up to `stop`.

`stop` defaults to 9223372036854775807. `step` defaults to 1.

## Usage

```
sqlite> .load ./stats
sqlite> select median(value) from generate_series(1, 99);
```

[⬇️ Download](https://github.com/nalgeon/sqlean/releases/latest) •
[✨ Explore](https://github.com/nalgeon/sqlean) •
[🚀 Follow](https://twitter.com/ohmypy)
