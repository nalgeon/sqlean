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
select '6_08', crypto_encode('abcd', 'base64') = 'YWJjZA==';
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
-- an all-zero group is encoded as a single 'z',
-- but only if the group is full
select '14_10', crypto_encode(x'00000000', 'base85') = 'z';
select '14_11', crypto_encode(x'0000000000000000', 'base85') = 'zz';
select '14_12', crypto_encode(x'00', 'base85') = '!!';
select '14_13', crypto_encode(x'000000', 'base85') = '!!!!';
select '14_14', crypto_encode(x'0000000000', 'base85') = 'z!!';
select '14_15', crypto_encode(x'000000006162636400000000', 'base85') = 'z@:E_Wz';
-- the largest tuple
select '14_16', crypto_encode(x'ffffffff', 'base85') = 's8W-!';
select '14_17', crypto_encode(x'ffffff', 'base85') = 's8W*';

select '15_01', crypto_decode(null, 'base85') is null;
select '15_02', crypto_decode('', 'base85') = cast('' as blob);
select '15_03', crypto_decode('BOu!r', 'base85') = cast('hell' as blob);
select '15_04', crypto_decode('BOu!rDZ', 'base85') = cast('hello' as blob);
select '15_05', crypto_decode('BOu!rDg)', 'base85') = cast('hellow' as blob);
select '15_06', crypto_decode('BOu!rDg-+', 'base85') = cast('hellowo' as blob);
select '15_07', crypto_decode('d>cM`/0N<Pd(%JR+T', 'base85') = cast('эй, мир!' as blob);
select '15_08', crypto_decode('.,_D(?f85_.0', 'base85') = cast('(ಠ_ಠ)' as blob);
select '15_09', crypto_decode('<+ohcEHPu*CER),Dg-(An=QS8+DQ%9E-!.?G%G\:0f''qg@=!2An=PfN/c', 'base85') = cast('The quick brown 🦊 jumps over 13 lazy 🐶.' as blob);
-- 'z' expands into four zero bytes
select '15_10', crypto_decode('z', 'base85') = x'00000000';
select '15_11', crypto_decode('zz', 'base85') = x'0000000000000000';
select '15_12', crypto_decode('z!!', 'base85') = x'0000000000';
select '15_13', crypto_decode('z@:E_Wz', 'base85') = x'000000006162636400000000';
-- the same tuple spelled out in full is also valid
select '15_14', crypto_decode('!!!!!', 'base85') = x'00000000';
select '15_15', crypto_decode('!!', 'base85') = x'00';
-- partial groups
select '15_16', crypto_decode('@/', 'base85') = cast('a' as blob);
select '15_17', crypto_decode('@:B', 'base85') = cast('ab' as blob);
select '15_18', crypto_decode('@:E^', 'base85') = cast('abc' as blob);
select '15_19', crypto_decode('@:E_W', 'base85') = cast('abcd' as blob);
select '15_20', crypto_decode('@:E_WAH', 'base85') = cast('abcde' as blob);
-- a full group followed by a partial one
select '15_21', crypto_decode('F*2M7/c', 'base85') = cast('sure.' as blob);
-- the largest tuple
select '15_22', crypto_decode('s8W-!', 'base85') = x'ffffffff';
select '15_23', crypto_decode('s8W*', 'base85') = x'ffffff';
-- round trips
select '15_24', crypto_decode(crypto_encode(x'0000000000', 'base85'), 'base85') = x'0000000000';
select '15_25', crypto_decode(crypto_encode('The quick brown fox', 'base85'), 'base85') = cast('The quick brown fox' as blob);

select '16_01', crypto_blake3(null) is NULL;
select '16_02', hex(crypto_blake3('')) = upper('af1349b9f5f9a1a6a0404dea36dcc9499bcb25c9adc112b7cc9a93cae41f3262');
select '16_03', hex(crypto_blake3('abc')) = upper('6437b3ac38465133ffb63b75273a8db548c558465d79db03fd359c6cd5bd9d85');

select '17_01', crypto_xxh32(null) is NULL;
select '17_02', hex(crypto_xxh32('')) = '02CC5D05';
select '17_03', hex(crypto_xxh32('abc')) = '32D153FF';
select '17_04', hex(crypto_xxh32('The quick brown 🦊 jumps over 13 lazy 🐶.')) = '2C4A0402';

select '18_01', crypto_xxh64(null) is NULL;
select '18_02', hex(crypto_xxh64('')) = 'EF46DB3751D8E999';
select '18_03', hex(crypto_xxh64('abc')) = '44BC2CF5AD770999';
select '18_04', hex(crypto_xxh64('The quick brown 🦊 jumps over 13 lazy 🐶.')) = '1ED3E109F48F652C';

