-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/regexp

-- replace case
select '01', regexp_replace('the year is 2021', '[0-9]+', '2050') = 'the year is 2050';
select '02', regexp_replace('the year is 2021', '([0-9]+)', '-$1-') = 'the year is -2021-';
select '03', regexp_replace('the year is 2021', '2k21', '2050') = 'the year is 2021';
select '21', regexp_replace('10 10 10', '10$', '') = '10 10 ';
select '22', regexp_replace('10 10 10', '^10', '') = ' 10 10';
select '23', regexp_replace('1 10 100', '\d+', '**') = '** ** **';
select '31', regexp_replace('hello', 'h', '') = 'ello';
select '32', regexp_replace('hello', 'h', '.') = '.ello';
select '33', regexp_replace('hello', 'h', '..') = '..ello';
select '34', regexp_replace('hello', 'e', '') = 'hllo';
select '35', regexp_replace('hello', 'e', '.') = 'h.llo';
select '36', regexp_replace('hello', 'e', '..') = 'h..llo';
select '37', regexp_replace('hello', 'o', '') = 'hell';
select '38', regexp_replace('hello', 'o', '.') = 'hell.';
select '39', regexp_replace('hello', 'o', '..') = 'hell..';

-- regexp syntax
select '101', regexp_substr('the year is 2021', '[year]') = 'e';
select '102', regexp_substr('the year is 2021', '[^year]') = 't';
select '103', regexp_substr('the year is 2021', '(20|21)') = '20';
select '104', regexp_substr('the year is 2021', '\d+') = '2021';
select '105', regexp_substr('the year is 2021', '\d{1,2}') = '20';
select '106', regexp_substr('the year is 2020', '(20)\1') = '2020';
select '107', regexp_substr('the year is 2021', '\sis\s') = ' is ';
select '108', regexp_substr('the year is 2021', '\bis\b') = 'is';
select '109', regexp_substr('the year is 2021', '^the') = 'the';
select '110', regexp_substr('the year is 2021', '2021$') = '2021';
select '111', regexp_substr('the year is 2021', 'y(?=ear)') = 'y';
select '112', regexp_substr('the YEAR is 2021', '(?i)year') = 'YEAR';

-- unicode
select '151', regexp_substr('-- привет --', '\b\w+\b') = 'привет';

-- regexp substr
select '171', regexp_substr('abcdef', 'b.d') = 'bcd';
select '172', regexp_substr('abcdef', 'b(.)d') = 'bcd';
select '173', regexp_substr('abcdef', 'z') is null;

-- regexp capture
select '181', regexp_capture('abcdef', 'b.d', 0) = 'bcd';
select '182', regexp_capture('abcdef', 'b(.)d') = 'bcd';
select '183', regexp_capture('abcdef', 'b(.)d', 0) = 'bcd';
select '184', regexp_capture('abcdef', 'b(.)d', 1) = 'c';
select '185', regexp_capture('abcdef', 'b(.)d', 2) is null;
select '186', regexp_capture('abcdef', 'z', 0) is null;
