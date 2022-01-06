-- Copyright (c) 2022 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/dbdump

select '01', dbdump() = 'PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
COMMIT;
';

create table employees (id integer primary key, name text);
insert into employees (name) values ('Diane'), ('Bob');

select '11', instr(dbdump(), 'CREATE TABLE employees') > 0;
select '12', instr(dbdump(), 'VALUES(1,''Diane'');') > 0;
select '13', instr(dbdump(), 'VALUES(2,''Bob'');') > 0;

select '14', instr(dbdump('main'), 'CREATE TABLE employees') > 0;
select '15', instr(dbdump('temp'), 'CREATE TABLE employees') = 0;

create table expenses (year integer, month integer, expense integer);
insert into expenses values (2020, 1, 82), (2020, 2, 75), (2020, 3, 104);

select '21', instr(dbdump(), 'CREATE TABLE expenses') > 0;
select '22', instr(dbdump('main', 'employees'), 'CREATE TABLE employees') > 0;
select '23', instr(dbdump('main', 'employees'), 'CREATE TABLE expenses') = 0;
select '24', instr(dbdump('main', 'expenses'), 'CREATE TABLE employees') = 0;
select '25', instr(dbdump('main', 'expenses'), 'CREATE TABLE expenses') > 0;
select '26', instr(dbdump('main', 'whatever'), 'CREATE TABLE employees') = 0;
select '27', instr(dbdump('main', 'whatever'), 'CREATE TABLE expenses') = 0;
