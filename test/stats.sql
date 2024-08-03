-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/stats

select '1_01', stats_perc(value, 25) = 25.5 from stats_seq(1, 99);
select '1_02', stats_p25(value) = 25.5 from stats_seq(1, 99);

select '1_03', stats_perc(value, 50) = 50 from stats_seq(1, 99);
select '1_04', stats_median(value) = 50 from stats_seq(1, 99);

select '1_05', stats_perc(value, 75) = 74.5 from stats_seq(1, 99);
select '1_06', stats_p75(value) = 74.5 from stats_seq(1, 99);

select '1_07', stats_perc(value, 90) = 89.2 from stats_seq(1, 99);
select '1_08', stats_p90(value) = 89.2 from stats_seq(1, 99);

select '1_09', stats_perc(value, 95) = 95.05 from stats_seq(1, 100);
select '1_10', stats_p95(value) = 95.05 from stats_seq(1, 100);

select '1_11', stats_perc(value, 99) = 98.02 from stats_seq(1, 99);
select '1_12', stats_p99(value) = 98.02 from stats_seq(1, 99);

select '2_01', round(stats_stddev(value), 1) = 28.7 from stats_seq(1, 99);
select '2_02', round(stats_stddev_samp(value), 1) = 28.7 from stats_seq(1, 99);
select '2_03', round(stats_stddev_pop(value), 1) = 28.6 from stats_seq(1, 99);

select '3_01', stats_var(value) = 825 from stats_seq(1, 99);
select '3_02', stats_var_samp(value) = 825 from stats_seq(1, 99);
select '3_03', round(stats_var_pop(value), 0) = 817 from stats_seq(1, 99);

select '4_01', (count(*), min(value), max(value)) = (99, 1, 99) from stats_seq(1, 99);
select '4_02', (count(*), min(value), max(value)) = (20, 0, 95) from stats_seq(0, 99, 5);
with tmp as (select * from stats_seq(20) limit 10)
select '4_03', (count(*), min(value), max(value)) = (10, 20, 29) from tmp;
