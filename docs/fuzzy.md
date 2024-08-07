# fuzzy: Fuzzy string matching and phonetics in SQLite

The `sqlean-fuzzy` extension provides fuzzy-matching helpers:

-   Measure distance between two strings.
-   Compute phonetic string code.
-   Transliterate a string.

If you want a ready-to-use mechanism to search a large vocabulary for close matches, see the [spellfix](https://github.com/nalgeon/sqlean/issues/27#issuecomment-1002297477) extension instead.

[String distances](#string-distances) •
[Phonetic codes](#phonetic-codes) •
[Transliteration](#transliteration) •
[Acknowledgements](#acknowledgements) •
[Installation and usage](#installation-and-usage)

## String distances

These functions measure the distance between two strings.

Only ASCII strings are supported. Use the [translit](#transliteration) function to convert the input string from UTF-8 to plain ASCII.

[damlev](#fuzzy_damlev) •
[editdist](#fuzzy_editdist) •
[hamming](#fuzzy_hamming) •
[jarowin](#fuzzy_jarowin) •
[leven](#fuzzy_leven) •
[osadist](#fuzzy_osadist)

### fuzzy_damlev

```text
fuzzy_damlev(x, y)
```

Calculates the Damerau-Levenshtein distance.

```sql
select fuzzy_damlev('awesome', 'aewsme');
-- 2
```

### fuzzy_editdist

```text
fuzzy_editdist(x, y)
```

Calculates the spellcheck edit distance.

```sql
select fuzzy_editdist('awesome', 'aewsme');
-- 215
```

### fuzzy_hamming

```text
fuzzy_hamming(x, y)
```

Calculates the Hamming distance.

```sql
select fuzzy_hamming('awesome', 'aewsome');
-- 2
```

### fuzzy_jarowin

```text
fuzzy_jarowin(x, y)
```

Calculates the Jaro-Winkler distance.

```sql
select fuzzy_jarowin('awesome', 'aewsme');
-- 0.907142857142857
```

### fuzzy_leven

Calculates the Levenshtein distance.

```sql
select fuzzy_leven('awesome', 'aewsme');
-- 3
```

### fuzzy_osadist

```text
fuzzy_osadist(x, y)
```

Calculates the Optimal String Alignment distance.

```sql
select fuzzy_osadist('awesome', 'aewsme');
-- 3
```

## Phonetic codes

These functions compute phonetic string codes.

Only ASCII strings are supported. Use the [translit](#transliteration) function to convert the input string from UTF-8 to plain ASCII.

[caver](#fuzzy_caver) •
[phonetic](#fuzzy_phonetic) •
[soundex](#fuzzy_soundex) •
[rsoundex](#fuzzy_rsoundex)

### fuzzy_caver

```text
fuzzy_caver(x)
```

Calculates the Caverphone code.

```sql
select fuzzy_caver('awesome');
-- AWSM111111
```

### fuzzy_phonetic

```text
fuzzy_phonetic(x)
```

Calsulates the spellcheck phonetic code.

```sql
select fuzzy_phonetic('awesome');
-- ABACAMA
```

### fuzzy_soundex

```text
fuzzy_soundex(x)
```

Calculates the Soundex code.

```sql
select fuzzy_soundex('awesome');
-- A250
```

### fuzzy_rsoundex

```text
fuzzy_rsoundex(x)
```

Calculates the Refined Soundex code.

```sql
select fuzzy_rsoundex('awesome');
-- A03080
```

## Transliteration

```text
fuzzy_translit(str)
```

Transliteration converts the input string from UTF-8 into plain ASCII
by converting all non-ASCII characters to some combination of characters
in the ASCII subset.

The distance and phonetic functions are ASCII only, so to work
with a Unicode string, you should first transliterate it:

```sql
select fuzzy_translit('sí señor');
-- si senor

select fuzzy_translit('привет');
-- privet
```

Some characters may be lost:

```sql
select fuzzy_translit('oh my 😅');
-- oh my ?
```

## Acknowledgements

Adapted from [libstrcmp](https://github.com/Rostepher/libstrcmp) by Ross Bayer and [spellfix.c](https://www.sqlite.org/src/file/ext/misc/spellfix.c) by D. Richard Hipp.

## Installation and usage

SQLite command-line interface:

```
sqlite> .load ./fuzzy
sqlite> select fuzzy_soundex('hello');
```

See [How to install an extension](install.md) for usage with IDE, Python, etc.

↓ [Download](https://github.com/nalgeon/sqlean/releases/latest) the extension.

⛱ [Explore](https://github.com/nalgeon/sqlean) other extensions.

★ [Subscribe](https://antonz.org/subscribe/) to stay on top of new features.
