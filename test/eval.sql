-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/eval

select '01', eval('select 42') = '42';
select eval('create table people(id integer, name text)');
select eval('insert into people values (11, "Alice"), (12, "Bob")');
select '02', eval('select count(*) from people') = '2';