select '19_01', crypto_xxh3_64(null) is NULL;
select '19_02', hex(crypto_xxh3_64('')) = '2D06800538D394C2';
select '19_03', hex(crypto_xxh3_64('abc')) = '78AF5F94892F3950';
select '19_04', hex(crypto_xxh3_64('The quick brown 🦊 jumps over 13 lazy 🐶.')) = '09A3FF1CBDB28B84';

select '20_01', crypto_xxh3_128(null) is NULL;
select '20_02', hex(crypto_xxh3_128('')) = '99AA06D3014798D86001C324468D497F';
select '20_03', hex(crypto_xxh3_128('abc')) = '06B05AB6733A618578AF5F94892F3950';
select '20_04', hex(crypto_xxh3_128('The quick brown 🦊 jumps over 13 lazy 🐶.')) = 'E688EC7B9C40A08C87C989C0484E449E';

-- aliases
select '21_01', md5('abc') = crypto_md5('abc');
select '21_02', sha1('abc') = crypto_sha1('abc');
select '21_03', sha256('abc') = crypto_sha256('abc');
select '21_04', sha384('abc') = crypto_sha384('abc');
select '21_05', sha512('abc') = crypto_sha512('abc');
select '21_06', blake3('abc') = crypto_blake3('abc');
select '21_07', xxh32('abc') = crypto_xxh32('abc');
select '21_08', xxh64('abc') = crypto_xxh64('abc');
select '21_09', xxh3_64('abc') = crypto_xxh3_64('abc');
select '21_10', xxh3_128('abc') = crypto_xxh3_128('abc');
select '21_11', encode('hello', 'base64') = crypto_encode('hello', 'base64');
select '21_12', decode('aGVsbG8=', 'base64') = crypto_decode('aGVsbG8=', 'base64');

