-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/unhex

with data as (select randomblob(8) as value)
select '01', unhex(hex(data.value)) = data.value from data;
select '02', unhex(hex('hello')) = cast('hello' as blob);
select '03', hex(unhex('AABBCC')) = 'AABBCC';
select '04', unhex('ZZ') = zeroblob(1);
select '05', unhex('AAAAA') is null;
select '06', unhex('A') is null;
select '07', unhex(null) is null;
