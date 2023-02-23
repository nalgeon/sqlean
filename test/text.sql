-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/text

-- Substring
select '01', text_substring('hello world', 0) = 'hello world';
select '02', text_substring('hello world', 1) = 'hello world';
select '03', text_substring('hello world', 7) = 'world';
select '04', text_substring('hello world', 11) = 'd';
select '05', text_substring('hello world', 12) = '';
select '06', text_substring('hello world', -1) = 'd';
select '07', text_substring('hello world', -5) = 'world';

select '11', text_substring('hello world', 1, 5) = 'hello';
select '12', text_substring('hello world', 7, 5) = 'world';
select '13', text_substring('hello world', 1, 1) = 'h';
select '14', text_substring('hello world', 11, 1) = 'd';
select '15', text_substring('hello world', 12, 1) = '';
select '16', text_substring('hello world', 6, -2) = 'lo';
select '17', text_substring('hello world', 6, -5) = 'hello';
select '18', text_substring('hello world', 6, -10) = '';
select '19', text_substring('hello world', -3, -2) = 'wo';

-- Slice
select '21', text_slice('hello world', 0) = 'hello world';
select '22', text_slice('hello world', 1) = 'hello world';
select '23', text_slice('hello world', 7) = 'world';
select '24', text_slice('hello world', 11) = 'd';
select '25', text_slice('hello world', 12) = '';
select '26', text_slice('hello world', -1) = 'd';
select '27', text_slice('hello world', -5) = 'world';

select '31', text_slice('hello world', 1, 6) = 'hello';
select '32', text_slice('hello world', 7, 12) = 'world';
select '33', text_slice('hello world', 1, 2) = 'h';
select '34', text_slice('hello world', 11, 12) = 'd';
select '35', text_slice('hello world', 12, 13) = '';
select '36', text_slice('hello world', 6, 1) = '';
select '37', text_slice('hello world', 7, -1) = 'worl';
select '38', text_slice('hello world', -5, -2) = 'wor';
select '39', text_slice('hello world', -2, -5) = '';

-- Reverse string
select 'x1', text_reverse(null) is NULL;
select 'x2', text_reverse('hello') = 'olleh';
select 'x3', text_reverse('привет') = 'тевирп';
select 'x4', text_reverse("𐌀𐌁𐌂") = '𐌂𐌁𐌀';
select 'x5', text_reverse('hello 42@ world') = 'dlrow @24 olleh';

-- Extract part from string
select 'y1', text_split(NULL, ',', 2) is NULL;
select 'y2', text_split('', ',', 2) = '';
select 'y3', text_split('one,two,three', ',', 2) = 'two';
select 'y4', text_split('one|two|three', '|', 2) = 'two';
select 'y5', text_split('один,два,три', ',', 2) = 'два';
select 'y6', text_split('one,two,three', ',', 10) = '';
select 'y7', text_split('one,two,three', ';', 2) = '';
select 'y8', text_split('one,two,three', '', 1) = 'one,two,three';
select 'y9', text_split('one,two,three', NULL, 2) is NULL;
select 'y0', text_split('one,,,four', ',', 2) = '';
select 'y1', text_split('one,,,four', ',', 4) = 'four';
select 'y2', text_split('one/\two/\three', '/\', 2) = 'two';
