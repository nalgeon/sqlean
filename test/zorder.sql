-- Copyright (c) 2022 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/zorder

-- Let's say we have a 2D space
-- and the rectangular area [2,3] - [4,5]
-- 6
-- 5     x  x  x
-- 4     x  x  x
-- 3     x  x  x
-- 2
-- 1
-- 0  1  2  3  4  5

-- Let's map it into 1D space and search for some points
-- minimum z-value (zmin)
select '01', zorder(2, 3) = 14;
-- maximum z-value (zmax)
select '02', zorder(4, 5) = 50;

-- if the point is inside the target area,
-- then its z-value is in [zmin, zmax]
select '03', zorder(2, 5) between zorder(2, 3) and zorder(4, 5);
select '04', zorder(3, 4) between zorder(2, 3) and zorder(4, 5);

-- if the point is outside the target area,
-- then its z-value may be not in [zmin, zmax]
select '05', zorder(2, 2) not between zorder(2, 3) and zorder(4, 5);
select '06', zorder(5, 6) not between zorder(2, 3) and zorder(4, 5);
-- but it also may be in [zmin, zmax]
-- (so there are some false positives)
select '07', zorder(5, 4) between zorder(2, 3) and zorder(4, 5);

-- reverse operation: extract a 2D point from its z-value
-- zmin
select '11', unzorder(14, 2, 0) = 2;
select '12', unzorder(14, 2, 1) = 3;
-- zmax
select '13', unzorder(50, 2, 0) = 4;
select '14', unzorder(50, 2, 1) = 5;
-- some other point
select '15', unzorder(zorder(3, 4), 2, 0) = 3;
select '16', unzorder(zorder(3, 4), 2, 1) = 4;
