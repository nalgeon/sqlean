-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/fuzzy

-- Errors (NULL and non-ASCII arguments to the distance functions, non-ASCII
-- arguments to the phonetic functions) can't be asserted here, so they are
-- covered in test/fuzzy/fuzzy.test.c.

-- fuzzy_damlev
select '01_01', fuzzy_damlev('abc', 'abc') = 0;
select '01_02', fuzzy_damlev('abc', '') = 3;
select '01_03', fuzzy_damlev('', 'abc') = 3;
select '01_04', fuzzy_damlev('', '') = 0;
select '01_05', fuzzy_damlev('abc', 'ab') = 1;
select '01_06', fuzzy_damlev('abc', 'abcd') = 1;
select '01_07', fuzzy_damlev('kitten', 'sitting') = 3;
select '01_08', fuzzy_damlev('awesome', 'aewsme') = 2;
-- a transposition costs 1
select '01_09', fuzzy_damlev('abc', 'acb') = 1;
select '01_10', fuzzy_damlev('ab', 'ba') = 1;
-- unlike OSA, a substring can be edited more than once
select '01_11', fuzzy_damlev('abc', 'ca') = 2;
select '01_12', fuzzy_damlev('ca', 'abc') = 2;
-- case-sensitive
select '01_13', fuzzy_damlev('abc', 'ABC') = 3;

-- fuzzy_editdist
-- costs depend on the character classes, see spellfix.c
select '02_01', fuzzy_editdist('abc', 'abc') = 0;
select '02_02', fuzzy_editdist('abc', '') = 300;
select '02_03', fuzzy_editdist('', 'abc') = 75;
select '02_04', fuzzy_editdist('', '') = 0;
select '02_05', fuzzy_editdist('abc', 'ab') = 100;
select '02_06', fuzzy_editdist('abc', 'abcd') = 25;
select '02_07', fuzzy_editdist('abc', 'acb') = 110;
select '02_08', fuzzy_editdist('abc', 'ca') = 225;
select '02_09', fuzzy_editdist('kitten', 'sitting') = 105;
select '02_10', fuzzy_editdist('awesome', 'aewsme') = 215;
-- case is ignored
select '02_11', fuzzy_editdist('abc', 'ABC') = 0;
-- substitution within a character class is cheaper
select '02_12', fuzzy_editdist('bat', 'bad') = 40;
select '02_13', fuzzy_editdist('bat', 'bap') = 75;
select '02_14', fuzzy_editdist('bat', 'ba1') = 100;
-- inserting at the end is cheaper than at the start
select '02_15', fuzzy_editdist('abc', 'abcx') = 12;
select '02_16', fuzzy_editdist('abc', 'xabc') = 100;
-- a trailing * matches any suffix
select '02_17', fuzzy_editdist('ab*', 'abxyz') = 0;
select '02_18', fuzzy_editdist('ax*', 'abxyz') = 75;
select '02_19', fuzzy_editdist('*', 'abc') = 0;
-- a * that matches nothing is deleted (current behavior)
select '02_20', fuzzy_editdist('ab*', 'ab') = 100;
select '02_21', fuzzy_editdist('*', '') = 50;

-- fuzzy_hamming
select '03_01', fuzzy_hamming('abc', 'abc') = 0;
select '03_02', fuzzy_hamming('', '') = 0;
select '03_03', fuzzy_hamming('hello', 'hellp') = 1;
select '03_04', fuzzy_hamming('hello', 'heloh') = 2;
select '03_05', fuzzy_hamming('karolin', 'kathrin') = 3;
select '03_06', fuzzy_hamming('abc', 'ABC') = 3;
-- strings of different lengths are NULL
select '03_07', fuzzy_hamming('abc', '') is null;
select '03_08', fuzzy_hamming('', 'abc') is null;
select '03_09', fuzzy_hamming('abc', 'abcd') is null;

