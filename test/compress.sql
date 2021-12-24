-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/compress

select '01', length(compress('ooo')) = 12;
select '02', length(compress('oooooooooo')) = 12;
select '03', length(compress('oooooooooooooooooooo')) = 12;
select '04', length(compress('oooooooooooooooooooooooooooooo')) = 12;

select '05', cast(uncompress(compress('ooo')) as text) = 'ooo';
select '06', cast(uncompress(compress('oooooooooo')) as text) = 'oooooooooo';
select '07', cast(uncompress(compress('oooooooooooooooooooo')) as text) = 'oooooooooooooooooooo';
select '08', cast(uncompress(compress('oooooooooooooooooooooooooooooo')) as text) = 'oooooooooooooooooooooooooooooo';
