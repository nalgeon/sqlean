-- Copyright (c) 2022 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/stmt

select '01', count(*) = 1 from sqlite_stmt;
select '02', ncol > 0 and busy = 1 and run = 1 and mem > 0 from sqlite_stmt;
select '03', instr(sql, 'select') > 0 from sqlite_stmt;
