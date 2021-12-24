-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/compress

select '01', length(compress(printf('%.3c', 'o'))) = 12;
select '02', length(compress(printf('%.10c', 'o'))) = 12;
select '03', length(compress(printf('%.20c', 'o'))) = 12;
select '04', length(compress(printf('%.30c', 'o'))) = 12;
select '05', length(compress(zeroblob(1024))) = 19;

select '11', uncompress(compress(printf('%.3c', 'o')))
    = cast('ooo' as blob);
select '12', uncompress(compress(printf('%.10c', 'o')))
    = cast('oooooooooo' as blob);
select '13', uncompress(compress(printf('%.20c', 'o')))
    = cast('oooooooooooooooooooo' as blob);
select '14', uncompress(compress(printf('%.30c', 'o')))
    = cast('oooooooooooooooooooooooooooooo' as blob);
select '15', uncompress(compress(zeroblob(1024)))
    = zeroblob(1024);
