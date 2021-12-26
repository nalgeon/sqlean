-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/uint

select '01', '01' < '2' collate uint;
select '02', '10' > '2' collate uint;
select '03', '01' = '01' collate uint;

create table photos(name text);
insert into photos(name) values
('p-1'), ('p-10'), ('p-21'), ('p-2'), ('p-20'), ('p-5');

select '11', name = 'p-1' from photos order by name collate uint limit 1;
select '12', name = 'p-2' from photos order by name collate uint limit 1 offset 1;
select '13', name = 'p-5' from photos order by name collate uint limit 1 offset 2;
select '14', name = 'p-10' from photos order by name collate uint limit 1 offset 3;
select '15', name = 'p-20' from photos order by name collate uint limit 1 offset 4;
select '16', name = 'p-21' from photos order by name collate uint limit 1 offset 5;

drop table photos;
