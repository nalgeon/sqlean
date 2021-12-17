-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/pivotvtab

create table sales (
    year integer,
    quarter integer,
    revenue integer
);

insert into sales values
(2018, 1, 12000),
(2018, 2, 39600),
(2018, 3, 24000),
(2018, 4, 18000),
(2019, 1, 26400),
(2019, 2, 32400),
(2019, 3, 26400),
(2019, 4, 26400),
(2020, 1, 15000),
(2020, 2, 25200),
(2020, 3, 29700),
(2020, 4, 26400),
(2021, 1, 27000),
(2021, 2, 61200),
(2021, 3, 42000),
(2021, 4, 39000);

create table years as
select value as year from generate_series(2018, 2021);

create table quarters as
select value as quarter, 'Q'||value as name from generate_series(1, 4);

create virtual table sales_by_year using pivot_vtab (
    (select year from years),
    (select quarter, name from quarters),
    (select revenue from sales where year = ?1 and quarter = ?2)
);

select '01', count(*) = 4 from sales_by_year;
select '02', (q1, q2, q3, q4) = (27000, 61200, 42000, 39000)
from sales_by_year where year = 2021;
select '03', sum(q3) = 122100 from sales_by_year;