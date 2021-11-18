# stats: Mathematical statistics in SQLite

Common statistical functions.
Adapted from [extension-functions.c](https://sqlite.org/contrib/) by Liam Healy.

Provides following functions:

-   `mode` - mode,
-   `median` - median (50th percentile),
-   `percentile_25` - 25th percentile,
-   `percentile_75` - 75th percentile,
-   `percentile_90` - 90th percentile,
-   `percentile_95` - 95th percentile,
-   `percentile_99` - 99th percentile,
-   `stddev` or `stddev_samp` - sample standard deviation,
-   `stddev_pop` - population standard deviation,
-   `variance` or `var_samp` - sample variance,
-   `var_pop` - population variance.

## Usage

```
sqlite> .load ./stats
sqlite> select median(value) from generate_series(1, 100);
```

[Download](https://github.com/nalgeon/sqlean/releases/latest)
