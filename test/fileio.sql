-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/fileio

-- Every file is created in fileio.tmp, which is removed at the end.
--
-- Fixtures:
--   hello.txt   'hello world' (11 bytes)
--   lines.txt   'one\ntwo\r\n\nlast' (4 lines, the last one without \n)
--   empty.txt   (0 bytes)
--   dir/        .hidden, a.txt, sub/b.txt
--
-- Modes (decimal = octal):
--   33188 = 0100644 -rw-r--r--    16877 = 040755 drwxr-xr-x
--   33206 = 0100666 -rw-rw-rw-    16832 = 040700 drwx------
--   33152 = 0100600 -rw-------    16384 = 040000 d---------
--
-- Files created by the shell, and directories created with the default
-- permissions, assume umask 022.
--
-- The .shell lines double the backslashes: the shell passes an argument
-- without spaces to sh unquoted, and sh removes one level of escaping.

.shell chmod -R u+rwx fileio.tmp 2>/dev/null; rm -rf fileio.tmp
.shell mkdir -p fileio.tmp/dir/sub
.shell printf 'hello world' > fileio.tmp/hello.txt
.shell printf 'one\\ntwo\\r\\n\\nlast' > fileio.tmp/lines.txt
.shell touch fileio.tmp/empty.txt fileio.tmp/dir/.hidden fileio.tmp/dir/a.txt fileio.tmp/dir/sub/b.txt

-- fileio_read
select '01_01', typeof(fileio_read('fileio.tmp/hello.txt')) = 'blob';
select '01_02', cast(fileio_read('fileio.tmp/hello.txt') as text) = 'hello world';
select '01_03', fileio_read('fileio.tmp/empty.txt') = x'';
-- a missing file is NULL
select '01_04', fileio_read('fileio.tmp/nosuch.txt') is null;
select '01_05', fileio_read('') is null;
-- file names are UTF-8
.shell printf 'hello' > fileio.tmp/привет世界.txt
select '01_06', cast(fileio_read('fileio.tmp/привет世界.txt') as text) = 'hello';

-- fileio_read: offset and limit
select '02_01', cast(fileio_read('fileio.tmp/hello.txt', 0) as text) = 'hello world';
select '02_02', cast(fileio_read('fileio.tmp/hello.txt', 6) as text) = 'world';
select '02_03', fileio_read('fileio.tmp/hello.txt', 11) = x'';
-- an offset past the end gives an empty blob
select '02_04', fileio_read('fileio.tmp/hello.txt', 12) = x'';
select '02_05', fileio_read('fileio.tmp/hello.txt', 12, 1) = x'';
select '02_06', cast(fileio_read('fileio.tmp/hello.txt', 0, 5) as text) = 'hello';
select '02_07', cast(fileio_read('fileio.tmp/hello.txt', 6, 1) as text) = 'w';
-- a limit past the end reads to the end
select '02_08', cast(fileio_read('fileio.tmp/hello.txt', 6, 10) as text) = 'world';
-- limit 0 means no limit
select '02_09', cast(fileio_read('fileio.tmp/hello.txt', 6, 0) as text) = 'world';
-- numeric text is converted
select '02_10', cast(fileio_read('fileio.tmp/hello.txt', '6') as text) = 'world';
select '02_11', cast(fileio_read('fileio.tmp/hello.txt', 0, '5') as text) = 'hello';
-- offset and limit are 64-bit
select '02_12', fileio_read('fileio.tmp/hello.txt', 4294967296 + 6) = x'';
select '02_13', cast(fileio_read('fileio.tmp/hello.txt', 0, 4294967296 + 1) as text) = 'hello world';
select '02_14', fileio_read('fileio.tmp/hello.txt', 9223372036854775807) = x'';
select '02_15', cast(fileio_read('fileio.tmp/hello.txt', 6, 9223372036854775807) as text) = 'world';

