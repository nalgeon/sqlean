-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/regexp

-- Patterns use PCRE2 syntax, compiled with UTF and UCP: they work on characters,
-- and \d, \w, \b and case folding follow Unicode.
-- NULL in any argument gives NULL, and so does a blob or invalid UTF-8 text.
-- An invalid pattern, replacement or group number, and a match that hits
-- the backtracking limit, raise an error: the C tests cover them.

-- regexp: X regexp Y calls regexp(Y, X)
select '01_01', 'the year is 2021' regexp '[0-9]+';
select '01_02', ('the year is 2021' regexp '2k21') = 0;
select '01_03', regexp('[0-9]+', 'the year is 2021');
select '01_04', regexp('2k21', 'the year is 2021') = 0;
-- the operator uses PCRE2, not the shell's built-in regexp (no lookahead there)
select '01_05', 'year' regexp 'y(?=ear)';
select '01_06', typeof('abc' regexp 'b') = 'integer';
select '01_07', (null regexp 'a') is null;
select '01_08', ('a' regexp null) is null;
select '01_09', regexp('a', null) is null;
select '01_10', regexp(null, 'a') is null;
select '01_11', (x'61' regexp 'a') is null;
select '01_12', ('a' regexp x'61') is null;

-- regexp_like
select '02_01', regexp_like('the year is 2021', '[0-9]+');
select '02_02', regexp_like('the year is 2021', '2k21') = 0;
select '02_03', typeof(regexp_like('abc', 'b')) = 'integer';
select '02_04', regexp_like('abc', 'b') = 1;
-- a partial match counts, anchors make it full
select '02_05', regexp_like('abc', 'b');
select '02_06', regexp_like('abc', '^b$') = 0;
select '02_07', regexp_like('abc', '^abc$');
-- an empty pattern matches anything, including an empty source
select '02_08', regexp_like('abc', '');
select '02_09', regexp_like('', '');
select '02_10', regexp_like('', 'a') = 0;
select '02_11', regexp_like(null, 'a') is null;
select '02_12', regexp_like('a', null) is null;
-- numbers are matched as text, in both arguments
select '02_13', regexp_like(123, '^\d+$');
select '02_14', regexp_like(1.5, '^1\.5$');
select '02_15', regexp_like('a123', 12);
-- a blob is NULL
select '02_16', regexp_like(x'616263', 'b') is null;
select '02_17', regexp_like('abc', x'62') is null;
-- invalid UTF-8 text is NULL
select '02_18', regexp_like(cast(x'ff' as text), 'a') is null;
select '02_19', regexp_like(cast(x'61ff62' as text), 'a') is null;
-- NUL is a character like any other, in the source and in the pattern
select '02_20', regexp_like(cast(x'610062' as text), 'b');
select '02_21', regexp_like(cast(x'610062' as text), '^a\x00b$');
select '02_22', regexp_like('a', cast(x'61007a' as text)) = 0;
select '02_23', regexp_like(cast(x'61007a' as text), cast(x'61007a' as text));
-- a long subject doesn't exhaust the stack
select '02_24', regexp_like(printf('%.*c', 100000, 'a'), '(a|b)*$');