-- fuzzy_jarowin
select '04_01', fuzzy_jarowin('abc', 'abc') = 1.0;
select '04_02', fuzzy_jarowin('abc', '') = 0.0;
select '04_03', fuzzy_jarowin('', 'abc') = 0.0;
select '04_04', fuzzy_jarowin('', '') = 1.0;
select '04_05', fuzzy_jarowin('abc', 'xyz') = 0.0;
select '04_06', fuzzy_jarowin('ab', 'ab') = 1.0;
select '04_07', round(fuzzy_jarowin('my string', 'my tsring'), 3) = 0.974;
select '04_08', round(fuzzy_jarowin('my string', 'my ntrisg'), 3) = 0.896;
select '04_09', round(fuzzy_jarowin('martha', 'marhta'), 3) = 0.961;
select '04_10', round(fuzzy_jarowin('dwayne', 'duane'), 3) = 0.840;
select '04_11', round(fuzzy_jarowin('dixon', 'dicksonx'), 3) = 0.813;
select '04_12', round(fuzzy_jarowin('ab', 'acb'), 3) = 0.650;
select '04_13', round(fuzzy_jarowin('awesome', 'aewsme'), 3) = 0.907;
-- the similarity is symmetric
select '04_14', round(fuzzy_jarowin('dicksonx', 'dixon'), 3) = 0.813;
select '04_15', round(fuzzy_jarowin('acb', 'ab'), 3) = 0.650;
-- the match window is max(len)/2 - 1 characters, so 'b' doesn't match
select '04_16', fuzzy_jarowin('ab', 'b') = 0.0;
select '04_17', fuzzy_jarowin('b', 'ab') = 0.0;
-- the prefix boost is limited to 4 characters
select '04_18', round(fuzzy_jarowin('abcdefgh', 'abcdefhg'), 3) = 0.975;

-- fuzzy_leven
select '05_01', fuzzy_leven('abc', 'abc') = 0;
select '05_02', fuzzy_leven('abc', '') = 3;
select '05_03', fuzzy_leven('', 'abc') = 3;
select '05_04', fuzzy_leven('', '') = 0;
select '05_05', fuzzy_leven('abc', 'ab') = 1;
select '05_06', fuzzy_leven('abc', 'abcd') = 1;
select '05_07', fuzzy_leven('abc', 'acb') = 2;
select '05_08', fuzzy_leven('abc', 'ca') = 3;
select '05_09', fuzzy_leven('kitten', 'sitting') = 3;
select '05_10', fuzzy_leven('flaw', 'lawn') = 2;
select '05_11', fuzzy_leven('awesome', 'aewsme') = 3;
select '05_12', fuzzy_leven('abc', 'ABC') = 3;

-- fuzzy_osadist
select '06_01', fuzzy_osadist('abc', 'abc') = 0;
select '06_02', fuzzy_osadist('abc', '') = 3;
select '06_03', fuzzy_osadist('', 'abc') = 3;
select '06_04', fuzzy_osadist('', '') = 0;
select '06_05', fuzzy_osadist('abc', 'ab') = 1;
select '06_06', fuzzy_osadist('abc', 'abcd') = 1;
select '06_07', fuzzy_osadist('kitten', 'sitting') = 3;
-- unlike Damerau-Levenshtein, a substring is edited at most once
select '06_08', fuzzy_osadist('abc', 'ca') = 3;
select '06_09', fuzzy_osadist('ca', 'abc') = 3;
-- a transposition costs 1
select '06_10', fuzzy_osadist('abc', 'acb') = 1;
select '06_11', fuzzy_osadist('ab', 'ba') = 1;
select '06_12', fuzzy_osadist('martha', 'marhta') = 1;
select '06_13', fuzzy_osadist('awesome', 'aewsme') = 2;

-- fuzzy_caver
select '11_01', fuzzy_caver('') = '';
select '11_02', fuzzy_caver('phonetics') = 'FNTKS11111';
select '11_03', fuzzy_caver('is') = 'AS11111111';
select '11_04', fuzzy_caver('awesome') = 'AWSM111111';
-- examples from the Caverphone 2.0 specification
select '11_05', fuzzy_caver('Peter') = 'PTA1111111';
select '11_06', fuzzy_caver('ready') = 'RTA1111111';
select '11_07', fuzzy_caver('social') = 'SSA1111111';
select '11_08', fuzzy_caver('able') = 'APA1111111';
select '11_09', fuzzy_caver('Tedder') = 'TTA1111111';
select '11_10', fuzzy_caver('Karleen') = 'KLN1111111';
select '11_11', fuzzy_caver('Dyun') = 'TN11111111';
select '11_12', fuzzy_caver('STEVENSON') = 'STFNSN1111';
select '11_13', fuzzy_caver('Lee') = 'LA11111111';
-- a final mb is m2
select '11_14', fuzzy_caver('mb') = 'M111111111';
select '11_15', fuzzy_caver('mbmb') = 'MPM1111111';
-- an initial gn is 2n, an initial *ough is *ou2f
select '11_16', fuzzy_caver('Gnome') = 'NM11111111';
select '11_17', fuzzy_caver('Tough') = 'TF11111111';
-- common code AT11111111
select '11_18', fuzzy_caver('add') = 'AT11111111';
select '11_19', fuzzy_caver('earth') = 'AT11111111';
select '11_20', fuzzy_caver('heart') = 'AT11111111';
select '11_21', fuzzy_caver('old') = 'AT11111111';
-- case-insensitive
select '11_22', fuzzy_caver('Lee') = fuzzy_caver('lee');
-- anything but letters is ignored
select '11_23', fuzzy_caver('O''Brien') = 'APRN111111';
select '11_24', fuzzy_caver('van der Berg') = 'FNTPK11111';
select '11_25', fuzzy_caver('123') = '1111111111';
-- the code is cut to 10 characters
select '11_26', fuzzy_caver('abcdefghijklmnopqrstuvwxyz') = 'APKTFKMNPK';