-- fileio_read: NULL
select '03_01', fileio_read(null) is null;
select '03_02', fileio_read(null, 0) is null;
select '03_04', fileio_read('fileio.tmp/hello.txt', null) is null;
select '03_05', fileio_read('fileio.tmp/hello.txt', 0, null) is null;

-- fileio_read: bad data
-- a directory is NULL
select '04_01', fileio_read('fileio.tmp/dir') is null;
-- an offset or limit that is not a non-negative integer is NULL
select '04_02', fileio_read('fileio.tmp/hello.txt', 'abc') is null;
select '04_03', fileio_read('fileio.tmp/hello.txt', x'36') is null;
select '04_04', fileio_read('fileio.tmp/hello.txt', 6.9) is null;
select '04_05', fileio_read('fileio.tmp/hello.txt', -1) is null;
select '04_06', fileio_read('fileio.tmp/hello.txt', 0, 'abc') is null;
select '04_07', fileio_read('fileio.tmp/hello.txt', 0, -1) is null;
-- a path that is not text is NULL
select '04_08', fileio_read(cast('fileio.tmp/hello.txt' as blob)) is null;
select '04_09', fileio_read(42) is null;
-- a path with an embedded NUL is NULL
select '04_10', fileio_read('fileio.tmp/hello.txt' || char(0) || 'x') is null;

-- fileio_scan
select '11_01', count(*) = 4 from fileio_scan('fileio.tmp/lines.txt');
-- lines are split on \n, a trailing \r is removed, the last line may lack \n
select '11_02', group_concat(rowid || ':' || value, '|') = '1:one|2:two|3:|4:last'
  from fileio_scan('fileio.tmp/lines.txt');
select '11_03', count(*) = 4 from fileio_scan('fileio.tmp/lines.txt') where typeof(value) = 'text';
select '11_04', group_concat(distinct name) = 'fileio.tmp/lines.txt'
  from fileio_scan('fileio.tmp/lines.txt');
select '11_05', count(*) = 1 from fileio_scan('fileio.tmp/hello.txt');
select '11_06', count(*) = 0 from fileio_scan('fileio.tmp/empty.txt');
-- a directory has no lines
select '11_07', count(*) = 0 from fileio_scan('fileio.tmp/dir');
-- lines longer than the initial buffer
select '11_08', fileio_write('fileio.tmp/long.txt', printf('%.*c', 1000, 'x') || char(10) || 'y') = 1002;
select '11_09', group_concat(length(value), '|') = '1000|1' from fileio_scan('fileio.tmp/long.txt');
-- an embedded NUL is kept
.shell printf 'a\\000b\\nc\\n' > fileio.tmp/nulbyte.txt
select '11_10', group_concat(hex(value), '|') = '610062|63' from fileio_scan('fileio.tmp/nulbyte.txt');
-- the path can come from another table
select '11_11', count(*) = 5
  from (select 'fileio.tmp/hello.txt' as path union all select 'fileio.tmp/lines.txt') as files,
  fileio_scan(files.path);
-- name = path is the same as the argument
select '11_12', count(*) = 4 from fileio_scan where name = 'fileio.tmp/lines.txt';
-- only one \n, \r\n or \r is removed at the end of the line
.shell printf 'a\\r\\r\\nb\\rc\\nd\\r' > fileio.tmp/cr.txt
select '11_13', group_concat(hex(value), '|') = '610D|620D63|64' from fileio_scan('fileio.tmp/cr.txt');
-- other constraints are left to SQLite
select '11_14', group_concat(value, '|') = 'one|two'
  from (select value from fileio_scan('fileio.tmp/lines.txt') limit 2);
select '11_15', group_concat(value, '|') = 'two|last'
  from fileio_scan('fileio.tmp/lines.txt') where value in ('two', 'last');
select '11_16', value = 'two' from fileio_scan('fileio.tmp/lines.txt') where rowid = 2;