-- regexp_substr
select '03_01', regexp_substr('abcdef', 'b.d') = 'bcd';
select '03_02', regexp_substr('the year is 2021', '[0-9]+') = '2021';
-- groups don't matter, the whole match is returned
select '03_03', regexp_substr('abcdef', 'b(.)d') = 'bcd';
-- no match is NULL
select '03_04', regexp_substr('abcdef', 'z') is null;
select '03_05', regexp_substr('the year is 2021', '2k21') is null;
-- the first match only
select '03_06', regexp_substr('a1b2', '\d') = '1';
-- the leftmost match wins, even if it's empty
select '03_07', regexp_substr('abc', 'x*|b') = '';
-- an empty match is an empty string, not NULL
select '03_08', regexp_substr('abc', 'x*') = '';
select '03_09', regexp_substr('', '') = '';
select '03_10', regexp_substr('abc', '$') = '';
select '03_11', regexp_substr(null, 'a') is null;
select '03_12', regexp_substr('a', null) is null;
-- numbers are matched as text, the result is text
select '03_13', regexp_substr(12345, '3.') = '34';
select '03_14', typeof(regexp_substr(12345, '3.')) = 'text';
select '03_15', regexp_substr(x'616263', 'b') is null;
select '03_16', regexp_substr(cast(x'61ff62' as text), 'b') is null;
-- NUL is a character, and the result keeps it
select '03_17', regexp_substr(cast(x'610062' as text), 'b') = 'b';
select '03_18', regexp_substr(cast(x'610062' as text), 'a.*') = cast(x'610062' as text);
select '03_19', length(cast(regexp_substr(cast(x'61006200' as text), '.+') as blob)) = 4;
-- a long match
select '03_20', length(regexp_substr(printf('%.*c', 100000, 'a'), 'a+')) = 100000;
-- a match in the middle of multibyte text
select '03_21', regexp_substr('привет мир', 'м.р') = 'мир';

-- regexp_capture
select '04_01', regexp_capture('abcdef', 'b(.)d', 1) = 'c';
select '04_02', regexp_capture('years is 2021', '\d\d(\d\d)', 0) = '2021';
select '04_03', regexp_capture('years is 2021', '\d\d(\d\d)', 1) = '21';
-- group 0 is the whole match, and is the default
select '04_04', regexp_capture('abcdef', 'b(.)d') = 'bcd';
select '04_05', regexp_capture('abcdef', 'b(.)d', 0) = 'bcd';
select '04_06', regexp_capture('abcdef', 'b.d', 0) = 'bcd';
select '04_07', regexp_capture('abcdef', 'b.d') = 'bcd';
-- groups are numbered by their opening parenthesis
select '04_08', regexp_capture('abcd', '((a)(b))(c)', 1) = 'ab';
select '04_09', regexp_capture('abcd', '((a)(b))(c)', 2) = 'a';
select '04_10', regexp_capture('abcd', '((a)(b))(c)', 3) = 'b';
select '04_11', regexp_capture('abcd', '((a)(b))(c)', 4) = 'c';
-- non-capturing groups are not counted, named groups are
select '04_12', regexp_capture('abc', '(?:a)(b)', 1) = 'b';
select '04_13', regexp_capture('abc', '(?<x>b)(c)', 1) = 'b';
select '04_14', regexp_capture('abc', '(?<x>b)(c)', 2) = 'c';
-- the first match only
select '04_15', regexp_capture('a1b2', '[a-z](\d)', 1) = '1';
-- no match is NULL
select '04_16', regexp_capture('abcdef', 'z') is null;
select '04_17', regexp_capture('abcdef', 'z', 0) is null;
select '04_18', regexp_capture('abcdef', '(z)', 1) is null;
-- a group that didn't participate in the match is NULL, wherever it is
select '04_19', regexp_capture('ac', 'a(b)?c', 1) is null;
select '04_20', regexp_capture('ac', 'a(b)?(c)', 1) is null;
select '04_21', regexp_capture('ac', 'a(b)?(c)', 2) = 'c';
select '04_22', regexp_capture('b', '(a)|(b)', 1) is null;
select '04_23', regexp_capture('b', '(a)|(b)', 2) = 'b';
-- an empty group is an empty string
select '04_24', regexp_capture('ac', 'a(b*)c', 1) = '';
select '04_25', regexp_capture(null, 'a') is null;
select '04_26', regexp_capture(null, '(a)', 1) is null;
select '04_27', regexp_capture('a', null, 1) is null;
select '04_28', regexp_capture('a', '(a)', null) is null;
-- numbers are matched as text, and the group number can be numeric text
select '04_29', regexp_capture(12345, '3(.)', 1) = '4';
select '04_30', regexp_capture('abc', 'a(b)', '1') = 'b';
select '04_31', regexp_capture(x'616263', 'a(b)', 1) is null;
select '04_32', regexp_capture('abc', 'a(b)', x'01') is null;
select '04_33', regexp_capture(cast(x'61ff62' as text), 'a', 0) is null;
-- NUL is a character
select '04_34', regexp_capture(cast(x'610062' as text), 'a(.*)', 1) = cast(x'0062' as text);
select '04_35', regexp_capture('при вет', '(\w+)$', 1) = 'вет';