-- fuzzy_phonetic
select '12_01', fuzzy_phonetic('') = '';
select '12_02', fuzzy_phonetic('phonetics') = 'BAMADAC';
select '12_03', fuzzy_phonetic('PHONETICS') = 'BAMADAC';
select '12_04', fuzzy_phonetic('is') = 'AC';
select '12_05', fuzzy_phonetic('awesome') = 'ABACAMA';
-- an initial kn or gn is n
select '12_06', fuzzy_phonetic('knight') = 'MACD';
select '12_07', fuzzy_phonetic('gnome') = 'MAMA';
-- unless the word is too short (current behavior)
select '12_08', fuzzy_phonetic('kn') = 'CM';
-- w before r, t before ch, d before j or g are silent
select '12_09', fuzzy_phonetic('wright') = 'RCD';
select '12_10', fuzzy_phonetic('match') = 'MAC';
select '12_11', fuzzy_phonetic('edge') = 'ACA';
-- spaces are skipped, digits are kept, other characters only after a digit
select '12_12', fuzzy_phonetic('hello world') = 'LBRLD';
select '12_13', fuzzy_phonetic('a1b2') = 'A9B9';
select '12_14', fuzzy_phonetic(1.5) = '9?9';

-- fuzzy_soundex
select '13_01', fuzzy_soundex('') = '';
select '13_02', fuzzy_soundex('phonetics') = 'P532';
select '13_03', fuzzy_soundex('is') = 'I200';
select '13_04', fuzzy_soundex('awesome') = 'A250';
-- examples from the Soundex specification
select '13_05', fuzzy_soundex('Robert') = 'R163';
select '13_06', fuzzy_soundex('Rupert') = 'R163';
select '13_07', fuzzy_soundex('Honeyman') = 'H555';
select '13_08', fuzzy_soundex('Washington') = 'W252';
select '13_09', fuzzy_soundex('Lee') = 'L000';
select '13_10', fuzzy_soundex('Lloyd') = 'L300';
-- the first letter's code counts as a previous digit
select '13_11', fuzzy_soundex('Pfister') = 'P236';
-- letters with the same code separated by h or w are coded once
select '13_12', fuzzy_soundex('Ashcraft') = 'A261';
select '13_13', fuzzy_soundex('Tymczak') = 'T522';
-- the first letter is uppercased
select '13_14', fuzzy_soundex('robert') = 'R163';
-- a short code is padded with zeros
select '13_15', fuzzy_soundex('a') = 'A000';
-- leading non-letters are skipped
select '13_16', fuzzy_soundex('1abc') = 'A120';
select '13_17', fuzzy_soundex(' a') = 'A000';
-- a string without letters has an empty code
select '13_19', fuzzy_soundex('123') = '';
-- h and w separate letters with the same code in any case
select '13_20', fuzzy_soundex('ASHCRAFT') = 'A261';
select '13_18', fuzzy_soundex('O''Hara') = 'O600';

-- fuzzy_rsoundex
select '14_01', fuzzy_rsoundex('') = '';
select '14_02', fuzzy_rsoundex('phonetics') = 'P1080603';
select '14_03', fuzzy_rsoundex('is') = 'I03';
select '14_04', fuzzy_rsoundex('awesome') = 'A03080';
select '14_05', fuzzy_rsoundex('Braz') = 'B1905';
select '14_06', fuzzy_rsoundex('Caren') = 'C30908';
select '14_07', fuzzy_rsoundex('Lloyd') = 'L706';
select '14_08', fuzzy_rsoundex('robert') = 'R901096';
select '14_09', fuzzy_rsoundex('a') = 'A0';
-- the code is not cut
select '14_10', length(fuzzy_rsoundex(replace(printf('%.*c', 500, 'x'), 'x', 'ab'))) = 1001;

