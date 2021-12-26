-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/zipfile

create virtual table temp.zip using zipfile('test.zip');
insert into temp.zip(name, data) values('parent', null);
insert into temp.zip(name, data) values('readme.txt', 'a glorious zip file');
insert into temp.zip(name, data) values('parent/child-01.txt', 'child #1');
insert into temp.zip(name, data) values('parent/child-02.txt', 'child #2');

select '01', count(*) = 4 from zipfile('test.zip');

select '02', cast(data as text) = 'a glorious zip file' from zipfile('test.zip')
where name = 'readme.txt';

select '03', cast(data as text) = 'child #1' from zipfile('test.zip')
where name = 'parent/child-01.txt';

select '04', cast(data as text) = 'child #2' from zipfile('test.zip')
where name = 'parent/child-02.txt';

insert into temp.zip(name, data) values('parent/child-03.txt', 'child #3');
select '05', count(*) = 5 from zipfile('test.zip');

delete from temp.zip where name = 'parent/child-03.txt';
select '06', count(*) = 4 from zipfile('test.zip');

with contents(name, data) as (
  values('parent', null) union all
  values('readme.txt', 'a glorious zip file') union all
  values('parent/child-01.txt', 'child #1') union all
  values('parent/child-02.txt', 'child #2')
)
select '07', length(zipfile(name, data)) = 543 from contents;

drop table temp.zip;
.shell rm -f test.zip
