-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/vsv

-- Every file is created in vsv.tmp, which is removed at the end.
--
-- Fixtures:
--   people.csv     '11,Diane,London\n22,Grace,Berlin\n33,Alice,Paris\n'
--   header.csv     'id,name\n1,one\n2,two\n'
--   lines.csv      'h\n1\n2\n3' (no trailing newline)
--   crlf.csv       'a,b\r\nc,d\r\n'
--   quoted.csv     '"a","b"\r\n"c","d"\r\n'
--   bom.csv        BOM + 'h1,h2\nx,y\n'
--   bomonly.csv    BOM only
--   ff.csv         '\xffa,b\nc,d\n'
--   boundary.csv   1023 x 'a' + ',\xffb,c\nd,e,f\n' (0xff is the 1025th byte)
--   utf8.csv       'ok,\xc3\xa9,\xff,\xc3,a\x00b\n'
--   badutf8.csv    'a\xc0\x80,\xed\xa0\x80,\xc3\xa9\xc3\n'
--
-- Tests marked "pinned" record the current behavior, which is a bug or an
-- open question (see TODO.md). They change when the behavior is fixed.
--
-- The .shell lines double the backslashes: the shell passes an argument
-- without spaces to sh unquoted, and sh removes one level of escaping.
-- Bytes are written in octal, since sh's printf doesn't support \x.

.shell rm -rf vsv.tmp
.shell mkdir -p vsv.tmp
.shell printf '11,Diane,London\\n22,Grace,Berlin\\n33,Alice,Paris\\n' > vsv.tmp/people.csv
.shell printf 'id,name\\n1,one\\n2,two\\n' > vsv.tmp/header.csv
.shell printf 'h\\n1\\n2\\n3' > vsv.tmp/lines.csv
.shell printf 'a,b\\r\\nc,d\\r\\n' > vsv.tmp/crlf.csv
.shell printf '"a","b"\\r\\n"c","d"\\r\\n' > vsv.tmp/quoted.csv
.shell printf '\\357\\273\\277h1,h2\\nx,y\\n' > vsv.tmp/bom.csv
.shell printf '\\357\\273\\277' > vsv.tmp/bomonly.csv
.shell printf '\\377a,b\\nc,d\\n' > vsv.tmp/ff.csv
.shell printf '%01023d' 0 | tr 0 a > vsv.tmp/boundary.csv
.shell printf ',\\377b,c\\nd,e,f\\n' >> vsv.tmp/boundary.csv
.shell printf 'ok,\\303\\251,\\377,\\303,a\\000b\\n' > vsv.tmp/utf8.csv
.shell printf 'a\\300\\200,\\355\\240\\200,\\303\\251\\303\\n' > vsv.tmp/badutf8.csv

-- filename
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
    schema="create table people(id integer, name text, city text)",
    columns=3,
    affinity=integer
);
select '01_06', count(*) = 3 from people;
select '01_07', (id, name, city) = (22, 'Grace', 'Berlin') from people where id = 22;
select '01_08', typeof(id) = 'integer' from people where id = 22;
drop table people;

