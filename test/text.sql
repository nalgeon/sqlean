-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/text

-- Positions are 1-based and count characters, not bytes.
-- 'hello world' has 11 characters, 'привет' has 6 characters and 12 bytes.
-- x'41ff42' is 'A', an invalid byte, 'B'; x'410042' is 'A', a zero byte, 'B'.
-- A comment starting with "now:" marks current behavior that is wrong (see TODO.md).

-- text_substring
select '01_01', text_substring('hello world', 1) = 'hello world';
select '01_02', text_substring('hello world', 7) = 'world';
select '01_03', text_substring('hello world', 11) = 'd';
select '01_04', text_substring('hello world', 12) = '';
select '01_05', text_substring('hello world', 15) = '';
-- start <= 0 is the beginning of the string
select '01_06', text_substring('hello world', 0) = 'hello world';
select '01_07', text_substring('hello world', -1) = 'hello world';
select '01_08', text_substring('hello world', -11) = 'hello world';
select '01_09', text_substring('hello world', -15) = 'hello world';
select '01_10', text_substring('привет', 3) = 'ивет';
select '01_11', text_substring('', 1) = '';
select '01_12', text_substring(null, 1) is null;
select '01_13', text_substring(12345, 2) = '2345';
-- now: the start is truncated to 32 bits, 4294967297 becomes 1
select '01_14', text_substring('hello', 4294967297) = 'hello';
-- start + length
select '01_21', text_substring('hello world', 1, 5) = 'hello';
select '01_22', text_substring('hello world', 7, 5) = 'world';
select '01_23', text_substring('hello world', 1, 1) = 'h';
select '01_24', text_substring('hello world', 1, 0) = '';
select '01_25', text_substring('hello world', 11, 1) = 'd';
select '01_26', text_substring('hello world', 11, 5) = 'd';
select '01_27', text_substring('hello world', 12, 1) = '';
-- a start before the beginning shortens the length, as in Postgres
select '01_28', text_substring('hello world', 0, 5) = 'hell';
select '01_29', text_substring('hello world', -1, 5) = 'hel';
select '01_30', text_substring('hello world', -3, 5) = 'h';
select '01_31', text_substring('hello world', -4, 5) = '';
select '01_32', text_substring('hello world', -5, 5) = '';
select '01_33', text_substring('hello world', -5, 11) = 'hello';
select '01_34', text_substring('hello world', -5, 17) = 'hello world';
select '01_35', text_substring('hello world', -5, 18) = 'hello world';
select '01_36', text_substring('привет', 2, 3) = 'рив';
select '01_37', text_substring('世界abc', 1, 2) = '世界';
select '01_38', text_substring('', 1, 2) = '';
select '01_39', text_substring(null, 1, 2) is null;
select '01_40', text_substring('hello', 2147483647, 2147483647) = '';
select '01_41', text_substring('hello', -2147483647, 2147483647) = '';

-- text_slice
select '02_01', text_slice('hello world', 1) = 'hello world';
select '02_02', text_slice('hello world', 7) = 'world';
select '02_03', text_slice('hello world', 11) = 'd';
select '02_04', text_slice('hello world', 12) = '';
select '02_05', text_slice('hello world', 15) = '';
-- 0 is the beginning of the string, like 1
select '02_06', text_slice('hello world', 0) = 'hello world';
-- a negative start counts from the end
select '02_07', text_slice('hello world', -1) = 'd';
select '02_08', text_slice('hello world', -5) = 'world';
select '02_09', text_slice('hello world', -11) = 'hello world';
select '02_10', text_slice('hello world', -12) = 'hello world';
select '02_11', text_slice('hello world', -15) = 'hello world';
select '02_12', text_slice('hello', -2147483648) = 'hello';
select '02_13', text_slice('привет', -3) = 'вет';
select '02_14', text_slice('', 1) = '';
select '02_15', text_slice(null, 1) is null;
-- now: the start is truncated to 32 bits, 4294967298 becomes 2
select '02_16', text_slice('hello', 4294967298) = 'ello';
-- start >= 0, end >= 0; the end is exclusive, and 0 and 1 are both the beginning
select '02_21', text_slice('hello world', 0, 0) = '';
select '02_22', text_slice('hello world', 0, 5) = 'hell';
select '02_23', text_slice('hello world', 1, 0) = '';
select '02_24', text_slice('hello world', 1, 1) = '';
select '02_25', text_slice('hello world', 1, 2) = 'h';
select '02_26', text_slice('hello world', 1, 5) = 'hell';
select '02_27', text_slice('hello world', 1, 11) = 'hello worl';
select '02_28', text_slice('hello world', 1, 12) = 'hello world';
select '02_29', text_slice('hello world', 1, 15) = 'hello world';
select '02_30', text_slice('hello world', 7, 8) = 'w';
select '02_31', text_slice('hello world', 7, 12) = 'world';
select '02_32', text_slice('hello world', 11, 12) = 'd';
select '02_33', text_slice('hello world', 11, 15) = 'd';
select '02_34', text_slice('hello world', 12, 1) = '';
select '02_35', text_slice('hello', 1, 2147483647) = 'hello';
select '02_36', text_slice('привет', 2, 4) = 'ри';
select '02_37', text_slice(null, 1, 2) is null;
-- start >= 0, end < 0
select '02_41', text_slice('hello world', 0, -1) = 'hello worl';
select '02_42', text_slice('hello world', 1, -1) = 'hello worl';
select '02_43', text_slice('hello world', 1, -6) = 'hello';
select '02_44', text_slice('hello world', 1, -10) = 'h';
select '02_45', text_slice('hello world', 1, -11) = '';
select '02_46', text_slice('hello world', 1, -15) = '';
select '02_47', text_slice('hello world', 4, -6) = 'lo';
select '02_48', text_slice('hello world', 7, -1) = 'worl';
select '02_49', text_slice('hello world', 10, -1) = 'l';
select '02_50', text_slice('hello world', 11, -1) = '';
select '02_51', text_slice('hello world', 15, -1) = '';
select '02_52', text_slice('hello', 1, -2147483648) = '';
-- start < 0, end >= 0
select '02_61', text_slice('hello world', -1, 0) = '';
select '02_62', text_slice('hello world', -1, 10) = '';
select '02_63', text_slice('hello world', -1, 11) = '';
select '02_64', text_slice('hello world', -1, 12) = 'd';
select '02_65', text_slice('hello world', -1, 15) = 'd';
select '02_66', text_slice('hello world', -5, 1) = '';
select '02_67', text_slice('hello world', -5, 7) = '';
select '02_68', text_slice('hello world', -5, 8) = 'w';
select '02_69', text_slice('hello world', -5, 9) = 'wo';
select '02_70', text_slice('hello world', -5, 11) = 'worl';
select '02_71', text_slice('hello world', -5, 12) = 'world';
select '02_72', text_slice('hello world', -5, 15) = 'world';
select '02_73', text_slice('hello world', -10, 6) = 'ello';
select '02_74', text_slice('hello world', -11, 6) = 'hello';
select '02_75', text_slice('hello world', -12, 6) = 'hello';
select '02_76', text_slice('hello world', -15, 6) = 'hello';
select '02_77', text_slice('hello', -2147483648, 2) = 'h';
-- start < 0, end < 0
select '02_81', text_slice('hello world', -1, -1) = '';
select '02_82', text_slice('hello world', -2, -1) = 'l';
select '02_83', text_slice('hello world', -10, -1) = 'ello worl';
select '02_84', text_slice('hello world', -11, -1) = 'hello worl';
select '02_85', text_slice('hello world', -15, -1) = 'hello worl';
select '02_86', text_slice('hello world', -5, -1) = 'worl';
select '02_87', text_slice('hello world', -5, -8) = '';
select '02_88', text_slice('hello world', -8, -3) = 'lo wo';
select '02_89', text_slice('hello world', -3, -8) = '';

