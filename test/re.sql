-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/re
select '01', regexp_replace('the year is 2021', '[0-9]+', '2050') = 'the year is 2050';
select '02', regexp_replace('the year is 2021', '([0-9]+)', '!\1!') = 'the year is !2021!';
select '03', regexp_replace('the year is 2021', '2k21', '2050') = 'the year is 2021';
select '21', regexp_replace('10 10 10', '10$', '') = '10 10 ';
select '22', regexp_replace('10 10 10', '^10', '') = ' 10 10';
select '31', regexp_replace('hello', 'h', '') = 'ello';
select '32', regexp_replace('hello', 'h', '.') = '.ello';
select '33', regexp_replace('hello', 'h', '..') = '..ello';
select '34', regexp_replace('hello', 'e', '') = 'hllo';
select '35', regexp_replace('hello', 'e', '.') = 'h.llo';
select '36', regexp_replace('hello', 'e', '..') = 'h..llo';
select '37', regexp_replace('hello', 'o', '') = 'hell';
select '38', regexp_replace('hello', 'o', '.') = 'hell.';
select '39', regexp_replace('hello', 'o', '..') = 'hell..';
