# fuzzy: Fuzzy string matching and phonetics in SQLite

The `sqlean-fuzzy` extension provides fuzzy-matching helpers:

- Measure distance between two strings.
- Compute phonetic string code.
- Transliterate a string.

If you want a ready-to-use mechanism to search a large vocabulary for close matches, see the [spellfix](https://github.com/nalgeon/sqlean/issues/27#issuecomment-1002297477) extension instead.

[Concepts](#concepts) •
[String distances](#string-distances) •
[Phonetic codes](#phonetic-codes) •
[Transliteration](#transliteration) •
[Acknowledgements](#acknowledgements) •
[Installation and usage](#installation-and-usage)

## Concepts

### Unicode and ASCII

The string distances, except for `fuzzy_editdist`, work with Unicode characters, not bytes:

```sql
select fuzzy_leven('café', 'cafe');
-- 1
```

`fuzzy_editdist` and the phonetic codes only support ASCII strings and return `NULL` for anything else. Use the [translit](#transliteration) function to convert a string from UTF-8 to plain ASCII first:

```sql
select fuzzy_soundex('Müller');
-- NULL

select fuzzy_soundex(fuzzy_translit('Müller'));
-- M460
```

Invalid UTF-8 bytes are treated as separate replacement characters (U+FFFD). A NUL character is an ordinary character, it doesn't end the string.

### NULL and invalid input

If any function argument is `NULL`, the result is `NULL`.

Like SQLite's own string functions, the extension returns `NULL` for invalid data instead of raising an error, so one bad row doesn't abort the whole query:

- A BLOB argument: `fuzzy_leven(x'616263', 'abc')` is `NULL`. Numbers are converted to text, so `fuzzy_leven(123, 124)` is `1`.
- A non-ASCII string passed to `fuzzy_editdist` or a phonetic code function: `fuzzy_caver('café')` is `NULL`.
- Strings of different lengths passed to `fuzzy_hamming`: `fuzzy_hamming('abc', 'ab')` is `NULL`.

## String distances

These functions measure the distance between two strings.

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

Calculates the [Damerau-Levenshtein distance](https://en.wikipedia.org/wiki/Damerau%E2%80%93Levenshtein_distance): the minimum number of insertions, deletions, substitutions and transpositions of two adjacent characters needed to turn one string into the other.

```sql
select fuzzy_damlev('awesome', 'aewsme');
-- 2
```

Unlike [fuzzy_osadist](#fuzzy_osadist), it allows editing a substring more than once:

```sql
select fuzzy_damlev('ca', 'abc');
-- 2
```

Aliased as `dlevenshtein(x, y)`.

### fuzzy_editdist

```text
fuzzy_editdist(x, y)
```

Calculates the spellcheck edit distance from the [spellfix](https://www.sqlite.org/spellfix1.html#the_editdist3_function) extension. The cost of each edit depends on the characters involved: changing a letter's case is free, replacing a consonant with a similar one is cheaper than with a dissimilar one, and inserting characters at the end of the string is cheap. A smaller number means a closer match.

```sql
select fuzzy_editdist('awesome', 'aewsme');
-- 215
```

If `x` ends with `*`, it matches any string that starts with the rest of `x`.

Only supports ASCII strings, returns `NULL` otherwise.

Aliased as `edit_distance(x, y)`.

### fuzzy_hamming

```text
fuzzy_hamming(x, y)
```

Calculates the [Hamming distance](https://en.wikipedia.org/wiki/Hamming_distance): the number of positions at which the characters differ. Returns `NULL` if the strings have a different number of characters.

```sql
select fuzzy_hamming('awesome', 'aewsome');
-- 2
```

Aliased as `hamming(x, y)`.

### fuzzy_jarowin

```text
fuzzy_jarowin(x, y)
```

Calculates the [Jaro-Winkler similarity](https://en.wikipedia.org/wiki/Jaro%E2%80%93Winkler_distance), from 0 (no similarity) to 1 (equal strings). Strings with a common prefix (of up to 4 characters) are considered more similar.

```sql
select fuzzy_jarowin('awesome', 'aewsme');
-- 0.907142857142857
```

Aliased as `jaro_winkler(x, y)`.

### fuzzy_leven

```text
fuzzy_leven(x, y)
```

Calculates the [Levenshtein distance](https://en.wikipedia.org/wiki/Levenshtein_distance): the minimum number of insertions, deletions and substitutions needed to turn one string into the other.

```sql
select fuzzy_leven('awesome', 'aewsme');
-- 3
```

Aliased as `levenshtein(x, y)`.

### fuzzy_osadist

```text
fuzzy_osadist(x, y)
```

Calculates the [Optimal String Alignment distance](https://en.wikipedia.org/wiki/Damerau%E2%80%93Levenshtein_distance#Optimal_string_alignment_distance): like [fuzzy_damlev](#fuzzy_damlev), but each substring can be edited at most once.

```sql
select fuzzy_osadist('awesome', 'aewsme');
-- 2

select fuzzy_osadist('ca', 'abc');
-- 3
```

Aliased as `osa_distance(x, y)`.

## Phonetic codes

These functions compute phonetic string codes: similar-sounding strings have the same code.

The functions only support ASCII strings, and return `NULL` otherwise. Use the [translit](#transliteration) function to convert the input string from UTF-8 to plain ASCII.

[caver](#fuzzy_caver) •
[phonetic](#fuzzy_phonetic) •
[soundex](#fuzzy_soundex) •
[rsoundex](#fuzzy_rsoundex)

### fuzzy_caver

```text
fuzzy_caver(x)
```

Calculates the [Caverphone 2.0](https://en.wikipedia.org/wiki/Caverphone) code, which is always 10 characters long (or empty for an empty string).

```sql
select fuzzy_caver('awesome');
-- AWSM111111
```

Aliased as `caverphone(x)`.

### fuzzy_phonetic

```text
fuzzy_phonetic(x)
```

Calculates the spellcheck phonetic code from the [spellfix](https://www.sqlite.org/spellfix1.html) extension.

```sql
select fuzzy_phonetic('awesome');
-- ABACAMA
```

Aliased as `phonetic_hash(x)`.

### fuzzy_soundex

```text
fuzzy_soundex(x)
```

Calculates the [Soundex](https://en.wikipedia.org/wiki/Soundex) code: the first letter followed by three digits. Leading non-letters are skipped, and a string without letters has an empty code.

```sql
select fuzzy_soundex('awesome');
-- A250
```

Aliased as `soundex(x)`.

### fuzzy_rsoundex

```text
fuzzy_rsoundex(x)
```

Calculates the Refined Soundex code: the first letter followed by a variable number of digits. Leading non-letters are skipped, and a string without letters has an empty code.

```sql
select fuzzy_rsoundex('awesome');
-- A03080
```

Aliased as `rsoundex(x)`.

## Transliteration

[translit](#fuzzy_translit) •
[script](#fuzzy_script)

### fuzzy_translit

```text
fuzzy_translit(x)
```

Converts the string from UTF-8 into plain ASCII by replacing each non-ASCII character with some combination of ASCII characters.

The phonetic functions and `fuzzy_editdist` are ASCII only, so to work with a Unicode string, you should first transliterate it:

```sql
select fuzzy_translit('sí señor');
-- si senor

select fuzzy_translit('привет');
-- privet
```

Characters without a transliteration (and invalid UTF-8 bytes) become `?`:

```sql
select fuzzy_translit('oh my 😅');
-- oh my ?
```

Aliased as `translit(x)`.

### fuzzy_script

```text
fuzzy_script(x)
```

Returns the [ISO 15924](https://en.wikipedia.org/wiki/ISO_15924) numeric code of the script the string is written in:

- `215` for Latin,
- `220` for Cyrillic,
- `200` for Greek,
- `125` for Hebrew,
- `160` for Arabic,
- `998` for a mix of these,
- `999` for none of them.

Characters that belong to no script (digits, punctuation, spaces) are ignored, except that a string with digits but no letters is considered Latin.

```sql
select fuzzy_script('Ελλάδα');
-- 200

select fuzzy_script('hello мир');
-- 998

select fuzzy_script('日本');
-- 999
```

Aliased as `script_code(x)`.

## Acknowledgements

Adapted from [go-edlib](https://github.com/hbollon/go-edlib) by Hugo Bollon, [libstrcmp](https://github.com/Rostepher/libstrcmp) by Ross Bayer and [spellfix.c](https://www.sqlite.org/src/file/ext/misc/spellfix.c) by D. Richard Hipp.

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
