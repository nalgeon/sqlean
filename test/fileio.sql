-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/fileio

-- fileio_ls
select '01', (name, mode, size) = ('LICENSE', 33188, 1108) from fileio_ls('LICENSE');
select '02', count(*) >= 10 from fileio_ls('test');
select '03', count(*) = 0 from fileio_ls('whatever.txt');
.shell mkdir parentdir
.shell touch parentdir/parent.txt
.shell mkdir parentdir/subdir
.shell touch parentdir/subdir/child.txt
select '04', count(*) = 3 from fileio_ls('parentdir');
select '05', count(*) = 3 from fileio_ls('parentdir', false);
select '06', count(*) = 4 from fileio_ls('parentdir', true);
.shell rm -rf parentdir

-- fileio_mode
select '11', fileio_mode(16877) = 'drwxr-xr-x';
select '12', fileio_mode(33206) = '-rw-rw-rw-';
select '13', fileio_mode(33188) = '-rw-r--r--';
select '14', fileio_mode(384) = '?rw-------';
select '15', fileio_mode(420) = '?rw-r--r--';
select '16', fileio_mode(436) = '?rw-rw-r--';
select '17', fileio_mode(438) = '?rw-rw-rw-';
select '18', fileio_mode(493) = '?rwxr-xr-x';
select '19', fileio_mode(511) = '?rwxrwxrwx';

-- fileio_mkdir
.shell rm -rf hellodir
select '21', fileio_mkdir('hellodir') is null;
select '22', (name, mode) = ('hellodir', 16877) from fileio_ls('hellodir');
.shell rm -rf hellodir

-- fileio_read
.shell printf 'hello world' > hello.txt
select '31', typeof(fileio_read('hello.txt')) = 'blob';
select '32', length(fileio_read('hello.txt')) = 11;
select '33', cast(fileio_read('hello.txt') as text) = 'hello world';
select '34', fileio_read('whatever') is null;
select '35', cast(fileio_read('hello.txt', 0) as text) = 'hello world';
select '36', cast(fileio_read('hello.txt', 6) as text) = 'world';
select '37', fileio_read('hello.txt', 12) = zeroblob(0);
select '38', cast(fileio_read('hello.txt', 0, 5) as text) = 'hello';
select '39', cast(fileio_read('hello.txt', 6, 0) as text) = 'world';
select '40', cast(fileio_read('hello.txt', 6, 1) as text) = 'w';
select '41', cast(fileio_read('hello.txt', 6, 10) as text) = 'world';

-- fileio_symlink
.shell printf 'hello world' > hello.txt
select '51', fileio_symlink('hello.txt', 'hello.lnk') is null;
select '52', length(fileio_read('hello.lnk')) = 11;
.shell rm -f hello.lnk

-- fileio_write
.shell rm -f hello.txt
select '61', fileio_write('hello.txt', 'hello world') = 11;
select '62', (name, mode) = ('hello.txt', 33206) from fileio_ls('hello.txt');
select '63', fileio_write('hello.txt', 'hello world', 420) = 11;
select '64', (name, mode) = ('hello.txt', 33188) from fileio_ls('hello.txt');

-- fileio_scan
.shell printf 'one\\ntwo\\nthr\\n' > hello.txt
create table hello as select rowid, name, value from fileio_scan('hello.txt');
select '71', count(*) = 3 from hello;
select '72', (name, value) = ('hello.txt', 'one') from hello where rowid = 1;
select '73', (name, value) = ('hello.txt', 'two') from hello where rowid = 2;
select '74', (name, value) = ('hello.txt', 'thr') from hello where rowid = 3;
drop table hello;

-- fileio_append
.shell rm -f hello.txt
create table hello(value text);
insert into hello(value) values ('one'), ('two'), ('three');
select '81', sum(fileio_append('hello.txt', value)) = 11 from hello;
select '82', cast(fileio_read('hello.txt') as text) = 'onetwothree';

.shell rm -f hello.txt
