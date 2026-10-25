-- Copyright (c) 2022 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/define

-- define: parameters
select define('sumn', '?1 * (?1 + 1) / 2');
select '01_01', sumn(0) = 0;
select '01_02', sumn(1) = 1;
select '01_03', sumn(2) = 1 + 2;
select '01_04', sumn(3) = 1 + 2 + 3;
select '01_05', sumn(4) = 1 + 2 + 3 + 4;
select '01_06', sumn(5) = 1 + 2 + 3 + 4 + 5;
-- a named parameter used twice is a single argument
select define('sumn2', ':n * (:n + 1) / 2');
select '01_07', sumn2(5) = 15;
select define('subxy', '?1 - ?2');
select '01_08', subxy(5, 1) = 4;
select define('subyx', '?2 - ?1');
select '01_09', subyx(5, 1) = -4;
select define('subnm', ':n - :m');
select '01_10', subnm(5, 1) = 4;
select define('subxx', '? - ?');
select '01_11', subxx(5, 1) = 4;
select define('answer', '42');
select '01_12', answer() = 42;

-- define: arguments
-- arguments are bound as is, so NULLs and types follow the body's SQL semantics
select '02_01', subxy(null, 1) is null;
select '02_02', subxy(5, null) is null;
select '02_03', subxy('5', '1') = 4;
select '02_04', subxy('a', 1) = -1;
select define('ident', '?');
select '02_05', ident(null) is null;
select '02_06', typeof(ident(1)) = 'integer';
select '02_07', typeof(ident(1.5)) = 'real';
select '02_08', typeof(ident('1')) = 'text';
select '02_09', ident(x'00ff') = x'00ff';
select '02_10', ident('привет') = 'привет';

-- define: results
-- only the first row of the body is used
select define('firstrow', 'value from (select 1 as value union all select 2)');
select '03_01', firstrow() = 1;
-- only the first column of the body is used
select define('firstcol', '1, 2');
select '03_02', firstcol() = 1;
-- only the first statement of the body is compiled, the rest is ignored
select define('firststmt', '1; select 2');
select '03_03', firststmt() = 1;

-- define: non-deterministic
-- defined functions are not deterministic, so they are evaluated for each row
select define('randint', ':a + abs(random()) % (:b - :a + 1)');
select '04_01', randint(10, 20) between 10 and 20;
select '04_02', count(distinct randint(1, 1000000)) > 1 from (
  with recursive c(x) as (select 1 union all select x + 1 from c where x < 100) select x from c
);

-- define: composition
select '05_01', subxy(sumn(3), sumn(2)) = 3;
select '05_02', sumn(sumn(2)) = 6;
-- a defined function can call another one
select define('sumn_plus', 'sumn(?) + 1');
select '05_03', sumn_plus(3) = 7;
select '05_04', eval('select sumn(3)') = '6';
select define('evalf', 'eval(?)');
select '05_05', evalf('select 42') = '42';

-- define: management
select '06_01', define('noop', '1') is null;
-- the name is converted to text
select define(1, '11');
select '06_02', "1"() = 11;
select '06_03', body = '42' from sqlean_define where name = 'answer';
-- redefining replaces the body
select define('redef', '? + 1');
select define('ReDef', '? + 2');
select '06_04', redef(1) = 3;
select '06_05', count(*) = 1 from sqlean_define where name = 'ReDef' and body = '? + 2';
-- with a different number of parameters
select define('redef', '? + ?');
select '06_06', redef(1, 2) = 3;
select '06_07', count(*) = 1 from sqlean_define where name like 'redef' and body = '? + ?';
select undefine('redef');
-- a body that returns no rows gives NULL
select define('norows', '1 where 0');
select '06_08', norows() is null;
select undefine('norows');

-- table-valued functions
create virtual table strcut using define((
  select
    substr(:str, 1, instr(:str, :sep) - 1) as left,
    :sep as separator,
    substr(:str, instr(:str, :sep) + 1) as right
));
select '11_01', (left, separator, right) = ('one', ';', 'two') from strcut('one;two', ';');
select '11_02', (left, right) = ('one', 'two') from strcut('one;two', ';');
select '11_03', count(*) = 1 from strcut('one;two', ';');

-- parameters are hidden columns, named after the parameter or its position
select '12_01', group_concat(name) = 'left,separator,right' from pragma_table_info('strcut');
select '12_02', group_concat(name) = 'left,separator,right,str,sep'
  from pragma_table_xinfo('strcut');