-- data
create virtual table temp.t using vsv(data='a,b
c,d');
select '01_09', (select group_concat(c0 || c1, ' ') from t) = 'ab cd';
select '01_10', (select group_concat(rowid, ',') from t) = '1,2';
select '01_11', (select count(*) from t a, t b) = 4;
drop table t;

-- empty input has no rows and one column
create virtual table temp.t using vsv(data='');
select '01_12', (select count(*) from t) = 0;
select '01_13', (select group_concat(name) from pragma_table_info('t')) = 'c0';
drop table t;
create virtual table temp.t using vsv(data='', columns=2);
select '01_14', (select count(*) from t) = 0;
drop table t;
-- pinned: the header of empty input is a column named '(NULL)'
create virtual table temp.t using vsv(data='', header);
select '01_15', (select group_concat(name) from pragma_table_info('t')) = '(NULL)';
drop table t;

-- vsv tables are read-only and keep nothing: copy the data to store it
create virtual table temp.t using vsv(filename=vsv.tmp/people.csv, affinity=integer);
create table stored as select c0 as id, c1 as name, c2 as city from t;
select '01_16', (select count(*) from stored) = 3;
select '01_17', (select sum(id) from stored) = 66;
select '01_18', (select typeof(id) from stored where name = 'Diane') = 'integer';
drop table stored;
drop table t;

-- columns: names and count
-- without a schema or a header, the columns are named c0, c1, ...
-- and counted from the first record
create virtual table temp.t using vsv(data='a,b,c
d,e,f');
select '02_01', (select group_concat(name) from pragma_table_info('t')) = 'c0,c1,c2';
drop table t;
-- columns=N with fewer fields: the rest are dropped
create virtual table temp.t using vsv(data='a,b,c
d,e,f', columns=2);
select '02_02', (select group_concat(name) from pragma_table_info('t')) = 'c0,c1';
select '02_03', (select group_concat(c0 || c1, ' ') from t) = 'ab de';
drop table t;
-- columns=N with more fields: the missing ones are NULL
create virtual table temp.t using vsv(data='a,b', columns=4);
select '02_04', (select group_concat(name) from pragma_table_info('t')) = 'c0,c1,c2,c3';
select '02_05', (select c0 = 'a' and c1 = 'b' and c2 is null and c3 is null from t);
drop table t;
-- the count comes from the first record, later records may differ
create virtual table temp.t using vsv(data='a,b
d,e,f
g');
select '02_06', (select group_concat(name) from pragma_table_info('t')) = 'c0,c1';
select '02_07', (select group_concat(c0 || ':' || ifnull(c1, 'null'), ' ') from t) = 'a:b d:e g:null';
drop table t;

-- header: the names come from the first record, which is not a row
create virtual table temp.t using vsv(filename=vsv.tmp/header.csv, header);
select '02_08', (select group_concat(name) from pragma_table_info('t')) = 'id,name';
select '02_09', (select group_concat(id || name, ' ') from t) = '1one 2two';
select '02_10', (select group_concat(rowid, ',') from t) = '1,2';
drop table t;
create virtual table temp.t using vsv(data='"x y","q""z",
1,2,3', header);
select '02_11', (select group_concat(name, '|') from pragma_table_info('t')) = 'x y|q"z|';
drop table t;
-- header with columns=N: the first N names
create virtual table temp.t using vsv(filename=vsv.tmp/header.csv, header, columns=1);
select '02_12', (select group_concat(name) from pragma_table_info('t')) = 'id';
select '02_13', (select group_concat(id, ',') from t) = '1,2';
drop table t;
-- pinned: the extra columns are numbered from 1, not 0 (c2 would match c0, c1)
create virtual table temp.t using vsv(filename=vsv.tmp/header.csv, header, columns=3);
select '02_14', (select group_concat(name) from pragma_table_info('t')) = 'id,name,c3';
select '02_15', (select count(*) from t where c3 is null) = 2;
drop table t;

-- schema: its names, columns=N fields
create virtual table temp.t using vsv(
    filename=vsv.tmp/people.csv,
    schema='create table x(id, name, city)',
    columns=3
);
select '02_16', (select group_concat(name) from pragma_table_info('t')) = 'id,name,city';
select '02_17', (select name from t where id = '33') = 'Alice';
drop table t;
-- a schema with fewer columns than columns=N
create virtual table temp.t using vsv(data='a,b,c', schema='create table x(p)', columns=3);
select '02_18', (select p from t) = 'a';
drop table t;
-- schema with header: the header is skipped, its names are not used
create virtual table temp.t using vsv(
    filename=vsv.tmp/header.csv,
    schema='create table x(a, b)',
    columns=2,
    header
);
select '02_19', (select group_concat(a || b, ' ') from t) = '1one 2two';
drop table t;
-- A schema with more columns than columns=N reads past the end of the
-- cursor's arrays, and a schema without columns=N fails with "out of memory".
-- Neither can be tested here (see TODO.md).

-- records
-- a blank line is a row with an empty first field
create virtual table temp.t using vsv(data='a,b

c,d
');
select '03_01', (select count(*) from t) = 3;
select '03_02', (select c0 = '' and c1 is null from t where rowid = 2);
drop table t;
-- a trailing newline doesn't add a row
create virtual table temp.t using vsv(data='a
b
');
select '03_03', (select count(*) from t) = 2;
drop table t;
-- a single newline is one empty row
create virtual table temp.t using vsv(data='
');
select '03_04', (select count(*) from t) = 1;
select '03_05', (select c0 from t) = '';
drop table t;
-- an empty last field is empty text
create virtual table temp.t using vsv(data='a,
b,
');
select '03_06', (select count(*) from t where c1 = '') = 2;
drop table t;
-- pinned: ... except at the end of input without a newline, where it's NULL
create virtual table temp.t using vsv(data='a,
b,');
select '03_07', (select c1 from t where c0 = 'a') = '';
select '03_08', (select c1 is null from t where c0 = 'b');
drop table t;
-- \r before \n is removed
create virtual table temp.t using vsv(filename=vsv.tmp/crlf.csv);
select '03_09', (select group_concat(c0 || c1, ' ') from t) = 'ab cd';
select '03_10', (select count(*) from t where length(c1) = 1) = 2;
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/quoted.csv);
select '03_11', (select group_concat(c0 || c1, ' ') from t) = 'ab cd';
select '03_12', (select count(*) from t where length(c1) = 1) = 2;
drop table t;
-- a UTF-8 BOM at the start is skipped
create virtual table temp.t using vsv(filename=vsv.tmp/bom.csv, header);
select '03_13', (select group_concat(name) from pragma_table_info('t')) = 'h1,h2';
select '03_14', (select h1 from t) = 'x';
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/bom.csv);
select '03_15', (select c0 from t where rowid = 1) = 'h1';
-- pinned: a second scan by the same cursor keeps the BOM
select '03_16', (select count(*) from t a, t b where b.c0 = 'h1') = 1;
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/bomonly.csv);
select '03_17', (select count(*) from t) = 0;
drop table t;
-- Where char is signed (x86, macOS), a 0xff byte read at the start of the
-- 1024-byte input buffer is taken for EOF: ff.csv has no rows, and
-- boundary.csv has 3 rows with the first one cut at 0xff. Where char is
-- unsigned (Linux on ARM), both are read correctly. Not tested here, since
-- the result depends on the platform (see TODO.md).
create virtual table temp.t using vsv(filename=vsv.tmp/utf8.csv, affinity=blob);
-- a 0xff in the middle of the buffer is a regular byte
select '03_18', (select c2 from t) = x'ff';
-- an embedded NUL is kept in the field
select '03_19', (select c4 from t) = x'610062';
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
-- a quote inside an unquoted field is kept
create virtual table temp.t using vsv(data='ab"c,d', columns=2);
select '04_07', (select c0 from t) = 'ab"c';
drop table t;
-- pinned: an unterminated quoted field gives the rest of the input, no error
create virtual table temp.t using vsv(data='"abc
d', columns=1);
select '04_08', (select count(*) from t) = 1;
select '04_09', (select c0 from t) = 'abc' || char(10) || 'd';
drop table t;
-- pinned: text after a closing quote ends the field, no error,
-- and the rest of the field becomes the next row
create virtual table temp.t using vsv(data='"ab"c,d', columns=2);
select '04_10', (select count(*) from t) = 2;
select '04_11', (select c0 = 'ab"' and c1 is null from t where rowid = 1);
select '04_12', (select c0 = '' and c1 = 'd' from t where rowid = 2);
drop table t;
-- pinned: with a custom rsep, a closing quote followed by text doesn't close
-- the field, and the field separator after it is part of the field
create virtual table temp.t using vsv(data='"ab"c,d', columns=2, rsep=';');
select '04_13', (select count(*) from t) = 1;
select '04_14', (select c0 = 'ab"c,d' and c1 is null from t);
drop table t;

-- separators
create virtual table temp.t using vsv(data='a;b', fsep=';');
select '05_01', (select c0 || c1 from t) = 'ab';
drop table t;
create virtual table temp.t using vsv(data='a	b', fsep='\t');
select '05_02', (select c0 || c1 from t) = 'ab';
drop table t;
create virtual table temp.t using vsv(data='a|b', fsep='\x7c');
select '05_03', (select c0 || c1 from t) = 'ab';
drop table t;
create virtual table temp.t using vsv(data='a|b', fsep='\X7C');
select '05_04', (select c0 || c1 from t) = 'ab';
drop table t;
create virtual table temp.t using vsv(data='a,b;c,d', rsep=';');
select '05_05', (select group_concat(c0 || c1, ' ') from t) = 'ab cd';
drop table t;
-- with rsep, a newline is a regular character
create virtual table temp.t using vsv(data='a
b;c', rsep=';');
select '05_06', (select c0 from t where rowid = 1) = 'a' || char(10) || 'b';
drop table t;
-- an empty separator is the default
create virtual table temp.t using vsv(data='a,b', fsep='');
select '05_07', (select c0 || c1 from t) = 'ab';
drop table t;
-- pinned: a 3-character separator is accepted as the NUL byte
create virtual table temp.t using vsv(data='a,b', fsep='abc');
select '05_08', (select c0 from t) = 'a,b';
drop table t;
-- pinned: fsep equal to rsep makes the whole input one row
create virtual table temp.t using vsv(data='a
b
c', fsep='\n', columns=3);
select '05_09', (select count(*) from t) = 1;
select '05_10', (select c0 || c1 || c2 from t) = 'abc';
drop table t;

-- skip
create virtual table temp.t using vsv(filename=vsv.tmp/lines.csv, skip=2);
select '06_01', (select group_concat(c0, ' ') from t) = '2 3';
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/lines.csv, skip=1, header);
select '06_02', (select group_concat(name) from pragma_table_info('t')) = 'h';
select '06_03', (select group_concat(h, ' ') from t) = '2 3';
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/lines.csv, skip=1, columns=1);
select '06_04', (select group_concat(c0, ' ') from t) = '1 2 3';
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/lines.csv, skip=3);
select '06_05', (select group_concat(c0, ' ') from t) = '3';
drop table t;
-- skip=N skips N records whatever the count of fields
create virtual table temp.t using vsv(data='a,b
c
d', skip=1, columns=2);
select '06_06', (select group_concat(c0, ' ') from t) = 'c d';
drop table t;
-- pinned: with data=, the record read to count the columns or name them
-- is skipped on top of skip=N
create virtual table temp.t using vsv(data='h
1
2
3', skip=1);
select '06_07', (select group_concat(c0, ' ') from t) = '2 3';
drop table t;
create virtual table temp.t using vsv(data='h
1
2
3', skip=1, header);
select '06_08', (select group_concat(h, ' ') from t) = '3';
drop table t;
create virtual table temp.t using vsv(data='h
1
2
3', skip=1, columns=1);
select '06_09', (select group_concat(c0, ' ') from t) = '1 2 3';
drop table t;

-- nulls
create virtual table temp.t using vsv(data=',"",x
,,
');
select '07_01', (select count(*) from t where c0 = '' and c1 = '') = 2;
select '07_02', (select typeof(c0) from t where rowid = 1) = 'text';
drop table t;
-- nulls: an empty unquoted field is NULL, a quoted one is ''
create virtual table temp.t using vsv(data=',"",x
,,
', nulls);
select '07_03', (select c0 is null and c1 = '' and c2 = 'x' from t where rowid = 1);
select '07_04', (select c0 is null and c1 is null and c2 is null from t where rowid = 2);
drop table t;
-- nulls: a field with only \r is empty
create virtual table temp.t using vsv(filename=vsv.tmp/crlf.csv, columns=3, nulls);
select '07_05', (select count(*) from t where c2 is null) = 2;
drop table t;
create virtual table temp.t using vsv(data='a,', nulls=off);
select '07_06', (select c1 is null from t);
drop table t;
create virtual table temp.t using vsv(data='x
,
', affinity=blob, columns=2);
select '07_07', (select c0 = x'' and c1 = x'' from t where rowid = 2);
drop table t;
-- pinned: an empty first field of the input is a NUL byte
create virtual table temp.t using vsv(data=',
', affinity=blob);
select '07_08', (select c0 = x'00' and c1 = x'' from t);
drop table t;
create virtual table temp.t using vsv(data=',a
,b', affinity=text, validatetext);
select '07_09', (select c0 = x'00' from t where rowid = 1);
select '07_10', (select c0 = '' and typeof(c0) = 'text' from t where rowid = 2);
drop table t;
create virtual table temp.t using vsv(data=',
', affinity=blob, nulls);
select '07_11', (select c0 is null and c1 is null from t);
drop table t;

-- affinity: none, text and blob
-- Fields: 1 | -2 | ' 3 ' | +4 | 1.5 | 1.0 | 1e3 | .5 | 5. | abc | '' | 1e
--         | 99999999999999999999 | 1e20 | 0x10 | -0 | '1 2'
create virtual table temp.none using vsv(
    data='1|-2| 3 |+4|1.5|1.0|1e3|.5|5.|abc||1e|99999999999999999999|1e20|0x10|-0|1 2',
    fsep='|'
);
select '08_01', (select c0 = '1' and c2 = ' 3 ' and c4 = '1.5' and c10 = '' from none);
select '08_02', (select typeof(c0) || typeof(c4) || typeof(c10) from none) = 'texttexttext';
drop table none;
create virtual table temp.text using vsv(
    data='1|-2| 3 |+4|1.5|1.0|1e3|.5|5.|abc||1e|99999999999999999999|1e20|0x10|-0|1 2',
    fsep='|',
    affinity=text
);
select '08_03', (select c0 = '1' and c2 = ' 3 ' and c4 = '1.5' and c10 = '' from text);
select '08_04', (select typeof(c0) || typeof(c4) || typeof(c10) from text) = 'texttexttext';
drop table text;
create virtual table temp.blob using vsv(
    data='1|-2| 3 |+4|1.5|1.0|1e3|.5|5.|abc||1e|99999999999999999999|1e20|0x10|-0|1 2',
    fsep='|',
    affinity=blob
);
select '08_05', (select c0 = x'31' and c2 = x'203320' and c9 = x'616263' and c10 = x'' from blob);
select '08_06', (select typeof(c0) || typeof(c9) || typeof(c10) from blob) = 'blobblobblob';
drop table blob;
-- affinity names are case-insensitive
create virtual table temp.t using vsv(data='1', affinity=BLOB);
select '08_07', (select typeof(c0) from t) = 'blob';
drop table t;
-- pinned: affinity names are matched by prefix
create virtual table temp.t using vsv(data='1', affinity=integerfoo);
select '08_08', (select typeof(c0) from t) = 'integer';
drop table t;
create virtual table temp.t using vsv(data='1', affinity=textual);
select '08_09', (select typeof(c0) from t) = 'text';
drop table t;

-- affinity: integer
-- an optional sign, digits, and spaces around them
create virtual table temp.t using vsv(
    data='1|-2| 3 |+4|1.5|1.0|1e3|.5|5.|abc||1e|99999999999999999999|1e20|0x10|-0|1 2',
    fsep='|',
    affinity=integer
);
select '09_01', (select c0 = 1 and c1 = -2 and c2 = 3 and c3 = 4 from t);
select '09_02', (select typeof(c0) || typeof(c1) || typeof(c2) || typeof(c3) from t) = 'integerintegerintegerinteger';
select '09_03', (select c15 = 0 and typeof(c15) = 'integer' from t);
-- anything else is text, including decimals with no fraction
select '09_04', (select c4 = '1.5' and c5 = '1.0' and c6 = '1e3' and c7 = '.5' and c8 = '5.' from t);
select '09_05', (select c9 = 'abc' and c10 = '' and c11 = '1e' and c14 = '0x10' and c16 = '1 2' from t);
select '09_06', (select typeof(c5) || typeof(c10) from t) = 'texttext';
-- pinned: an integer out of the 64-bit range saturates
select '09_07', (select c12 = 9223372036854775807 from t);
drop table t;
create virtual table temp.t using vsv(
    data='9223372036854775807|-9223372036854775808',
    fsep='|',
    affinity=integer
);
select '09_08', (select c0 = 9223372036854775807 and c1 = -9223372036854775808 from t);
drop table t;

-- affinity: real
create virtual table temp.t using vsv(
    data='1|-2| 3 |+4|1.5|1.0|1e3|.5|5.|abc||1e|99999999999999999999|1e20|0x10|-0|1 2',
    fsep='|',
    affinity=real
);
select '10_01', (select c0 = 1.0 and c1 = -2.0 and c2 = 3.0 and c3 = 4.0 from t);
select '10_02', (select c4 = 1.5 and c5 = 1.0 and c6 = 1000.0 and c7 = 0.5 and c8 = 5.0 from t);
select '10_03', (select c12 = 1e20 and c13 = 1e20 and c15 = 0.0 from t);
select '10_04', (select typeof(c0) || typeof(c4) || typeof(c12) from t) = 'realrealreal';
select '10_05', (select c9 = 'abc' and c10 = '' and c11 = '1e' and c14 = '0x10' and c16 = '1 2' from t);
select '10_06', (select typeof(c9) || typeof(c10) from t) = 'texttext';
drop table t;
create virtual table temp.t using vsv(data='1E3|1e+3|1e-3|-1.5e2', fsep='|', affinity=real);
select '10_07', (select c0 = 1000.0 and c1 = 1000.0 and c2 = 0.001 and c3 = -150.0 from t);
drop table t;

-- affinity: numeric
-- integers, and reals with no fraction, are integers
create virtual table temp.t using vsv(
    data='1|-2| 3 |+4|1.5|1.0|1e3|.5|5.|abc||1e|99999999999999999999|1e20|0x10|-0|1 2',
    fsep='|',
    affinity=numeric
);
select '11_01', (select c0 = 1 and c1 = -2 and c2 = 3 and c3 = 4 and c5 = 1 and c6 = 1000 and c8 = 5 from t);
select '11_02', (select typeof(c0) || typeof(c5) || typeof(c6) || typeof(c8) from t) = 'integerintegerintegerinteger';
select '11_03', (select c4 = 1.5 and c7 = 0.5 and typeof(c4) = 'real' from t);
select '11_04', (select c13 = 1e20 and typeof(c13) = 'real' from t);
select '11_05', (select c9 = 'abc' and c10 = '' and c11 = '1e' and c14 = '0x10' and c16 = '1 2' from t);
-- pinned: an integer out of the 64-bit range saturates, as with integer
select '11_06', (select c12 = 9223372036854775807 from t);
drop table t;
-- Whether 1e15 is an integer depends on the platform's long double (see TODO.md).

-- dsep
create virtual table temp.t using vsv(data='1,5;2.5;3', fsep=';', dsep=',', affinity=real);
select '12_01', (select c0 = 1.5 from t);
-- pinned: with dsep, a point is not a number
select '12_02', (select c1 = '2.5' from t);
select '12_03', (select c2 = 3.0 from t);
drop table t;
create virtual table temp.t using vsv(data='1,0;1,5', fsep=';', dsep=',', affinity=numeric);
select '12_04', (select c0 = 1 from t);
select '12_05', (select typeof(c0) from t) = 'integer';
select '12_06', (select c1 = 1.5 from t);
drop table t;
-- pinned: reading a column twice gives a different result,
-- because the first read replaces dsep with a point in the field
create virtual table temp.t using vsv(data='1,5', fsep=';', dsep=',', affinity=real);
select '12_07', (select typeof(c0) || typeof(c0) from t) = 'realtext';
drop table t;

-- validatetext
-- without it, the text is passed as is
create virtual table temp.t using vsv(filename=vsv.tmp/utf8.csv, affinity=text);
select '13_01', (select typeof(c2) from t) = 'text';
-- an embedded NUL ends the text
select '13_02', (select c4 = 'a' from t);
drop table t;
-- with affinity=text, invalid UTF-8 is a blob
create virtual table temp.t using vsv(filename=vsv.tmp/utf8.csv, affinity=text, validatetext);
select '13_03', (select c0 = 'ok' and c1 = 'é' from t);
select '13_04', (select c2 = x'ff' and typeof(c2) = 'blob' from t);
-- an embedded NUL makes a blob
select '13_05', (select c4 = x'610062' from t);
-- pinned: a truncated sequence at the end is accepted
select '13_06', (select typeof(c3) = 'text' and hex(c3) = 'C3' from t);
drop table t;
-- pinned: overlong encodings and surrogates are accepted,
-- and so is a truncated sequence at the end of the field
create virtual table temp.t using vsv(filename=vsv.tmp/badutf8.csv, affinity=text, validatetext);
select '13_07', (select typeof(c0) || typeof(c1) || typeof(c2) from t) = 'texttexttext';
drop table t;
-- with integer, real and numeric, non-numbers are checked as with text
create virtual table temp.t using vsv(filename=vsv.tmp/utf8.csv, affinity=integer, validatetext);
select '13_08', (select typeof(c1) || typeof(c2) from t) = 'textblob';
drop table t;
-- with affinity=blob, validatetext has no effect
create virtual table temp.t using vsv(filename=vsv.tmp/utf8.csv, affinity=blob, validatetext);
select '13_09', (select typeof(c0) || typeof(c1) from t) = 'blobblob';
drop table t;
-- with affinity=none, valid fields are text and invalid ones raise an error
-- (not tested here, since the error stops the suite)
create virtual table temp.t using vsv(filename=vsv.tmp/utf8.csv, validatetext);
select '13_10', (select c0 = 'ok' and c1 = 'é' from t);
drop table t;

-- parameters
-- booleans: yes, on, true, 1 and no, off, false, 0, case-insensitive;
-- the name alone is true
create virtual table temp.t using vsv(filename=vsv.tmp/header.csv, header);
select '14_01', (select count(*) from t) = 2;
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/header.csv, header=yes);
select '14_02', (select count(*) from t) = 2;
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/header.csv, header=ON);
select '14_03', (select count(*) from t) = 2;
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/header.csv, header=true);
select '14_04', (select count(*) from t) = 2;
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/header.csv, header=1);
select '14_05', (select count(*) from t) = 2;
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/header.csv, header=no);
select '14_06', (select count(*) from t) = 3;
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/header.csv, header=off);
select '14_07', (select count(*) from t) = 3;
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/header.csv, header=FALSE);
select '14_08', (select count(*) from t) = 3;
drop table t;
create virtual table temp.t using vsv(filename=vsv.tmp/header.csv, header=0);
select '14_09', (select count(*) from t) = 3;
drop table t;
-- strings: single, double or no quotes, with spaces around =
create virtual table temp.t using vsv(filename='vsv.tmp/people.csv');
select '14_10', (select count(*) from t) = 3;
drop table t;
create virtual table temp.t using vsv(filename="vsv.tmp/people.csv");
select '14_11', (select count(*) from t) = 3;
drop table t;
create virtual table temp.t using vsv(filename = vsv.tmp/people.csv);
select '14_12', (select count(*) from t) = 3;
drop table t;
-- two quotes in a quoted string are one quote
create virtual table temp.t using vsv(data='it''s');
select '14_13', (select c0 from t) = 'it''s';
drop table t;
-- numbers
create virtual table temp.t using vsv(data='a,b,c', columns = 2);
select '14_14', (select group_concat(name) from pragma_table_info('t')) = 'c0,c1';
drop table t;
-- the table can be created in any schema
create virtual table main.t using vsv(data='a');
select '14_15', (select c0 from main.t) = 'a';
drop table main.t;

.shell rm -rf vsv.tmp