-- hash inputs: a blob is hashed as is, everything else as text
select '22_01', md5(x'616263') = md5('abc');
select '22_02', sha1(x'616263') = sha1('abc');
select '22_03', sha256(x'616263') = sha256('abc');
select '22_04', sha384(x'616263') = sha384('abc');
select '22_05', sha512(x'616263') = sha512('abc');
select '22_06', blake3(x'616263') = blake3('abc');
select '22_07', xxh32(x'616263') = xxh32('abc');
select '22_08', xxh64(x'616263') = xxh64('abc');
select '22_09', xxh3_64(x'616263') = xxh3_64('abc');
select '22_10', xxh3_128(x'616263') = xxh3_128('abc');
select '22_11', md5(zeroblob(0)) = md5('');
select '22_12', sha1(zeroblob(0)) = sha1('');
select '22_13', sha256(zeroblob(0)) = sha256('');
select '22_14', sha384(zeroblob(0)) = sha384('');
select '22_15', sha512(zeroblob(0)) = sha512('');
select '22_16', blake3(zeroblob(0)) = blake3('');
select '22_17', xxh32(zeroblob(0)) = xxh32('');
select '22_18', xxh64(zeroblob(0)) = xxh64('');
select '22_19', xxh3_64(zeroblob(0)) = xxh3_64('');
select '22_20', xxh3_128(zeroblob(0)) = xxh3_128('');
select '22_21', md5(42) = md5('42');
select '22_22', md5(-42) = md5('-42');
select '22_23', md5(4.5) = md5('4.5');
select '22_24', sha256(42) = sha256('42');
select '22_25', typeof(md5('abc')) = 'blob';
select '22_26', length(md5('abc')) = 16;
select '22_27', length(sha1('abc')) = 20;
select '22_28', length(sha256('abc')) = 32;
select '22_29', length(sha384('abc')) = 48;
select '22_30', length(sha512('abc')) = 64;
select '22_31', length(blake3('abc')) = 32;
select '22_32', length(xxh32('abc')) = 4;
select '22_33', length(xxh64('abc')) = 8;
select '22_34', length(xxh3_64('abc')) = 8;
select '22_35', length(xxh3_128('abc')) = 16;
-- multi-block inputs (NIST vectors and more)
select '22_36', hex(md5('abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq')) = '8215EF0796A20BCAAAE116D3876C664A';
select '22_37', hex(sha1('abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq')) = '84983E441C3BD26EBAAE4AA1F95129E5E54670F1';
select '22_38', hex(sha256('abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq')) = '248D6A61D20638B8E5C026930C3E6039A33CE45964FF2167F6ECEDD419DB06C1';
select '22_39', hex(sha384('abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq')) = '3391FDDDFC8DC7393707A65B1B4709397CF8B1D162AF05ABFE8F450DE5F36BC6B0455A8520BC4E6F5FE95B1FE3C8452B';
select '22_40', hex(sha512('abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq')) = '204A8FC6DDA82F0A0CED7BEB8E08A41657C16EF468B228A8279BE331A703C33596FD15C13B1B07F9AA1D3BEA57789CA031AD85C7A71DD70354EC631238CA3445';
select '22_41', hex(blake3('abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq')) = 'C19012CC2AAF0DC3D8E5C45A1B79114D2DF42ABB2A410BF54BE09E891AF06FF8';
select '22_42', hex(xxh32('abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq')) = '89EA60C3';
select '22_43', hex(xxh64('abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq')) = 'F06103773E8585DF';
select '22_44', hex(xxh3_64('abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq')) = '5BBCBBABCDCC3D3F';
select '22_45', hex(xxh3_128('abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq')) = '3D62D22A5169B016C0D894FD4828A1A7';
select '22_46', hex(md5('abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu')) = '03DD8807A93175FB062DFB55DC7D359C';
select '22_47', hex(sha1('abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu')) = 'A49B2446A02C645BF419F995B67091253A04A259';
select '22_48', hex(sha256('abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu')) = 'CF5B16A778AF8380036CE59E7B0492370B249B11E8F07A51AFAC45037AFEE9D1';
select '22_49', hex(sha384('abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu')) = '09330C33F71147E83D192FC782CD1B4753111B173B3B05D22FA08086E3B0F712FCC7C71A557E2DB966C3E9FA91746039';
select '22_50', hex(sha512('abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu')) = '8E959B75DAE313DA8CF4F72814FC143F8F7779C6EB9F7FA17299AEADB6889018501D289E4900F7E4331B99DEC4B5433AC7D329EEB6DD26545E96E55B874BE909';
select '22_51', hex(blake3('abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu')) = '553E1AA2A477CB3166E6AB38C12D59F6C5017F0885AAF079F217DA00CFCA363F';
select '22_52', hex(xxh32('abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu')) = '590B4165';
select '22_53', hex(xxh64('abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu')) = 'BAFC02122DED1D21';
select '22_54', hex(xxh3_64('abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu')) = '458619CD5260AA9A';
select '22_55', hex(xxh3_128('abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu')) = '97D535CB0C62BF199C12D0B7E499EDB8';
select '22_56', hex(md5('The quick brown 🦊 jumps over 13 lazy 🐶.')) = '0A8A1860C93EA376F9980B500298F75D';
select '22_57', hex(sha1('The quick brown 🦊 jumps over 13 lazy 🐶.')) = 'D490327FE0C8318CE43421E2DB908B3AF9E65942';
select '22_58', hex(sha256('The quick brown 🦊 jumps over 13 lazy 🐶.')) = 'FD3B8EE333CD9DB16B75FBAA4ADFB04A823509F0F1F873A491A03ADB09EB3289';
select '22_59', hex(sha384('The quick brown 🦊 jumps over 13 lazy 🐶.')) = '1E7D4E2AD0182755DF93F7F247F5B52D47D8037191E5BA82EF4F9A5DDFDEE9BC65E83D43A6EC01469D8DEBDB546AD007';
select '22_60', hex(sha512('The quick brown 🦊 jumps over 13 lazy 🐶.')) = 'B1FBF8090360224A28105AD736031287CF562ACF485CD1397DED308821862BC017B074D9B5A7E09CCC8DD5B940DA8C0EB1E86057D9AC6D2187F0423F4A68945C';
select '22_61', hex(blake3('The quick brown 🦊 jumps over 13 lazy 🐶.')) = '9B0F4109AB1DAD89430F71196F5A176367C5CC9D6EDBC34C23D06E8D6184A1C2';