-- regexp_replace
select '05_01', regexp_replace('the year is 2021', '[0-9]+', '2050') = 'the year is 2050';
select '05_02', regexp_replace('the year is 2021', '2k21', '2050') = 'the year is 2021';
-- every match is replaced
select '05_03', regexp_replace('1 10 100', '\d+', '**') = '** ** **';
select '05_04', regexp_replace('aaa', 'a', 'b') = 'bbb';
select '05_05', regexp_replace('10 10 10', '10$', '') = '10 10 ';
select '05_06', regexp_replace('10 10 10', '^10', '') = ' 10 10';
-- a match at the start, in the middle and at the end, replaced with 0, 1 and 2 characters
select '05_07', regexp_replace('hello', 'h', '') = 'ello';
select '05_08', regexp_replace('hello', 'h', '.') = '.ello';
select '05_09', regexp_replace('hello', 'h', '..') = '..ello';
select '05_10', regexp_replace('hello', 'e', '') = 'hllo';
select '05_11', regexp_replace('hello', 'e', '.') = 'h.llo';
select '05_12', regexp_replace('hello', 'e', '..') = 'h..llo';
select '05_13', regexp_replace('hello', 'o', '') = 'hell';
select '05_14', regexp_replace('hello', 'o', '.') = 'hell.';
select '05_15', regexp_replace('hello', 'o', '..') = 'hell..';
-- an empty pattern matches between every character
select '05_16', regexp_replace('abc', '', '-') = '-a-b-c-';
select '05_17', regexp_replace('', '', 'x') = 'x';
select '05_18', regexp_replace('', 'a', 'x') = '';
select '05_19', regexp_replace('при', '', '-') = '-п-р-и-';

-- regexp_replace: the replacement string
-- $n, ${n} and ${name} insert a group, $0 the whole match, $$ a dollar sign
select '06_01', regexp_replace('the year is 2021', '([0-9]+)', '-$1-') = 'the year is -2021-';
select '06_02', regexp_replace('the year is 2021', '([0-9]+)', '$1 or 2050') = 'the year is 2021 or 2050';
select '06_03', regexp_replace('abc', 'b', '[$0]') = 'a[b]c';
select '06_04', regexp_replace('abc', '(b)', '[${1}]') = 'a[b]c';
select '06_05', regexp_replace('abc', '(?<x>b)', '[${x}]') = 'a[b]c';
select '06_06', regexp_replace('abc', 'b', '$$') = 'a$c';
select '06_07', regexp_replace('ab', '(a)(b)', '$2$1') = 'ba';
-- group numbers are not limited to one digit
select '06_08', regexp_replace('abcdefghijk', '(a)(b)(c)(d)(e)(f)(g)(h)(i)(j)', '$10') = 'jk';
select '06_09', regexp_replace('abcdefghijk', '(a)(b)(c)(d)(e)(f)(g)(h)(i)(j)', '${1}0') = 'a0k';
-- extended syntax: escapes and case conversion
select '06_10', regexp_replace('abc', '(b)', '\U$1') = 'aBc';
select '06_11', regexp_replace('ABC', '(B)', '\L$1') = 'AbC';
select '06_12', regexp_replace('abc', 'b', '\$') = 'a$c';
select '06_13', regexp_replace('abc', 'b', '\t') = 'a' || char(9) || 'c';
select '06_14', regexp_replace('abc abc', '(a)(\w+)', '\u$1\E$2') = 'Abc Abc';
-- a group that didn't participate in the match inserts an empty string
select '06_15', regexp_replace('ac', 'a(b)?c', '[$1]') = '[]';
select '06_16', regexp_replace('ac', 'a(b?)c', '[$1]') = '[]';
select '06_17', regexp_replace('ab', '(a)|(b)', '[$1$2]') = '[a][b]';
-- NUL in the replacement is kept
select '06_18', regexp_replace('abc', 'b', cast(x'00' as text)) = cast(x'610063' as text);

