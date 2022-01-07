-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/decimal

select '01', decimal('42') =  '42';
select '02', cast(decimal('42') as integer) = 42;
select '03', cast(decimal('42.1234') as real) = 42.1234;
select '04', decimal('+42.1') = decimal('42.1');
select '05', decimal_add(decimal('0.1'), decimal('0.2')) = decimal('0.3');
select '06', decimal_sub(decimal('0.3'), decimal('0.2')) = decimal('0.1');
select '07', decimal_mul(decimal('0.2'), decimal('0.1')) = decimal('0.02');
