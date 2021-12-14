-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/stats

select '01', percentile(value, 25) = 25.5 from generate_series(1, 99);
select '02', percentile_25(value) = 25.5 from generate_series(1, 99);

select '03', percentile(value, 50) = 50 from generate_series(1, 99);
select '04', median(value) = 50 from generate_series(1, 99);

select '05', percentile(value, 75) = 74.5 from generate_series(1, 99);
select '06', percentile_75(value) = 74.5 from generate_series(1, 99);

select '07', percentile(value, 90) = 89.2 from generate_series(1, 99);
select '08', percentile_90(value) = 89.2 from generate_series(1, 99);

select '09', percentile(value, 95) = 95.05 from generate_series(1, 100);
select '10', percentile_95(value) = 95.05 from generate_series(1, 100);

select '11', percentile(value, 99) = 98.02 from generate_series(1, 99);
select '12', percentile_99(value) = 98.02 from generate_series(1, 99);

select '21', round(stddev(value), 1) = 28.7 from generate_series(1, 99);
select '22', round(stddev_samp(value), 1) = 28.7 from generate_series(1, 99);
select '23', round(stddev_pop(value), 1) = 28.6 from generate_series(1, 99);

select '31', variance(value) = 825 from generate_series(1, 99);
select '32', var_samp(value) = 825 from generate_series(1, 99);
select '33', round(var_pop(value), 0) = 817 from generate_series(1, 99);