-- fuzzy_translit
select '21_01', fuzzy_translit('') = '';
select '21_02', fuzzy_translit('abc') = 'abc';
select '21_03', fuzzy_translit('sí señor') = 'si senor';
select '21_04', fuzzy_translit('crème brûlée') = 'creme brulee';
select '21_05', fuzzy_translit('привет') = 'privet';
select '21_06', fuzzy_translit('Ελλάδα') = 'Ellada';
-- a character can become several
select '21_07', fuzzy_translit('Ærøskøbing') = 'AEroskobing';
select '21_08', fuzzy_translit('Straße') = 'Strasse';
select '21_09', fuzzy_translit('Œuvre') = 'OEuvre';
select '21_10', fuzzy_translit('ﬁnal') = 'final';
-- an unknown character is ?
select '21_11', fuzzy_translit('oh my 😅') = 'oh my ?';
select '21_12', fuzzy_translit('日本') = '??';
-- long strings
select '21_13', length(fuzzy_translit(printf('%.*c', 1000, 'x'))) = 1000;
select '21_14', fuzzy_translit(replace(printf('%.*c', 1000, 'x'), 'x', 'Œ')) = replace(printf('%.*c', 1000, 'x'), 'x', 'OE');
select '21_15', fuzzy_translit(replace(printf('%.*c', 1000, 'x'), 'x', 'ﬁ')) = replace(printf('%.*c', 1000, 'x'), 'x', 'fi');

-- fuzzy_script
-- ISO 15924: 215 Latin, 220 Cyrillic, 200 Greek, 125 Hebrew, 160 Arabic,
-- 998 mixed, 999 unknown
select '22_01', fuzzy_script('hello') = 215;
select '22_02', fuzzy_script('sí señor') = 215;
select '22_03', fuzzy_script('привет') = 220;
select '22_04', fuzzy_script('γεια') = 200;
select '22_05', fuzzy_script('שלום') = 125;
select '22_06', fuzzy_script('مرحبا') = 160;
select '22_07', fuzzy_script('hello привет') = 998;
-- digits alone are Latin
select '22_08', fuzzy_script('123') = 215;
select '22_09', fuzzy_script('привет 123') = 220;
select '22_10', fuzzy_script('') = 999;
select '22_11', fuzzy_script('日本') = 999;
select '22_12', fuzzy_script('!?') = 999;

-- aliases
select '31_01', dlevenshtein('abc', 'acb') = 1;
select '31_02', edit_distance('abc', 'ab') = 100;
select '31_03', hamming('hello', 'hellp') = 1;
select '31_04', jaro_winkler('abc', 'abc') = 1.0;
select '31_05', levenshtein('abc', 'acb') = 2;
select '31_06', osa_distance('abc', 'ab') = 1;
select '31_07', caverphone('awesome') = 'AWSM111111';
select '31_08', phonetic_hash('awesome') = 'ABACAMA';
select '31_09', soundex('awesome') = 'A250';
select '31_10', rsoundex('awesome') = 'A03080';
select '31_11', translit('привет') = 'privet';
select '31_12', script_code('привет') = 220;

-- null propagation
select '41_01', fuzzy_caver(null) is null;
select '41_02', fuzzy_phonetic(null) is null;
select '41_03', fuzzy_soundex(null) is null;
select '41_04', fuzzy_rsoundex(null) is null;
select '41_05', fuzzy_translit(null) is null;
select '41_06', fuzzy_script(null) is null;
select '41_07', caverphone(null) is null;
select '41_08', phonetic_hash(null) is null;
select '41_09', soundex(null) is null;
select '41_10', rsoundex(null) is null;
select '41_11', translit(null) is null;
select '41_12', script_code(null) is null;
select '41_13', fuzzy_damlev(null, 'abc') is null;
select '41_14', fuzzy_damlev('abc', null) is null;
select '41_15', fuzzy_editdist(null, 'abc') is null;
select '41_16', fuzzy_editdist('abc', null) is null;
select '41_17', fuzzy_hamming(null, 'abc') is null;
select '41_18', fuzzy_hamming('abc', null) is null;
select '41_19', fuzzy_jarowin(null, 'abc') is null;
select '41_20', fuzzy_jarowin('abc', null) is null;
select '41_21', fuzzy_leven(null, 'abc') is null;
select '41_22', fuzzy_leven('abc', null) is null;
select '41_23', fuzzy_osadist(null, 'abc') is null;
select '41_24', fuzzy_osadist('abc', null) is null;
select '41_25', dlevenshtein(null, null) is null;
select '41_26', edit_distance(null, null) is null;
select '41_27', hamming(null, null) is null;
select '41_28', jaro_winkler(null, null) is null;
select '41_29', levenshtein(null, null) is null;
select '41_30', osa_distance(null, null) is null;