-- fileio_scan: NULL and bad data
select '12_01', count(*) = 0 from fileio_scan(null);
-- a path that is not text gives no rows
select '12_02', count(*) = 0 from fileio_scan(cast('fileio.tmp/hello.txt' as blob));
select '12_03', count(*) = 0 from fileio_scan('fileio.tmp/hello.txt' || char(0) || 'x');
-- a missing file gives no rows
select '12_04', count(*) = 0 from fileio_scan('fileio.tmp/nosuch.txt');

-- fileio_write
select '21_01', fileio_write('fileio.tmp/w1.txt', 'hello world') = 11;
select '21_02', cast(fileio_read('fileio.tmp/w1.txt') as text) = 'hello world';
-- overwrites an existing file
select '21_03', fileio_write('fileio.tmp/w1.txt', 'bye') = 3;
select '21_04', cast(fileio_read('fileio.tmp/w1.txt') as text) = 'bye';
-- blobs are written as is
select '21_05', fileio_write('fileio.tmp/w2.bin', x'00ff00') = 3;
select '21_06', fileio_read('fileio.tmp/w2.bin') = x'00ff00';
-- numbers are written as text
select '21_07', fileio_write('fileio.tmp/w3.txt', 42) = 2;
select '21_08', cast(fileio_read('fileio.tmp/w3.txt') as text) = '42';
select '21_09', fileio_write('fileio.tmp/w4.txt', 1.5) = 3;
select '21_10', cast(fileio_read('fileio.tmp/w4.txt') as text) = '1.5';
select '21_11', fileio_write('fileio.tmp/w5.txt', '') = 0;
select '21_12', fileio_read('fileio.tmp/w5.txt') = x'';
-- the count is in bytes, not characters
select '21_13', fileio_write('fileio.tmp/w6.txt', 'мир') = 6;
-- creates missing parent directories
select '21_14', fileio_write('fileio.tmp/a/b/c.txt', 'x') = 1;
select '21_15', cast(fileio_read('fileio.tmp/a/b/c.txt') as text) = 'x';
select '21_16', fileio_mode(mode) = 'drwxr-xr-x' from fileio_ls('fileio.tmp/a/b') limit 1;

-- fileio_write: permissions and mtime
-- the default permissions are 0666, regardless of the umask
select '22_01', mode = 33206 from fileio_ls('fileio.tmp/w1.txt');
select '22_02', fileio_write('fileio.tmp/p1.txt', 'x', 420) = 1;
select '22_03', mode = 33188 from fileio_ls('fileio.tmp/p1.txt');
select '22_04', fileio_write('fileio.tmp/p2.txt', 'x', 384) = 1;
select '22_05', mode = 33152 from fileio_ls('fileio.tmp/p2.txt');
-- an existing file gets the new permissions
select '22_06', fileio_write('fileio.tmp/p2.txt', 'x', 420) = 1;
select '22_07', mode = 33188 from fileio_ls('fileio.tmp/p2.txt');
-- perm 0 is no permissions
select '22_08', fileio_write('fileio.tmp/p3.txt', 'x', 0) = 1;
select '22_09', mode = 32768 from fileio_ls('fileio.tmp/p3.txt');
select '22_10', fileio_write('fileio.tmp/m1.txt', 'x', 420, 1000000000) = 1;
select '22_11', mtime = 1000000000 from fileio_ls('fileio.tmp/m1.txt');
select '22_12', fileio_write('fileio.tmp/m2.txt', 'x', 420, 0) = 1;
select '22_13', mtime = 0 from fileio_ls('fileio.tmp/m2.txt');
-- a negative mtime is before the epoch
select '22_14', fileio_write('fileio.tmp/m3.txt', 'x', 420, -1) = 1;
select '22_15', mtime = -1 from fileio_ls('fileio.tmp/m3.txt');
-- numeric text is converted
select '22_16', fileio_write('fileio.tmp/m4.txt', 'x', '420', '1000000000') = 1;
select '22_17', (mode, mtime) = (33188, 1000000000) from fileio_ls('fileio.tmp/m4.txt');

