-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/crypto

select '1_01', crypto_md5(null) is NULL;
select '1_02', hex(crypto_md5('')) = upper('d41d8cd98f00b204e9800998ecf8427e');
select '1_03', hex(crypto_md5('abc')) = upper('900150983cd24fb0d6963f7d28e17f72');

select '2_01', crypto_sha1(null) is NULL;
select '2_02', hex(crypto_sha1('')) = upper('da39a3ee5e6b4b0d3255bfef95601890afd80709');
select '2_03', hex(crypto_sha1('abc')) = upper('a9993e364706816aba3e25717850c26c9cd0d89d');

select '3_01', crypto_sha256(null) is NULL;
select '3_02', hex(crypto_sha256('')) = upper('e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855');
select '3_03', hex(crypto_sha256('abc')) = upper('ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad');

select '4_01', crypto_sha384(null) is NULL;
select '4_02', hex(crypto_sha384('')) = upper('38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f65fbd51ad2f14898b95b');
select '4_03', hex(crypto_sha384('abc')) = upper('cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7');

select '5_01', crypto_sha512(null) is NULL;
select '5_02', hex(crypto_sha512('')) = upper('cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e');
select '5_03', hex(crypto_sha512('abc')) = upper('ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f');

select '6_01', crypto_encode(null, 'base64') is null;
select '6_02', crypto_encode('', 'base64') = '';
select '6_03', crypto_encode('hello', 'base64') = 'aGVsbG8=';
select '6_04', crypto_encode(sha256('hello'), 'base64') = 'LPJNul+wow4m6DsqxbninhsWHlwfp0JecwQzYpOLmCQ=';
select '6_05', crypto_encode('a', 'base64') = 'YQ==';
select '6_06', crypto_encode('ab', 'base64') = 'YWI=';
select '6_07', crypto_encode('abc', 'base64') = 'YWJj';
select '6_07', crypto_encode('abcd', 'base64') = 'YWJjZA==';
select '6_09', crypto_encode('abcde', 'base64') = 'YWJjZGU=';
select '6_10', crypto_encode('abcdef', 'base64') = 'YWJjZGVm';
select '6_11', crypto_encode('эй, мир!', 'base64') = '0Y3QuSwg0LzQuNGAIQ==';
select '6_12', crypto_encode('(ಠ_ಠ)', 'base64') = 'KOCyoF/gsqAp';
select '6_13', crypto_encode('The quick brown 🦊 jumps over 13 lazy 🐶.', 'base64') = 'VGhlIHF1aWNrIGJyb3duIPCfpooganVtcHMgb3ZlciAxMyBsYXp5IPCfkLYu';

select '7_01', crypto_decode(null, 'base64') is null;
select '7_02', crypto_decode('', 'base64') = cast('' as blob);
select '7_03', crypto_decode('aGVsbG8=', 'base64') = cast('hello' as blob);
select '7_04', crypto_decode('LPJNul+wow4m6DsqxbninhsWHlwfp0JecwQzYpOLmCQ=', 'base64') = sha256('hello');
select '7_05', crypto_decode('YQ==', 'base64') = cast('a' as blob);
select '7_06', crypto_decode('YWI=', 'base64') = cast('ab' as blob);
select '7_07', crypto_decode('YWJj', 'base64') = cast('abc' as blob);
select '7_08', crypto_decode('YWJjZA==', 'base64') = cast('abcd' as blob);
select '7_09', crypto_decode('YWJjZGU=', 'base64') = cast('abcde' as blob);
select '7_10', crypto_decode('YWJjZGVm', 'base64') = cast('abcdef' as blob);
select '7_11', crypto_decode('0Y3QuSwg0LzQuNGAIQ==', 'base64') = cast('эй, мир!' as blob);
select '7_12', crypto_decode('KOCyoF/gsqAp', 'base64') = cast('(ಠ_ಠ)' as blob);
select '7_13', crypto_decode('VGhlIHF1aWNrIGJyb3duIPCfpooganVtcHMgb3ZlciAxMyBsYXp5IPCfkLYu', 'base64') = cast('The quick brown 🦊 jumps over 13 lazy 🐶.' as blob);

select '8_01', crypto_encode(null, 'base32') is null;
select '8_02', crypto_encode('', 'base32') = '';
select '8_03', crypto_encode('hello', 'base32') = 'NBSWY3DP';
select '8_05', crypto_encode('a', 'base32') = 'ME======';
select '8_06', crypto_encode('ab', 'base32') = 'MFRA====';
select '8_07', crypto_encode('abc', 'base32') = 'MFRGG===';
select '8_08', crypto_encode('abcd', 'base32') = 'MFRGGZA=';
select '8_09', crypto_encode('abcde', 'base32') = 'MFRGGZDF';
select '8_10', crypto_encode('abcdef', 'base32') = 'MFRGGZDFMY======';
select '8_11', crypto_encode('эй, мир!', 'base32') = '2GG5BOJMEDILZUFY2GACC===';
select '8_12', crypto_encode('(ಠ_ಠ)', 'base32') = 'FDQLFIC74CZKAKI=';
select '8_13', crypto_encode('The quick brown 🦊 jumps over 13 lazy 🐶.', 'base32') = 'KRUGKIDROVUWG2ZAMJZG653OEDYJ7JUKEBVHK3LQOMQG65TFOIQDCMZANRQXU6JA6CPZBNRO';

