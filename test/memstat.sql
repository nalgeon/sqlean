-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/memstat

select '01', value > 0 from sqlite_memstat where name = 'MEMORY_USED';
select '02', value > 0 from sqlite_memstat where name = 'DB_CACHE_USED';
select '03', count(*) = 1 from sqlite_memstat where name = 'DB_CACHE_HIT';
select '04', count(*) = 1 from sqlite_memstat where name = 'DB_CACHE_MISS';