-- fileio_write: NULL
select '23_01', fileio_write(null, 'x') is null;
select '23_02', fileio_write(null, 'x', 420) is null;
select '23_03', fileio_write(null, 'x', 420, 0) is null;
-- NULL data, perm or mtime is NULL, and writes nothing
select '23_04', fileio_write('fileio.tmp/n1.txt', null) is null;
select '23_05', fileio_write('fileio.tmp/n1.txt', 'x', null) is null;
select '23_06', fileio_write('fileio.tmp/n1.txt', 'x', 420, null) is null;
select '23_07', count(*) = 0 from fileio_ls('fileio.tmp/n1.txt');

-- fileio_write: bad data
-- a perm that is not an integer in the 0..4095 (07777) range is NULL
select '24_01', fileio_write('fileio.tmp/b1.txt', 'x', 'abc') is null;
select '24_02', fileio_write('fileio.tmp/b1.txt', 'x', 420.5) is null;
select '24_03', fileio_write('fileio.tmp/b1.txt', 'x', -1) is null;
select '24_04', fileio_write('fileio.tmp/b1.txt', 'x', 4096) is null;
-- an mtime that is not an integer is NULL
select '24_05', fileio_write('fileio.tmp/b1.txt', 'x', 420, 'abc') is null;
select '24_06', fileio_write('fileio.tmp/b1.txt', 'x', 420, 1.5) is null;
-- a path that is not text is NULL
select '24_07', fileio_write(cast('fileio.tmp/b1.txt' as blob), 'x') is null;
select '24_08', fileio_write('fileio.tmp/b1.txt' || char(0) || 'x', 'x') is null;
-- none of the above writes anything
select '24_09', count(*) = 0 from fileio_ls('fileio.tmp/b1.txt');
-- the setuid bit is kept (2541 = 04755)
select '24_10', fileio_write('fileio.tmp/b2.txt', 'x', 2541) = 1;
select '24_11', mode = 35309 from fileio_ls('fileio.tmp/b2.txt');

-- fileio_append
select '31_01', fileio_append('fileio.tmp/ap1.txt', 'one') = 3;
select '31_02', fileio_append('fileio.tmp/ap1.txt', 'two') = 3;
select '31_03', cast(fileio_read('fileio.tmp/ap1.txt') as text) = 'onetwo';
-- a constant path stays open for the whole statement
create table lines(value text);
insert into lines values ('one'), ('two'), ('three');
select '31_04', sum(fileio_append('fileio.tmp/ap2.txt', value || char(10))) = 14 from lines;
select '31_05', cast(fileio_read('fileio.tmp/ap2.txt') as text) =
  'one' || char(10) || 'two' || char(10) || 'three' || char(10);
-- each row can have its own path
create table targets(path text, value text);
insert into targets values
  ('fileio.tmp/ap3.txt', 'a'), ('fileio.tmp/ap4.txt', 'b'), ('fileio.tmp/ap3.txt', 'c');
select '31_06', sum(fileio_append(path, value)) = 3 from targets;
select '31_07', cast(fileio_read('fileio.tmp/ap3.txt') as text) = 'ac';
select '31_08', cast(fileio_read('fileio.tmp/ap4.txt') as text) = 'b';
-- numbers are appended as text
select '31_09', fileio_append('fileio.tmp/ap5.txt', 42) = 2;
select '31_10', cast(fileio_read('fileio.tmp/ap5.txt') as text) = '42';
-- the count is in bytes, not characters
select '31_11', fileio_append('fileio.tmp/ap6.txt', 'мир') = 6;
select '31_12', fileio_append('fileio.tmp/ap6.txt', '') = 0;
select '31_13', cast(fileio_read('fileio.tmp/ap6.txt') as text) = 'мир';
-- creates missing parent directories
select '31_14', fileio_append('fileio.tmp/c/d/ap.txt', 'x') = 1;
select '31_15', cast(fileio_read('fileio.tmp/c/d/ap.txt') as text) = 'x';
-- blobs are appended as is
select '31_16', fileio_append('fileio.tmp/ap7.txt', x'610062') = 3;
select '31_17', fileio_read('fileio.tmp/ap7.txt') = x'610062';

