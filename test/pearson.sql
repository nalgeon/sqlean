-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/pearson

create table series as
with recursive tmp(value) as (
  select 1
  union all
  select value+1 from tmp
  where value+1 <= 8
)
select value from tmp;

create table data as
select value as x, value*2 as y from series;

select '01', pearson(x, y) = 1.0 from data;

drop table data;
create table data as
select value as x, 16 - (2*(value-1)) as y from series;

select '02', pearson(x, y) = -1.0 from data;

drop table data;
create table data (x integer, y integer);
insert into data(x, y) values
(7,8), (3,6), (4,1), (1,7), (5,4), (8,5), (6,3), (2,2);

select '03', abs(pearson(x, y)) < 0.2 from data;