-- regexp_replace: types and invalid input
select '07_01', regexp_replace(null, 'a', 'b') is null;
select '07_02', regexp_replace('abc', null, 'x') is null;
select '07_03', regexp_replace('abc', 'b', null) is null;
-- numbers are matched as text, the result is always text
select '07_04', regexp_replace(123, '2', 'x') = '1x3';
select '07_05', regexp_replace('abc', 'b', 5) = 'a5c';
select '07_06', regexp_replace(123, 'z', 'x') = '123';
select '07_07', typeof(regexp_replace(123, 'z', 'x')) = 'text';
select '07_08', typeof(regexp_replace(123, '2', 'x')) = 'text';
select '07_09', typeof(regexp_replace(1.5, 'z', 'x')) = 'text';
select '07_10', regexp_replace(x'616263', 'b', 'x') is null;
select '07_11', regexp_replace('abc', x'62', 'x') is null;
select '07_12', regexp_replace('abc', 'b', x'78') is null;
-- invalid UTF-8 in the source or the replacement is NULL
select '07_13', regexp_replace(cast(x'61ff62' as text), 'b', 'x') is null;
select '07_14', regexp_replace('abc', 'b', cast(x'ff' as text)) is null;
-- NUL is a character: what follows it is kept
select '07_15', regexp_replace(cast(x'610062' as text), 'a', 'x') = cast(x'780062' as text);
select '07_16', regexp_replace(cast(x'610062' as text), '\x00', '-') = 'a-b';

-- regexp_replace: result size
select '08_01', regexp_replace('a', 'a', printf('%.*c', 1024, 'b')) = printf('%.*c', 1024, 'b');
select '08_02', length(regexp_replace(printf('%.*c', 1000, 'a'), 'a', 'bb')) = 2000;
select '08_03', length(regexp_replace(printf('%.*c', 100000, 'a'), 'a', 'b')) = 100000;
-- a result much longer than the source
select '08_04', regexp_replace('a', 'a', printf('%.*c', 1025, 'b')) = printf('%.*c', 1025, 'b');
select '08_05', length(regexp_replace(printf('%.*c', 2000, 'a'), 'a', 'bb')) = 4000;
select '08_06', length(regexp_replace(printf('%.*c', 100000, 'a'), 'a', 'bcd')) = 300000;
select '08_07', length(regexp_replace(printf('%.*c', 100000, 'a'), '', '-')) = 200001;
-- a result much shorter than the source
select '08_08', regexp_replace(printf('%.*c', 100000, 'a'), 'a+', 'b') = 'b';
-- the result just over the initial guess (source length + 1/4 + 16) and around it
select '08_09', length(regexp_replace(printf('%.*c', 64, 'a'), 'a{4}', 'aaaaa')) = 80;
select '08_10', length(regexp_replace(printf('%.*c', 64, 'a'), '^', printf('%.*c', 31, 'b'))) = 95;
select '08_11', length(regexp_replace(printf('%.*c', 64, 'a'), '^', printf('%.*c', 32, 'b'))) = 96;
select '08_12', length(regexp_replace(printf('%.*c', 64, 'a'), '^', printf('%.*c', 33, 'b'))) = 97;