-- fileio_append: NULL and bad data
select '32_01', fileio_append(null, 'x') is null;
select '32_02', fileio_append('fileio.tmp/ap8.txt', null) is null;
select '32_03', fileio_append(cast('fileio.tmp/ap8.txt' as blob), 'x') is null;
select '32_04', count(*) = 0 from fileio_ls('fileio.tmp/ap8.txt');

-- fileio_mkdir
select '41_01', fileio_mkdir('fileio.tmp/d1') is null;
-- the default permissions are 0777 minus the umask
select '41_02', mode = 16877 from fileio_ls('fileio.tmp/d1');
select '41_03', fileio_mkdir('fileio.tmp/d2', 448) is null;
select '41_04', mode = 16832 from fileio_ls('fileio.tmp/d2');
-- an existing directory is fine, and gets the new permissions
select '41_05', fileio_mkdir('fileio.tmp/d2', 448) is null;
select '41_06', fileio_mkdir('fileio.tmp/d2', 493) is null;
select '41_07', mode = 16877 from fileio_ls('fileio.tmp/d2');
-- numeric text is converted
select '41_08', fileio_mkdir('fileio.tmp/d3', '448') is null;
select '41_09', mode = 16832 from fileio_ls('fileio.tmp/d3');

-- fileio_mkdir: NULL
select '42_01', fileio_mkdir(null) is null;
select '42_02', fileio_mkdir(null, 493) is null;
-- NULL perm is NULL, and creates nothing
select '42_03', fileio_mkdir('fileio.tmp/d4', null) is null;
select '42_04', count(*) = 0 from fileio_ls('fileio.tmp/d4');