-- a million zero bytes
select '23_01', hex(md5(zeroblob(1000000))) = '879F4BBA57ED37C9EC5E5AEDF9864698';
select '23_02', hex(sha1(zeroblob(1000000))) = 'BEF3595266A65A2FF36B700A75E8ED95C68210B6';
select '23_03', hex(sha256(zeroblob(1000000))) = 'D29751F2649B32FF572B5E0A9F541EA660A50F94FF0BEEDFB0B692B924CC8025';
select '23_04', hex(sha384(zeroblob(1000000))) = '8A1979F9049B3FFF15EA3A43A4CF84C634FD14ACAD1C333FECB72C588B68868B66A994386DC0CD1687B9EE2E34983B81';
select '23_05', hex(sha512(zeroblob(1000000))) = 'CE044BC9FD43269D5BBC946CBEBC3BB711341115CC4ABDF2EDBC3FF2C57AD4B15DEB699BDA257FEA5AEF9C6E55FCF4CF9DC25A8C3CE25F2EFE90908379BFF7ED';
select '23_06', hex(blake3(zeroblob(1000000))) = 'C211BB2E5AFBD0EFA21659D5578EA30217D5382734BE1B494FAF705D9AA202A1';
select '23_07', hex(xxh32(zeroblob(1000000))) = '5FFF0BCB';
select '23_08', hex(xxh64(zeroblob(1000000))) = '8A76D36D39CAAECC';
select '23_09', hex(xxh3_64(zeroblob(1000000))) = '9D648391F361D99A';
select '23_10', hex(xxh3_128(zeroblob(1000000))) = 'EF233FC372A159319D648391F361D99A';

-- encode: a blob is encoded as is, everything else as text
select '24_01', encode(42, 'hex') = '3432';
select '24_02', encode(-42, 'hex') = '2d3432';
select '24_03', encode(4.5, 'hex') = '342e35';
select '24_04', encode(x'610062', 'hex') = '610062';
select '24_05', encode(zeroblob(3), 'base64') = 'AAAA';
select '24_06', typeof(encode('', 'hex')) = 'text';
select '24_07', typeof(encode(x'', 'hex')) = 'text';
select '24_08', typeof(encode('hello', 'base64')) = 'text';
select '24_09', encode(null, 'base32') is null;
select '24_10', encode(null, 'base64') is null;
select '24_11', encode(null, 'base85') is null;
select '24_12', encode(null, 'hex') is null;
select '24_13', encode(null, 'url') is null;
-- decode: the input is read as text
select '24_14', decode(6869, 'hex') = x'6869';
select '24_15', decode(cast('6869' as blob), 'hex') = x'6869';
select '24_16', decode(cast('aGVsbG8=' as blob), 'base64') = cast('hello' as blob);
select '24_17', typeof(decode('', 'hex')) = 'blob';
select '24_18', typeof(decode('6869', 'hex')) = 'blob';
select '24_19', decode(null, 'base32') is null;
select '24_20', decode(null, 'base64') is null;
select '24_21', decode(null, 'base85') is null;
select '24_22', decode(null, 'hex') is null;
select '24_23', decode(null, 'url') is null;

-- the format is an exact name; an unknown one is an error (not tested here)
select '25_01', encode('hello', 'base64') = 'aGVsbG8=';
-- a NULL format gives NULL
select '25_02', encode('hello', null) is null;
select '25_03', decode('aGVsbG8=', null) is null;
-- url encoding escapes the NUL byte
select '25_04', encode(x'00', 'url') = '%00';
select '25_05', encode(x'610062', 'url') = 'a%00b';
-- invalid encoded data gives NULL
select '25_06', decode('MY=MY===', 'base32') is null;
select '25_07', decode('my======', 'base32') is null;
select '25_08', decode('YQ=A', 'base64') is null;
select '25_09', decode('Zg', 'base64') is null;
select '25_10', decode('z!', 'base85') is null;
select '25_11', decode('zz', 'hex') is null;
select '25_12', decode('abc', 'hex') is null;
select '25_13', decode('%4', 'url') is null;
select '25_14', decode('=', 'base64') is null;

-- round trips over every byte value
select '26_01', decode(encode(x'000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff', 'base32'), 'base32') = x'000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff';
select '26_02', decode(encode(x'000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff', 'base64'), 'base64') = x'000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff';
select '26_03', decode(encode(x'000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff', 'base85'), 'base85') = x'000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff';
select '26_04', decode(encode(x'000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff', 'hex'), 'hex') = x'000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff';
select '26_05', decode(encode(x'0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff', 'url'), 'url') = x'0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff';
-- round trips over a million bytes
select '26_06', decode(encode(zeroblob(1000000), 'base32'), 'base32') = zeroblob(1000000);
select '26_07', decode(encode(zeroblob(1000000), 'base64'), 'base64') = zeroblob(1000000);
select '26_08', decode(encode(zeroblob(1000000), 'base85'), 'base85') = zeroblob(1000000);
select '26_09', decode(encode(zeroblob(1000000), 'hex'), 'hex') = zeroblob(1000000);
select '26_10', decode(encode(zeroblob(1000000), 'url'), 'url') = zeroblob(1000000);
