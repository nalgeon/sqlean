// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Fuzzy string matching and phonetics.

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT3

#include "fuzzy/fuzzy.h"

// is_ascii checks if the string consists of ASCII symbols only
static bool is_ascii(const unsigned char* str) {
    for (int idx = 0; str[idx]; idx++) {
        if (str[idx] & 0x80) {
            return false;
        }
    }
    return true;
}

// Below are functions extracted from the
// https://github.com/Rostepher/libstrcmp/

// fuzzy_dlevenshtein implements Damerau-Levenshtein distance
static void fuzzy_dlevenshtein(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    const unsigned char* str1 = sqlite3_value_text(argv[0]);
    const unsigned char* str2 = sqlite3_value_text(argv[1]);
    if (str1 == 0 || str2 == 0) {
        sqlite3_result_error(context, "arguments should not be NULL", -1);
        return;
    }
    if (!is_ascii(str1) || !is_ascii(str2)) {
        sqlite3_result_error(context, "arguments should be ASCII strings", -1);
        return;
    }
    unsigned distance = damerau_levenshtein((const char*)str1, (const char*)str2);
    sqlite3_result_int(context, distance);
}

// fuzzy_hamming implements Hamming distance
static void fuzzy_hamming(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    const unsigned char* str1 = sqlite3_value_text(argv[0]);
    const unsigned char* str2 = sqlite3_value_text(argv[1]);
    if (str1 == 0 || str2 == 0) {
        sqlite3_result_error(context, "arguments should not be NULL", -1);
        return;
    }
    if (!is_ascii(str1) || !is_ascii(str2)) {
        sqlite3_result_error(context, "arguments should be ASCII strings", -1);
        return;
    }
    int distance = hamming((const char*)str1, (const char*)str2);
    sqlite3_result_int(context, distance);
}

// fuzzy_jaro_winkler implements Jaro-Winkler distance
static void fuzzy_jaro_winkler(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    const unsigned char* str1 = sqlite3_value_text(argv[0]);
    const unsigned char* str2 = sqlite3_value_text(argv[1]);
    if (str1 == 0 || str2 == 0) {
        sqlite3_result_error(context, "arguments should not be NULL", -1);
        return;
    }
    if (!is_ascii(str1) || !is_ascii(str2)) {
        sqlite3_result_error(context, "arguments should be ASCII strings", -1);
        return;
    }
    double distance = jaro_winkler((const char*)str1, (const char*)str2);
    sqlite3_result_double(context, distance);
}

// fuzzy_levenshtein implements Levenshtein distance
static void fuzzy_levenshtein(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    const unsigned char* str1 = sqlite3_value_text(argv[0]);
    const unsigned char* str2 = sqlite3_value_text(argv[1]);
    if (str1 == 0 || str2 == 0) {
        sqlite3_result_error(context, "arguments should not be NULL", -1);
        return;
    }
    if (!is_ascii(str1) || !is_ascii(str2)) {
        sqlite3_result_error(context, "arguments should be ASCII strings", -1);
        return;
    }
    unsigned distance = levenshtein((const char*)str1, (const char*)str2);
    sqlite3_result_int(context, distance);
}

// fuzzy_osa_distance implements Optimal String Alignment distance
static void fuzzy_osa_distance(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    const unsigned char* str1 = sqlite3_value_text(argv[0]);
    const unsigned char* str2 = sqlite3_value_text(argv[1]);
    if (str1 == 0 || str2 == 0) {
        sqlite3_result_error(context, "arguments should not be NULL", -1);
        return;
    }
    if (!is_ascii(str1) || !is_ascii(str2)) {
        sqlite3_result_error(context, "arguments should be ASCII strings", -1);
        return;
    }
    unsigned distance = optimal_string_alignment((const char*)str1, (const char*)str2);
    sqlite3_result_int(context, distance);
}

// fuzzy_soundex implements Soundex coding
static void fuzzy_soundex(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    const unsigned char* source = sqlite3_value_text(argv[0]);
    if (source == 0) {
        return;
    }
    if (!is_ascii(source)) {
        sqlite3_result_error(context, "argument should be ASCII string", -1);
        return;
    }
    char* result = soundex((const char*)source);
    sqlite3_result_text(context, result, -1, free);
}

// fuzzy_rsoundex implements Refined Soundex coding
static void fuzzy_rsoundex(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    const unsigned char* source = sqlite3_value_text(argv[0]);
    if (source == 0) {
        return;
    }
    if (!is_ascii(source)) {
        sqlite3_result_error(context, "argument should be ASCII string", -1);
        return;
    }
    char* result = refined_soundex((const char*)source);
    sqlite3_result_text(context, result, -1, free);
}

// Below are functions extracted from the spellfix SQLite exension
// https://www.sqlite.org/src/file/ext/misc/spellfix.c

