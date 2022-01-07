-- Copyright (c) 2022 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/unionvtab

create table empl_london(id integer primary key, name text);
insert into empl_london(id, name)
values (11, 'Diane'), (12, 'Bob'), (13, 'Emma'), (14, 'Henry'), (15, 'Dave');

create table empl_berlin(id integer primary key, name text);
insert into empl_berlin(id, name)
values (21, 'Grace'), (22, 'Irene'), (23, 'Frank'), (24, 'Cindy'), (25, 'Alice');

create virtual table temp.employees using unionvtab("
    values
    ('main', 'empl_london', 10, 19),
    ('main', 'empl_berlin', 20, 29)
");

select '01', count(*) = 10 from employees;
select '02', count(*) = 1 from employees where id = 13 and name = 'Emma';
select '03', count(*) = 1 from employees where id = 23 and name = 'Frank';
drop table temp.employees;
