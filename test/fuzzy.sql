-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/fuzzy

-- Damerau-Levenshtein distance
select '01', fuzzy_damlev('abc', 'abc') = 0;
select '02', fuzzy_damlev('abc', '') = 3;
select '03', fuzzy_damlev('', 'abc') = 3;
select '04', fuzzy_damlev('abc', 'ab') = 1;
select '05', fuzzy_damlev('abc', 'abcd') = 1;
select '06', fuzzy_damlev('abc', 'acb') = 1;
select '07', fuzzy_damlev('abc', 'ca') = 2;

-- Hamming distance
select '21', fuzzy_hamming('abc', 'abc') = 0;
select '22', fuzzy_hamming('abc', '') = -1;
select '23', fuzzy_hamming('', 'abc') = -1;
select '24', fuzzy_hamming('hello', 'hellp') = 1;
select '25', fuzzy_hamming('hello', 'heloh') = 2;

-- Jaro-Winkler distance
select '31', fuzzy_jarowin('abc', 'abc') = 1.0;
select '32', fuzzy_jarowin('abc', '') = 0.0;
select '33', fuzzy_jarowin('', 'abc') = 0.0;
select '34', round(fuzzy_jarowin('my string', 'my tsring'), 3) = 0.974;
select '35', round(fuzzy_jarowin('my string', 'my ntrisg'), 3) = 0.896;

-- Levenshtein distance
select '41', fuzzy_leven('abc', 'abc') = 0;
select '42', fuzzy_leven('abc', '') = 3;
select '43', fuzzy_leven('', 'abc') = 3;
select '44', fuzzy_leven('abc', 'ab') = 1;
select '45', fuzzy_leven('abc', 'abcd') = 1;
select '46', fuzzy_leven('abc', 'acb') = 2;
select '47', fuzzy_leven('abc', 'ca') = 3;

-- Optimal String Alignment distance
select '51', fuzzy_osadist('abc', 'abc') = 0;
select '52', fuzzy_osadist('abc', '') = 3;
select '53', fuzzy_osadist('', 'abc') = 3;
select '54', fuzzy_osadist('abc', 'ab') = 1;
select '55', fuzzy_osadist('abc', 'abcd') = 1;
select '56', fuzzy_osadist('abc', 'acb') = 2;
select '57', fuzzy_osadist('abc', 'ca') = 3;

-- Spellcheck edit distance
select '61', fuzzy_editdist('abc', 'abc') = 0;
select '62', fuzzy_editdist('abc', '') = 300;
select '63', fuzzy_editdist('', 'abc') = 75;
select '64', fuzzy_editdist('abc', 'ab') = 100;
select '65', fuzzy_editdist('abc', 'abcd') = 25;
select '66', fuzzy_editdist('abc', 'acb') = 110;
select '67', fuzzy_editdist('abc', 'ca') = 225;

-- Spellcheck phonetic code
select '101', fuzzy_phonetic(null) is null;
select '102', fuzzy_phonetic('') = '';
select '103', fuzzy_phonetic('phonetics') = 'BAMADAC';
select '104', fuzzy_phonetic('is') = 'AC';
select '105', fuzzy_phonetic('awesome') = 'ABACAMA';

-- Soundex code
select '111', fuzzy_soundex(null) is null;
select '112', fuzzy_soundex('') = '';
select '113', fuzzy_soundex('phonetics') = 'P532';
select '114', fuzzy_soundex('is') = 'I200';
select '115', fuzzy_soundex('awesome') = 'A250';

-- Refined Soundex code
select '121', fuzzy_rsoundex(null) is null;
select '122', fuzzy_rsoundex('') = '';
select '123', fuzzy_rsoundex('phonetics') = 'P1080603';
select '124', fuzzy_rsoundex('is') = 'I03';
select '125', fuzzy_rsoundex('awesome') = 'A03080';

-- Caverphone phonetic code
select '131', fuzzy_caver(null) is null;
select '132', fuzzy_caver('') = '';
select '133', fuzzy_caver('phonetics') = 'FNTKS11111';
select '134', fuzzy_caver('is') = 'AS11111111';
select '135', fuzzy_caver('awesome') = 'AWSM111111';
