-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/interpolate

-- interpolate a single table
create table measurements(timestamp integer primary key, value real);
insert into measurements(timestamp, value) values
(100, 20), (150, null), (200, 30), (300, 40);

create virtual table temp.interpolated using interpolate(measurements);

select '01', value = 20 from interpolated where timestamp = 100;
select '02', value = 25 from interpolated where timestamp = 150;
select '03', value = 29 from interpolated where timestamp = 190;
select '04', value = 35 from interpolated where timestamp = 250;

drop table temp.interpolated;
drop table measurements;

-- interpolate multiple joined tables
create table m1(timestamp integer primary key, temp_in real);
insert into m1(timestamp, temp_in) values
(100, 20), (150, 21), (200, 20), (250, 22), (300, 19);

create table m2(timestamp integer primary key, temp_out real);
insert into m2(timestamp, temp_out) values
(100, 3), (150, 4), (200, 5), (250, 6), (300, 7);

create virtual table temp.interpolated using interpolate(m1, m2);

select '11', (temp_in - temp_out) = 17 from interpolated where timestamp = 100;
select '12', (temp_in - temp_out) = 17 from interpolated where timestamp = 125;
select '13', (temp_in - temp_out) = 17 from interpolated where timestamp = 150;
select '14', (temp_in - temp_out) = 16 from interpolated where timestamp = 175;
select '15', (temp_in - temp_out) = 15 from interpolated where timestamp = 200;
select '16', (temp_in - temp_out) = 16 from interpolated where timestamp = 225;
select '17', (temp_in - temp_out) = 16 from interpolated where timestamp = 250;
select '18', (temp_in - temp_out) = 14 from interpolated where timestamp = 275;
select '19', (temp_in - temp_out) = 12 from interpolated where timestamp = 300;

drop table temp.interpolated;
drop table m1;
drop table m2;

-- scalar average
select '21', havg(1) = 1.0;
select '22', havg(1, 2) = 1.5;
select '23', havg(1, 2, 3) = 2.0;
select '23', havg(350, 10) = 180;

-- scalar angle average
select '31', havg_angle(1) = 1.0;
select '32', havg_angle(1, 2) = 1.5;
select '33', havg_angle(1, 2, 3) = 2.0;
select '34', havg_angle(350, 10) = 0;

-- aggregate angle average
with data(x) as (values (1), (2), (3))
select '41', avg_angle(x) = 2.0 from data;
with data(x) as (values (350), (10))
select '42', avg_angle(x) = 00 from data;

-- average with respect to timestamps
-- compensates for the missing measurements
-- (timestamps 200, 250, 350, 450, 500 and 550)
create table measurements(timestamp integer primary key, value real);
insert into measurements(timestamp, value) values
(100, 20), (150, 25), (300, 40), (400, 50), (600, 70);

select '51', avg(value) = 41 from measurements;
select '52', avg(value, timestamp) = 45 from measurements;