select '12_03', sum(hidden) = 2 from pragma_table_xinfo('strcut');
create virtual table abc using define((select ?1 as a, ?2 as b, ?3 as c));
select '12_04', group_concat(name) = 'a,b,c,1,2,3' from pragma_table_xinfo('abc');
-- unnamed columns are named after the expression
create virtual table unnamed using define((select 1, 'x'));
select '12_05', group_concat(name) = '1,''x''' from pragma_table_info('unnamed');
select '12_06', (str, sep) = ('one;two', ';') from strcut('one;two', ';');

-- no parameters
create virtual table one using define((select 1 as value));
select '13_01', value = 1 from one;
select '13_02', count(*) = 1 from one;

-- multiple rows
create virtual table series using define((
  with recursive c(x) as (select :lo union all select x + 1 from c where x < :hi)
  select x as value from c
));
select '14_01', group_concat(value) = '1,2,3' from series(1, 3);
select '14_02', count(*) = 0 from series(3, 1) where value > 3;
select '14_03', group_concat(rowid) = '1,2,3' from series(1, 3);
select '14_04', group_concat(value) = '4,5' from series(1, 5) where value > 3;
-- only the first statement of the body is compiled, the rest is ignored
create virtual table firststmt_t using define((select 1 as a; select 2 as b));
select '14_05', group_concat(name) = 'a' from pragma_table_info('firststmt_t');
select '14_06', a = 1 from firststmt_t;

-- missing arguments are NULL
-- (the recursive body yields a single NULL row when :lo is NULL)
select '15_01', count(*) = 1 and max(value) is null from series;
select '15_02', group_concat(value) = '1' from series(1);
select '15_03', count(*) = 1 and max(value) is null from series(null, 3);
select '15_04', (a, b, c) is (1, null, null) from abc(1);

-- arguments as constraints
select '16_01', group_concat(value) = '2,3,4' from series where lo = 2 and hi = 4;
select '16_02', group_concat(value) = '2,3,4' from series where hi = 4 and lo = 2;
-- non-contiguous parameters
select '16_03', (a, b, c) is ('x', null, 'z') from abc where "1" = 'x' and "3" = 'z';
select '16_04', (a, b, c) is (null, null, 'z') from abc where "3" = 'z';
select '16_05', group_concat(value) = '1,2,3,2,3' from series where lo in (1, 2) and hi = 3;
-- the same named parameter used twice
create virtual table twice using define((select :x as a, :x as b));
select '16_06', (a, b) = (5, 5) from twice(5);

-- joins
create table strs(id integer primary key, value text);
insert into strs(value) values ('one;two'), ('three;four'), ('five;six');
select '17_01', group_concat(id || left || right, ',') = '1onetwo,2threefour,3fivesix'
  from strs, strcut(strs.value, ';');
select '17_02', count(*) = 6 from strs, series(strs.id, strs.id + 1);
select '17_03', group_concat(value) = '1,2,3' from series(sumn(1), sumn(2));

-- sqlean_define
select '21_01', (type, body) = ('scalar', ':n - :m') from sqlean_define where name = 'subnm';
-- table functions are stored with the parentheses
select '21_02', (type, body) = ('table', '(select 1 as value)') from sqlean_define where name = 'one';
select '21_03', count(*) = 16 from sqlean_define where type = 'scalar';
select '21_04', count(*) = 7 from sqlean_define where type = 'table';
select '21_05', count(*) = 1 from sqlite_master where type = 'table' and name = 'strcut';
-- dropping a table function directly deletes it from sqlean_define
drop table one;
select '21_06', count(*) = 0 from sqlean_define where name = 'one';

