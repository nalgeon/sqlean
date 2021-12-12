.load dist/fileio

with data as (
  select name || ',' || mode as value from fsdir('LICENSE')
)
select '01', value = 'LICENSE,33188' from data;
select '02', count(*) >= 10 from fsdir('test');
select '03', writefile('hello.txt', 'hello world') = 11;
select '04', typeof(readfile('hello.txt')) = 'blob';
select '05', length(readfile('hello.txt')) = 11;
select '06', readfile('whatever') is null;