-- text_left, left
select '03_01', text_left('hello world', 0) = '';
select '03_02', text_left('hello world', 1) = 'h';
select '03_03', text_left('hello world', 5) = 'hello';
select '03_04', text_left('hello world', 10) = 'hello worl';
select '03_05', text_left('hello world', 11) = 'hello world';
select '03_06', text_left('hello world', 15) = 'hello world';
-- a negative length drops characters from the end
select '03_07', text_left('hello world', -1) = 'hello worl';
select '03_08', text_left('hello world', -6) = 'hello';
select '03_09', text_left('hello world', -10) = 'h';
select '03_10', text_left('hello world', -11) = '';
select '03_11', text_left('hello world', -15) = '';
select '03_12', text_left('hello', -2147483648) = '';
select '03_13', text_left('привет', 2) = 'пр';
select '03_14', text_left('', 5) = '';
select '03_15', text_left(null, 5) is null;
select '03_16', left('hello world', 5) = 'hello';
-- now: the length is truncated to 32 bits, 4294967297 becomes 1
select '03_17', text_left('hello', 4294967297) = 'h';

-- text_right, right
select '04_01', text_right('hello world', 0) = '';
select '04_02', text_right('hello world', 1) = 'd';
select '04_03', text_right('hello world', 5) = 'world';
select '04_04', text_right('hello world', 10) = 'ello world';
select '04_05', text_right('hello world', 11) = 'hello world';
select '04_06', text_right('hello world', 15) = 'hello world';
select '04_07', text_right('hello', 2147483647) = 'hello';
-- a negative length drops characters from the beginning
select '04_08', text_right('hello world', -1) = 'ello world';
select '04_09', text_right('hello world', -6) = 'world';
select '04_10', text_right('hello world', -10) = 'd';
select '04_11', text_right('hello world', -11) = '';
select '04_12', text_right('hello world', -15) = '';
select '04_13', text_right('привет', 2) = 'ет';
select '04_14', text_right('', 5) = '';
select '04_15', text_right(null, 5) is null;
select '04_16', right('hello world', 5) = 'world';

-- text_index, strpos
select '11_01', text_index('hello yellow', 'hello') = 1;
select '11_02', text_index('hello yellow', 'yellow') = 7;
select '11_03', text_index('hello yellow', 'ello') = 2;
select '11_04', text_index('hello yellow', 'x') = 0;
select '11_05', text_index('aab', 'ab') = 2;
-- case-sensitive
select '11_06', text_index('Hello', 'h') = 0;
-- the index counts characters, not bytes
select '11_07', text_index('привет мир', 'мир') = 8;
-- an empty substring is found at the beginning
select '11_08', text_index('hello', '') = 1;
select '11_09', text_index('', '') = 1;
select '11_10', text_index('', 'a') = 0;
select '11_11', text_index('hello', 'hello!') = 0;
select '11_12', text_index(12345, 34) = 3;
select '11_13', text_index(null, 'ello') is null;
select '11_14', text_index('hello yellow', null) is null;
select '11_15', strpos('hello yellow', 'ello') = 2;

-- text_last_index
select '12_01', text_last_index('hello yellow', 'hello') = 1;
select '12_02', text_last_index('hello yellow', 'yellow') = 7;
select '12_03', text_last_index('hello yellow', 'ello') = 8;
select '12_04', text_last_index('hello yellow', 'x') = 0;
select '12_05', text_last_index('abaa', 'ab') = 1;
select '12_06', text_last_index('мир мир', 'мир') = 5;
-- now: an empty substring is found at the last character, but not in an empty string
-- (text_index finds it in both)
select '12_07', text_last_index('hello', '') = 5;
select '12_08', text_last_index('', '') = 0;
select '12_09', text_last_index('', 'a') = 0;
select '12_10', text_last_index(null, 'ello') is null;
select '12_11', text_last_index('hello yellow', null) is null;

