-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

-- .load dist/unicode
select load_extension('dist/unicode');

select '01', lower('hElLo') = 'hello';
select '02', nlower('hElLo') = 'hello';
select '03', upper('hElLo') = 'HELLO';
select '04', nupper('hElLo') = 'HELLO';
select '05', casefold('hElLo') = 'hello';

select '11', lower('пРиВеТ') = 'привет';
select '12', nlower('пРиВеТ') = 'привет';
select '13', upper('пРиВеТ') = 'ПРИВЕТ';
select '14', nupper('пРиВеТ') = 'ПРИВЕТ';
select '15', casefold('пРиВеТ') = 'привет';

select '21', unaccent('hôtel') = 'hotel';

select '31', like('hEl_o', 'hello') = 1;
select '32', like('пРиВ_Т', 'привет') = 1;
select '33', ('привет' like 'пРиВ_Т') = 1;
