.load dist/re
select '01', regexp_replace('the year is 2021', '[0-9]+', '2050') = 'the year is 2050';
select '02', regexp_replace('the year is 2021', '2k21', '2050') = 'the year is 2021';
select '03', regexp_replace('10 10 10', '10$', '') = '10 10 ';
select '04', regexp_replace('10 10 10', '^10', '') = ' 10 10';
select '05', regexp_replace('hello', 'h', '') = 'ello';
select '06', regexp_replace('hello', 'h', '.') = '.ello';
select '07', regexp_replace('hello', 'h', '..') = '..ello';
select '08', regexp_replace('hello', 'e', '') = 'hllo';
select '09', regexp_replace('hello', 'e', '.') = 'h.llo';
select '10', regexp_replace('hello', 'e', '..') = 'h..llo';
select '11', regexp_replace('hello', 'o', '') = 'hell';
select '12', regexp_replace('hello', 'o', '.') = 'hell.';
select '13', regexp_replace('hello', 'o', '..') = 'hell..';