-- text_contains
select '13_01', text_contains('hello yellow', 'hello') = 1;
select '13_02', text_contains('hello yellow', 'yellow') = 1;
select '13_03', text_contains('hello yellow', 'ello') = 1;
select '13_04', text_contains('hello yellow', 'x') = 0;
select '13_05', text_contains('Hello', 'h') = 0;
select '13_06', text_contains('привет', 'иве') = 1;
select '13_07', text_contains('hello', '') = 1;
select '13_08', text_contains('', '') = 1;
select '13_09', text_contains('', 'a') = 0;
select '13_10', text_contains(null, 'ello') is null;
select '13_11', text_contains('hello yellow', null) is null;

-- text_has_prefix, starts_with
select '14_01', text_has_prefix('hello yellow', 'hello') = 1;
select '14_02', text_has_prefix('hello yellow', 'yellow') = 0;
select '14_03', text_has_prefix('hello yellow', 'ello') = 0;
select '14_04', text_has_prefix('hello yellow', 'x') = 0;
select '14_05', text_has_prefix('Hello', 'h') = 0;
select '14_06', text_has_prefix('привет', 'пр') = 1;
select '14_07', text_has_prefix('hello', '') = 1;
select '14_08', text_has_prefix('', '') = 1;
select '14_09', text_has_prefix('he', 'hello') = 0;
select '14_10', text_has_prefix(null, 'ello') is null;
select '14_11', text_has_prefix('hello yellow', null) is null;
select '14_12', starts_with('hello yellow', 'hello') = 1;

-- text_has_suffix
select '15_01', text_has_suffix('hello yellow', 'hello') = 0;
select '15_02', text_has_suffix('hello yellow', 'yellow') = 1;
select '15_03', text_has_suffix('hello yellow', 'ello') = 0;
select '15_04', text_has_suffix('hello yellow', 'x') = 0;
select '15_05', text_has_suffix('ab', 'B') = 0;
select '15_06', text_has_suffix('привет', 'ет') = 1;
select '15_07', text_has_suffix('hello', '') = 1;
select '15_08', text_has_suffix('', '') = 1;
select '15_09', text_has_suffix('lo', 'hello') = 0;
select '15_10', text_has_suffix(null, 'ello') is null;
select '15_11', text_has_suffix('hello yellow', null) is null;

-- text_count
select '16_01', text_count('hello yellow', 'hello') = 1;
select '16_02', text_count('hello yellow', 'yellow') = 1;
select '16_03', text_count('hello yellow', 'ello') = 2;
select '16_04', text_count('hello yellow', 'l') = 4;
select '16_05', text_count('hello yellow', 'x') = 0;
-- matches don't overlap
select '16_06', text_count('aaaa', 'aa') = 2;
select '16_07', text_count('мама', 'ма') = 2;
-- now: an empty substring is never counted, though text_contains finds it
select '16_08', text_count('hello', '') = 0;
select '16_09', text_count('', 'a') = 0;
select '16_10', text_count(null, 'ello') is null;
select '16_11', text_count('hello yellow', null) is null;

-- text_like
select '17_01', text_like('hello', 'hello') = 1;
select '17_02', text_like('h%', 'hello') = 1;
select '17_03', text_like('Hel_o, w__ld!', 'hello, world!') = 1;
select '17_04', text_like('H%l_, w%ld!', 'hello, world!') = 1;
select '17_05', text_like('H%l_, w%ld.', 'hello, world!') = 0;
select '17_06', text_like('%a', 'ba') = 1;
select '17_07', text_like('a%b%c', 'aXbYc') = 1;
select '17_08', text_like('ab', 'a') = 0;
-- case-insensitive for all of Unicode
select '17_09', text_like('HELLO', 'hello') = 1;
select '17_10', text_like('прив_т', 'пРиВеТ') = 1;
select '17_11', text_like('cóm_ está_', 'CÓMO ESTÁS') = 1;
-- _ matches a character, not a byte
select '17_12', text_like('c_mo est_s', 'cómo estás') = 1;
select '17_13', text_like('_', 'é') = 1;
select '17_14', text_like('', '') = 1;
select '17_15', text_like('%', '') = 1;
select '17_16', text_like('a', '') = 0;
select '17_17', text_like('_', '') = 0;
-- there is no escape character
select '17_18', text_like('a\%', 'a%') = 0;
select '17_19', text_like(null, 'hello') is null;
select '17_20', text_like('hello', null) is null;

