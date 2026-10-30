-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/stats

-- Percentiles interpolate linearly between the closest ranks
-- (as Excel's PERCENTILE.INC and NumPy's default method do):
-- the p-th percentile of n sorted values is at the zero-based index p/100*(n-1).
-- 1..99: sample variance = n(n+1)/12 = 825, population variance = (n^2-1)/12 = 816.67
-- 2,4,4,4,5,5,7,9: population stddev = 2, sample variance = 32/7

-- stats_median, median
select '01_01', stats_median(value) = 50.0 from stats_seq(1, 99);
select '01_02', median(value) = 50.0 from stats_seq(1, 99);
-- an even count gives the mean of the two middle values
select '01_03', stats_median(value) = 2.5 from stats_seq(1, 4);
select '01_04', stats_median(v) = 2.0 from (select 3 v union all select 1 union all select 2);
select '01_05', stats_median(5) = 5.0;
select '01_06', stats_median(v) = -1.5 from (select -1 v union all select -2);
select '01_07', stats_median(v) = 2.5 from (select 1.5 v union all select 3.5);
-- the result is always real
select '01_08', typeof(stats_median(value)) = 'real' from stats_seq(1, 3);
select '01_09', stats_median(distinct v) = 5.5 from (select 1 v union all select 1 union all select 10);
-- the interpolation overflows: the true median is 0
select '01_10', stats_median(v) = 9e999 from (select 1e308 v union all select -1e308);
-- each group gets its own context
select '01_11', group_concat(m) = '6.0,5.0' from (select stats_median(value) m from stats_seq(1, 10) group by value % 2);

-- stats_p25, stats_p75, stats_p90, stats_p95, stats_p99
select '02_01', stats_p25(value) = 25.5 from stats_seq(1, 99);
select '02_02', stats_p75(value) = 74.5 from stats_seq(1, 99);
select '02_03', abs(stats_p90(value) - 89.2) < 1e-12 from stats_seq(1, 99);
select '02_04', abs(stats_p95(value) - 95.05) < 1e-12 from stats_seq(1, 100);
select '02_05', abs(stats_p99(value) - 98.02) < 1e-12 from stats_seq(1, 99);
select '02_06', percentile_25(value) = 25.5 from stats_seq(1, 99);
select '02_07', percentile_75(value) = 74.5 from stats_seq(1, 99);
select '02_08', abs(percentile_90(value) - 89.2) < 1e-12 from stats_seq(1, 99);
select '02_09', abs(percentile_95(value) - 95.05) < 1e-12 from stats_seq(1, 100);
select '02_10', abs(percentile_99(value) - 98.02) < 1e-12 from stats_seq(1, 99);
select '02_11', stats_p25(5) = 5.0;
select '02_12', abs(stats_p99(v) - 1.99) < 1e-12 from (select 1 v union all select 2);

-- stats_perc, percentile
select '03_01', stats_perc(value, 25) = 25.5 from stats_seq(1, 99);
select '03_02', stats_perc(value, 50) = 50.0 from stats_seq(1, 99);
select '03_03', stats_perc(value, 75) = 74.5 from stats_seq(1, 99);
select '03_04', abs(stats_perc(value, 90) - 89.2) < 1e-12 from stats_seq(1, 99);
select '03_05', abs(stats_perc(value, 95) - 95.05) < 1e-12 from stats_seq(1, 100);
select '03_06', abs(stats_perc(value, 99) - 98.02) < 1e-12 from stats_seq(1, 99);
select '03_07', percentile(value, 25) = 25.5 from stats_seq(1, 99);
-- 0 is the minimum, 100 is the maximum
select '03_08', stats_perc(value, 0) = 1.0 from stats_seq(1, 99);
select '03_09', stats_perc(value, 100) = 99.0 from stats_seq(1, 99);
select '03_10', stats_perc(value, 12.5) = 2.0 from stats_seq(1, 9);
select '03_11', stats_perc(value, 50) = 2.0 from stats_seq(1, 3);
-- the percentile is converted from numeric text
select '03_12', stats_perc(value, '50') = 5.0 from stats_seq(1, 9);
-- a percentile that differs from the first row's by 0.001 or less is accepted,
-- and the first row's is used
select '03_13', stats_perc(value, 50 + value * 0.0001) = 5.000008 from stats_seq(1, 9);
-- two percentiles in the same query are independent
select '03_14', (stats_perc(value, 25), stats_perc(value, 75)) = (3.0, 7.0) from stats_seq(1, 9);
-- the percentile is not checked if there are no rows
select '03_15', stats_perc(value, 500) is null from stats_seq(1, 0);

