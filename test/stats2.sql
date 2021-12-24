-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/stats2

create table series as
with recursive tmp(v) as (
  select 1
  union all
  select v+1 from tmp
  where v+1 <= 99
)
select v from tmp;

-- window aggregates
select '01',        avg(v)    over ()     = 50.0    from series limit 1;
select '02',        aavg(v)   over ()     = 50.0    from series limit 1;
select '03', round( gavg(v)   over (), 3) = 37.623  from series limit 1;
select '04', round( rms(v)    over (), 3) = 57.591  from series limit 1;
select '05', round( stdev(v)  over (), 3) = 28.723  from series limit 1;
select '06', round( stdevp(v) over (), 3) = 28.577  from series limit 1;
select '07',        var(v)    over ()     = 825.0   from series limit 1;
select '08', round( varp(v)   over (), 3) = 816.667 from series limit 1;
select '09', round( sem(v)    over (), 3) = 2.887   from series limit 1;
select '10', round( ci(v)     over (), 3) = 5.658   from series limit 1;
select '11',        skew(v)   over ()     = 0       from series limit 1;
select '12',        skewp(v)  over ()     = 0       from series limit 1;
select '13',        kurt(v)   over ()     = -1.2    from series limit 1;
select '14', round( kurtp(v)  over (), 3) = -1.2    from series limit 1;

-- weighted window aggregates
select '21', round( avg(v, v)    over (), 3) = 66.333  from series limit 1;
select '22', round( stdev(v, v)  over (), 3) = 23.569  from series limit 1;
select '23', round( stdevp(v, v) over (), 3) = 23.45   from series limit 1;
select '24', round( var(v, v)    over (), 3) = 555.5   from series limit 1;
select '25', round( varp(v, v)   over (), 3) = 549.889 from series limit 1;
select '26', round( sem(v, v)    over (), 3) = 2.369   from series limit 1;
select '27', round( ci(v, v)     over (), 3) = 4.643   from series limit 1;

-- other window functions
select '31', firstnotnull(v) over () = 1 from series limit 1;
select '32', lastnotnull(v) over () = 99 from series limit 1;

-- aggregates
select '33', range(v) = 49 from series;
select '34', median(v) = 50 from series;
select '35', round(covar(v,v), 3) = -6.249 from series;
