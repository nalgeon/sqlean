-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/isodate

select '01', iso_weekday('2006-01-02') = 1;
select '02', iso_weekday('2006-01-02 15:04:05.123') = 1;
select '03', iso_weekday('2006-01-03') = 2;
select '04', iso_weekday('2006-01-04') = 3;
select '05', iso_weekday('2006-01-05') = 4;
select '06', iso_weekday('2006-01-06') = 5;
select '07', iso_weekday('2006-01-07') = 6;
select '08', iso_weekday('2006-01-08') = 7;
select '09', iso_weekday('2006-01-09') = 1;
select '10', iso_weekday('') is null;
select '11', iso_weekday(null) is null;

select '21', iso_week('2005-12-28') = 52;
select '22', iso_week('2006-01-02') = 1;
select '23', iso_week('2006-01-02 15:04:05.123') = 1;
select '24', iso_week('2006-01-08') = 1;
select '25', iso_week('2006-01-09') = 2;
select '26', iso_week('') is null;
select '27', iso_week(null) is null;

select '31', iso_year('2005-12-28') = 2005;
select '32', iso_year('2006-01-02') = 2006;
select '33', iso_year('2006-01-02 15:04:05.123') = 2006;
select '34', iso_year('') is null;
select '35', iso_year(null) is null;

select '41', unixepoch('2006-01-02') = 1136160000;
select '42', unixepoch('2006-01-02 15:04') = 1136214240;
select '43', unixepoch('2006-01-02 15:04:05') = 1136214245;
select '44', unixepoch('2006-01-02 15:04:05.123') = 1136214245;
select '45', unixepoch('2006-01-02 15:04:05.999') = 1136214245;
select '46', unixepoch('') is null;
select '47', unixepoch(null) is null;
