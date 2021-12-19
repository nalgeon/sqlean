-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/cron

-- *      *    *            *     *
-- minute hour day-of-month month day-of-week

select '01', cron_match('2006-01-02 15:04:05','*/4 * * * *') = 1;
select '02', cron_match('2006-01-02 15:04','*/4 * * * *') = 1;
select '03', cron_match('2006-01-02 15:04:05','4 15 * * *') = 1;
select '04', cron_match('2006-01-02 15:04:05','* * 2 1 *') = 1;
select '05', cron_match('2006-01-02 15:04:05','* * * * 1') = 1;
select '06', cron_match('2006-01-02 15:04:05','4 15 2 1 1') = 1;
select '07', cron_match('2006-01-02 15:04:05','*/5 * * * *') = 0;

-- does not work with dates
select '11', cron_match('2006-01-02','* * 2 1 *') = 0;
select '12', cron_match('2006-01-02','* * * * 1') = 0;

-- does not work with times
select '21', cron_match('15:04:05','*/4 * * * *') = 0;
select '22', cron_match('15:04:05','4 15 * * *') = 0;
