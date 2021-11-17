.load dist/fuzzy

-- Damerau-Levenshtein distance
select '01', dlevenshtein('abc', 'abc') = 0;
select '02', dlevenshtein('abc', '') = 3;
select '03', dlevenshtein('', 'abc') = 3;
select '04', dlevenshtein('abc', 'ab') = 1;
select '05', dlevenshtein('abc', 'abcd') = 1;
select '06', dlevenshtein('abc', 'acb') = 1;
select '07', dlevenshtein('abc', 'ca') = 2;

-- Hamming distance
select '21', hamming('abc', 'abc') = 0;
select '22', hamming('abc', '') = -1;
select '23', hamming('', 'abc') = -1;
select '24', hamming('hello', 'hellp') = 1;
select '25', hamming('hello', 'heloh') = 2;

-- Jaro-Winkler distance
select '31', jaro_winkler('abc', 'abc') = 1.0;
select '32', jaro_winkler('abc', '') = 0.0;
select '33', jaro_winkler('', 'abc') = 0.0;
select '34', round(jaro_winkler('my string', 'my tsring'), 3) = 0.974;
select '35', round(jaro_winkler('my string', 'my ntrisg'), 3) = 0.896;

-- Levenshtein distance
select '41', levenshtein('abc', 'abc') = 0;
select '42', levenshtein('abc', '') = 3;
select '43', levenshtein('', 'abc') = 3;
select '44', levenshtein('abc', 'ab') = 1;
select '45', levenshtein('abc', 'abcd') = 1;
select '46', levenshtein('abc', 'acb') = 2;
select '47', levenshtein('abc', 'ca') = 3;

-- Optimal String Alignment distance
select '51', osa_distance('abc', 'abc') = 0;
select '52', osa_distance('abc', '') = 3;
select '53', osa_distance('', 'abc') = 3;
select '54', osa_distance('abc', 'ab') = 1;
select '55', osa_distance('abc', 'abcd') = 1;
select '56', osa_distance('abc', 'acb') = 2;
select '57', osa_distance('abc', 'ca') = 3;

-- Spellcheck edit distance
select '61', edit_distance('abc', 'abc') = 0;
select '62', edit_distance('abc', '') = 300;
select '63', edit_distance('', 'abc') = 75;
select '64', edit_distance('abc', 'ab') = 100;
select '65', edit_distance('abc', 'abcd') = 25;
select '66', edit_distance('abc', 'acb') = 110;
select '67', edit_distance('abc', 'ca') = 225;

-- Spellcheck phonetic code
select '101', phonetic_hash('') = '';
select '102', phonetic_hash('phonetics') = 'BAMADAC';
select '103', phonetic_hash('is') = 'AC';
select '104', phonetic_hash('awesome') = 'ABACAMA';

-- Soundex code
select '111', soundex('') = '';
select '112', soundex('phonetics') = 'P532';
select '113', soundex('is') = 'I200';
select '114', soundex('awesome') = 'A250';

-- Refined Soundex code
select '121', rsoundex('') = '';
select '122', rsoundex('phonetics') = 'P1080603';
select '123', rsoundex('is') = 'I03';
select '124', rsoundex('awesome') = 'A03080';
