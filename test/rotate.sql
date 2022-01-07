-- Copyright (c) 2022 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/rotate

select '01', rot13('hello world') = 'uryyb jbeyq';
select '02', rot13('HELLO WORLD') = 'URYYB JBEYQ';
select '03', rot13(rot13('hello world')) = 'hello world';
select '04', rot13('12-34/56_78~90!') = '12-34/56_78~90!';

select '11', rot47('hello world') = '96==@ H@C=5';
select '12', rot47('HELLO WORLD') = 'wt{{~ (~#{s';
select '13', rot47(rot47('hello world')) = 'hello world';
select '14', rot47('12-34/56_78~90!') ='`a\bc^de0fgOh_P';
