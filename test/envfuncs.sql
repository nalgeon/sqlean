-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/envfuncs

select '01', getenv('USER') is not null;
select '02', getenv('whatever') is null;

select '11', length(getenv('USER')) = getenvlen('USER');

select '21', getenvexists('USER') = true;
select '22', getenvexists('whatever') = false;

select '31', getenvdefault('USER', 'oops') = getenv('USER');
select '32', getenvdefault('whatever', 'oops') = 'oops';
