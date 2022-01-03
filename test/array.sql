-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/array

create table data(arr blob);

-- read-only operations
insert into data(arr) values (array(11, 12, 13));

select '01', array_length(arr) = 3 from data;
select '02', array_length(null) is null from data;

select '03', array_at(arr, 1) = 11 from data;
select '04', array_at(arr, 2) = 12 from data;
select '05', array_at(arr, 3) = 13 from data;
select '06', array_at(arr, 10) is null from data;
select '07', array_at(arr, 0) is null from data;
select '08', array_at(null, 1) is null from data;

select '09', array_index(arr, 13) = 3 from data;
select '10', array_index(arr, 42) = 0 from data;
select '11', array_index(arr, 'one') = 0 from data;
select '12', array_index(arr, null) = 0 from data;
select '13', array_index(null, 13) is null from data;

select '14', array_contains(arr, 13) = 1 from data;
select '15', array_contains(arr, 42) = 0 from data;
select '16', array_contains(arr, 'one') = 0 from data;
select '17', array_contains(arr, null) = 0 from data;
select '18', array_contains(null, 13) is null from data;

select '19', array_to_json(arr) = '[11,12,13]' from data;
select '20', array_to_json(null) is null from data;

select '21', array(11, 12, 13) = array(11, 12, 13);
select '22', array(11, 12, 13) <> array(11, 12, 14);
select '23', array(11, 12, 13) <> array(11, 12, 13, 1);
select '24', array(11, null, 13) == array(11, 13);
select '25', array(11, 2.2, 13) == array(11, 13);
select '26', array(11, 'two', 13) == array(11, 13);

-- append
delete from data;
insert into data(arr) values (array(11, 12, 13));
update data set arr = array_append(arr, 42);
select '51', array_length(arr) = 4 from data;
select '52', arr = array(11, 12, 13, 42) from data;
select '53', array_append(arr, null) = arr from data;
select '54', array_append(arr, 1.1) = arr from data;
select '55', array_append(arr, 'one') = arr from data;
select '56', array_append(null, 42) is null;

update data set arr = intarray();
update data set arr = array_append(arr, 11);
update data set arr = array_append(arr, 12);
update data set arr = array_append(arr, 13);
select '57', array_length(arr) = 3 from data;
select '58', arr = array(11, 12, 13) from data;

-- insert
delete from data;
insert into data(arr) values (array(11, 12, 13));
update data set arr = array_insert(arr, 3, 42);
select '61', array_length(arr) = 4 from data;
select '62', arr = array(11, 12, 42, 13) from data;
select '63', array_insert(arr, 1, null) = arr from data;
select '64', array_insert(arr, 1, 1.1) = arr from data;
select '65', array_insert(arr, 1, 'one') = arr from data;
select '66', array_insert(null, 1, 42) is null;

-- remove at
delete from data;
insert into data(arr) values (array(11, 12, 13));
update data set arr = array_remove_at(arr, 1);
select '71', array_length(arr) = 2 from data;
select '72', arr = array(12, 13) from data;

update data set arr = array(11, 12, 13);
update data set arr = array_remove_at(arr, 2);
select '73', array_length(arr) = 2 from data;
select '74', arr = array(11, 13) from data;

update data set arr = array(11, 12, 13);
update data set arr = array_remove_at(arr, 3);
select '75', array_length(arr) = 2 from data;
select '76', arr = array(11, 12) from data;

update data set arr = array(11, 12, 13);
select '77', array_remove_at(arr, 4) = arr from data;
select '78', array_remove_at(arr, 0) = arr from data;
select '79', array_remove_at(null, 1) is null;

update data set arr = array(11, 12, 13);
update data set arr = array_remove_at(arr, 1);
update data set arr = array_remove_at(arr, 1);
update data set arr = array_remove_at(arr, 1);
select '80', array_length(arr) = 0 from data;
select '81', arr = intarray() from data;

-- remove
delete from data;
insert into data(arr) values (array(11, 12, 13));
update data set arr = array_remove(arr, 12);
select '91', array_length(arr) = 2 from data;
select '92', arr = array(11, 13) from data;