-- stats_stddev, stats_stddev_samp, stddev, stddev_samp
select '04_01', abs(stats_stddev(value) - 28.722813232690143) < 1e-12 from stats_seq(1, 99);
select '04_02', abs(stats_stddev_samp(value) - 28.722813232690143) < 1e-12 from stats_seq(1, 99);
select '04_03', abs(stddev(value) - 28.722813232690143) < 1e-12 from stats_seq(1, 99);
select '04_04', abs(stddev_samp(value) - 28.722813232690143) < 1e-12 from stats_seq(1, 99);
select '04_05', abs(stats_stddev(column1) - 2.138089935299395) < 1e-12 from (values (2), (4), (4), (4), (5), (5), (7), (9));
-- a single value gives 0, though the sample deviation is undefined
select '04_06', stats_stddev(5) = 0.0;
select '04_07', typeof(stats_stddev(value)) = 'real' from stats_seq(1, 3);

-- stats_stddev_pop, stddev_pop
select '05_01', abs(stats_stddev_pop(value) - 28.577380332470412) < 1e-12 from stats_seq(1, 99);
select '05_02', abs(stddev_pop(value) - 28.577380332470412) < 1e-12 from stats_seq(1, 99);
select '05_03', stats_stddev_pop(column1) = 2.0 from (values (2), (4), (4), (4), (5), (5), (7), (9));
select '05_04', stats_stddev_pop(5) = 0.0;

-- stats_var, stats_var_samp, variance, var_samp
select '06_01', stats_var(value) = 825.0 from stats_seq(1, 99);
select '06_02', stats_var_samp(value) = 825.0 from stats_seq(1, 99);
select '06_03', variance(value) = 825.0 from stats_seq(1, 99);
select '06_04', var_samp(value) = 825.0 from stats_seq(1, 99);
select '06_05', abs(stats_var(column1) - 32.0 / 7) < 1e-12 from (values (2), (4), (4), (4), (5), (5), (7), (9));
-- a single value gives 0, though the sample variance is undefined
select '06_06', stats_var(5) = 0.0;
-- the algorithm is numerically stable: a large offset doesn't lose precision
select '06_07', stats_var(column1) = 30.0 from (values (1e9 + 4), (1e9 + 7), (1e9 + 13), (1e9 + 16));
-- the computation overflows: the true variance is 1e616
select '06_08', stats_var(v) = -9e999 from (select 1e308 v union all select -1e308);
-- an infinite value gives NaN, which is NULL
select '06_09', stats_var(v) is null from (select 9e999 v union all select 1);

-- stats_var_pop, var_pop
select '07_01', abs(stats_var_pop(value) - 816.6666666666666) < 1e-9 from stats_seq(1, 99);
select '07_02', abs(var_pop(value) - 816.6666666666666) < 1e-9 from stats_seq(1, 99);
select '07_03', stats_var_pop(column1) = 4.0 from (values (2), (4), (4), (4), (5), (5), (7), (9));
select '07_04', stats_var_pop(value) = 0.25 from stats_seq(1, 2);
select '07_05', stats_var_pop(5) = 0.0;
select '07_06', stats_var_pop(column1) = 22.5 from (values (1e9 + 4), (1e9 + 7), (1e9 + 13), (1e9 + 16));

