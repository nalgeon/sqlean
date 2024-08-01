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

-- Count
select '8_01', text_count(null, 'ello') is null;
select '8_02', text_count('hello yellow', null) is null;
select '8_03', text_count('hello yellow', 'hello') = 1;
select '8_04', text_count('hello yellow', 'yellow') = 1;
select '8_05', text_count('hello yellow', 'ello') = 2;
select '8_06', text_count('hello yellow', 'l') = 4;
select '8_07', text_count('hello yellow', 'x') = 0;

-- Split
select '9_01', text_split(null, '|', 2) is null;
select '9_02', text_split('', '|', 2) = '';
select '9_03', text_split('one|two|three', '|', 2) = 'two';
select '9_04', text_split('one;two;three', ';', 2) = 'two';
select '9_05', text_split('один|два|три', '|', 2) = 'два';
select '9_06', text_split('one|two|three', '|', 10) = '';
select '9_07', text_split('one|two|three', ';', 2) = '';
select '9_08', text_split('one|two|three', '', 1) = 'one|two|three';
select '9_09', text_split('one|two|three', '', 2) = '';
select '9_10', text_split('one,two,three', null, 2) is null;

select '9_11', text_split('one|||four', '|', 1) = 'one';
select '9_12', text_split('one|||four', '|', 2) = '';
select '9_13', text_split('one|||four', '|', 3) = '';
select '9_14', text_split('one|||four', '|', 4) = 'four';
select '9_15', text_split('one/\two/\three', '/\', 2) = 'two';
select '9_16', text_split('one|two|thr', 'two', 1) = 'one|';
select '9_17', text_split('one|two|thr', 'two', 2) = '|thr';
select '9_18', text_split('one|two|thr', 'two', 3) = '';

select '9_21', text_split('|one|two|', '|', 1) = '';
select '9_22', text_split('|one|two|', '|', 2) = 'one';
select '9_23', text_split('|one|two|', '|', 3) = 'two';
select '9_24', text_split('|one|two|', '|', 4) = '';
select '9_25', text_split('one|two', 'one|two', 1) = '';
select '9_26', text_split('one|two', 'one|two', 2) = '';

select '9_31', text_split('one|two|thr', '|', -1) = 'thr';
select '9_32', text_split('one|two|thr', '|', -2) = 'two';
select '9_33', text_split('one|two|thr', '|', -3) = 'one';
select '9_34', text_split('one|two|thr', '|', -4) = '';

-- Join
select '10_01', text_join('|', 'one') = 'one';
select '10_02', text_join('|', 'one', 'two') = 'one|two';
select '10_03', text_join('|', 'one', 'two', 'three') = 'one|two|three';
select '10_04', text_join('|', '', 'two', 'three') = '|two|three';
select '10_05', text_join('|', 'one', '', 'three') = 'one||three';
select '10_06', text_join('|', 'one', 'two', '') = 'one|two|';

select '10_11', text_join('|', 'один', 'два', 'три') = 'один|два|три';
select '10_12', text_join('(ಠ_ಠ)', 'one', 'two', 'three') = 'one(ಠ_ಠ)two(ಠ_ಠ)three';
select '10_13', text_join('(ಠ_ಠ)', 'один', 'два', 'три') = 'один(ಠ_ಠ)два(ಠ_ಠ)три';

select '10_21', text_join(null, 'one', 'two', 'three') is null;
select '10_22', text_join('|', null, 'two', 'three') = 'two|three';
select '10_23', text_join('|', 'one', null, 'three') = 'one|three';
select '10_24', text_join('|', 'one', 'two', null) = 'one|two';
select '10_25', text_join('|', null, 'two', null) = 'two';
select '10_26', text_join('|', null, null, null) = '';

-- Concat
select '11_01', text_concat('one') = 'one';
select '11_02', text_concat('one', 'two') = 'onetwo';
select '11_03', text_concat('one', 'two', 'three') = 'onetwothree';
select '11_04', text_concat('', 'two', 'three') = 'twothree';
select '11_05', text_concat('one', '', 'three') = 'onethree';
select '11_06', text_concat('one', 'two', '') = 'onetwo';
select '11_07', text_concat('один', 'два', 'три') = 'одиндватри';

select '11_11', text_concat(null, 'two', 'three') = 'twothree';
select '11_12', text_concat('one', null, 'three') = 'onethree';
select '11_13', text_concat('one', 'two', null) = 'onetwo';
select '11_14', text_concat(null, 'two', null) = 'two';
select '11_15', text_concat(null, null, null) = '';

-- Repeat
select '12_01', text_repeat(null, 1) is null;
select '12_02', text_repeat('one', -1) = '';
select '12_03', text_repeat('one', 0) = '';
select '12_04', text_repeat('one', 1) = 'one';
select '12_05', text_repeat('one', 2) = 'oneone';
select '12_06', text_repeat('one', 3) = 'oneoneone';
select '12_07', text_repeat('два', 3) = 'двадвадва';

-- Trim left
select '13_01', text_ltrim(null) is null;
select '13_02', text_ltrim('hello') = 'hello';
select '13_03', text_ltrim('  hello') = 'hello';
select '13_04', text_ltrim('  ') = '';
select '13_05', text_ltrim('') = '';
select '13_06', text_ltrim('  привет') = 'привет';

select '13_11', text_ltrim('hello', null) is null;
select '13_12', text_ltrim('hello', '') = 'hello';
select '13_13', text_ltrim('  hello', ' ') = 'hello';
select '13_14', text_ltrim('273hello', '987654321') = 'hello';
select '13_15', text_ltrim('273hello', '98765421') = '3hello';
select '13_16', text_ltrim('273', '987654321') = '';
select '13_17', text_ltrim('', '987654321') = '';
select '13_18', text_ltrim('хохохпривет', 'ох') = 'привет';

-- Trim right
select '14_01', text_rtrim(null) is null;
select '14_02', text_rtrim('hello') = 'hello';
select '14_03', text_rtrim('hello  ') = 'hello';
select '14_04', text_rtrim('  ') = '';
select '14_05', text_rtrim('') = '';
select '14_06', text_rtrim('привет  ') = 'привет';

select '14_11', text_rtrim('hello', null) is null;
select '14_12', text_rtrim('hello', '') = 'hello';
select '14_13', text_rtrim('hello  ', ' ') = 'hello';
select '14_14', text_rtrim('hello372', '987654321') = 'hello';
select '14_15', text_rtrim('hello372', '98765421') = 'hello3';
select '14_16', text_rtrim('372', '987654321') = '';
select '14_17', text_rtrim('', '987654321') = '';
select '14_18', text_rtrim('приветхохох', 'ох') = 'привет';

-- Trim both
select '15_01', text_trim(null) is null;
select '15_02', text_trim('hello') = 'hello';
select '15_03', text_trim('  hello') = 'hello';
select '15_04', text_trim('  ') = '';
select '15_05', text_trim('') = '';
select '15_06', text_trim('  привет') = 'привет';

select '15_11', text_trim('hello', null) is null;
select '15_12', text_trim('hello', '') = 'hello';
select '15_13', text_trim('  hello', ' ') = 'hello';
select '15_14', text_trim('273hello', '987654321') = 'hello';
select '15_15', text_trim('273hello', '98765421') = '3hello';
select '15_16', text_trim('273', '987654321') = '';
select '15_17', text_trim('', '987654321') = '';
select '15_18', text_trim('хохохпривет', 'ох') = 'привет';

select '15_21', text_trim('hello  ') = 'hello';
select '15_22', text_trim('  ') = '';
select '15_23', text_trim('') = '';
select '15_24', text_trim('привет  ') = 'привет';

select '15_31', text_trim('hello  ', ' ') = 'hello';
select '15_32', text_trim('hello372', '987654321') = 'hello';
select '15_33', text_trim('hello372', '98765421') = 'hello3';
select '15_34', text_trim('приветхохох', 'ох') = 'привет';

select '15_41', text_trim('   hello  ') = 'hello';
select '15_42', text_trim('19hello372', '987654321') = 'hello';
select '15_43', text_trim('193hello372', '98765421') = '3hello3';
select '15_44', text_trim('хххприветхохох', 'ох') = 'привет';

-- Pad left
select '16_01', text_lpad(null, 5) is null;
select '16_02', text_lpad('', 5)  = '     ';
select '16_03', text_lpad('hello', -1)  = '';
select '16_04', text_lpad('hello', 0)  = '';
select '16_05', text_lpad('hello', 1)  = 'h';
select '16_06', text_lpad('hello', 3)  = 'hel';
select '16_07', text_lpad('hello', 5)  = 'hello';
select '16_08', text_lpad('hello', 6)  = ' hello';
select '16_09', text_lpad('hello', 8)  = '   hello';
select '16_10', text_lpad('мир', 5)  = '  мир';

select '16_11', text_lpad(null, 5, '*') is null;
select '16_12', text_lpad('hello', 5, null) is null;
select '16_13', text_lpad('', 5, '*') = '*****';
select '16_14', text_lpad('hello', 8, '*') = '***hello';
select '16_15', text_lpad('hello', 8, 'xo') = 'xoxhello';
select '16_16', text_lpad('мир', 6, 'хо') = 'хохмир';

-- Pad right
select '17_01', text_rpad(null, 5) is null;
select '17_02', text_rpad('', 5)  = '     ';
select '17_03', text_rpad('hello', -1)  = '';
select '17_04', text_rpad('hello', 0)  = '';
select '17_05', text_rpad('hello', 1)  = 'h';
select '17_06', text_rpad('hello', 3)  = 'hel';
select '17_07', text_rpad('hello', 5)  = 'hello';
select '17_08', text_rpad('hello', 6)  = 'hello ';
select '17_09', text_rpad('hello', 8)  = 'hello   ';
select '17_10', text_rpad('мир', 5)  = 'мир  ';

select '17_11', text_rpad(null, 5, '*') is null;
select '17_12', text_rpad('hello', 5, null) is null;
select '17_13', text_rpad('', 5, '*') = '*****';
select '17_14', text_rpad('hello', 8, '*') = 'hello***';
select '17_15', text_rpad('hello', 8, 'xo') = 'helloxox';
select '17_16', text_rpad('мир', 6, 'хо') = 'мирхох';

-- Replace all
select '18_01', text_replace(null, 'a', '*') is null;
select '18_02', text_replace('abc', null, '*') is null;
select '18_03', text_replace('abc', 'a', null) is null;
select '18_04', text_replace('hello', 'l', '*')  = 'he**o';
select '18_05', text_replace('hello', 'l', 'xo')  = 'hexoxoo';
select '18_06', text_replace('hello', 'ell', '*')  = 'h*o';
select '18_07', text_replace('hello', 'ello', 'argh')  = 'hargh';
select '18_08', text_replace('hello', 'hello', '-')  = '-';
select '18_09', text_replace('hello', '', '*')  = 'hello';
select '18_10', text_replace('hello', 'l', '')  = 'heo';
select '18_11', text_replace('', 'l', '*')  = '';
select '18_12', text_replace('нетто', 'т', 'три')  = 'нетритрио';

-- Replace
select '19_01', text_replace(null, 'a', '*', 1) is null;
select '19_02', text_replace('abc', null, '*', 1) is null;
select '19_03', text_replace('abc', 'a', null, 1) is null;
select '19_04', text_replace('hello', 'l', '*', 2)  = 'he**o';
select '19_05', text_replace('hello', 'l', 'xo', 2)  = 'hexoxoo';
select '19_06', text_replace('hello', 'ell', '*', 1)  = 'h*o';
select '19_07', text_replace('hello', 'ello', 'argh', 1)  = 'hargh';
select '19_08', text_replace('hello', 'hello', '-', 1)  = '-';
select '19_09', text_replace('hello', '', '*', 1)  = 'hello';
select '19_10', text_replace('hello', 'l', '', 2)  = 'heo';
select '19_11', text_replace('', 'l', '*', 1)  = '';
select '19_12', text_replace('нетто', 'т', 'три', 2)  = 'нетритрио';

select '19_21', text_replace('hello', 'l', '*', -1)  = 'hello';
select '19_22', text_replace('hello', 'l', '*', 0)  = 'hello';
select '19_23', text_replace('hello', 'l', '*', 1)  = 'he*lo';
select '19_24', text_replace('hello', 'l', '*', 2)  = 'he**o';
select '19_25', text_replace('hello', 'l', '*', 3)  = 'he**o';
select '19_16', text_replace('нетто', 'т', 'три', 1)  = 'нетрито';

-- Translate
select '24_01', text_translate(null, 'l', '1') is null;
select '24_02', text_translate('hello', null, '1') is null;
select '24_03', text_translate('hello', 'l', null) is null;
select '24_04', text_translate('hello', 'l', '1')  = 'he11o';
select '24_05', text_translate('hello', 'ole', '013')  = 'h3110';
select '24_06', text_translate('hello', 'oleh', '01')  = '110';
select '24_07', text_translate('12345', '143', 'ax')  = 'a2x5';
select '24_08', text_translate('hello', '', '1')  = 'hello';
select '24_09', text_translate('hello', 'l', '')  = 'heo';
select '24_10', text_translate('hello', '', '')  = 'hello';
select '24_11', text_translate('', 'l', '1')  = '';
select '24_12', text_translate('нетто', 'от', '03')  = 'не330';

-- Reverse
select '20_01', text_reverse(null) is NULL;
select '20_02', text_reverse('') = '';
select '20_03', text_reverse('***') = '***';
select '20_04', text_reverse('hello') = 'olleh';
select '20_05', text_reverse('привет') = 'тевирп';
select '20_06', text_reverse('𐌀𐌁𐌂') = '𐌂𐌁𐌀';
select '20_07', text_reverse('hello 42@ world') = 'dlrow @24 olleh';

-- Length
select '21_01', text_length(null) is NULL;
select '21_02', text_length('') = 0;
select '21_03', text_length('h') = 1;
select '21_04', text_length('hello') = 5;
select '21_05', text_length('привет') = 6;

-- Size
select '22_01', text_size(null) is NULL;
select '22_02', text_size('') = 0;
select '22_03', text_size('h') = 1;
select '22_04', text_size('hello') = 5;
select '22_05', text_size('привет') = 12;

-- Bit size
select '23_01', text_bitsize(null) is NULL;
select '23_02', text_bitsize('') = 0;
select '23_03', text_bitsize('h') = 1*8;
select '23_04', text_bitsize('hello') = 5*8;
select '23_05', text_bitsize('привет') = 2*6*8;

-- Upper
select '25_01', text_upper(null) is null;
select '25_02', text_upper('') = '';
select '25_03', text_upper('hello') = 'HELLO';
select '25_04', text_upper('cómo estás') = 'CÓMO ESTÁS';
select '25_05', text_upper('привет') = 'ПРИВЕТ';
select '25_06', text_upper('пРиВеТ') = 'ПРИВЕТ';

-- Lower
select '26_01', text_lower(null) is null;
select '26_02', text_lower('') = '';
select '26_03', text_lower('HELLO') = 'hello';
select '26_04', text_lower('CÓMO ESTÁS') = 'cómo estás';
select '26_05', text_lower('ПРИВЕТ') = 'привет';
select '26_06', text_lower('пРиВеТ') = 'привет';

-- Title
select '27_01', text_title(null) is null;
select '27_02', text_title('') = '';
select '27_03', text_title('hello world') = 'Hello World';
select '27_04', text_title('cómo estás') = 'Cómo Estás';
select '27_05', text_title('привет мир') = 'Привет Мир';
select '27_06', text_title('пРиВеТ мир') = 'Привет Мир';

-- Casefold
select '28_01', text_casefold(null) is null;
select '28_02', text_casefold('') = '';
select '28_03', text_casefold('HELLO') = 'hello';
select '28_04', text_casefold('CÓMO ESTÁS') = 'cómo estás';
select '28_05', text_casefold('ПРИВЕТ') = 'привет';
select '28_06', text_casefold('пРиВеТ') = 'привет';

-- Like
select '29_01', text_like(null, 'hello') is null;
select '29_02', text_like('hello', null) is null;
select '29_03', text_like('hello', 'hello') = 1;
select '29_04', text_like('h%', 'hello') = 1;
select '29_05', text_like('Hel_o, w__ld!', 'hello, world!') = 1;
select '29_06', text_like('H%l_, w%ld!', 'hello, world!') = 1;
select '29_07', text_like('H%l_, w%ld.', 'hello, world!') = 0;
select '29_08', text_like('c_mo est_s', 'cómo estás') = 1;
select '29_09', text_like('прив_т', 'пРиВеТ') = 1;

-- nocase collation
select '31_01', (select 1 where 'hello' = 'hello' collate text_nocase) = 1;
select '31_02', (select 1 where 'hell0' = 'hello' collate text_nocase) is null;
select '31_03', (select 1 where 'привет' = 'ПРИВЕТ' collate text_nocase) = 1;
