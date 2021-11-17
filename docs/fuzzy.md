# fuzzy: Fuzzy string matching and phonetics

Provides fuzzy-matching helpers:

-   Measure distance between two strings.
-   Compute phonetic string code.
-   Transliterate a string.

Adapted from [libstrcmp](https://github.com/Rostepher/libstrcmp) by Ross Bayer and [spellfix.c](https://www.sqlite.org/src/file?name=ext/misc/spellfix.c) by D. Richard Hipp.

## String distances

Measure distance between two strings:

-   `dlevenshtein(x, y)` - Damerau-Levenshtein distance,
-   `edit_distance(x, y)` - Spellcheck edit distance,
-   `hamming(x, y)` - Hamming distance,
-   `jaro_winkler(x, y)` - Jaro-Winkler distance,
-   `levenshtein(x, y)` - Levenshtein distance,
-   `osa_distance(x, y)` - Optimal String Alignment distance.

```
sqlite> select dlevenshtein('awesome', 'aewsme');
2

sqlite> select edit_distance('awesome', 'aewsme');
215

sqlite> select hamming('awesome', 'aewsome');
2

sqlite> select jaro_winkler('awesome', 'aewsme');
0.907

sqlite> select levenshtein('awesome', 'aewsme');
3

sqlite> select osa_distance('awesome', 'aewsme');
3
```

Only ASCII strings are supported.

## Phonetic codes

Compute phonetic string code:

-   `phonetic_hash(x)` - Spellcheck phonetic code,
-   `soundex(x)` - Soundex code,
-   `rsoundex(x)` - Refined Soundex code.

```
sqlite> select phonetic_hash('awesome');
ABACAMA

sqlite> select soundex('awesome');
A250

sqlite> select rsoundex('awesome');
A03080
```

Only ASCII strings are supported.

## Transliteration

Transliteration converts the input string from UTF-8 into pure ASCII
by converting all non-ASCII characters to some combination of characters
in the ASCII subset.

Distance and phonetics functions are ASCII-only, so to work
with Unicode string one should transliterate it first.

```
sqlite> select translit('привет');
privet
```

[Download](https://github.com/nalgeon/sqlean/releases/latest)

## Usage

```
sqlite> .load ./fuzzy
sqlite> select soundex('hello');
```
