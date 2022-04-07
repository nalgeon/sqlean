-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/fileio

-- lsdir
select '01', (name, mode, size) = ('LICENSE', 33188, 1108) from lsdir('LICENSE');
select '02', count(*) >= 10 from lsdir('test');
select '03', count(*) = 0 from lsdir('whatever.txt');
.shell mkdir parentdir
.shell touch parentdir/parent.txt
.shell mkdir parentdir/subdir
.shell touch parentdir/subdir/child.txt
select '04', count(*) = 3 from lsdir('parentdir');
select '05', count(*) = 3 from lsdir('parentdir', false);
select '06', count(*) = 4 from lsdir('parentdir', true);
.shell rm -rf parentdir

-- lsmode
select '11', lsmode(16877) = 'drwxr-xr-x';
select '12', lsmode(33206) = '-rw-rw-rw-';
select '13', lsmode(33188) = '-rw-r--r--';
select '14', lsmode(384) = '?rw-------';
select '15', lsmode(420) = '?rw-r--r--';
select '16', lsmode(436) = '?rw-rw-r--';
select '17', lsmode(438) = '?rw-rw-rw-';
select '18', lsmode(493) = '?rwxr-xr-x';
select '19', lsmode(511) = '?rwxrwxrwx';

-- mkdir
.shell rm -rf hellodir
select '21', mkdir('hellodir') is null;
select '22', (name, mode) = ('hellodir', 16877) from fsdir('hellodir');

-- readfile
.shell rm -f hello.txt
.shell printf 'hello world' > hello.txt
select '31', typeof(readfile('hello.txt')) = 'blob';
select '32', length(readfile('hello.txt')) = 11;
select '33', readfile('whatever') is null;

-- symlink
.shell rm -f hello.txt
.shell printf 'hello world' > hello.txt
select '41', symlink('hello.txt', 'hello.lnk') is null;
select '42', length(readfile('hello.lnk')) = 11;

-- writefile
.shell rm -f hello.txt
select '51', writefile('hello.txt', 'hello world') = 11;
select '52', (name, mode) = ('hello.txt', 33206) from fsdir('hello.txt');
select '53', writefile('hello.txt', 'hello world', 420) = 11;
select '54', (name, mode) = ('hello.txt', 33188) from fsdir('hello.txt');

.shell rm -rf hellodir
.shell rm -f hello.txt
.shell rm -f hello.lnk