-- stats_seq, generate_series
select '08_01', (count(*), min(value), max(value)) = (99, 1, 99) from stats_seq(1, 99);
select '08_02', (count(*), min(value), max(value)) = (20, 0, 95) from stats_seq(0, 99, 5);
select '08_03', group_concat(value) = '1,2,3' from generate_series(1, 3);
select '08_04', group_concat(value) = '5' from stats_seq(5, 5);
select '08_05', count(*) = 0 from stats_seq(5, 1);
select '08_06', group_concat(value) = '-3,-2,-1' from stats_seq(-3, -1);
select '08_07', typeof(value) = 'integer' from stats_seq(1, 1);
-- stop defaults to 4294967295 (2^32-1), step to 1
with tmp as (select * from stats_seq(20) limit 10)
select '08_08', (count(*), min(value), max(value)) = (10, 20, 29) from tmp;
select '08_09', group_concat(value) = '4294967294,4294967295' from stats_seq(4294967294);
select '08_10', count(*) = 0 from stats_seq(4294967296);
-- a step of 0 is 1
select '08_11', group_concat(value) = '1,2,3' from stats_seq(1, 3, 0);
-- a negative step counts down from the last value of the ascending series
select '08_12', group_concat(value) = '10,7,4,1' from stats_seq(1, 10, -3);
select '08_13', group_concat(value) = '7,4,1' from stats_seq(1, 9, -3);
-- order by value is consumed by the table
select '08_14', group_concat(value) = '10,7,4,1' from (select value from stats_seq(1, 10, 3) order by value desc);
select '08_15', group_concat(value) = '1,4,7,10' from (select value from stats_seq(1, 10, -3) order by value);
-- the arguments are hidden columns: start, stop, step;
-- step is returned as the absolute value used
select '08_16', (start, stop, step) = (1, 10, 2) from stats_seq(1, 10, 2) limit 1;
select '08_17', (start, stop, step) = (1, 4294967295, 1) from stats_seq(1) limit 1;
select '08_18', (start, stop, step) = (1, 10, 2) from stats_seq(1, 10, -2) limit 1;
select '08_19', group_concat(value) = '1,2,3' from stats_seq where start = 1 and stop = 3;
-- rowid is the row number in output order
select '08_20', group_concat(rowid) = '1,2,3' from stats_seq(5, 7);
select '08_21', group_concat(rowid || ':' || value) = '1:7,2:6,3:5' from (select rowid, value from stats_seq(5, 7) order by value desc);
-- order by rowid desc is consumed as order by value desc, so the rowids come out ascending
select '08_22', group_concat(rowid || ':' || value) = '1:3,2:2,3:1' from (select rowid, value from stats_seq(1, 3) order by rowid desc);
-- other constraints are left to SQLite
select '08_23', group_concat(value) = '8,9,10' from stats_seq(1, 10) where value > 7;
select '08_24', group_concat(a.value || '-' || b.value) = '1-1,1-2,2-2' from stats_seq(1, 2) a, stats_seq(a.value, 2) b;
-- extreme values
select '08_25', group_concat(value) = '-9223372036854775808,-9223372036854775807' from stats_seq(-9223372036854775808, -9223372036854775807);
select '08_26', group_concat(value) = '9223372036854775807,9223372036854775806' from (select value from stats_seq(9223372036854775806, 9223372036854775807) order by value desc);

-- NULL values are skipped, no rows or only NULLs give NULL for percentiles and 0 for the rest
select '09_01', stats_median(column1) = 2.0 from (values (1), (null), (3));
select '09_02', stats_perc(column1, 50) = 2.0 from (values (1), (null), (3));
select '09_03', stats_var(column1) = 2.0 from (values (1), (null), (3));
select '09_04', stats_var_pop(column1) = 1.0 from (values (1), (null), (3));
select '09_05', stats_median(null) is null;
select '09_06', stats_p25(null) is null;
select '09_07', stats_p75(null) is null;
select '09_08', stats_p90(null) is null;
select '09_09', stats_p95(null) is null;
select '09_10', stats_p99(null) is null;
select '09_11', stats_perc(null, 50) is null;
select '09_12', stats_median(value) is null from stats_seq(1, 0);
select '09_13', stats_perc(value, 50) is null from stats_seq(1, 0);
select '09_14', stats_stddev(null) = 0.0;
select '09_15', stats_stddev_pop(null) = 0.0;
select '09_16', stats_var(null) = 0.0;
select '09_17', stats_var_pop(null) = 0.0;
select '09_18', stats_stddev(value) = 0.0 from stats_seq(1, 0);
select '09_19', stats_stddev_pop(value) = 0.0 from stats_seq(1, 0);
select '09_20', stats_var(value) = 0.0 from stats_seq(1, 0);
select '09_21', stats_var_pop(value) = 0.0 from stats_seq(1, 0);
select '09_22', group_concat(quote(m)) = 'NULL,5.0' from (select stats_median(v) m from (select 1 g, null v union all select 2, 5) group by g);
-- a NULL argument gives no rows
select '09_23', count(*) = 0 from stats_seq(null, 5);
select '09_24', count(*) = 0 from stats_seq(1, null);
select '09_25', count(*) = 0 from stats_seq(1, 5, null);

-- input conversion
-- stddev and variance convert numeric text, and treat non-numeric text and blobs as 0
select '10_01', stats_var(column1) = 2.0 from (values ('1'), ('3'));
select '10_02', abs(stats_var(column1) - 7.0 / 3) < 1e-12 from (values ('abc'), (1), (3));
select '10_03', abs(stats_var(column1) - 7.0 / 3) < 1e-12 from (values (x'01'), (1), (3));
-- stats_seq converts numeric text, truncates reals, and treats non-numeric text and blobs as 0
select '10_04', group_concat(value) = '2,3,4' from stats_seq('2', 4);
select '10_05', group_concat(value) = '1,2,3,4' from stats_seq(1.9, 4);
select '10_06', group_concat(value) = '1,2,3,4' from stats_seq(1, 4, 1.5);
select '10_07', group_concat(value) = '0,1,2' from stats_seq('abc', 2);
select '10_08', group_concat(value) = '0,1,2' from stats_seq(x'01', 2);