-- bad data: numbers are converted to text
select '51_01', fuzzy_damlev(123, 124) = 1;
select '51_02', fuzzy_editdist(123, 124) = 40;
select '51_03', fuzzy_hamming(123, 124) = 1;
select '51_04', round(fuzzy_jarowin(123, 124), 3) = 0.822;
select '51_05', fuzzy_leven(1.5, 1.25) = 1;
select '51_06', fuzzy_osadist(123, 124) = 1;
select '51_07', fuzzy_caver(123) = '1111111111';
select '51_08', fuzzy_phonetic(123) = '9';
select '51_09', fuzzy_soundex(123) = '';
select '51_10', fuzzy_rsoundex(123) = '';
select '51_11', fuzzy_translit(123) = '123';
select '51_12', fuzzy_script(123) = 215;
-- bad data: blobs are NULL
select '52_01', fuzzy_damlev(x'616263', 'abd') is null;
select '52_02', fuzzy_editdist(x'616263', 'abd') is null;
select '52_03', fuzzy_hamming(x'616263', 'abd') is null;
select '52_04', fuzzy_jarowin('abc', x'616263') is null;
select '52_05', fuzzy_leven(x'616263', x'616264') is null;
select '52_06', fuzzy_osadist(x'616263', 'abd') is null;
select '52_07', fuzzy_caver(x'616263') is null;
select '52_08', fuzzy_phonetic(x'616263') is null;
select '52_09', fuzzy_soundex(x'616263') is null;
select '52_10', fuzzy_rsoundex(x'616263') is null;
select '52_11', fuzzy_translit(x'616263') is null;
select '52_12', fuzzy_script(x'd0bf') is null;
select '52_13', fuzzy_translit(x'') is null;

-- bad data: invalid UTF-8
-- each invalid byte is a separate U+FFFD character
select '53_01', fuzzy_translit(cast(x'ff' as text)) = '?';
select '53_02', fuzzy_translit(cast(x'61ff62' as text)) = 'a?b';
-- a truncated sequence
select '53_03', fuzzy_translit(cast(x'61d0' as text)) = 'a?';
-- an overlong encoding
select '53_04', fuzzy_translit(cast(x'c0af' as text)) = '??';
-- a surrogate
select '53_05', fuzzy_translit(cast(x'eda080' as text)) = '???';
-- the longest invalid lead byte run
select '53_06', fuzzy_translit(cast(x'dfbfbfbfbfbfbfbfbf' as text)) = '????????';
select '53_07', fuzzy_leven(cast(x'ff' as text), cast(x'fe' as text)) = 0;
select '53_08', fuzzy_leven(cast(x'ff' as text), 'a') = 1;
select '53_09', fuzzy_hamming(cast(x'ffff' as text), 'ab') = 2;
select '53_10', fuzzy_script(cast(x'ff' as text)) = 999;
select '53_11', fuzzy_script(cast(x'80' as text) || 'привет') = 220;

-- non-ASCII text
-- the distances count characters, not bytes
select '54_01', fuzzy_damlev('привет', 'превед') = 2;
select '54_02', fuzzy_damlev('привет', 'пирвет') = 1;
select '54_03', fuzzy_hamming('café', 'cafe') = 1;
select '54_04', fuzzy_hamming('café', 'caf') is null;
select '54_05', round(fuzzy_jarowin('café', 'cafe'), 3) = 0.883;
select '54_06', fuzzy_leven('café', 'cafe') = 1;
select '54_07', fuzzy_leven('日本語', '日本') = 1;
select '54_08', fuzzy_osadist('привет', 'пирвет') = 1;
select '54_09', fuzzy_leven('😀', '😃') = 1;
-- the phonetic functions and editdist only accept ASCII
select '54_10', fuzzy_editdist('é', 'é') is null;
select '54_11', fuzzy_editdist('xé', 'x') is null;
select '54_12', fuzzy_editdist('x', 'xé') is null;
select '54_13', fuzzy_caver('café') is null;
select '54_14', fuzzy_phonetic('привет') is null;
select '54_15', fuzzy_soundex('café') is null;
select '54_16', fuzzy_rsoundex('café') is null;
-- transliterate first to use them with non-ASCII text
select '54_17', fuzzy_soundex(fuzzy_translit('Müller')) = 'M460';

