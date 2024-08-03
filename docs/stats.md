# stats: Mathematical Statistics in SQLite

Common statistical functions. Adapted from [extension-functions.c](https://sqlite.org/contrib/) by Liam Healy, [percentile.c](https://sqlite.org/src/file/ext/misc/percentile.c) and [series.c](https://sqlite.org/src/file/ext/misc/series.c) by D. Richard Hipp.

### Aggregate Functions

-   `stats_median(x)` — median (50th percentile),
-   `stats_p25(x)` — 25th percentile,
-   `stats_p75(x)` — 75th percentile,
-   `stats_p90(x)` — 90th percentile,
-   `stats_p95(x)` — 95th percentile,
-   `stats_p99(x)` — 99th percentile,
-   `stats_perc(x, p)` — custom percentile (`p` between 0 and 100),
-   `stats_stddev(x)` or `stats_stddev_samp(x)` — sample standard deviation,
-   `stats_stddev_pop(x)` — population standard deviation,
-   `stats_var(x)` or `stats_var_samp(x)` — sample variance,
-   `stats_var_pop(x)` — population variance.

### stats_seq

```text
stats_seq(start[, stop[, step]])
generate_series(start[, stop[, step]])
```

This table-valued function generates a sequence of integer values starting with `start`, ending with `stop` (inclusive) with an optional `step`.

Generate all integers from 1 to 99:

```sql
select * from stats_seq(1, 99);
```

Generate all multiples of 5 less than or equal to 100:

```sql
select * from stats_seq(5, 100, 5);
```

Generate 20 random integer values:

```sql
select random() from stats_seq(1, 20);
```

The `stats_seq()` table has a single result column named `value` holding integer values, and a number of rows determined by the parameters `start`, `stop`, and `step`. The first row of the table has a value of `start`. Subsequent rows increase by `step` up to `stop`.

`stop` defaults to 9223372036854775807. `step` defaults to 1.

## Installation and Usage

SQLite command-line interface:

```
sqlite> .load ./stats
sqlite> select stats_median(value) from stats_seq(1, 99);
```

See [How to Install an Extension](install.md) for usage with IDE, Python, etc.

[⬇️ Download](https://github.com/nalgeon/sqlean/releases/latest) •
[✨ Explore](https://github.com/nalgeon/sqlean) •
[🚀 Follow](https://antonz.org/subscribe/)
