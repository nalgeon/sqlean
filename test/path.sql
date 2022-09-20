-- Copyright (c) 2022 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/path

select '01', path_dirname('parent/child.txt') = 'parent/';
select '02', path_basename('parent/child.txt') = 'child.txt';
select '03', path_name('parent/child.txt') = 'child';
select '04', path_extension('parent/child.txt') = '.txt';
select '05', path_intersection('parent/child.txt', 'parent/other.txt') = 'parent';
select '06', path_join('parent', 'child.txt') = 'parent/child.txt';
select '07', path_normalize('parent/sub/../child.txt') = 'parent/child.txt';
select '08', path_root('/parent/child.txt') = '/';
select '09', path_absolute('/parent/child.txt') = true;

select '11', path_part_at('one/two/three.txt', 0) = 'one';
select '12', path_part_at('one/two/three.txt', 1) = 'two';
select '13', path_part_at('one/two/three.txt', 2) = 'three.txt';
select '14', path_part_at('one/two/three.txt', -1) = 'three.txt';

select '21', count(*) = 3 from path_parts('one/two/three.txt');