update data set arr = array(11, 12, 13);
select '93', array_remove(arr, 42) = arr from data;
select '94', array_remove(arr, null) = arr from data;
select '95', array_remove(null, 11) is null;

update data set arr = array(0, 1, 2);
select '96', array_remove(arr, null) == arr from data;

update data set arr = array(11, 12, 11);
update data set arr = array_remove(arr, 11);
select '97', arr = array(12, 11) from data;
update data set arr = array_remove(arr, 11);
select '98', arr = array(12) from data;

-- clear
delete from data;
insert into data(arr) values (array(11, 12, 13));
update data set arr = array_clear(arr);
select '101', array_length(arr) = 0 from data;
select '102', arr = intarray() from data;
select '103', array_clear(arr) = arr from data;
select '104', array_clear(null) is null;

-- slice
delete from data;
insert into data(arr) values (array(11, 12, 13, 14));
select '111', array_slice(arr, 2, 4) = array(12, 13) from data;
select '112', array_slice(arr, 2) = array(12, 13, 14) from data;
select '113', array_slice(arr, 1, 2) = array(11) from data;
select '114', array_slice(arr, 1, 1) is null from data;

update data set arr = array_slice(array(11, 12, 13, 14), 2, 4);
select '115', array_length(arr) = 2 from data;

update data set arr = array_slice(array(11, 12, 13, 14), 2);
select '116', array_length(arr) = 3 from data;

update data set arr = array(11, 12, 13);
select '117', array_slice(arr, 1, 7) is null from data;
select '118', array_slice(arr, 5) is null from data;
select '119', array_slice(arr, 5, 7) is null from data;
select '120', array_slice(null, 1, 1) is null;

-- concat
select '131', array_concat(array(11), array(12, 13)) = array(11, 12, 13);
select '132', array_concat(array(11, 12), array(13)) = array(11, 12, 13);
select '133', array_concat(intarray(), array(11, 12, 13)) = array(11, 12, 13);
select '134', array_concat(array(11, 12, 13), intarray()) = array(11, 12, 13);
select '135', array_concat(intarray(), intarray()) = intarray();
select '136', array_concat(array(11), null) is null;
select '137', array_concat(null, array(11)) is null;
select '138', array_concat(null, null) is null;

-- aggregate
with tmp as (select 11 as val union all select 12 union all select 13)
select '201', array_agg(val) = array(11, 12, 13) from tmp;

with tmp as (select 11 as val union all select 12 union all select 13)
select '202', array_agg(val) over () = array(11, 12, 13) from tmp limit 1;

with tmp as (select 11 as val union all select null union all select 13)
select '203', array_agg(val) = array(11, 13) from tmp;

with tmp as (select 11 as val union all select 'two' union all select 13)
select '204', array_agg(val) = array(11, 13) from tmp;

with tmp as (select null as val union all select null union all select null)
select '205', array_agg(val) is null from tmp;

with tmp as (select 'one' as val union all select 'two' union all select 'three')
select '206', array_agg(val, 5) = array('one', 'two', 'three') from tmp;

with tmp as (select 'one' as val union all select 'two' union all select 'three')
select '207', array_agg(val, 5) over () = array('one', 'two', 'three') from tmp limit 1;

with tmp as (select 'one' as val union all select null union all select 'three')
select '208', array_agg(val, 5) = array('one', 'three') from tmp;

with tmp as (select 'one' as val union all select 2 union all select 'three')
select '209', array_agg(val, 5) = array('one', 'three') from tmp;

with tmp as (select null as val union all select null union all select null)
select '210', array_agg(val, 5) is null from tmp;

-- unnest
select '251', count(*) = 3 from unnest(array(11, 12, 13)) limit 1;
select '252', value = 11 from unnest(array(11, 12, 13)) limit 1;
select '253', value = 12 from unnest(array(11, 12, 13)) limit 1 offset 1;
select '254', value = 13 from unnest(array(11, 12, 13)) limit 1 offset 2;
select '255', array_agg(value) = array(11, 12, 13) from unnest(array(11, 12, 13));