/*
** phonetic_hash(X)
**
** Generate a "phonetic hash" from a string of ASCII characters in X.
**
**   * Map characters by character class as defined above.
**   * Omit double-letters
**   * Omit vowels beside R and L
**   * Omit T when followed by CH
**   * Omit W when followed by R
**   * Omit D when followed by J or G
**   * Omit K in KN or G in GN at the beginning of a word
**
** Space to hold the result is obtained from sqlite3_malloc()
**
** Return NULL if memory allocation fails.
*/
static void fuzzy_phonetic_hash(sqlite3_context* context, int argc, sqlite3_value** argv) {
    const unsigned char* zIn;
    unsigned char* zOut;

    zIn = sqlite3_value_text(argv[0]);
    if (zIn == 0)
        return;
    zOut = phonetic_hash(zIn, sqlite3_value_bytes(argv[0]));
    if (zOut == 0) {
        sqlite3_result_error_nomem(context);
    } else {
        sqlite3_result_text(context, (char*)zOut, -1, free);
    }
}

/*
** edit_distance(A,B)
**
** Return the cost of transforming string A into string B.  Both strings
** must be pure ASCII text.  If A ends with '*' then it is assumed to be
** a prefix of B and extra characters on the end of B have minimal additional
** cost.
*/
static void fuzzy_edit_distance(sqlite3_context* context, int argc, sqlite3_value** argv) {
    int res = edit_distance((const char*)sqlite3_value_text(argv[0]),
                            (const char*)sqlite3_value_text(argv[1]), 0);
    if (res < 0) {
        if (res == (-3)) {
            sqlite3_result_error_nomem(context);
        } else if (res == (-2)) {
            sqlite3_result_error(context, "non-ASCII input to editdist()", -1);
        } else {
            sqlite3_result_error(context, "NULL input to editdist()", -1);
        }
    } else {
        sqlite3_result_int(context, res);
    }
}

/*
** translit(X)
**
** Convert a string that contains non-ASCII Roman characters into
** pure ASCII.
*/
static void fuzzy_transliterate(sqlite3_context* context, int argc, sqlite3_value** argv) {
    const unsigned char* zIn = sqlite3_value_text(argv[0]);
    int nIn = sqlite3_value_bytes(argv[0]);
    unsigned char* zOut = transliterate(zIn, nIn);
    if (zOut == 0) {
        sqlite3_result_error_nomem(context);
    } else {
        sqlite3_result_text(context, (char*)zOut, -1, free);
    }
}

/*
** script_code(X)
**
** Try to determine the dominant script used by the word X and return
** its ISO 15924 numeric code.
**
** The current implementation only understands the following scripts:
**
**    215  (Latin)
**    220  (Cyrillic)
**    200  (Greek)
**
** This routine will return 998 if the input X contains characters from
** two or more of the above scripts or 999 if X contains no characters
** from any of the above scripts.
*/
static void fuzzy_script_code(sqlite3_context* context, int argc, sqlite3_value** argv) {
    const unsigned char* zIn = sqlite3_value_text(argv[0]);
    int nIn = sqlite3_value_bytes(argv[0]);
    int res = script_code(zIn, nIn);
    sqlite3_result_int(context, res);
}

// Below are custom functions

// fuzzy_caverphone implements Caverphone coding
static void fuzzy_caverphone(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    const unsigned char* source = sqlite3_value_text(argv[0]);
    if (source == 0) {
        return;
    }
    if (!is_ascii(source)) {
        sqlite3_result_error(context, "argument should be ASCII string", -1);
        return;
    }
    char* result = caverphone((const char*)source);
    sqlite3_result_text(context, result, -1, free);
}

int fuzzy_init(sqlite3* db) {
    static const int flags = SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC;
    // libstrcmp
    sqlite3_create_function(db, "dlevenshtein", 2, flags, 0, fuzzy_dlevenshtein, 0, 0);
    sqlite3_create_function(db, "hamming", 2, flags, 0, fuzzy_hamming, 0, 0);
    sqlite3_create_function(db, "jaro_winkler", 2, flags, 0, fuzzy_jaro_winkler, 0, 0);
    sqlite3_create_function(db, "levenshtein", 2, flags, 0, fuzzy_levenshtein, 0, 0);
    sqlite3_create_function(db, "osa_distance", 2, flags, 0, fuzzy_osa_distance, 0, 0);
    sqlite3_create_function(db, "soundex", 1, flags, 0, fuzzy_soundex, 0, 0);
    sqlite3_create_function(db, "rsoundex", 1, flags, 0, fuzzy_rsoundex, 0, 0);
    // spellfix
    sqlite3_create_function(db, "edit_distance", 2, flags, 0, fuzzy_edit_distance, 0, 0);
    sqlite3_create_function(db, "phonetic_hash", 1, flags, 0, fuzzy_phonetic_hash, 0, 0);
    sqlite3_create_function(db, "script_code", 1, flags, 0, fuzzy_script_code, 0, 0);
    sqlite3_create_function(db, "translit", 1, flags, 0, fuzzy_transliterate, 0, 0);
    // custom
    sqlite3_create_function(db, "caverphone", 1, flags, 0, fuzzy_caverphone, 0, 0);
    return SQLITE_OK;
}
