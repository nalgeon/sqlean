-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/text

-- Reverse string
select '01', reverse(null) is NULL;
select '02', reverse('hello') = 'olleh';
select '03', reverse('привет') = 'тевирп';
select '04', reverse("𐌀𐌁𐌂") = '𐌂𐌁𐌀';
select '05', reverse('hello 42@ world') = 'dlrow @24 olleh';

-- Extract part from string
select '11', split_part(NULL, ',', 2) is NULL;
select '12', split_part('', ',', 2) = '';
select '13', split_part('one,two,three', ',', 2) = 'two';
select '14', split_part('one|two|three', '|', 2) = 'two';
select '15', split_part('один,два,три', ',', 2) = 'два';
select '16', split_part('one,two,three', ',', 10) = '';
select '17', split_part('one,two,three', ';', 2) = '';
select '18', split_part('one,two,three', '', 1) = 'one,two,three';
select '19', split_part('one,two,three', NULL, 2) is NULL;
select '20', split_part('one,,,four', ',', 2) = '';
select '21', split_part('one,,,four', ',', 4) = 'four';
select '22', split_part('one/two|three', '/|', 2) = 'two';