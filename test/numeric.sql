-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/numeric

-- null returns null
select '01', tobesttype(null) is null;

-- text/blob that "looks like a number" returns the number as integer or real as appropriate
select '02', tobesttype('42') = 42;
select '03', tobesttype('42.13') = 42.13;
select '04', tobesttype(cast('42' as blob)) = 42;
select '05', tobesttype(cast('42.13' as blob)) = 42.13;

-- text/blob that is zero-length returns null
select '06', tobesttype('') is null;
select '07', tobesttype(zeroblob(0)) is null;

-- otherwise returns what was given
select '08', tobesttype('answer = 42') = 'answer = 42';
