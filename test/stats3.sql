-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/stats3

create table series as
with recursive tmp(v) as (
  select 1
  union all
  select v+1 from tmp
  where v+1 <= 99
)
select v from tmp;

-- statistics
select '01', round(corr(v, v) over (), 1) = 1.0 from series limit 1;
select '02', round(covar_samp(v, v) over ()) = 825 from series limit 1;
select '03', round(covar_pop(v, v) over ()) = 817 from series limit 1;
select '04', round(geo_mean(v) over ()) = 38 from series limit 1;
select '05', round(harm_mean(v)over ()) = 19 from series limit 1;
select '06', median(v) over () = 50 from series limit 1;
with data as (
    select 1 as v
    union all select 2 union all select 3 union all select 2
    union all select 3 union all select 2 union all select 4
)
select '07', mode(v) over () = 2 from data limit 1;
select '08', q1(v) over () = 24.5 from series limit 1;
select '09', q3(v) over () = 74.5 from series limit 1;
select '10', iqr(v) over () = 50.0 from series limit 1;

-- other
with data as (
    select 1 as v union all select 2 union all select 3
)
select '21', product(v) over () = 6 from data limit 1;
