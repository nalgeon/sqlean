-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/crypto

select '01', md5(null) is NULL;
select '02', hex(md5('')) = upper('d41d8cd98f00b204e9800998ecf8427e');
select '03', hex(md5('abc')) = upper('900150983cd24fb0d6963f7d28e17f72');

select '11', sha1(null) is NULL;
select '12', hex(sha1('')) = upper('da39a3ee5e6b4b0d3255bfef95601890afd80709');
select '13', hex(sha1('abc')) = upper('a9993e364706816aba3e25717850c26c9cd0d89d');

select '21', sha256(null) is NULL;
select '22', hex(sha256('')) = upper('e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855');
select '23', hex(sha256('abc')) = upper('ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad');

select '31', sha384(null) is NULL;
select '32', hex(sha384('')) = upper('38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f65fbd51ad2f14898b95b');
select '33', hex(sha384('abc')) = upper('cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7');

select '41', sha512(null) is NULL;
select '42', hex(sha512('')) = upper('cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e');
select '43', hex(sha512('abc')) = upper('ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f');

select '51', encode(null, 'base64') is null;
select '52', encode('', 'base64') = '';
select '53', encode('hello', 'base64') = 'aGVsbG8=';
select '54', encode(sha256('hello'), 'base64') = 'LPJNul+wow4m6DsqxbninhsWHlwfp0JecwQzYpOLmCQ=';
select '55', encode('a', 'base64') = 'YQ==';
select '56', encode('ab', 'base64') = 'YWI=';
select '57', encode('abc', 'base64') = 'YWJj';
select '58', encode('abcd', 'base64') = 'YWJjZA==';
select '59', encode('abcde', 'base64') = 'YWJjZGU=';
select '60', encode('abcdef', 'base64') = 'YWJjZGVm';

select '61', decode(null, 'base64') is null;
select '62', decode('', 'base64') = cast('' as blob);
select '63', decode('aGVsbG8=', 'base64') = cast('hello' as blob);
select '64', decode('LPJNul+wow4m6DsqxbninhsWHlwfp0JecwQzYpOLmCQ=', 'base64') = sha256('hello');
select '65', decode('YQ==', 'base64') = cast('a' as blob);
select '66', decode('YWI=', 'base64') = cast('ab' as blob);
select '67', decode('YWJj', 'base64') = cast('abc' as blob);
select '68', decode('YWJjZA==', 'base64') = cast('abcd' as blob);
select '69', decode('YWJjZGU=', 'base64') = cast('abcde' as blob);
select '70', decode('YWJjZGVm', 'base64') = cast('abcdef' as blob);

select '71', encode(null, 'base32') is null;
select '72', encode('', 'base32') = '';
select '73', encode('hello', 'base32') = 'NBSWY3DP';
select '74', encode('a', 'base32') = 'ME======';
select '75', encode('ab', 'base32') = 'MFRA====';
select '76', encode('abc', 'base32') = 'MFRGG===';
select '77', encode('abcd', 'base32') = 'MFRGGZA=';
select '78', encode('abcde', 'base32') = 'MFRGGZDF';
select '79', encode('abcdef', 'base32') = 'MFRGGZDFMY======';
select '80', encode('2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824', 'base32') = 'GJRWMMRUMRRGCNLGMIYGCMZQMUZDMZJYGNRDEYLDGVRDSZJSHFSTCYRRGYYWKNLDGFTGCNZUGI2WKNZTGA2DGMZWGI4TGODCHE4DENA=';

select '81', decode(null, 'base32') is null;
select '82', decode('', 'base32') = cast('' as blob);
select '83', decode('NBSWY3DP', 'base32') = cast('hello' as blob);
select '84', decode('ME======', 'base32') = cast('a' as blob);
select '85', decode('MFRA====', 'base32') = cast('ab' as blob);
select '86', decode('MFRGG===', 'base32') = cast('abc' as blob);
select '87', decode('MFRGGZA=', 'base32') = cast('abcd' as blob);
select '88', decode('MFRGGZDF', 'base32') = cast('abcde' as blob);
select '89', decode('MFRGGZDFMY======', 'base32') = cast('abcdef' as blob);
select '90', decode('GJRWMMRUMRRGCNLGMIYGCMZQMUZDMZJYGNRDEYLDGVRDSZJSHFSTCYRRGYYWKNLDGFTGCNZUGI2WKNZTGA2DGMZWGI4TGODCHE4DENA=', 'base32') = cast('2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824' as blob);
