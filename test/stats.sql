-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/stats
select '01', median(value) = 50 from generate_series(1, 99);
select '02', mode(value) = 2 from (select 1 as value union all select 2 union all select 2);
select '03', percentile_25(value) = 25 from generate_series(1, 99);
select '04', percentile_75(value) = 75 from generate_series(1, 99);
select '05', percentile_90(value) = 90 from generate_series(1, 99);
select '06', percentile_95(value) = 95 from generate_series(1, 99);
select '07', percentile_99(value) = 99 from generate_series(1, 99);
select '08', round(stddev(value), 1) = 28.7 from generate_series(1, 99);
select '09', round(stddev_samp(value), 1) = 28.7 from generate_series(1, 99);
select '10', round(stddev_pop(value), 1) = 28.6 from generate_series(1, 99);
select '11', variance(value) = 825 from generate_series(1, 99);
select '12', var_samp(value) = 825 from generate_series(1, 99);
select '13', round(var_pop(value), 0) = 817 from generate_series(1, 99);