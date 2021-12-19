-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/stats
.load dist/pearson

create table data as
select value as x, value*2 as y from generate_series(1, 8);

select '01', pearson(x, y) = 1.0 from data;

drop table data;
create table data as
select value as x, 16 - (2*(value-1)) as y from generate_series(1, 8);

select '02', pearson(x, y) = -1.0 from data;

drop table data;
create table data (x integer, y integer);
insert into data(x, y) values
(7,8), (3,6), (4,1), (1,7), (5,4), (8,5), (6,3), (2,2);

select '03', abs(pearson(x, y)) < 0.2 from data;