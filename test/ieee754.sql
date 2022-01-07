-- Copyright (c) 2013 D. Richard Hipp, Public Domain
-- https://sqlite.org/src/file/ext/misc/ieee754.c

.load dist/ieee754

select '01', ieee754(2.0) = 'ieee754(2,0)';
select '02', ieee754(45.25) = 'ieee754(181,-2)';
select '03', ieee754(2, 0) = 2.0;
select '04', ieee754(181, -2) = 45.25;
select '05', ieee754_mantissa(45.25)= 181;
select '06', ieee754_exponent(45.25) = -2;
select '07', ieee754_from_blob(x'3ff0000000000000') = 1.0;
select '08', ieee754_to_blob(1.0) = x'3ff0000000000000';