select '9_01', crypto_decode(null, 'base32') is null;
select '9_02', crypto_decode('', 'base32') = cast('' as blob);
select '9_03', crypto_decode('NBSWY3DP', 'base32') = cast('hello' as blob);
select '9_05', crypto_decode('ME======', 'base32') = cast('a' as blob);
select '9_06', crypto_decode('MFRA====', 'base32') = cast('ab' as blob);
select '9_07', crypto_decode('MFRGG===', 'base32') = cast('abc' as blob);
select '9_08', crypto_decode('MFRGGZA=', 'base32') = cast('abcd' as blob);
select '9_09', crypto_decode('MFRGGZDF', 'base32') = cast('abcde' as blob);
select '9_10', crypto_decode('MFRGGZDFMY======', 'base32') = cast('abcdef' as blob);
select '9_11', crypto_decode('2GG5BOJMEDILZUFY2GACC===', 'base32') = cast('эй, мир!' as blob);
select '9_12', crypto_decode('FDQLFIC74CZKAKI=', 'base32') = cast('(ಠ_ಠ)' as blob);
select '9_13', crypto_decode('KRUGKIDROVUWG2ZAMJZG653OEDYJ7JUKEBVHK3LQOMQG65TFOIQDCMZANRQXU6JA6CPZBNRO', 'base32') = cast('The quick brown 🦊 jumps over 13 lazy 🐶.' as blob);

select '10_01', crypto_encode(null, 'hex') is null;
select '10_02', crypto_encode('', 'hex') = '';
select '10_03', crypto_encode('hello', 'hex') = '68656c6c6f';
select '10_04', crypto_encode(sha256('hello'), 'hex') = '2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824';
select '10_05', crypto_encode('x', 'hex') = '78';
select '10_06', crypto_encode('xyz', 'hex') = '78797a';
select '10_07', crypto_encode('xyz{|}', 'hex') = '78797a7b7c7d';
select '10_08', crypto_encode('эй, мир!', 'hex') = 'd18dd0b92c20d0bcd0b8d18021';
select '10_09', crypto_encode('(ಠ_ಠ)', 'hex') = '28e0b2a05fe0b2a029';
select '10_10', crypto_encode('The quick brown 🦊 jumps over 13 lazy 🐶.', 'hex') = '54686520717569636b2062726f776e20f09fa68a206a756d7073206f766572203133206c617a7920f09f90b62e';

select '11_01', crypto_decode(null, 'hex') is null;
select '11_02', crypto_decode('', 'hex') = cast('' as blob);
select '11_03', crypto_decode('68656c6c6f', 'hex') = cast('hello' as blob);
select '11_04', crypto_decode('2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824', 'hex') = sha256('hello');
select '11_05', crypto_decode('78', 'hex') = cast('x' as blob);
select '11_06', crypto_decode('78797a', 'hex') = cast('xyz' as blob);
select '11_07', crypto_decode('78797a7b7c7d', 'hex') = cast('xyz{|}' as blob);
select '11_08', crypto_decode('d18dd0b92c20d0bcd0b8d18021', 'hex') = cast('эй, мир!' as blob);
select '11_09', crypto_decode('28e0b2a05fe0b2a029', 'hex') = cast('(ಠ_ಠ)' as blob);
select '11_10', crypto_decode('54686520717569636b2062726f776e20f09fa68a206a756d7073206f766572203133206c617a7920f09f90b62e', 'hex') = cast('The quick brown 🦊 jumps over 13 lazy 🐶.' as blob);
select '11_11', crypto_decode('68656C6C6F', 'hex') = cast('hello' as blob);
select '11_12', crypto_decode('2CF24DBA5FB0A30E26E83B2AC5B9E29E1B161E5C1FA7425E73043362938B9824', 'hex') = sha256('hello');

