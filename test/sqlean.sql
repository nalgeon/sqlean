-- Copyright (c) 2023 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/sqlean

-- crypto
select '1_01', hex(sha1('abc')) = upper('a9993e364706816aba3e25717850c26c9cd0d89d');
select '1_02', encode('abcd', 'base64') = 'YWJjZA==';
select '1_03', decode('YWJjZA==', 'base64') = cast('abcd' as blob);

-- define
select define('sumn', '?1 * (?1 + 1) / 2');
select '2_01', sumn(5) = 1 + 2 + 3 + 4 + 5;
select '2_02', eval('select abs(-42)') = '42';
select define_free();

-- fileio
select '3_01', count(*) >= 10 from fileio_ls('test');
select '3_02', fileio_mode(420) = '?rw-r--r--';

-- fuzzy
select '4_01', dlevenshtein('abc', 'abcd') = 1;
select '4_02', caverphone('awesome') = 'AWSM111111';

-- ipaddr
select '12_01', iphost('192.168.16.12/24') = '192.168.16.12';
select '12_02', ipcontains('192.168.16.0/24', '192.168.16.3') = 1;

-- math
select '5_01', trunc(3.9) = 3;
select '5_02', sqrt(100) = 10;
select '5_03', round(degrees(pi())) = 180;

-- regexp
select '6_01', regexp_replace('1 10 100', '\d+', '**') = '** ** **';
select '6_02', regexp_substr('abcdef', 'b(.)d') = 'bcd';
select '6_03', regexp_capture('abcdef', 'b(.)d', 1) = 'c';

-- stats
select '7_01', percentile(value, 50) = 50 from generate_series(1, 99);
select '7_02', median(value) = 50 from generate_series(1, 99);

-- text
select '8_01', text_substring('hello world', 7) = 'world';
select '8_02', text_split('one|two|three', '|', 2) = 'two';
select '8_03', text_translate('hello', 'l', '1')  = 'he11o';

-- unicode
select '9_01', nupper('пРиВеТ') = 'ПРИВЕТ';
select '9_02', unaccent('hôtel') = 'hotel';

-- uuid
select '10_01', uuid4() like '________-____-4___-____-____________';

-- vsv
.shell echo '11,Diane,London' > people.csv
.shell echo '22,Grace,Berlin' >> people.csv
.shell echo '33,Alice,Paris' >> people.csv
create virtual table people using vsv(
    filename=people.csv,
    schema="create table people(id integer, name text, city text)",
    columns=3,
    affinity=integer
);
select '11_01', count(*) = 3 from people;
.shell rm -f people.csv
