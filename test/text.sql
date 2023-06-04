-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/text

-- Substring: start only
select '1_01', text_substring('hello world', 0) = 'hello world';
select '1_02', text_substring('hello world', 1) = 'hello world';
select '1_03', text_substring('hello world', 7) = 'world';
select '1_04', text_substring('hello world', 11) = 'd';
select '1_05', text_substring('hello world', 12) = '';
select '1_06', text_substring('hello world', 15) = '';
select '1_07', text_substring('hello world', -1) = 'hello world';
select '1_08', text_substring('hello world', -5) = 'hello world';
select '1_09', text_substring('hello world', -11) = 'hello world';
select '1_10', text_substring('hello world', -12) = 'hello world';
select '1_11', text_substring('hello world', -15) = 'hello world';

-- Substring: start + length
select '1_21', text_substring('hello world', 0, 5) = 'hell';
select '1_22', text_substring('hello world', -1, 5) = 'hel';
select '1_23', text_substring('hello world', -3, 5) = 'h';
select '1_24', text_substring('hello world', -4, 5) = '';
select '1_25', text_substring('hello world', -5, 5) = '';
select '1_26', text_substring('hello world', -5, 11) = 'hello';
select '1_27', text_substring('hello world', -5, 17) = 'hello world';
select '1_28', text_substring('hello world', -5, 18) = 'hello world';
select '1_29', text_substring('hello world', 1, 0) = '';
select '1_30', text_substring('hello world', 1, 5) = 'hello';
select '1_31', text_substring('hello world', 7, 5) = 'world';
select '1_32', text_substring('hello world', 1, 1) = 'h';
select '1_33', text_substring('hello world', 11, 1) = 'd';
select '1_34', text_substring('hello world', 11, 5) = 'd';
select '1_35', text_substring('hello world', 12, 1) = '';

-- Slice: start only
select '2_01', text_slice('hello world', 0) = 'hello world';
select '2_02', text_slice('hello world', 1) = 'hello world';
select '2_03', text_slice('hello world', 7) = 'world';
select '2_04', text_slice('hello world', 11) = 'd';
select '2_05', text_slice('hello world', 12) = '';
select '2_05', text_slice('hello world', 15) = '';
select '2_06', text_slice('hello world', -1) = 'd';
select '2_07', text_slice('hello world', -5) = 'world';
select '2_08', text_slice('hello world', -11) = 'hello world';
select '2_09', text_slice('hello world', -12) = 'hello world';
select '2_10', text_slice('hello world', -15) = 'hello world';
select '2_11', text_slice(null, 1) is null;

-- Slice: start >= 0, end >= 0
select '2_21', text_slice('hello world', 0, 0) = '';
select '2_22', text_slice('hello world', 0, 5) = 'hell';
select '2_23', text_slice('hello world', 1, 0) = '';
select '2_24', text_slice('hello world', 1, 1) = '';
select '2_25', text_slice('hello world', 1, 2) = 'h';
select '2_26', text_slice('hello world', 1, 5) = 'hell';
select '2_27', text_slice('hello world', 1, 11) = 'hello worl';
select '2_28', text_slice('hello world', 1, 12) = 'hello world';
select '2_29', text_slice('hello world', 1, 15) = 'hello world';
select '2_30', text_slice('hello world', 7, 8) = 'w';
select '2_31', text_slice('hello world', 7, 12) = 'world';
select '2_32', text_slice('hello world', 11, 12) = 'd';
select '2_33', text_slice('hello world', 11, 15) = 'd';
select '2_34', text_slice('hello world', 12, 1) = '';
select '2_35', text_slice(null, 1, 2) is null;

-- Slice: start >= 0, end < 0
select '2_41', text_slice('hello world', 0, -1) = 'hello worl';
select '2_42', text_slice('hello world', 1, -1) = 'hello worl';
select '2_43', text_slice('hello world', 1, -6) = 'hello';
select '2_44', text_slice('hello world', 1, -10) = 'h';
select '2_45', text_slice('hello world', 1, -11) = '';
select '2_46', text_slice('hello world', 1, -15) = '';
select '2_47', text_slice('hello world', 4, -6) = 'lo';
select '2_48', text_slice('hello world', 7, -1) = 'worl';
select '2_49', text_slice('hello world', 10, -1) = 'l';
select '2_50', text_slice('hello world', 11, -1) = '';
select '2_51', text_slice('hello world', 15, -1) = '';

