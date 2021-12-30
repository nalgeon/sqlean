-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/text2

select '01', repeat('*', 3) = '***';
select '02', repeat('wow', 3) = 'wowwowwow';
select '03', repeat('*', 1) = '*';
select '04', repeat('*', 0) is null;
select '05', repeat('', 3) = '';
select '06', repeat(null, 3) is null;

select '11', concat('om', 'nom', 'nom') = 'omnomnom';
select '12', concat('hello') = 'hello';
select '13', concat('hello', null, ' ', null, 'world') = 'hello world';
select '14', concat('') = '';
select '15', concat(null) is null;

select '21', concat_ws('-', 'om', 'nom', 'nom') = 'om-nom-nom';
select '22', concat_ws(', ', 'one', 'two', 'three') = 'one, two, three';
select '23', concat_ws('-', 'hello') = 'hello';
select '24', concat_ws(' ', 'hello', null, 'world') = 'hello world';
select '25', concat_ws('-', '') = '';
select '26', concat_ws('-', null) is null;
