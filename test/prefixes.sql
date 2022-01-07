-- Copyright (c) 2022 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/prefixes

select '01', count(*) = 6 from prefixes('hello');
select '02', prefix = 'hello' from prefixes('hello') limit 1;
select '03', prefix = 'hell' from prefixes('hello') limit 1 offset 1;
select '04', prefix = 'hel' from prefixes('hello') limit 1 offset 2;
select '05', prefix = 'he' from prefixes('hello') limit 1 offset 3;
select '06', prefix = 'h' from prefixes('hello') limit 1 offset 4;
select '07', prefix = '' from prefixes('hello') limit 1 offset 5;

select '11', count(*) = 5 from prefixes(1234);
select '12', prefix = '' from prefixes('');
select '13', prefix = '' from prefixes(null);
select '14', prefix is null from prefixes();
