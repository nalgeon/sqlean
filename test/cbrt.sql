-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/cbrt

select '01', round(cbrt(27), 1) = 3.0;
select '02', round(cbrt(-27), 1) = -3.0;
select '03', cbrt(8.0) = 2.0;
select '04', cbrt(1) = 1;
select '05', cbrt(0) = 0;
select '06', cbrt('whatever') is null;
