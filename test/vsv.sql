-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/vsv

-- Errors (bad parameters, schemas and records) stop the suite,
-- so they are tested in test/vsv/extension.test.c.
--
-- Every file is created in vsv.tmp, which is removed at the end.
--
-- Fixtures:
--   people.csv     '11,Diane,London\n22,Grace,Berlin\n33,Alice,Paris\n'
--   header.csv     'id,name\n1,one\n2,two\n'
--   crlf.csv       'a,b\r\nc,d\r\n'
--   quoted.csv     '"a","b"\r\n"c","d"\r\n'
--   bom.csv        BOM + 'h1,h2\nx,y\n'
--   bomonly.csv    BOM only
--   bytes.csv      '\xffa,b\nok,\xc3\xa9,\xff,\xc3,a\x00b\n'
--   cr.csv         'a\rb,c\n'
--   crquoted.csv   '"a\r\n\r\nb",c\n'
--   us.csv         'a\x1fb\n'
--   boundary.csv   4095 x 'a' + ',\xffb,c\nd,e,f\n' (0xff is the 4097th byte,
--                  the first one of the second buffer)
--
-- The .shell lines double the backslashes: the shell passes an argument
-- without spaces to sh unquoted, and sh removes one level of escaping.
-- Bytes are written in octal, since sh's printf doesn't support \x,
-- and so are double quotes (\042), which the shell removes.

.shell rm -rf vsv.tmp
.shell mkdir -p vsv.tmp
.shell printf '11,Diane,London\\n22,Grace,Berlin\\n33,Alice,Paris\\n' > vsv.tmp/people.csv
.shell printf 'id,name\\n1,one\\n2,two\\n' > vsv.tmp/header.csv
.shell printf 'a,b\\r\\nc,d\\r\\n' > vsv.tmp/crlf.csv
.shell printf '\\042a\\042,\\042b\\042\\r\\n\\042c\\042,\\042d\\042\\r\\n' > vsv.tmp/quoted.csv
.shell printf '\\357\\273\\277h1,h2\\nx,y\\n' > vsv.tmp/bom.csv
.shell printf '\\357\\273\\277' > vsv.tmp/bomonly.csv
.shell printf '\\377a,b\\nok,\\303\\251,\\377,\\303,a\\000b\\n' > vsv.tmp/bytes.csv
.shell printf 'a\\rb,c\\n' > vsv.tmp/cr.csv
.shell printf '\\042a\\r\\n\\r\\nb\\042,c\\n' > vsv.tmp/crquoted.csv
.shell printf 'a\\037b\\n' > vsv.tmp/us.csv
.shell printf '%04095d' 0 | tr 0 a > vsv.tmp/boundary.csv
.shell printf ',\\377b,c\\nd,e,f\\n' >> vsv.tmp/boundary.csv

-- input: filename
create virtual table temp.t using vsv(filename=vsv.tmp/people.csv);
select '01_01', count(*) = 3 from t;
select '01_02', (select group_concat(c0 || ':' || c1 || ':' || c2, ' ') from t) = '11:Diane:London 22:Grace:Berlin 33:Alice:Paris';
-- rows are numbered from 1
select '01_03', (select group_concat(rowid, ',') from t) = '1,2,3';
-- each scan reads the file from the start
select '01_04', (select count(*) from t a, t b) = 9;
select '01_05', (select count(*) from t where c1 = 'Grace') = 1;
drop table t;

-- the example from the docs
create virtual table temp.people using vsv(
    filename=vsv.tmp/people.csv,
    schema="create table people(id integer, name text, city text)"
);
select '01_06', count(*) = 3 from people;
select '01_07', (id, name, city) = (22, 'Grace', 'Berlin') from people where id = 22;
select '01_08', typeof(id) = 'integer' from people where id = 22;
drop table people;

