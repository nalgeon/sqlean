-- Copyright (c) 2022 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/lines

select '01', line = 'one' from lines_read('test/lines.txt') limit 1;
select '02', line = 'two' from lines_read('test/lines.txt') limit 1 offset 1;
select '03', line = 'three' from lines_read('test/lines.txt') limit 1 offset 2;