delete from data;
insert into data(arr) values (array(11, 12, 13)), (array(21, 22, 23)), (array(31, 32, 33));
select '256', count(value) = 9 from data, unnest(data.arr);
select '257', value = 11 from data, unnest(data.arr) limit 1;
select '258', value = 21 from data, unnest(data.arr) limit 1 offset 3;
select '259', value = 31 from data, unnest(data.arr) limit 1 offset 6;

select '261', count(*) = 3 from unnest(array('one', 'two', 'three')) limit 1;
select '262', array_agg(value, 5) = array('one', 'two', 'three')
from unnest(array('one', 'two', 'three'));

-- real array
delete from data;
insert into data(arr) values (array(1.1, 2.2, 3.3));
select '301', array_length(arr) = 3 from data;
select '302', array_at(arr, 1) = 1.1 from data;
select '303', array_at(arr, 2) = 2.2 from data;
select '304', array_at(arr, 3) = 3.3 from data;
select '305', array_at(arr, 10) is null from data;
select '306', array_at(arr, 0) is null from data;
select '307', array_index(arr, 3.3) = 3 from data;
select '308', array_index(arr, 4.2) = 0 from data;
select '309', array_contains(arr, 3.3) = 1 from data;
select '310', array_contains(arr, 4.2) = 0 from data;
select '311', array_to_json(arr) = '[1.1,2.2,3.3]' from data;
select '312', array(1.1, 2.2, 3.3) = array(1.1, 2.2, 3.3);
select '313', array(1.1, 2.2, 3.3) <> array(1.1, 2.2, 3.4);
select '314', array(1.1, 2.2, 3.3) <> array(1.1, 2.2, 3.3, 4.4);
select '315', array(1.1, null, 3.3) == array(1.1, 3.3);

-- append to empty array
delete from data;
insert into data(arr) values (realarray());
update data set arr = array_append(arr, 12.34);
update data set arr = array_append(arr, 8.44443);
update data set arr = array_append(arr, 100500.3);
select '321', array_length(arr) = 3 from data;
select '322', array_at(arr, 1) = 12.34 from data;
select '323', array_at(arr, 2) = 8.44443 from data;
select '324', array_at(arr, 3) = 100500.3 from data;

-- null values
delete from data;
insert into data(arr) values (array(1.1, 2.2, 3.3));
select '331', array_append(arr, null) = arr from data;
select '332', array_insert(arr, 1, null) = arr from data;
select '333', array_remove(arr, null) = arr from data;

-- text array
delete from data;
insert into data(arr) values (array('one', 'two', 'three'));
select '401', array_length(arr) = 3 from data;
select '402', array_at(arr, 1) = 'one' from data;
select '403', array_at(arr, 2) = 'two' from data;
select '404', array_at(arr, 3) = 'three' from data;
select '405', array_at(arr, 10) is null from data;
select '406', array_at(arr, 0) is null from data;
select '407', array_index(arr, 'three') = 3 from data;
select '408', array_index(arr, 'four') = 0 from data;
select '409', array_contains(arr, 'three') = 1 from data;
select '410', array_contains(arr, 'four') = 0 from data;
select '411', array_to_json(arr) = '["one","two","three"]' from data;
select '412', array('one', 'two', 'three') = array('one', 'two', 'three');
select '413', array('one', 'two', 'three') <> array('one', 'two', 'thre');
select '414', array('one', 'two', 'three') <> array('one', 'two', 'three', '');
select '415', array('one', null, 'three') == array('one', 'three');

-- append to empty array
delete from data;
insert into data(arr) values (textarray(2));
update data set arr = array_append(arr, '');
update data set arr = array_append(arr, 'a');
update data set arr = array_append(arr, 'ab');
update data set arr = array_append(arr, 'abc');
select '421', array_length(arr) = 4 from data;
select '422', array_at(arr, 1) = '' from data;
select '423', array_at(arr, 2) = 'a' from data;
select '424', array_at(arr, 3) = 'ab' from data;
select '425', array_at(arr, 4) = 'ab' from data;

-- null values
delete from data;
insert into data(arr) values (array('one', 'two', 'three'));
select '431', array_append(arr, null) = arr from data;
select '432', array_insert(arr, 1, null) = arr from data;
select '433', array_remove(arr, null) = arr from data;

drop table data;