-- fileio_mkdir: bad data
-- a perm that is not an integer in the 0..4095 (07777) range is NULL
select '43_01', fileio_mkdir('fileio.tmp/d5', 'abc') is null;
select '43_02', fileio_mkdir('fileio.tmp/d5', 448.5) is null;
select '43_03', fileio_mkdir('fileio.tmp/d5', -1) is null;
select '43_04', fileio_mkdir('fileio.tmp/d5', 4096) is null;
-- a path that is not text is NULL
select '43_05', fileio_mkdir(cast('fileio.tmp/d5' as blob)) is null;
select '43_06', fileio_mkdir('fileio.tmp/d5' || char(0) || 'x') is null;
-- none of the above creates anything
select '43_07', count(*) = 0 from fileio_ls('fileio.tmp/d5');
-- perm 0 is no permissions
-- (listed from the parent: a d--------- directory can't be read)
select '43_08', fileio_mkdir('fileio.tmp/d6', 0) is null;
select '43_09', mode = 16384 from fileio_ls('fileio.tmp') where name = 'fileio.tmp/d6';

-- fileio_symlink
-- the target is relative to the link
select '51_01', fileio_symlink('hello.txt', 'fileio.tmp/link1') is null;
select '51_02', cast(fileio_read('fileio.tmp/link1') as text) = 'hello world';
-- fileio_ls doesn't follow links
select '51_03', fileio_mode(mode) like 'l%' from fileio_ls('fileio.tmp/link1');
-- a dangling link is fine
select '51_04', fileio_symlink('nosuch.txt', 'fileio.tmp/link2') is null;
select '51_05', count(*) = 1 from fileio_ls('fileio.tmp/link2');
select '51_06', fileio_read('fileio.tmp/link2') is null;

-- fileio_symlink: NULL
select '52_01', fileio_symlink(null, 'fileio.tmp/link3') is null;
select '52_02', count(*) = 0 from fileio_ls('fileio.tmp/link3');

select '52_03', fileio_symlink('hello.txt', null) is null;

-- fileio_symlink: bad data
-- a target or path that is not text is NULL, and creates nothing
select '53_01', fileio_symlink(cast('hello.txt' as blob), 'fileio.tmp/link4') is null;
select '53_02', fileio_symlink('hello.txt', cast('fileio.tmp/link4' as blob)) is null;
select '53_03', fileio_symlink('hello.txt' || char(0) || 'x', 'fileio.tmp/link4') is null;
select '53_04', count(*) = 0 from fileio_ls('fileio.tmp/link4');

-- fileio_ls
select '61_01', (name, mode, size) = ('fileio.tmp/hello.txt', 33188, 11)
  from fileio_ls('fileio.tmp/hello.txt');
select '61_02', typeof(mtime) = 'integer' and mtime > 1000000000
  from fileio_ls('fileio.tmp/hello.txt');
-- a missing path gives no rows
select '61_03', count(*) = 0 from fileio_ls('fileio.tmp/nosuch');
select '61_04', count(*) = 0 from fileio_ls('');
-- a directory gives itself and its direct children, including hidden files
select '61_05', group_concat(name, '|') =
  'fileio.tmp/dir|fileio.tmp/dir/.hidden|fileio.tmp/dir/a.txt|fileio.tmp/dir/sub'
  from (select name from fileio_ls('fileio.tmp/dir') order by name);
select '61_06', mode = 16877 from fileio_ls('fileio.tmp/dir') where name = 'fileio.tmp/dir';
select '61_07', count(*) = 4 from fileio_ls('fileio.tmp/dir', false);
-- recursive lists all the descendants
select '61_08', group_concat(name, '|') =
  'fileio.tmp/dir|fileio.tmp/dir/.hidden|fileio.tmp/dir/a.txt|fileio.tmp/dir/sub|fileio.tmp/dir/sub/b.txt'
  from (select name from fileio_ls('fileio.tmp/dir', true) order by name);
-- rowids are sequential
select '61_09', group_concat(rowid, '|') = '1|2|3|4' from fileio_ls('fileio.tmp/dir');
-- hidden columns are the arguments
select '61_10', path = 'fileio.tmp/hello.txt' and dir is null from fileio_ls('fileio.tmp/hello.txt');
select '61_11', (path, dir) = ('fileio.tmp/dir', 1) from fileio_ls('fileio.tmp/dir', 1) limit 1;
-- a trailing slash is not doubled in the names
select '61_15', group_concat(name, '|') = 'fileio.tmp/dir/|fileio.tmp/dir/a.txt'
  from fileio_ls('fileio.tmp/dir/') where name in ('fileio.tmp/dir/', 'fileio.tmp/dir/a.txt');
-- the path can come from another table
select '61_12', count(*) = 5
  from (select 'fileio.tmp/dir' as path union all select 'fileio.tmp/hello.txt') as files,
  fileio_ls(files.path);
-- path = ... and dir = ... are the same as the arguments
select '61_13', count(*) = 4 from fileio_ls where path = 'fileio.tmp/dir';
select '61_14', count(*) = 5 from fileio_ls where path = 'fileio.tmp/dir' and dir = true;

-- fileio_ls: NULL and bad data
select '62_01', count(*) = 0 from fileio_ls(null);
select '62_02', count(*) = 0 from fileio_ls('fileio.tmp/dir', null);
-- a recursive flag that is not an integer gives no rows
select '62_03', count(*) = 0 from fileio_ls('fileio.tmp/dir', 'abc');
select '62_04', count(*) = 0 from fileio_ls('fileio.tmp/dir', 1.5);
-- numeric text is converted, any non-zero integer is true
select '62_05', count(*) = 5 from fileio_ls('fileio.tmp/dir', '1');
select '62_06', count(*) = 5 from fileio_ls('fileio.tmp/dir', 2);
-- a path that is not text gives no rows
select '62_07', count(*) = 0 from fileio_ls(cast('fileio.tmp/hello.txt' as blob));
select '62_08', count(*) = 0 from fileio_ls('fileio.tmp/hello.txt' || char(0) || 'x');

-- fileio_mode
select '71_01', fileio_mode(16877) = 'drwxr-xr-x';
select '71_02', fileio_mode(33206) = '-rw-rw-rw-';
select '71_03', fileio_mode(33188) = '-rw-r--r--';
select '71_04', fileio_mode(41453) = 'lrwxr-xr-x';
-- permission bits alone have an unknown type
select '71_05', fileio_mode(384) = '?rw-------';
select '71_06', fileio_mode(420) = '?rw-r--r--';
select '71_07', fileio_mode(436) = '?rw-rw-r--';
select '71_08', fileio_mode(438) = '?rw-rw-rw-';
select '71_09', fileio_mode(493) = '?rwxr-xr-x';
select '71_10', fileio_mode(511) = '?rwxrwxrwx';
select '71_11', fileio_mode(0) = '?---------';
-- other file types (010644 FIFO, 020644 char device, 060644 block device, 0140644 socket)
select '71_12', fileio_mode(4516) = 'prw-r--r--';
select '71_16', fileio_mode(8612) = 'crw-r--r--';
select '71_17', fileio_mode(24996) = 'brw-r--r--';
select '71_18', fileio_mode(49572) = 'srw-r--r--';
-- setuid, setgid and sticky bits are ignored (35309 = 0104755)
select '71_13', fileio_mode(35309) = '-rwxr-xr-x';
-- numeric text is converted
select '71_14', fileio_mode('420') = '?rw-r--r--';
-- the maximum mode (0177777)
select '71_15', fileio_mode(65535) = '?rwxrwxrwx';

-- fileio_mode: NULL and bad data
select '72_01', fileio_mode(null) is null;
-- a mode that is not an integer in the 0..65535 (0177777) range is NULL
select '72_02', fileio_mode('abc') is null;
select '72_03', fileio_mode(x'01') is null;
select '72_04', fileio_mode(420.9) is null;
select '72_05', fileio_mode(-1) is null;
select '72_06', fileio_mode(65536) is null;
select '72_07', fileio_mode(4294967296 + 420) is null;

-- aliases
select '81_01', cast(readfile('fileio.tmp/hello.txt') as text) = 'hello world';
select '81_02', count(*) = 4 from scanfile('fileio.tmp/lines.txt');
select '81_03', writefile('fileio.tmp/alias.txt', 'abc') = 3;
select '81_04', mkdir('fileio.tmp/aliasdir') is null;
select '81_05', symlink('hello.txt', 'fileio.tmp/aliaslink') is null;
select '81_06', count(*) = 4 from lsdir('fileio.tmp/dir');
select '81_07', lsmode(420) = '?rw-r--r--';

-- roundtrip: write and read
create table blobs(id integer primary key, data blob);
insert into blobs(data) values
  (x''), (x'00'), (x'00ff'), (cast('hello' as blob)), (zeroblob(100000)), (randomblob(1000));
select '91_01', sum(fileio_write('fileio.tmp/rt/' || id, data)) = 101008 from blobs;
select '91_02', count(*) = 6 from blobs where fileio_read('fileio.tmp/rt/' || id) = data;
select '91_03', count(*) = 6 from blobs
  join fileio_ls('fileio.tmp/rt') as f on f.name = 'fileio.tmp/rt/' || blobs.id
  where f.size = length(blobs.data);

-- roundtrip: append and scan
select '92_01', sum(fileio_append('fileio.tmp/rt.txt', value || char(10))) = 14 from lines;
select '92_02', (select group_concat(value, '|') from fileio_scan('fileio.tmp/rt.txt')) =
  (select group_concat(value, '|') from lines);

.shell chmod -R u+rwx fileio.tmp; rm -rf fileio.tmp