-- undefine
select '31_01', undefine('subnm') is null;
select '31_02', count(*) = 0 from sqlean_define where name = 'subnm';
select '31_03', count(*) = 21 from sqlean_define;
-- calling an undefined function fails with "no such function" (see test_undefine)
select '31_04', count(*) = 1 from pragma_function_list where name = 'subnm';
select undefine('strcut');
select '31_05', count(*) = 0 from sqlean_define where name = 'strcut';
select '31_06', count(*) = 0 from sqlite_master where type = 'table' and name = 'strcut';
select '31_07', count(*) = 20 from sqlean_define;
select '31_08', undefine('nosuch') is null;
select '31_09', undefine(null) is null;
select '31_10', count(*) = 20 from sqlean_define;
-- undefine doesn't touch tables it didn't create
create table regular(x);
select undefine('regular');
select '31_11', count(*) = 1 from sqlite_master where name = 'regular';
-- a table function row whose table is gone (or replaced) is just deleted
insert into sqlean_define(name, type, body) values ('regular', 'table', '(select 1 as a)');
select undefine('regular');
select '31_12', count(*) = 0 from sqlean_define where name = 'regular';
select '31_13', count(*) = 1 from sqlite_master where name = 'regular';
-- names are case-insensitive
create virtual table CamelCase using define((select 1 as value));
select undefine('camelcase');
select '31_14', count(*) = 0 from sqlite_master where name = 'CamelCase';
select '31_15', count(*) = 0 from sqlean_define where name = 'CamelCase';
select define('CamelCase', '1');
select undefine('CAMELCASE');
select '31_16', count(*) = 0 from sqlean_define where name = 'CamelCase';

-- names are quoted properly
create table innocent (i);
select define('f ''; drop table innocent; --', '1');
select '41_01', "f '; drop table innocent; --"() = 1;
select undefine('f ''; drop table innocent; --');
select define('f "; drop table innocent; --', '1');
select '41_02', "f ""; drop table innocent; --"() = 1;
select undefine('f "; drop table innocent; --');
select '41_03', count(*) = 1 from sqlite_master where type = 'table' and name = 'innocent';

-- define_free does nothing, kept for compatibility
select '42_01', define_free() is null;
select '42_02', sumn(3) = 6;

-- eval
select '51_01', eval('select 42') = '42';
select '51_02', eval('select 1, 2, 3') = '1 2 3';
select '51_03', eval('select 1, 2, 3', ', ') = '1, 2, 3';
select '51_04', eval('select abs(-42)') = '42';
select '51_05', eval('select 10 + 32') = '42';
select '51_06', eval('select ''hello''') = 'hello';
select '51_07', eval('select 1; select 2; select 3;') = '1 2 3';
select '51_08', eval('select 1, 2; select 3, 4', '|') = '1|2|3|4';
select '51_09', eval('select 1 union all select 2') = '1 2';
select '51_10', eval('select 1.5') = '1.5';
select '51_11', eval('select x''41''') = 'A';
select '51_12', eval('select ''привет''') = 'привет';
select '51_13', eval('select eval(''select 5'')') = '5';

-- eval: NULLs and empty results
select '52_01', eval(null) is null;
select '52_02', eval('select 1', null) is null;
-- NULL values are empty strings
select '52_03', eval('select null') = '';
select '52_04', eval('select 1, null, 3') = '1  3';
-- no output at all is NULL
select '52_05', eval('') is null;
select '52_06', eval('select 1 where 0') is null;

-- eval: arguments
select '53_01', eval('select 1, 2', '') = '12';
select '53_02', eval('select 1, 2', '--') = '1--2';
-- non-text arguments are converted to text
select '53_03', eval('select 1, 2', 0) = '102';
select '53_04', eval(cast('select 42' as blob)) = '42';

-- eval: DDL and DML
select '54_01', eval('create table tmp(value int)') is null;
select '54_02', count(*) = 1 from sqlite_master where type = 'table' and name = 'tmp';
select '54_03', eval('insert into tmp(value) values (1), (2), (3)') is null;
select '54_04', count(*) = 3 from tmp;
select '54_05', eval('select value from tmp') = '1 2 3';
select '54_06', eval('drop table tmp') is null;
select '54_07', count(*) = 0 from sqlite_master where type = 'table' and name = 'tmp';

-- persistence
.shell rm -f define.db
.open define.db
.load dist/define
select define('plus', '? + 1');
create virtual table half using define((select :x / 2 as value));

-- functions are loaded from sqlean_define on connect
.open define.db
.load dist/define
select '61_01', plus(1) = 2;
select '61_02', value = 5 from half(10);
select '61_03', count(*) = 2 from sqlean_define;
-- a function that fails to compile stops loading the rest (current behavior)
insert into sqlean_define(name, type, body) values
  ('broken', 'scalar', 'nosuch(?)'), ('later', 'scalar', '? * 10');

.open define.db
.load dist/define
select '62_01', plus(1) = 2;
select '62_02', count(*) = 0 from pragma_function_list where name in ('broken', 'later');
.shell rm -f define.db
