-- Copyright (c) 2018 Shawn Wagner, MIT License
-- https://github.com/shawnw/useful_sqlite_extensions

.load dist/bloom

create virtual table plants using bloom_filter(20);
insert into plants values ('apple'), ('asparagus'), ('cabbage'), ('grass');

select '01', present = 1 from plants('apple');
select '02', count(*) = 1 from plants('apple');
select '03', count(*) = 1 from plants('grass');
select '04', count(*) = 0 from plants('lemon');
select '05', count(*) = 0 from plants('grape');
