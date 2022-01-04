-- Copyright (c) 2022 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/closure

create table employees (
    id integer primary key,
    parent_id integer,
    name varchar(50)
);

insert into employees
(id, parent_id, name)
values
(11, null, 'Diane'),
(12, 11, 'Bob'),
(21, 11, 'Emma'),
(22, 21, 'Grace'),
(23, 21, 'Henry'),
(24, 21, 'Irene'),
(25, 21, 'Frank'),
(31, 11, 'Cindy'),
(32, 31, 'Dave'),
(33, 31, 'Alice');

create index employees_parent_idx on employees(parent_id);

create virtual table hierarchy using transitive_closure(
    tablename = "employees",
    idcolumn = "id",
    parentcolumn = "parent_id"
);

-- all employees
select '01', count(*) = 10 from employees, hierarchy
where employees.id = hierarchy.id and hierarchy.root = 11;

-- Emma and her subordinates
select '02', count(*) = 5 from employees, hierarchy
where employees.id = hierarchy.id and hierarchy.root = 21;

-- Cindy and her subordinates
select '03', count(*) = 3 from employees, hierarchy
where employees.id = hierarchy.id and hierarchy.root = 31;
select '04', (employees.id, name) = (31, 'Cindy') from employees, hierarchy
where employees.id = hierarchy.id and hierarchy.root = 31 limit 1;
select '05', (employees.id, name) = (32, 'Dave') from employees, hierarchy
where employees.id = hierarchy.id and hierarchy.root = 31 limit 1 offset 1;
select '06', (employees.id, name) = (33, 'Alice') from employees, hierarchy
where employees.id = hierarchy.id and hierarchy.root = 31 limit 1 offset 2;

-- Just Bob
select (employees.id, name) = (12, 'Bob') from employees, hierarchy
where employees.id = hierarchy.id and hierarchy.root = 12;

drop table hierarchy;
drop table employees;