-- Slice: start < 0, end >= 0
select '2_61', text_slice('hello world', -1, 0) = '';
select '2_62', text_slice('hello world', -1, 10) = '';
select '2_63', text_slice('hello world', -1, 11) = '';
select '2_64', text_slice('hello world', -1, 12) = 'd';
select '2_65', text_slice('hello world', -1, 15) = 'd';
select '2_66', text_slice('hello world', -5, 1) = '';
select '2_67', text_slice('hello world', -5, 7) = '';
select '2_68', text_slice('hello world', -5, 8) = 'w';
select '2_69', text_slice('hello world', -5, 9) = 'wo';
select '2_70', text_slice('hello world', -5, 11) = 'worl';
select '2_71', text_slice('hello world', -5, 12) = 'world';
select '2_72', text_slice('hello world', -5, 15) = 'world';
select '2_73', text_slice('hello world', -10, 6) = 'ello';
select '2_74', text_slice('hello world', -11, 6) = 'hello';
select '2_75', text_slice('hello world', -12, 6) = 'hello';
select '2_76', text_slice('hello world', -15, 6) = 'hello';

-- Slice: start < 0, end < 0
select '2_81', text_slice('hello world', -1, -1) = '';
select '2_82', text_slice('hello world', -2, -1) = 'l';
select '2_83', text_slice('hello world', -10, -1) = 'ello worl';
select '2_84', text_slice('hello world', -11, -1) = 'hello worl';
select '2_85', text_slice('hello world', -15, -1) = 'hello worl';
select '2_86', text_slice('hello world', -5, -1) = 'worl';
select '2_87', text_slice('hello world', -5, -8) = '';
select '2_88', text_slice('hello world', -8, -3) = 'lo wo';
select '2_89', text_slice('hello world', -3, -8) = '';

-- Left
select '3_01', text_left(null, 5) is null;
select '3_02', text_left('hello world', 0) = '';
select '3_03', text_left('hello world', 1) = 'h';
select '3_04', text_left('hello world', 5) = 'hello';
select '3_05', text_left('hello world', 10) = 'hello worl';
select '3_06', text_left('hello world', 11) = 'hello world';
select '3_07', text_left('hello world', 15) = 'hello world';
select '3_08', text_left('hello world', -1) = 'hello worl';
select '3_09', text_left('hello world', -6) = 'hello';
select '3_10', text_left('hello world', -10) = 'h';
select '3_11', text_left('hello world', -11) = '';
select '3_12', text_left('hello world', -15) = '';

-- Right
select '4_01', text_right(null, 5) is null;
select '4_02', text_right('hello world', 0) = '';
select '4_03', text_right('hello world', 1) = 'd';
select '4_04', text_right('hello world', 5) = 'world';
select '4_05', text_right('hello world', 10) = 'ello world';
select '4_06', text_right('hello world', 11) = 'hello world';
select '4_07', text_right('hello world', 15) = 'hello world';
select '4_08', text_right('hello world', -1) = 'ello world';
select '4_09', text_right('hello world', -6) = 'world';
select '4_10', text_right('hello world', -10) = 'd';
select '4_11', text_right('hello world', -11) = '';
select '4_12', text_right('hello world', -15) = '';

-- Index
select '4_01', text_index(null, 'ello') is null;
select '4_02', text_index('hello yellow', null) is null;
select '4_03', text_index('hello yellow', 'hello') = 1;
select '4_04', text_index('hello yellow', 'yellow') = 7;
select '4_05', text_index('hello yellow', 'ello') = 2;
select '4_06', text_index('hello yellow', 'x') = 0;

-- Last index
select '5_01', text_last_index(null, 'ello') is null;
select '5_02', text_last_index('hello yellow', null) is null;
select '5_03', text_last_index('hello yellow', 'hello') = 1;
select '5_04', text_last_index('hello yellow', 'yellow') = 7;
select '5_05', text_last_index('hello yellow', 'ello') = 8;
select '5_06', text_last_index('hello yellow', 'x') = 0;

-- Contains
select '6_01', text_contains(null, 'ello') is null;
select '6_02', text_contains('hello yellow', null) is null;
select '6_03', text_contains('hello yellow', 'hello') = 1;
select '6_04', text_contains('hello yellow', 'yellow') = 1;
select '6_05', text_contains('hello yellow', 'ello') = 1;
select '6_06', text_contains('hello yellow', 'x') = 0;

-- Has prefix
select '7_01', text_has_prefix(null, 'ello') is null;
select '7_02', text_has_prefix('hello yellow', null) is null;
select '7_03', text_has_prefix('hello yellow', 'hello') = 1;
select '7_04', text_has_prefix('hello yellow', 'yellow') = 0;
select '7_05', text_has_prefix('hello yellow', 'ello') = 0;
select '7_06', text_has_prefix('hello yellow', 'x') = 0;

-- Has suffix
select '8_01', text_has_suffix(null, 'ello') is null;
select '8_02', text_has_suffix('hello yellow', null) is null;
select '8_03', text_has_suffix('hello yellow', 'hello') = 0;
select '8_04', text_has_suffix('hello yellow', 'yellow') = 1;
select '8_05', text_has_suffix('hello yellow', 'ello') = 0;
select '8_06', text_has_suffix('hello yellow', 'x') = 0;

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
