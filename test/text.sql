.load dist/unicode
.load dist/text
select '01', split_part('one,two,three', ',', 2) = 'two';
select '02', split_part('один,два,три', ',', 3) = 'три';
select '03', split_part('one,,,four', ',', 2) = '';
select '04', split_part('one,,,four', ',', 4) = 'four';
select '05', split_part('one/two|three', '/|', 2) = 'two';