-- text_split, split_part
select '21_01', text_split('one|two|three', '|', 2) = 'two';
select '21_02', text_split('one;two;three', ';', 2) = 'two';
select '21_03', text_split('один|два|три', '|', 2) = 'два';
select '21_04', text_split('один—два', '—', 2) = 'два';
select '21_05', text_split('one|two|three', '|', 10) = '';
select '21_06', text_split('one|two|three', ';', 1) = 'one|two|three';
select '21_07', text_split('one|two|three', ';', 2) = '';
select '21_08', text_split('', '|', 1) = '';
select '21_09', text_split('', '|', 2) = '';
select '21_10', text_split('a', 'abc', 1) = '';
-- an empty separator doesn't split
select '21_11', text_split('one|two|three', '', 1) = 'one|two|three';
select '21_12', text_split('one|two|three', '', 2) = '';
select '21_13', text_split('one|two|three', '', -1) = 'one|two|three';
-- empty parts
select '21_21', text_split('one|||four', '|', 1) = 'one';
select '21_22', text_split('one|||four', '|', 2) = '';
select '21_23', text_split('one|||four', '|', 3) = '';
select '21_24', text_split('one|||four', '|', 4) = 'four';
select '21_25', text_split('|one|two|', '|', 1) = '';
select '21_26', text_split('|one|two|', '|', 2) = 'one';
select '21_27', text_split('|one|two|', '|', 3) = 'two';
select '21_28', text_split('|one|two|', '|', 4) = '';
select '21_29', text_split('one|two', 'one|two', 1) = '';
select '21_30', text_split('one|two', 'one|two', 2) = '';
-- multi-character separators
select '21_31', text_split('one/\two/\three', '/\', 2) = 'two';
select '21_32', text_split('one|two|thr', 'two', 1) = 'one|';
select '21_33', text_split('one|two|thr', 'two', 2) = '|thr';
select '21_34', text_split('one|two|thr', 'two', 3) = '';
-- a negative part counts from the end
select '21_41', text_split('one|two|thr', '|', -1) = 'thr';
select '21_42', text_split('one|two|thr', '|', -2) = 'two';
select '21_43', text_split('one|two|thr', '|', -3) = 'one';
select '21_44', text_split('one|two|thr', '|', -4) = '';
select '21_45', text_split('a|b', '|', -2147483648) = '';
select '21_46', text_split(12345, 3, 2) = '45';
select '21_47', text_split(null, '|', 2) is null;
select '21_48', text_split('one,two,three', null, 2) is null;
select '21_49', split_part('one|two|three', '|', 2) = 'two';

-- text_join, concat_ws
select '22_01', text_join('|', 'one') = 'one';
select '22_02', text_join('|', 'one', 'two') = 'one|two';
select '22_03', text_join('|', 'one', 'two', 'three') = 'one|two|three';
select '22_04', text_join('|', '', 'two', 'three') = '|two|three';
select '22_05', text_join('|', 'one', '', 'three') = 'one||three';
select '22_06', text_join('|', 'one', 'two', '') = 'one|two|';
select '22_07', text_join('', 'one', 'two') = 'onetwo';
select '22_08', text_join('|', 'один', 'два', 'три') = 'один|два|три';
select '22_09', text_join('(ಠ_ಠ)', 'one', 'two', 'three') = 'one(ಠ_ಠ)two(ಠ_ಠ)three';
select '22_10', text_join('(ಠ_ಠ)', 'один', 'два', 'три') = 'один(ಠ_ಠ)два(ಠ_ಠ)три';
select '22_11', text_join(0, 1, 2.5) = '102.5';
-- now: blobs are joined as text
select '22_12', text_join('|', x'41', x'42') = 'A|B';
-- NULL parts are skipped, a NULL separator gives NULL
select '22_21', text_join(null, 'one', 'two', 'three') is null;
select '22_22', text_join('|', null, 'two', 'three') = 'two|three';
select '22_23', text_join('|', 'one', null, 'three') = 'one|three';
select '22_24', text_join('|', 'one', 'two', null) = 'one|two';
select '22_25', text_join('|', null, 'two', null) = 'two';
select '22_26', text_join('|', null, null, null) = '';
select '22_27', concat_ws('|', 'one', null, 'three') = 'one|three';

-- text_concat, concat
select '23_01', text_concat('one') = 'one';
select '23_02', text_concat('one', 'two') = 'onetwo';
select '23_03', text_concat('one', 'two', 'three') = 'onetwothree';
select '23_04', text_concat('', 'two', 'three') = 'twothree';
select '23_05', text_concat('one', '', 'three') = 'onethree';
select '23_06', text_concat('one', 'two', '') = 'onetwo';
select '23_07', text_concat('один', 'два', 'три') = 'одиндватри';
select '23_08', text_concat(1, 2.5) = '12.5';
-- now: blobs are concatenated as text
select '23_09', text_concat(x'41', x'42') = 'AB';
-- NULL parts are skipped
select '23_11', text_concat(null, 'two', 'three') = 'twothree';
select '23_12', text_concat('one', null, 'three') = 'onethree';
select '23_13', text_concat('one', 'two', null) = 'onetwo';
select '23_14', text_concat(null, 'two', null) = 'two';
select '23_15', text_concat(null, null, null) = '';
select '23_16', concat('one', null, 'three') = 'onethree';

-- text_repeat, repeat
select '24_01', text_repeat('one', 0) = '';
select '24_02', text_repeat('one', 1) = 'one';
select '24_03', text_repeat('one', 2) = 'oneone';
select '24_04', text_repeat('one', 3) = 'oneoneone';
select '24_05', text_repeat('два', 3) = 'двадвадва';
select '24_06', text_repeat('', 1000000000) = '';
-- a negative count is 0, as in Postgres
select '24_07', text_repeat('one', -1) = '';
select '24_08', text_repeat(12, 2) = '1212';
select '24_09', text_repeat(null, 1) is null;
select '24_10', repeat('one', 2) = 'oneone';
-- now: the count is truncated to 32 bits, 4294967297 becomes 1
select '24_11', text_repeat('a', 4294967297) = 'a';

-- text_ltrim, ltrim
select '31_01', text_ltrim('hello') = 'hello';
select '31_02', text_ltrim('  hello') = 'hello';
select '31_03', text_ltrim('hello  ') = 'hello  ';
select '31_04', text_ltrim('  ') = '';
select '31_05', text_ltrim('') = '';
select '31_06', text_ltrim('  привет') = 'привет';
-- only spaces by default, as in Postgres
select '31_07', text_ltrim(char(9) || 'hello') = char(9) || 'hello';
select '31_08', text_ltrim(null) is null;
-- with chars
select '31_11', text_ltrim('hello', '') = 'hello';
select '31_12', text_ltrim('  hello', ' ') = 'hello';
select '31_13', text_ltrim('273hello', '987654321') = 'hello';
select '31_14', text_ltrim('273hello', '98765421') = '3hello';
select '31_15', text_ltrim('273', '987654321') = '';
select '31_16', text_ltrim('', '987654321') = '';
select '31_17', text_ltrim('хохохпривет', 'ох') = 'привет';
select '31_18', text_ltrim(11123, 1) = '23';
select '31_19', text_ltrim(null, 'x') is null;
select '31_20', text_ltrim('hello', null) is null;
select '31_21', ltrim('xxhello', 'x') = 'hello';

-- text_rtrim, rtrim
select '32_01', text_rtrim('hello') = 'hello';
select '32_02', text_rtrim('hello  ') = 'hello';
select '32_03', text_rtrim('  hello') = '  hello';
select '32_04', text_rtrim('  ') = '';
select '32_05', text_rtrim('') = '';
select '32_06', text_rtrim('привет  ') = 'привет';
select '32_07', text_rtrim('hello' || char(9)) = 'hello' || char(9);
select '32_08', text_rtrim(null) is null;
-- with chars
select '32_11', text_rtrim('hello', '') = 'hello';
select '32_12', text_rtrim('hello  ', ' ') = 'hello';
select '32_13', text_rtrim('hello372', '987654321') = 'hello';
select '32_14', text_rtrim('hello372', '98765421') = 'hello3';
select '32_15', text_rtrim('372', '987654321') = '';
select '32_16', text_rtrim('', '987654321') = '';
select '32_17', text_rtrim('приветхохох', 'ох') = 'привет';
select '32_18', text_rtrim(12333, 3) = '12';
select '32_19', text_rtrim(null, 'x') is null;
select '32_20', text_rtrim('hello', null) is null;
select '32_21', rtrim('helloxx', 'x') = 'hello';

-- text_trim, btrim
select '33_01', text_trim('hello') = 'hello';
select '33_02', text_trim('  hello') = 'hello';
select '33_03', text_trim('hello  ') = 'hello';
select '33_04', text_trim('   hello  ') = 'hello';
select '33_05', text_trim('  ') = '';
select '33_06', text_trim('') = '';
select '33_07', text_trim('  привет  ') = 'привет';
select '33_08', text_trim(char(9) || 'hello') = char(9) || 'hello';
select '33_09', text_trim(null) is null;
-- with chars
select '33_11', text_trim('hello', '') = 'hello';
select '33_12', text_trim('  hello  ', ' ') = 'hello';
select '33_13', text_trim('273hello', '987654321') = 'hello';
select '33_14', text_trim('hello372', '987654321') = 'hello';
select '33_15', text_trim('19hello372', '987654321') = 'hello';
select '33_16', text_trim('193hello372', '98765421') = '3hello3';
select '33_17', text_trim('273', '987654321') = '';
select '33_18', text_trim('', '987654321') = '';
select '33_19', text_trim('хххприветхохох', 'ох') = 'привет';
-- chars is a set, so repeated characters don't matter
select '33_20', text_trim('xxaxx', 'xx') = 'a';
select '33_21', text_trim(null, 'x') is null;
select '33_22', text_trim('hello', null) is null;
select '33_23', btrim('xxhelloxx', 'x') = 'hello';

-- text_lpad, lpad
select '34_01', text_lpad('hello', 6) = ' hello';
select '34_02', text_lpad('hello', 8) = '   hello';
select '34_03', text_lpad('мир', 5) = '  мир';
select '34_04', text_lpad('', 5) = '     ';
-- a longer string is truncated on the right, as in Postgres
select '34_05', text_lpad('hello', 5) = 'hello';
select '34_06', text_lpad('hello', 3) = 'hel';
select '34_07', text_lpad('hello', 1) = 'h';
select '34_08', text_lpad('hello', 0) = '';
select '34_09', text_lpad('hello', -1) = '';
select '34_10', text_lpad(12, 4) = '  12';
select '34_11', text_lpad(null, 5) is null;
-- with fill
select '34_21', text_lpad('hello', 8, '*') = '***hello';
select '34_22', text_lpad('hello', 8, 'xo') = 'xoxhello';
select '34_23', text_lpad('мир', 6, 'хо') = 'хохмир';
select '34_24', text_lpad('', 5, '*') = '*****';
select '34_25', text_lpad('hello', 3, '*') = 'hel';
-- an empty fill can't pad
select '34_26', text_lpad('ab', 5, '') = 'ab';
select '34_27', text_lpad('a', 2147483647, '') = 'a';
select '34_28', text_lpad(12, 4, 0) = '0012';
select '34_29', text_lpad(null, 5, '*') is null;
select '34_30', text_lpad('hello', 5, null) is null;
select '34_31', lpad('hello', 8, '*') = '***hello';
-- now: the length is truncated to 32 bits, 4294967300 becomes 4
select '34_32', text_lpad('a', 4294967300) = '   a';

-- text_rpad, rpad
select '35_01', text_rpad('hello', 6) = 'hello ';
select '35_02', text_rpad('hello', 8) = 'hello   ';
select '35_03', text_rpad('мир', 5) = 'мир  ';
select '35_04', text_rpad('', 5) = '     ';
select '35_05', text_rpad('hello', 5) = 'hello';
select '35_06', text_rpad('hello', 3) = 'hel';
select '35_07', text_rpad('hello', 1) = 'h';
select '35_08', text_rpad('hello', 0) = '';
select '35_09', text_rpad('hello', -1) = '';
select '35_10', text_rpad(12, 4) = '12  ';
select '35_11', text_rpad(null, 5) is null;
-- with fill
select '35_21', text_rpad('hello', 8, '*') = 'hello***';
select '35_22', text_rpad('hello', 8, 'xo') = 'helloxox';
select '35_23', text_rpad('мир', 6, 'хо') = 'мирхох';
select '35_24', text_rpad('', 5, '*') = '*****';
select '35_25', text_rpad('hello', 3, '*') = 'hel';
select '35_26', text_rpad('ab', 5, '') = 'ab';
select '35_27', text_rpad(12, 4, 0) = '1200';
select '35_28', text_rpad(null, 5, '*') is null;
select '35_29', text_rpad('hello', 5, null) is null;
select '35_30', rpad('hello', 8, '*') = 'hello***';

-- text_upper
select '41_01', text_upper('hello') = 'HELLO';
select '41_02', text_upper('cómo estás') = 'CÓMO ESTÁS';
select '41_03', text_upper('привет') = 'ПРИВЕТ';
select '41_04', text_upper('пРиВеТ') = 'ПРИВЕТ';
select '41_05', text_upper('hello, 42!') = 'HELLO, 42!';
-- the uppercase form may have a different utf8 byte length
select '41_06', text_upper('ß') = 'ẞ';
select '41_07', text_upper('ſ') = 'S';
select '41_08', text_upper('ⱥ') = 'Ⱥ';
select '41_09', text_upper('ßx') = 'ẞX';
select '41_10', octet_length(text_upper('ß')) = 3;
select '41_11', text_upper('') = '';
select '41_12', text_upper(1.5e10) = '15000000000.0';
select '41_13', text_upper(null) is null;
-- now: a blob is converted as text
select '41_14', text_upper(x'61') = 'A';

-- text_lower
select '42_01', text_lower('HELLO') = 'hello';
select '42_02', text_lower('CÓMO ESTÁS') = 'cómo estás';
select '42_03', text_lower('ПРИВЕТ') = 'привет';
select '42_04', text_lower('пРиВеТ') = 'привет';
-- the lowercase form may have a different utf8 byte length
select '42_05', text_lower('İ') = 'i';
select '42_06', text_lower('İstanbul') = 'istanbul';
select '42_07', text_lower('İZMİR') = 'izmir';
select '42_08', text_lower('K') = 'k';        -- U+212A kelvin sign
select '42_09', text_lower('Å') = 'å';        -- U+212B angstrom sign
select '42_10', text_lower('ẞ') = 'ß';
select '42_11', text_lower('Ⱥ') = 'ⱥ';
-- the character after a width-changing one survives intact
select '42_12', text_lower('Ⱥbcd') = 'ⱥbcd';
select '42_13', octet_length(text_lower('İ')) = 1;
select '42_14', text_lower('') = '';
select '42_15', text_lower(null) is null;

-- text_title
select '43_01', text_title('hello world') = 'Hello World';
select '43_02', text_title('cómo estás') = 'Cómo Estás';
select '43_03', text_title('привет мир') = 'Привет Мир';
select '43_04', text_title('пРиВеТ мир') = 'Привет Мир';
-- a word is a run of letters, digits and underscores
select '43_05', text_title('don''t stop-me now_x') = 'Don''T Stop-Me Now_x';
select '43_06', text_title('1st 2nd') = '1st 2nd';
-- width-changing characters, both in initial and non-initial position
select '43_07', text_title('İzmir') = 'İzmir';
select '43_08', text_title('aK') = 'Ak';       -- U+212A kelvin sign
select '43_09', text_title('ﬁx') = 'ﬁx';
select '43_10', text_title('straße') = 'Straße';
-- a width change at the start of a word (2 bytes -> 3) and inside one (3 -> 1)
select '43_11', text_title('ßx') = 'ẞx';
select '43_12', octet_length(text_title('ßx')) = 4;
select '43_13', octet_length(text_title('a' || char(8490))) = 2;
-- now: the titlecase of ǳ is ǲ, not the uppercase Ǳ
select '43_14', text_title('ǳ') = 'Ǳ';
select '43_15', text_title('') = '';
select '43_16', text_title(null) is null;

-- text_casefold
select '44_01', text_casefold('HELLO') = 'hello';
select '44_02', text_casefold('CÓMO ESTÁS') = 'cómo estás';
select '44_03', text_casefold('ПРИВЕТ') = 'привет';
select '44_04', text_casefold('пРиВеТ') = 'привет';
-- the folded form may have a different utf8 byte length
select '44_05', text_casefold('ẞ') = 'ß';
select '44_06', text_casefold('ſ') = 's';
select '44_07', text_casefold('K') = 'k';      -- U+212A kelvin sign
select '44_08', text_casefold('Ⱥ') = 'ⱥ';
-- simple folding: ß stays as is (full folding gives ss)
select '44_09', text_casefold('ß') = 'ß';
select '44_10', text_casefold('') = '';
select '44_11', text_casefold(null) is null;

-- text_nocase collation
select '45_01', (select 1 where 'hello' = 'hello' collate text_nocase) = 1;
select '45_02', (select 1 where 'hell0' = 'hello' collate text_nocase) is null;
select '45_03', (select 1 where 'привет' = 'ПРИВЕТ' collate text_nocase) = 1;
select '45_04', (select 1 where 'cómo estás' = 'CÓMO ESTÁS' collate text_nocase) = 1;
-- casefolding may change the encoded width: ſ folds to s, K (the kelvin
-- sign, char(8490)) folds to k. equal folded strings compare equal
select '45_05', (select 1 where 'ſ' = 's' collate text_nocase) = 1;
select '45_06', (select 1 where char(8490) = 'k' collate text_nocase) = 1;
-- and the order stays total: s < ſx < sxx < sxy
select '45_07', (select 1 where 'ſx' = 'sxy' collate text_nocase) is null;
select '45_08', (select 1 where 'ſx' < 'sxx' collate text_nocase) = 1;
select '45_09', (select 1 where 'sxx' < 'sxy' collate text_nocase) = 1;
select '45_10', (select 1 where 's' < 'ſx' collate text_nocase) = 1;
select '45_11', (select 1 where '' < 'a' collate text_nocase) = 1;
select '45_12', (select 1 where '' = '' collate text_nocase) = 1;
select '45_13', (select group_concat(column1, ',') from (select column1 from (values ('b'), ('A'), ('a'), ('B')) order by column1 collate text_nocase, column1)) = 'A,a,B,b';
-- now: the comparison stops at a zero byte in the second string, so these are equal
select '45_14', (select 1 where cast(x'610062' as text) = cast(x'610063' as text) collate text_nocase) = 1;

-- text_replace
select '51_01', text_replace('hello', 'l', '*') = 'he**o';
select '51_02', text_replace('hello', 'l', 'xo') = 'hexoxoo';
select '51_03', text_replace('hello', 'ell', '*') = 'h*o';
select '51_04', text_replace('hello', 'ello', 'argh') = 'hargh';
select '51_05', text_replace('hello', 'hello', '-') = '-';
select '51_06', text_replace('hello', 'l', '') = 'heo';
select '51_07', text_replace('aaaa', 'aa', 'b') = 'bb';
select '51_08', text_replace('Hello', 'h', 'J') = 'Hello';
select '51_09', text_replace('нетто', 'т', 'три') = 'нетритрио';
-- an empty old string matches nothing
select '51_10', text_replace('hello', '', '*') = 'hello';
select '51_11', text_replace('', 'l', '*') = '';
select '51_12', text_replace(12321, 2, 0) = '10301';
select '51_13', text_replace(null, 'a', '*') is null;
select '51_14', text_replace('abc', null, '*') is null;
select '51_15', text_replace('abc', 'a', null) is null;
-- with count
select '51_21', text_replace('hello', 'l', '*', 1) = 'he*lo';
select '51_22', text_replace('hello', 'l', '*', 2) = 'he**o';
select '51_23', text_replace('hello', 'l', '*', 3) = 'he**o';
select '51_24', text_replace('hello', 'l', 'xo', 2) = 'hexoxoo';
select '51_25', text_replace('hello', 'ell', '*', 1) = 'h*o';
select '51_26', text_replace('hello', 'hello', '-', 1) = '-';
select '51_27', text_replace('hello', 'l', '', 2) = 'heo';
select '51_28', text_replace('нетто', 'т', 'три', 1) = 'нетрито';
select '51_29', text_replace('нетто', 'т', 'три', 2) = 'нетритрио';
select '51_30', text_replace('hello', '', '*', 1) = 'hello';
select '51_31', text_replace('', 'l', '*', 1) = '';
-- a count <= 0 replaces nothing
select '51_32', text_replace('hello', 'l', '*', 0) = 'hello';
select '51_33', text_replace('hello', 'l', '*', -1) = 'hello';
select '51_34', text_replace(null, 'a', '*', 1) is null;
select '51_35', text_replace('abc', null, '*', 1) is null;
select '51_36', text_replace('abc', 'a', null, 1) is null;
-- now: the count is truncated to 32 bits, 4294967297 becomes 1
select '51_37', text_replace('aaa', 'a', 'b', 4294967297) = 'baa';

-- text_translate, translate
select '52_01', text_translate('hello', 'l', '1') = 'he11o';
select '52_02', text_translate('hello', 'ole', '013') = 'h3110';
select '52_03', text_translate('12345', '143', 'ax') = 'a2x5';
select '52_04', text_translate('нетто', 'от', '03') = 'не330';
-- characters in from without a match in to are deleted
select '52_05', text_translate('hello', 'oleh', '01') = '110';
select '52_06', text_translate('hello', 'l', '') = 'heo';
-- extra characters in to are ignored
select '52_07', text_translate('abc', 'a', 'xyz') = 'xbc';
-- the first occurrence in from wins
select '52_08', text_translate('abc', 'aa', 'xy') = 'xbc';
select '52_09', text_translate('hello', '', '1') = 'hello';
select '52_10', text_translate('hello', '', '') = 'hello';
select '52_11', text_translate('', 'l', '1') = '';
select '52_12', text_translate(12321, 2, 0) = '10301';
select '52_13', text_translate(null, 'l', '1') is null;
select '52_14', text_translate('hello', null, '1') is null;
select '52_15', text_translate('hello', 'l', null) is null;
select '52_16', translate('hello', 'l', '1') = 'he11o';

-- text_reverse, reverse
select '53_01', text_reverse('hello') = 'olleh';
select '53_02', text_reverse('***') = '***';
select '53_03', text_reverse('hello 42@ world') = 'dlrow @24 olleh';
select '53_04', text_reverse('привет') = 'тевирп';
select '53_05', text_reverse('𐌀𐌁𐌂') = '𐌂𐌁𐌀';
-- code points are reversed, so a combining mark moves to the other side of its letter
select '53_06', hex(text_reverse('e' || char(769))) = 'CC8165';
select '53_07', text_reverse('') = '';
select '53_08', text_reverse(123) = '321';
select '53_09', text_reverse(null) is null;
select '53_10', reverse('hello') = 'olleh';

-- text_length, char_length, character_length
select '61_01', text_length('h') = 1;
select '61_02', text_length('hello') = 5;
select '61_03', text_length('привет') = 6;
select '61_04', text_length('Hello, 世界!') = 10;
select '61_05', text_length('𐌀𐌁𐌂') = 3;
select '61_06', text_length('') = 0;
select '61_07', text_length(12345) = 5;
select '61_08', text_length(1.5) = 3;
select '61_09', text_length(null) is null;
select '61_10', char_length('привет') = 6;
select '61_11', character_length('привет') = 6;
-- now: a blob is counted as text
select '61_12', text_length(x'414243') = 3;

-- text_size, octet_length
select '62_01', text_size('h') = 1;
select '62_02', text_size('hello') = 5;
select '62_03', text_size('привет') = 12;
select '62_04', text_size('𐌀𐌁𐌂') = 12;
select '62_05', text_size('') = 0;
select '62_06', text_size(12345) = 5;
select '62_07', text_size(x'414243') = 3;
select '62_08', text_size(null) is null;
select '62_09', octet_length('привет') = 12;

-- text_bitsize, bit_length
select '63_01', text_bitsize('h') = 1*8;
select '63_02', text_bitsize('hello') = 5*8;
select '63_03', text_bitsize('привет') = 2*6*8;
select '63_04', text_bitsize('') = 0;
select '63_05', text_bitsize(12345) = 5*8;
select '63_06', text_bitsize(x'4142') = 2*8;
select '63_07', text_bitsize(null) is null;
select '63_08', bit_length('one') = 24;

-- Invalid UTF-8
-- Character functions decode each maximal ill-formed subpart as U+FFFD = char(65533)
-- (x'80' is a stray continuation byte, x'c3' a truncated 2-byte sequence)
select '71_01', text_length(cast(x'80' as text)) = 1;
select '71_02', text_length(cast(x'c3' as text)) = 1;
select '71_03', text_length(cast(x'e4b8' as text)) = 1;
select '71_04', text_length(cast(x'41ff42' as text)) = 3;
select '71_05', text_length(cast(x'c341' as text)) = 2;
select '71_06', text_index(cast(x'80' as text), 'a') = 0;
select '71_07', text_index(cast(x'41ff42' as text), 'B') = 3;
select '71_08', text_substring(cast(x'c3' as text), 1, 1) = char(65533);
select '71_09', hex(text_reverse(cast(x'41ff42' as text))) = '42EFBFBD41';
select '71_10', text_trim(cast(x'e4b8' as text)) = char(65533);
select '71_11', text_left(cast(x'ff' as text), 1) = char(65533);
select '71_12', text_like('a_b', cast(x'41ff42' as text)) = 1;
select '71_13', hex(text_translate(cast(x'41ff42' as text), 'A', 'Z')) = '5AEFBFBD42';
select '71_14', hex(text_lpad(cast(x'ff' as text), 2, '*')) = '2AEFBFBD';
-- byte functions pass invalid bytes through unchanged
select '71_21', hex(text_replace(cast(x'41ff42' as text), 'A', 'Z')) = '5AFF42';
select '71_22', text_contains(cast(x'41ff42' as text), cast(x'ff' as text)) = 1;
select '71_23', text_count(cast(x'41ff42ff' as text), cast(x'ff' as text)) = 2;
select '71_24', hex(text_split(cast(x'41ff42' as text), 'B', 1)) = '41FF';
select '71_25', hex(text_concat(cast(x'41ff' as text), 'B')) = '41FF42';
select '71_26', text_size(cast(x'41ff42' as text)) = 3;
-- now: an invalid byte in the search string makes text_contains and text_index disagree
select '71_27', text_index(cast(x'41ff42' as text), cast(x'fe' as text)) = 2;
select '71_28', text_contains(cast(x'41ff42' as text), cast(x'fe' as text)) = 0;
-- case functions return the input unchanged
select '71_31', hex(text_upper(cast(x'61ff62' as text))) = '61FF62';
select '71_32', hex(text_lower(cast(x'41ff42' as text))) = '41FF42';
select '71_33', hex(text_title(cast(x'61ff62' as text))) = '61FF62';
select '71_34', hex(text_casefold(cast(x'80ff' as text))) = '80FF';
-- the collation decodes like the character functions
select '71_41', (select 1 where cast(x'80' as text) = cast(x'80' as text) collate text_nocase) = 1;
select '71_42', (select 1 where cast(x'80' as text) = 'a' collate text_nocase) is null;
select '71_43', (select 1 where cast(x'41ff42' as text) = cast(x'61ff62' as text) collate text_nocase) = 1;
-- both decode to a single U+FFFD, so they compare equal
select '71_44', (select 1 where cast(x'e4b8' as text) = cast(x'ff' as text) collate text_nocase) = 1;

-- Embedded zero bytes
select '72_01', text_size(cast(x'410042' as text)) = 3;
select '72_02', text_contains(cast(x'41004243' as text), 'B') = 1;
select '72_03', text_count(cast(x'41004241' as text), 'A') = 2;
select '72_04', text_has_suffix(cast(x'410042' as text), 'B') = 1;
-- case functions keep them
select '72_05', hex(text_lower(cast(x'41004243' as text))) = '61006263';
select '72_06', octet_length(text_lower(cast(x'41004243' as text))) = 4;
-- now: everything else stops at the first zero byte
select '72_11', text_length(cast(x'410042' as text)) = 1;
select '72_12', hex(text_substring(cast(x'41004243' as text), 1)) = '41';
select '72_13', text_index(cast(x'41004243' as text), 'B') = 0;
select '72_14', hex(text_reverse(cast(x'410042' as text))) = '41';
select '72_15', hex(text_ltrim(cast(x'20410042' as text))) = '41';
select '72_16', hex(text_translate(cast(x'410042' as text), 'B', 'C')) = '41';
select '72_17', text_like(cast(x'00' as text), '') = 1;
select '72_18', text_split(cast(x'41007C42' as text), '|', 2) = '';
select '72_19', hex(text_join('|', cast(x'4100' as text), 'B')) = '41';
select '72_20', hex(text_concat(cast(x'4100' as text), 'B')) = '41';
select '72_21', hex(text_repeat(cast(x'4100' as text), 2)) = '41';
select '72_22', hex(text_replace(cast(x'41004241' as text), 'A', 'Z')) = '5A';

-- SQLite built-ins
-- now: concat, concat_ws, ltrim, rtrim and octet_length replace the SQLite built-ins
-- of the same name, which keep zero bytes and don't decode invalid utf8
select '81_01', hex(concat(cast(x'4100' as text), 'B')) = '41';
select '81_02', hex(concat_ws('|', cast(x'4100' as text), 'B')) = '41';
select '81_03', hex(rtrim(cast(x'41ff' as text), 'x')) = '41EFBFBD';
select '81_04', hex(ltrim(cast(x'ff41' as text), 'x')) = 'EFBFBD41';