-- syntax
select '09_01', regexp_substr('the year is 2021', '[year]') = 'e';
select '09_02', regexp_substr('the year is 2021', '[^year]') = 't';
select '09_03', regexp_substr('the year is 2021', '(20|21)') = '20';
select '09_04', regexp_substr('the year is 2021', '\d+') = '2021';
select '09_05', regexp_substr('the year is 2021', '\d{1,2}') = '20';
select '09_06', regexp_substr('the year is 2020', '(20)\1') = '2020';
select '09_07', regexp_substr('the year is 2021', '\sis\s') = ' is ';
select '09_08', regexp_substr('the year is 2021', '\bis\b') = 'is';
select '09_09', regexp_substr('the year is 2021', '^the') = 'the';
select '09_10', regexp_substr('the year is 2021', '2021$') = '2021';
select '09_11', regexp_substr('the year is 2021', 'y(?=ear)') = 'y';
select '09_12', regexp_substr('the YEAR is 2021', '(?i)year') = 'YEAR';
select '09_13', regexp_substr('the YEAR is 2021', 'year') is null;
-- lazy quantifiers
select '09_14', regexp_substr('<a><b>', '<.+>') = '<a><b>';
select '09_15', regexp_substr('<a><b>', '<.+?>') = '<a>';
-- negative lookahead, lookbehind, atomic groups
select '09_16', regexp_substr('foobar foobaz', 'foo(?!bar)\w+') = 'foobaz';
select '09_17', regexp_substr('price $42', '(?<=\$)\d+') = '42';
select '09_18', regexp_like('aaab', '(?>a+)ab') = 0;
-- . doesn't match a newline, unless (?s)
select '09_19', regexp_like(char(10), '.') = 0;
select '09_20', regexp_like(char(10), '(?s).');
-- ^ and $ match at the ends of the string, unless (?m);
-- $ also matches before a trailing newline
select '09_21', regexp_like('a' || char(10) || 'b', '^b') = 0;
select '09_22', regexp_like('a' || char(10) || 'b', '(?m)^b');
select '09_23', regexp_substr('ab' || char(10), 'b$') = 'b';
-- (?x) ignores whitespace and comments in the pattern
select '09_24', regexp_substr('abc', '(?x) b  c # comment') = 'bc';

-- unicode
-- . matches a character, not a byte
select '10_01', regexp_substr('привет', '^.') = 'п';
select '10_02', regexp_substr('привет', '^.{3}') = 'при';
select '10_03', regexp_capture('日本語', '(.)$', 1) = '語';
-- \w, \b and \d follow Unicode
select '10_04', regexp_substr('-- привет --', '\b\w+\b') = 'привет';
select '10_05', regexp_replace('héllo wörld', '\b', '|') = '|héllo| |wörld|';
select '10_06', regexp_like('٣', '^\d$');
-- case-insensitive matching folds non-ASCII letters
select '10_07', regexp_like('ПРИВЕТ', '(?i)привет');
select '10_08', regexp_replace('привет', '(и)', '\U$1') = 'прИвет';
-- an emoji is one character
select '10_09', regexp_substr('a😀b', 'a.b') = 'a😀b';

-- per-row patterns
create table t(id integer primary key, s text, p text);
insert into t(s, p) values ('abc', 'a'), ('abc', 'z'), ('xyz', 'z'), ('xyz', '^y'), ('abc', null);
select '11_01', (select group_concat(coalesce(regexp_like(s, p), '-'), ',') from t order by id) = '1,0,1,0,-';
select '11_02', (select group_concat(coalesce(regexp_substr(s, p), '-'), ',') from t order by id) = 'a,-,z,-,-';
select '11_03', (select group_concat(coalesce(regexp_replace(s, p, '_'), '-'), ',') from t order by id) = '_bc,abc,xy_,xyz,-';
select '11_04', (select group_concat(id, ',') from t where s regexp p order by id) = '1,3';
-- a constant pattern over many rows, matching and not
select '11_05', (select count(*) from generate_series(1, 1000) where value regexp '^\d*7$') = 100;
select '11_06', (select count(*) from generate_series(1, 1000) where regexp_substr(value, '7$') is not null) = 100;
select '11_07', (select sum(length(regexp_replace(value, '7', '77'))) from generate_series(1, 1000))
              = (select sum(length(value)) from generate_series(1, 1000)) + 300;
-- the same pattern in several calls of one statement
select '11_08', (select count(*) from generate_series(1, 100)
                 where regexp_like(value, '^1') and regexp_substr(value, '^1') = '1') = 12;
drop table t;