-- input: data
create virtual table temp.t using vsv(data='a,b
c,d');
select '01_09', (select group_concat(c0 || c1, ' ') from t) = 'ab cd';
select '01_10', (select group_concat(rowid, ',') from t) = '1,2';
select '01_11', (select count(*) from t a, t b) = 4;
drop table t;

-- empty input has no rows and one column, c0
create virtual table temp.t using vsv(data='');
select '01_12', (select count(*) from t) = 0;
select '01_13', (select group_concat(name) from pragma_table_info('t')) = 'c0';
drop table t;
create virtual table temp.t using vsv(data='', header);
select '01_14', (select count(*) from t) = 0;
select '01_15', (select group_concat(name) from pragma_table_info('t')) = 'c0';
drop table t;
create virtual table temp.t using vsv(data='', schema='create table x(a, b)');
select '01_16', (select count(*) from t) = 0;
drop table t;

-- vsv tables are read-only and keep nothing: copy the data to store it
create virtual table temp.t using vsv(
    filename=vsv.tmp/people.csv,
    schema='create table x(id integer, name text, city text)'
);
create table stored as select * from t;
select '01_17', (select count(*) from stored) = 3;
select '01_18', (select sum(id) from stored) = 66;
select '01_19', (select typeof(id) from stored where name = 'Diane') = 'integer';
drop table stored;
drop table t;

-- columns: without a schema or a header, the columns are named c0, c1, ...
-- and counted from the first record
create virtual table temp.t using vsv(data='a,b,c
d,e,f');
select '02_01', (select group_concat(name) from pragma_table_info('t')) = 'c0,c1,c2';
drop table t;
-- later records may have more fields, which are dropped,
-- or fewer, and the missing ones are NULL
create virtual table temp.t using vsv(data='a,b
d,e,f
g');
select '02_02', (select group_concat(name) from pragma_table_info('t')) = 'c0,c1';
select '02_03', (select group_concat(c0 || ':' || ifnull(c1, 'null'), ' ') from t) = 'a:b d:e g:null';
drop table t;
-- columns=N is accepted for compatibility and ignored
create virtual table temp.t using vsv(data='a,b,c', columns=2);
select '02_04', (select group_concat(name) from pragma_table_info('t')) = 'c0,c1,c2';
drop table t;
create virtual table temp.t using vsv(data='a,b,c', columns = 4);
select '02_05', (select group_concat(name) from pragma_table_info('t')) = 'c0,c1,c2';
drop table t;

-- header: the names come from the first record, which is not a row
create virtual table temp.t using vsv(filename=vsv.tmp/header.csv, header);
select '02_06', (select group_concat(name) from pragma_table_info('t')) = 'id,name';
select '02_07', (select group_concat(id || name, ' ') from t) = '1one 2two';
select '02_08', (select group_concat(rowid, ',') from t) = '1,2';
select '02_09', (select count(*) from t a, t b) = 4;
drop table t;
-- names are used as is, even if they need quoting
create virtual table temp.t using vsv(data='"x y","q""z",select
1,2,3', header);
select '02_10', (select group_concat(name, '|') from pragma_table_info('t')) = 'x y|q"z|select';
select '02_11', (select "select" from t) = '3';
drop table t;
-- the header alone is a table with no rows
create virtual table temp.t using vsv(data='a,b', header);
select '02_12', (select group_concat(name) from pragma_table_info('t')) = 'a,b';
select '02_13', (select count(*) from t) = 0;
drop table t;

-- schema: names and types come from it, the fields from the input
create virtual table temp.t using vsv(
    filename=vsv.tmp/people.csv,
    schema='create table x(id, name, city)'
);
select '02_14', (select group_concat(name) from pragma_table_info('t')) = 'id,name,city';
select '02_15', (select name from t where id = '33') = 'Alice';
drop table t;
-- fewer columns than fields: the rest are dropped
create virtual table temp.t using vsv(data='a,b,c', schema='create table x(p)');
select '02_16', (select p from t) = 'a';
drop table t;
-- more columns than fields: the missing ones are NULL
create virtual table temp.t using vsv(data='a,b', schema='create table x(p, q, r int)');
select '02_17', (select p = 'a' and q = 'b' and r is null from t);
drop table t;
-- with a header, the header is skipped and its names are not used
create virtual table temp.t using vsv(
    filename=vsv.tmp/header.csv,
    schema='create table x(a, b)',
    header
);
select '02_18', (select group_concat(a || b, ' ') from t) = '1one 2two';
drop table t;
-- hidden columns are read too
create virtual table temp.t using vsv(data='1,2', schema='create table x(a, b int hidden)');
select '02_19', (select a = '1' and b = 2 from t);
select '02_20', (select count(*) from pragma_table_info('t')) = 1;
drop table t;
-- the table name in the schema doesn't matter
create virtual table temp.t using vsv(data='1', schema='create table "some other"(a)');
select '02_21', (select a from t) = '1';
drop table t;

-- records: blank lines are skipped
create virtual table temp.t using vsv(data='a,b

c,d

');
select '03_01', (select count(*) from t) = 2;
select '03_02', (select group_concat(c0 || c1, ' ') from t) = 'ab cd';
select '03_03', (select group_concat(rowid, ',') from t) = '1,2';
drop table t;
create virtual table temp.t using vsv(data='
');
select '03_04', (select count(*) from t) = 0;
drop table t;
-- a line of spaces is not blank
create virtual table temp.t using vsv(data='a
 
b');
select '03_05', (select group_concat(c0, '|') from t) = 'a| |b';
drop table t;
-- a trailing newline doesn't add a row
create virtual table temp.t using vsv(data='a
b
');
select '03_06', (select count(*) from t) = 2;
drop table t;
-- an empty field is empty text, never NULL
create virtual table temp.t using vsv(data=',"",x
a,
b,');
select '03_07', (select c0 = '' and c1 = '' and c2 = 'x' from t where rowid = 1);
select '03_08', (select typeof(c0) || typeof(c1) from t where rowid = 1) = 'texttext';
select '03_09', (select count(*) from t where c1 = '') = 3;
drop table t;
-- \r\n is read as \n
create virtual table temp.t using vsv(filename=vsv.tmp/crlf.csv);
select '03_10', (select group_concat(c0 || c1, ' ') from t) = 'ab cd';
select '03_11', (select count(*) from t where length(c1) = 1) = 2;
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/quoted.csv);
select '03_12', (select group_concat(c0 || c1, ' ') from t) = 'ab cd';
select '03_13', (select count(*) from t where length(c1) = 1) = 2;
drop table t;
-- a \r not followed by \n is a regular character
create virtual table temp.t using vsv(filename=vsv.tmp/cr.csv);
select '03_14', (select c0 = 'a' || char(13) || 'b' and c1 = 'c' from t);
drop table t;
-- a UTF-8 BOM at the start is skipped
create virtual table temp.t using vsv(filename=vsv.tmp/bom.csv, header);
select '03_15', (select group_concat(name) from pragma_table_info('t')) = 'h1,h2';
select '03_16', (select h1 from t) = 'x';
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/bom.csv);
select '03_17', (select c0 from t where rowid = 1) = 'h1';
select '03_18', (select count(*) from t a, t b where b.c0 = 'h1') = 2;
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/bomonly.csv);
select '03_19', (select count(*) from t) = 0;
drop table t;
-- fields are bytes, passed as text without UTF-8 validation
create virtual table temp.t using vsv(
    filename=vsv.tmp/bytes.csv,
    schema='create table x(c0, c1, c2, c3, c4)'
);
select '03_20', (select count(*) from t) = 2;
select '03_21', (select hex(c0) = 'FF61' and c1 = 'b' from t where rowid = 1);
select '03_22', (select c0 = 'ok' and c1 = 'é' and hex(c2) = 'FF' and hex(c3) = 'C3' from t where rowid = 2);
select '03_23', (select typeof(c2) = 'text' from t where rowid = 2);
-- an embedded NUL is kept in the field
select '03_24', (select hex(c4) = '610062' from t where rowid = 2);
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/boundary.csv);
select '03_25', (select count(*) from t) = 2;
select '03_26', (select length(c0) = 4095 and hex(c1) = 'FF62' and c2 = 'c' from t where rowid = 1);
select '03_27', (select c0 || c1 || c2 from t where rowid = 2) = 'def';
drop table t;

-- quoting
create virtual table temp.t using vsv(data='"a,1","b""q",""
"multi
line",x,"y"');
select '04_01', (select c0 from t where rowid = 1) = 'a,1';
select '04_02', (select c1 from t where rowid = 1) = 'b"q';
select '04_03', (select c2 from t where rowid = 1) = '';
select '04_04', (select c0 from t where rowid = 2) = 'multi' || char(10) || 'line';
select '04_05', (select c2 from t where rowid = 2) = 'y';
select '04_06', (select count(*) from t) = 2;
drop table t;
-- \r\n in a quoted field is \n, blank lines in it are kept
create virtual table temp.t using vsv(filename=vsv.tmp/crquoted.csv);
select '04_07', (select c0 = 'a' || char(10, 10) || 'b' and c1 = 'c' from t);
drop table t;
-- spaces are part of the field
create virtual table temp.t using vsv(data=' a , b ');
select '04_08', (select c0 = ' a ' and c1 = ' b ' from t);
drop table t;

-- separators
create virtual table temp.t using vsv(data='a;b,c', fsep=';');
select '05_01', (select c0 = 'a' and c1 = 'b,c' from t);
drop table t;
create virtual table temp.t using vsv(data='a	b', fsep='\t');
select '05_02', (select c0 || c1 from t) = 'ab';
drop table t;
create virtual table temp.t using vsv(data='a	b', fsep='	');
select '05_03', (select c0 || c1 from t) = 'ab';
drop table t;
create virtual table temp.t using vsv(data='a|b', fsep='\x7c');
select '05_04', (select c0 || c1 from t) = 'ab';
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/us.csv, fsep='\x1F');
select '05_05', (select c0 || c1 from t) = 'ab';
drop table t;
-- any single UTF-8 character
create virtual table temp.t using vsv(data='a→"b→c"→d', fsep='→');
select '05_06', (select c0 = 'a' and c1 = 'b→c' and c2 = 'd' from t);
drop table t;
create virtual table temp.t using vsv(data='a,b', fsep='→');
select '05_07', (select c0 = 'a,b' from t);
drop table t;

-- types: a field is converted as SQLite converts text inserted into
-- a column with the declared type (https://sqlite.org/datatype3.html).
-- ref is a regular table with the same types, filled with the fields
-- of the column s (no type, so the fields are text as is).
create virtual table temp.t using vsv(
    data='1|1|1|1|1|1|1|1|1|1
-2|-2|-2|-2|-2|-2|-2|-2|-2|-2
 3 | 3 | 3 | 3 | 3 | 3 | 3 | 3 | 3 | 3 
+4|+4|+4|+4|+4|+4|+4|+4|+4|+4
1.5|1.5|1.5|1.5|1.5|1.5|1.5|1.5|1.5|1.5
1.0|1.0|1.0|1.0|1.0|1.0|1.0|1.0|1.0|1.0
1e3|1e3|1e3|1e3|1e3|1e3|1e3|1e3|1e3|1e3
1E3|1E3|1E3|1E3|1E3|1E3|1E3|1E3|1E3|1E3
1e+3|1e+3|1e+3|1e+3|1e+3|1e+3|1e+3|1e+3|1e+3|1e+3
1e-3|1e-3|1e-3|1e-3|1e-3|1e-3|1e-3|1e-3|1e-3|1e-3
.5|.5|.5|.5|.5|.5|.5|.5|.5|.5
5.|5.|5.|5.|5.|5.|5.|5.|5.|5.
-0|-0|-0|-0|-0|-0|-0|-0|-0|-0
-0.0|-0.0|-0.0|-0.0|-0.0|-0.0|-0.0|-0.0|-0.0|-0.0
abc|abc|abc|abc|abc|abc|abc|abc|abc|abc
|||||||||
1e|1e|1e|1e|1e|1e|1e|1e|1e|1e
1.5e|1.5e|1.5e|1.5e|1.5e|1.5e|1.5e|1.5e|1.5e|1.5e
1 2|1 2|1 2|1 2|1 2|1 2|1 2|1 2|1 2|1 2
0x10|0x10|0x10|0x10|0x10|0x10|0x10|0x10|0x10|0x10
inf|inf|inf|inf|inf|inf|inf|inf|inf|inf
nan|nan|nan|nan|nan|nan|nan|nan|nan|nan
é|é|é|é|é|é|é|é|é|é
9223372036854775807|9223372036854775807|9223372036854775807|9223372036854775807|9223372036854775807|9223372036854775807|9223372036854775807|9223372036854775807|9223372036854775807|9223372036854775807
-9223372036854775808|-9223372036854775808|-9223372036854775808|-9223372036854775808|-9223372036854775808|-9223372036854775808|-9223372036854775808|-9223372036854775808|-9223372036854775808|-9223372036854775808
9223372036854775808|9223372036854775808|9223372036854775808|9223372036854775808|9223372036854775808|9223372036854775808|9223372036854775808|9223372036854775808|9223372036854775808|9223372036854775808
-9223372036854775809|-9223372036854775809|-9223372036854775809|-9223372036854775809|-9223372036854775809|-9223372036854775809|-9223372036854775809|-9223372036854775809|-9223372036854775809|-9223372036854775809
99999999999999999999|99999999999999999999|99999999999999999999|99999999999999999999|99999999999999999999|99999999999999999999|99999999999999999999|99999999999999999999|99999999999999999999|99999999999999999999
9223372036854774784.0|9223372036854774784.0|9223372036854774784.0|9223372036854774784.0|9223372036854774784.0|9223372036854774784.0|9223372036854774784.0|9223372036854774784.0|9223372036854774784.0|9223372036854774784.0
9223372036854775807.0|9223372036854775807.0|9223372036854775807.0|9223372036854775807.0|9223372036854775807.0|9223372036854775807.0|9223372036854775807.0|9223372036854775807.0|9223372036854775807.0|9223372036854775807.0
1e15|1e15|1e15|1e15|1e15|1e15|1e15|1e15|1e15|1e15
1e17|1e17|1e17|1e17|1e17|1e17|1e17|1e17|1e17|1e17
1e20|1e20|1e20|1e20|1e20|1e20|1e20|1e20|1e20|1e20
1e300|1e300|1e300|1e300|1e300|1e300|1e300|1e300|1e300|1e300
2251799813685248.0|2251799813685248.0|2251799813685248.0|2251799813685248.0|2251799813685248.0|2251799813685248.0|2251799813685248.0|2251799813685248.0|2251799813685248.0|2251799813685248.0',
    fsep='|',
    schema='create table x(
        s, i int, r real, n numeric, t text, b blob, a any,
        f floating point, d decimal(10,2), v varchar(10)
    )'
);
create table temp.ref(
    s, i int, r real, n numeric, t text, b blob, a any,
    f floating point, d decimal(10,2), v varchar(10)
);
insert into temp.ref(rowid, s, i, r, n, t, b, a, f, d, v)
select rowid, s, s, s, s, s, s, s, s, s, s from t;
select '06_01', (select count(*) from t) = 35;
select '06_02', (select count(*) from t where typeof(s) <> 'text') = 0;
select '06_03', (select count(*) from t join ref on ref.rowid = t.rowid where t.i is not ref.i or typeof(t.i) <> typeof(ref.i)) = 0;
select '06_04', (select count(*) from t join ref on ref.rowid = t.rowid where t.r is not ref.r or typeof(t.r) <> typeof(ref.r)) = 0;
select '06_05', (select count(*) from t join ref on ref.rowid = t.rowid where t.n is not ref.n or typeof(t.n) <> typeof(ref.n)) = 0;
select '06_06', (select count(*) from t join ref on ref.rowid = t.rowid where t.t is not ref.t or typeof(t.t) <> typeof(ref.t)) = 0;
select '06_07', (select count(*) from t join ref on ref.rowid = t.rowid where t.b is not ref.b or typeof(t.b) <> typeof(ref.b)) = 0;
select '06_08', (select count(*) from t join ref on ref.rowid = t.rowid where t.a is not ref.a or typeof(t.a) <> typeof(ref.a)) = 0;
select '06_09', (select count(*) from t join ref on ref.rowid = t.rowid where t.f is not ref.f or typeof(t.f) <> typeof(ref.f)) = 0;
select '06_10', (select count(*) from t join ref on ref.rowid = t.rowid where t.d is not ref.d or typeof(t.d) <> typeof(ref.d)) = 0;
select '06_11', (select count(*) from t join ref on ref.rowid = t.rowid where t.v is not ref.v or typeof(t.v) <> typeof(ref.v)) = 0;
-- a few of them spelled out
select '06_12', (select i = 3 and typeof(i) = 'integer' from t where s = ' 3 ');
select '06_13', (select i = 1000 and typeof(i) = 'integer' from t where s = '1e3');
select '06_14', (select i = 1.5 and typeof(i) = 'real' from t where s = '1.5');
select '06_15', (select i = 'abc' and typeof(i) = 'text' from t where s = 'abc');
select '06_16', (select i = '0x10' and typeof(i) = 'text' from t where s = '0x10');
select '06_17', (select i = '' and typeof(i) = 'text' from t where s = '');
select '06_18', (select r = 1.0 and typeof(r) = 'real' from t where s = '1');
select '06_19', (select n = 1 and typeof(n) = 'integer' from t where s = '1.0');
select '06_20', (select n = 100000000000000000 and typeof(n) = 'integer' from t where s = '1e17');
select '06_21', (select n = 1e20 and typeof(n) = 'real' from t where s = '1e20');
select '06_22', (select typeof(n) = 'real' from t where s = '9223372036854775808');
select '06_23', (select t = '1.0' and typeof(t) = 'text' from t where s = '1.0');
select '06_24', (select b = '1.0' and typeof(b) = 'text' from t where s = '1.0');
select '06_25', (select f = 1 and typeof(f) = 'integer' from t where s = '1.0');
drop table ref;
drop table t;

-- parameters: booleans are yes, on, true, 1 and no, off, false, 0,
-- in any case, and the name alone is true
create virtual table temp.t using vsv(filename=vsv.tmp/header.csv, header);
select '07_01', (select count(*) from t) = 2;
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/header.csv, header=yes);
select '07_02', (select count(*) from t) = 2;
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/header.csv, header=ON);
select '07_03', (select count(*) from t) = 2;
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/header.csv, header=true);
select '07_04', (select count(*) from t) = 2;
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/header.csv, header=1);
select '07_05', (select count(*) from t) = 2;
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/header.csv, header='yes');
select '07_06', (select count(*) from t) = 2;
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/header.csv, header=no);
select '07_07', (select count(*) from t) = 3;
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/header.csv, header=off);
select '07_08', (select count(*) from t) = 3;
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/header.csv, header=FALSE);
select '07_09', (select count(*) from t) = 3;
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/header.csv, header=0);
select '07_10', (select count(*) from t) = 3;
drop table t;
-- strings: single, double or no quotes, with spaces around =
create virtual table temp.t using vsv(filename='vsv.tmp/people.csv');
select '07_11', (select count(*) from t) = 3;
drop table t;
create virtual table temp.t using vsv(filename="vsv.tmp/people.csv");
select '07_12', (select count(*) from t) = 3;
drop table t;
create virtual table temp.t using vsv(filename = vsv.tmp/people.csv);
select '07_13', (select count(*) from t) = 3;
drop table t;
-- two quotes in a quoted string are one quote
create virtual table temp.t using vsv(data='it''s');
select '07_14', (select c0 from t) = 'it''s';
drop table t;
-- names are case-insensitive
create virtual table temp.t using vsv(DATA='a;b', FSep=';', Header);
select '07_15', (select group_concat(name) from pragma_table_info('t')) = 'a,b';
drop table t;
-- the table can be created in any schema
create virtual table main.t using vsv(data='a');
select '07_16', (select c0 from main.t) = 'a';
drop table main.t;

-- nulls: an unquoted empty field is NULL, a quoted one ("") stays empty
create virtual table temp.t using vsv(data=',"",x
"",,', nulls);
select '08_01', (select c0 is null and c1 = '' and c2 = 'x' from t where rowid = 1);
select '08_02', (select c0 = '' and c1 is null and c2 is null from t where rowid = 2);
drop table t;
create virtual table temp.t using vsv(data=',"",x', nulls=no);
select '08_03', (select c0 = '' and c1 = '' from t);
drop table t;
-- before the conversion, so empty numbers don't count in aggregates
create virtual table temp.t using vsv(data='1,
,
3,', schema='create table x(a int, b real)', nulls);
select '08_04', (select count(a) = 2 and avg(a) = 2 and max(a) = 3 from t);
select '08_05', (select count(b) = 0 from t);
drop table t;
-- the header is not affected
create virtual table temp.t using vsv(data='a,
1,', header, nulls);
select '08_06', (select group_concat(name, '|') from pragma_table_info('t')) = 'a|';
select '08_07', (select a = '1' and "" is null from t);
drop table t;

.shell rm -rf vsv.tmp
