-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/text

-- Substring
select '1_01', text_substring('hello world', 0) = 'hello world';
select '1_02', text_substring('hello world', 1) = 'hello world';
select '1_03', text_substring('hello world', 7) = 'world';
select '1_04', text_substring('hello world', 11) = 'd';
select '1_05', text_substring('hello world', 12) = '';
select '1_06', text_substring('hello world', -1) = 'd';
select '1_07', text_substring('hello world', -5) = 'world';

select '1_11', text_substring('hello world', 1, 5) = 'hello';
select '1_12', text_substring('hello world', 7, 5) = 'world';
select '1_13', text_substring('hello world', 1, 1) = 'h';
select '1_14', text_substring('hello world', 11, 1) = 'd';
select '1_15', text_substring('hello world', 12, 1) = '';
select '1_16', text_substring('hello world', 6, -2) = 'lo';
select '1_17', text_substring('hello world', 6, -5) = 'hello';
select '1_18', text_substring('hello world', 6, -10) = '';
select '1_19', text_substring('hello world', -3, -2) = 'wo';

-- Slice
select '2_01', text_slice('hello world', 0) = 'hello world';
select '2_02', text_slice('hello world', 1) = 'hello world';
select '2_03', text_slice('hello world', 7) = 'world';
select '2_04', text_slice('hello world', 11) = 'd';
select '2_05', text_slice('hello world', 12) = '';
select '2_06', text_slice('hello world', -1) = 'd';
select '2_07', text_slice('hello world', -5) = 'world';

select '2_11', text_slice('hello world', 1, 6) = 'hello';
select '2_12', text_slice('hello world', 7, 12) = 'world';
select '2_13', text_slice('hello world', 1, 2) = 'h';
select '2_14', text_slice('hello world', 11, 12) = 'd';
select '2_15', text_slice('hello world', 12, 13) = '';
select '2_16', text_slice('hello world', 6, 1) = '';
select '2_17', text_slice('hello world', 7, -1) = 'worl';
select '2_18', text_slice('hello world', -5, -2) = 'wor';
select '2_19', text_slice('hello world', -2, -5) = '';

-- Reverse string
select 'x_01', text_reverse(null) is NULL;
select 'x_02', text_reverse('hello') = 'olleh';
select 'x_03', text_reverse('привет') = 'тевирп';
select 'x_04', text_reverse("𐌀𐌁𐌂") = '𐌂𐌁𐌀';
select 'x_05', text_reverse('hello 42@ world') = 'dlrow @24 olleh';

-- Extract part from string
select 'y_01', text_split(NULL, ',', 2) is NULL;
select 'y_02', text_split('', ',', 2) = '';
select 'y_03', text_split('one,two,three', ',', 2) = 'two';
select 'y_04', text_split('one|two|three', '|', 2) = 'two';
select 'y_05', text_split('один,два,три', ',', 2) = 'два';
select 'y_06', text_split('one,two,three', ',', 10) = '';
select 'y_07', text_split('one,two,three', ';', 2) = '';
select 'y_08', text_split('one,two,three', '', 1) = 'one,two,three';
select 'y_09', text_split('one,two,three', NULL, 2) is NULL;
select 'y_10', text_split('one,,,four', ',', 2) = '';
select 'y_11', text_split('one,,,four', ',', 4) = 'four';
select 'y_12', text_split('one/\two/\three', '/\', 2) = 'two';
