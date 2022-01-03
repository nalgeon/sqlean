-- Copyright (c) 2022 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/define

select create_function("sumn", "?1 * (?1 + 1) / 2");
select '01', sumn(0) = 0;
select '02', sumn(1) = 1;
select '02', sumn(2) = 1 + 2;
select '03', sumn(3) = 1 + 2 + 3;
select '04', sumn(4) = 1 + 2 + 3 + 4;
select '05', sumn(5) = 1 + 2 + 3 + 4 + 5;

create virtual table split using statement((
  select
    substr(:str, 1, instr(:str, :sep) - 1) as left_part,
    :sep as separator,
    substr(:str, instr(:str, :sep) + 1) as right_part
));

select '11', (left_part, right_part) = ('one', 'two') from split('one;two', ';');