select '12_01', crypto_encode(null, 'url') is null;
select '12_02', crypto_encode('', 'url') = '';
select '12_03', crypto_encode('hello', 'url') = 'hello';
select '12_04', crypto_encode('ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~', 'url') = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~';
select '12_05', crypto_encode('!*\();:@&=+$,/?#[]', 'url') = '%21%2A%5C%28%29%3B%3A%40%26%3D%2B%24%2C%2F%3F%23%5B%5D';
select '12_06', crypto_encode('https://www.google.lu/search?q=hello+world&ie=UTF-8', 'url') = 'https%3A%2F%2Fwww.google.lu%2Fsearch%3Fq%3Dhello%2Bworld%26ie%3DUTF-8';
select '12_07', crypto_encode('one two three', 'url') = 'one%20two%20three';
select '12_08', crypto_encode('эй, мир!', 'url') = '%D1%8D%D0%B9%2C%20%D0%BC%D0%B8%D1%80%21';
select '12_09', crypto_encode('(ಠ_ಠ)', 'url') = '%28%E0%B2%A0_%E0%B2%A0%29';
select '12_10', crypto_encode('The quick brown 🦊 jumps over 13 lazy 🐶.', 'url') = 'The%20quick%20brown%20%F0%9F%A6%8A%20jumps%20over%2013%20lazy%20%F0%9F%90%B6.';

select '13_01', crypto_decode(null, 'url') is null;
select '13_02', crypto_decode('', 'url') = cast('' as blob);
select '13_03', crypto_decode('hello', 'url') = cast('hello' as blob);
select '13_04', crypto_decode('ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~', 'url') = cast('ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~' as blob);
select '13_05', crypto_decode('%21%2A%5C%28%29%3B%3A%40%26%3D%2B%24%2C%2F%3F%23%5B%5D', 'url') = cast('!*\();:@&=+$,/?#[]' as blob);
select '13_06', crypto_decode('https%3A%2F%2Fwww.google.lu%2Fsearch%3Fq%3Dhello%2Bworld%26ie%3DUTF-8', 'url') = cast('https://www.google.lu/search?q=hello+world&ie=UTF-8' as blob);
select '13_07', crypto_decode('one%20two%20three', 'url') = cast('one two three' as blob);
select '13_08', crypto_decode('%D1%8D%D0%B9%2C%20%D0%BC%D0%B8%D1%80%21', 'url') = cast('эй, мир!' as blob);
select '13_09', crypto_decode('%28%E0%B2%A0_%E0%B2%A0%29', 'url') = cast('(ಠ_ಠ)' as blob);
select '13_10', crypto_decode('The%20quick%20brown%20%f0%9f%a6%8a%20jumps%20over%2013%20lazy%20%f0%9f%90%b6.', 'url') = cast('The quick brown 🦊 jumps over 13 lazy 🐶.' as blob);
select '13_11', crypto_decode('one+two+three', 'url') = cast('one two three' as blob);

select '14_01', crypto_encode(null, 'base85') is null;
select '14_02', crypto_encode('', 'base85') = '';
select '14_03', crypto_encode('hell', 'base85') = 'BOu!r';
select '14_04', crypto_encode('hello', 'base85') = 'BOu!rDZ';
select '14_05', crypto_encode('hellow', 'base85') = 'BOu!rDg)';
select '14_06', crypto_encode('hellowo', 'base85') = 'BOu!rDg-+';
select '14_07', crypto_encode('эй, мир!', 'base85') = 'd>cM`/0N<Pd(%JR+T';
select '14_08', crypto_encode('(ಠ_ಠ)', 'base85') = '.,_D(?f85_.0';
select '14_09', crypto_encode('The quick brown 🦊 jumps over 13 lazy 🐶.', 'base85') = '<+ohcEHPu*CER),Dg-(An=QS8+DQ%9E-!.?G%G\:0f''qg@=!2An=PfN/c';

select '15_01', crypto_decode(null, 'base85') is null;
select '15_02', crypto_decode('', 'base85') = cast('' as blob);
select '15_03', crypto_decode('BOu!r', 'base85') = cast('hell' as blob);
select '15_04', crypto_decode('BOu!rDZ', 'base85') = cast('hello' as blob);
select '15_05', crypto_decode('BOu!rDg)', 'base85') = cast('hellow' as blob);
select '15_06', crypto_decode('BOu!rDg-+', 'base85') = cast('hellowo' as blob);
select '15_07', crypto_decode('d>cM`/0N<Pd(%JR+T', 'base85') = cast('эй, мир!' as blob);
select '15_08', crypto_decode('.,_D(?f85_.0', 'base85') = cast('(ಠ_ಠ)' as blob);
select '15_09', crypto_decode('<+ohcEHPu*CER),Dg-(An=QS8+DQ%9E-!.?G%G\:0f''qg@=!2An=PfN/c', 'base85') = cast('The quick brown 🦊 jumps over 13 lazy 🐶.' as blob);

select '16_01', crypto_blake3(null) is NULL;
select '16_02', hex(crypto_blake3('')) = upper('af1349b9f5f9a1a6a0404dea36dcc9499bcb25c9adc112b7cc9a93cae41f3262');
select '16_03', hex(crypto_blake3('abc')) = upper('6437b3ac38465133ffb63b75273a8db548c558465d79db03fd359c6cd5bd9d85');
