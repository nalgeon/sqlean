-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/recsize

select '01', recsize(null) = 2;

select '11', recsize(0) = 2;
select '12', recsize(1) = 2;
select '13', recsize(10) = 3;
select '14', recsize(127) = 3;
select '15', recsize(128) = 4;

select '21', recsize(0.1) = 10;
select '22', recsize(42.3) = 10;
select '23', recsize(1.0) = 2;
select '24', recsize(42.0) = 3;

select '31', recsize('') = 2;
select '32', recsize('h') = 3;
select '33', recsize('he') = 4;
select '34', recsize('hel') = 5;
select '35', recsize('hell') = 6;
select '36', recsize('hello') = 7;

select '37', recsize('м') = 4;
select '38', recsize('ми') = 6;
select '39', recsize('мир') = 8;

select '41', recsize(randomblob(0)) = 3;
select '42', recsize(randomblob(1)) = 3;
select '43', recsize(randomblob(2)) = 4;
select '44', recsize(randomblob(3)) = 5;
select '45', recsize(randomblob(4)) = 6;
select '46', recsize(randomblob(10)) = 12;

select '51', recsize(10, 20, 30) = 7;
select '52', recsize(10, 'hello', randomblob(3)) = 13;