-- text with an embedded NUL
-- NUL is an ordinary character
select '55_01', fuzzy_damlev('ab' || char(0) || 'c', 'ab') = 2;
select '55_02', fuzzy_editdist('ab' || char(0) || 'c', 'ab') = 200;
select '55_03', fuzzy_hamming('ab' || char(0) || 'c', 'ab') is null;
select '55_04', fuzzy_hamming('ab' || char(0), 'abc') = 1;
select '55_05', round(fuzzy_jarowin('abc' || char(0) || 'c', 'abc'), 3) = 0.907;
select '55_06', fuzzy_leven('ab' || char(0) || 'c', 'ab') = 2;
select '55_07', fuzzy_osadist('ab' || char(0) || 'c', 'ab') = 2;
-- and the phonetic functions ignore it as a non-letter
select '55_08', fuzzy_caver('ab' || char(0) || 'c') = 'APK1111111';
select '55_09', fuzzy_soundex('ab' || char(0) || 'c') = 'A120';
select '55_10', fuzzy_rsoundex('ab' || char(0) || 'c') = 'A0103';
select '55_11', fuzzy_phonetic('ab' || char(0) || 'cd') = 'ABCD';
select '55_12', hex(fuzzy_translit('a' || char(0) || 'b')) = '610062';
select '55_13', fuzzy_script('ab' || char(0) || 'привет') = 998;

-- long strings
-- 1000 x 'a' vs 1000 x 'b'
select '61_01', fuzzy_damlev(printf('%.*c', 1000, 'a'), printf('%.*c', 1000, 'b')) = 1000;
select '61_02', fuzzy_editdist(printf('%.*c', 1000, 'a'), printf('%.*c', 1000, 'b')) = 12113;
select '61_03', fuzzy_hamming(printf('%.*c', 1000, 'a'), printf('%.*c', 1000, 'b')) = 1000;
select '61_04', fuzzy_jarowin(printf('%.*c', 1000, 'a'), printf('%.*c', 1000, 'b')) = 0.0;
select '61_05', fuzzy_leven(printf('%.*c', 1000, 'a'), printf('%.*c', 1000, 'b')) = 1000;
select '61_06', fuzzy_osadist(printf('%.*c', 1000, 'a'), printf('%.*c', 1000, 'b')) = 1000;
-- vs a short or an empty string
select '61_07', fuzzy_damlev('a', printf('%.*c', 1000, 'a')) = 999;
select '61_08', fuzzy_editdist('', printf('%.*c', 1000, 'a')) = 2023;
select '61_09', fuzzy_editdist(printf('%.*c', 1000, 'a'), '') = 10090;
select '61_10', fuzzy_jarowin(printf('%.*c', 1000, 'a'), printf('%.*c', 1000, 'a')) = 1.0;
select '61_11', fuzzy_leven(printf('%.*c', 1000, 'a'), 'a') = 999;
select '61_12', fuzzy_osadist(printf('%.*c', 1000, 'a'), '') = 1000;
select '61_13', fuzzy_caver(printf('%.*c', 1000, 'b')) = 'P111111111';
select '61_14', length(fuzzy_phonetic(replace(printf('%.*c', 500, 'x'), 'x', 'ab'))) = 1000;
select '61_15', fuzzy_soundex(printf('%.*c', 1000, 'b')) = 'B000';

-- querying a table
create table words(word text);
insert into words values ('awesome'), ('awesom'), ('awful'), ('someone');
select '71_01', group_concat(word, ',') = 'awesom,awesome'
  from (select word from words where fuzzy_leven(word, 'awesome') <= 1 order by word);
select '71_02', group_concat(word, ',') = 'awesom,awesome'
  from (select word from words where fuzzy_soundex(word) = fuzzy_soundex('awsm') order by word);
select '71_03', word = 'awful' from words order by fuzzy_jarowin(word, 'awfully') desc limit 1;
drop table words;
