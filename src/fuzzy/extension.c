// Copyright (c) 2021 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Fuzzy string matching and phonetics.

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT3

#include "fuzzy/extension.h"
#include "fuzzy/fuzzy.h"

#pragma region Helpers

// has_null reports whether any of the arguments is NULL.
static bool has_null(int argc, sqlite3_value** argv) {
    for (int i = 0; i < argc; i++) {
        if (sqlite3_value_type(argv[i]) == SQLITE_NULL) {
            return true;
        }
    }
    return false;
}

// arg_texts decodes the argc text arguments into s and n. Numbers are
// converted to text. Sets the NULL result for a blob, and the error
// if out of memory. Returns false on failure.
static bool arg_texts(sqlite3_context* context,
                      int argc,
                      sqlite3_value** argv,
                      const char** s,
                      size_t* n) {
    for (int i = 0; i < argc; i++) {
        if (sqlite3_value_type(argv[i]) == SQLITE_BLOB) {
            sqlite3_result_null(context);
            return false;
        }
        s[i] = (const char*)sqlite3_value_text(argv[i]);
        if (s[i] == NULL) {
            sqlite3_result_error_nomem(context);
            return false;
        }
        n[i] = (size_t)sqlite3_value_bytes(argv[i]);
    }
    return true;
}

// check_status sets the NULL result for FUZZY_INVALID, and the error
// for FUZZY_NOMEM. Returns false for either of them.
static bool check_status(sqlite3_context* context, FuzzyStatus status) {
    switch (status) {
        case FUZZY_OK:
            return true;
        case FUZZY_INVALID:
            sqlite3_result_null(context);
            return false;
        case FUZZY_NOMEM:
            sqlite3_result_error_nomem(context);
            return false;
    }
    return false;
}

// alloc_buf allocates a result buffer of size bytes.
// Sets the error and returns NULL if out of memory.
static char* alloc_buf(sqlite3_context* context, sqlite3_uint64 size) {
    // sqlite3_malloc64(0) returns NULL, so allocate at least one byte.
    char* buf = sqlite3_malloc64(size > 0 ? size : 1);
    if (buf == NULL) {
        sqlite3_result_error_nomem(context);
    }
    return buf;
}

#pragma endregion

#pragma region Distances

// Distance is the signature of the functions fn_distance calls.
typedef FuzzyStatus (*Distance)(const char* a, size_t na, const char* b, size_t nb, size_t* dist);

// fuzzy_damlev(a, b)
// fuzzy_hamming(a, b)
// fuzzy_leven(a, b)
// fuzzy_osadist(a, b)
static void fn_distance(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* s[2];
    size_t n[2];
    if (!arg_texts(context, argc, argv, s, n)) {
        return;
    }
    Distance distance = (Distance)sqlite3_user_data(context);
    size_t dist;
    if (!check_status(context, distance(s[0], n[0], s[1], n[1], &dist))) {
        return;
    }
    sqlite3_result_int64(context, (sqlite3_int64)dist);
}

// fuzzy_editdist(a, b)
static void fn_editdist(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* s[2];
    size_t n[2];
    if (!arg_texts(context, argc, argv, s, n)) {
        return;
    }
    int64_t dist;
    if (!check_status(context, fuzzy_editdist(s[0], n[0], s[1], n[1], &dist))) {
        return;
    }
    sqlite3_result_int64(context, dist);
}

// fuzzy_jarowin(a, b)
static void fn_jarowin(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* s[2];
    size_t n[2];
    if (!arg_texts(context, argc, argv, s, n)) {
        return;
    }
    double sim;
    if (!check_status(context, fuzzy_jarowin(s[0], n[0], s[1], n[1], &sim))) {
        return;
    }
    sqlite3_result_double(context, sim);
}

#pragma endregion

#pragma region Phonetic codes

// Encoder is the signature of the functions fn_code calls.
typedef FuzzyStatus (*Encoder)(const char* s, size_t n, char* buf);

// fuzzy_caver(s)
// fuzzy_phonetic(s)
// fuzzy_soundex(s)
// fuzzy_rsoundex(s)
static void fn_code(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* s;
    size_t n;
    if (!arg_texts(context, argc, argv, &s, &n)) {
        return;
    }
    // the buffer must fit any of the codes: FUZZY_PHONETIC_SIZE(n) and
    // FUZZY_SOUNDEX_SIZE are never larger than FUZZY_RSOUNDEX_SIZE(n)
    sqlite3_uint64 size = FUZZY_RSOUNDEX_SIZE((sqlite3_uint64)n);
    if (size < FUZZY_CAVER_SIZE) {
        size = FUZZY_CAVER_SIZE;
    }
    char* buf = alloc_buf(context, size);
    if (buf == NULL) {
        return;
    }
    Encoder encode = (Encoder)sqlite3_user_data(context);
    if (!check_status(context, encode(s, n, buf))) {
        sqlite3_free(buf);
        return;
    }
    sqlite3_result_text(context, buf, -1, sqlite3_free);
}

#pragma endregion

#pragma region Transliteration

// fuzzy_translit(s)
static void fn_translit(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* s;
    size_t n;
    if (!arg_texts(context, argc, argv, &s, &n)) {
        return;
    }
    // n is at most SQLITE_MAX_LENGTH < 2^31, so the size doesn't overflow
    char* buf = alloc_buf(context, FUZZY_TRANSLIT_SIZE((sqlite3_uint64)n));
    if (buf == NULL) {
        return;
    }
    size_t len = fuzzy_translit(s, n, buf);
    // sqlite3_result_text64 frees buf and sets the error if it's too long
    sqlite3_result_text64(context, buf, len, sqlite3_free, SQLITE_UTF8);
}

// fuzzy_script(s)
static void fn_script(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    if (has_null(argc, argv)) {
        sqlite3_result_null(context);
        return;
    }
    const char* s;
    size_t n;
    if (!arg_texts(context, argc, argv, &s, &n)) {
        return;
    }
    sqlite3_result_int(context, (int)fuzzy_script(s, n));
}

#pragma endregion

#pragma region Registration

#define FLAGS (SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC)

// Function describes an SQL function to register.
typedef struct {
    const char* name;
    int nargs;
    int flags;
    void* user_data;
    void (*fn)(sqlite3_context* context, int argc, sqlite3_value** argv);
} Function;

static const Function functions[] = {
    // distances
    {"fuzzy_damlev", 2, FLAGS, fuzzy_damlev, fn_distance},
    {"fuzzy_editdist", 2, FLAGS, 0, fn_editdist},
    {"fuzzy_hamming", 2, FLAGS, fuzzy_hamming, fn_distance},
    {"fuzzy_jarowin", 2, FLAGS, 0, fn_jarowin},
    {"fuzzy_leven", 2, FLAGS, fuzzy_leven, fn_distance},
    {"fuzzy_osadist", 2, FLAGS, fuzzy_osadist, fn_distance},

    // phonetic codes
    {"fuzzy_caver", 1, FLAGS, fuzzy_caver, fn_code},
    {"fuzzy_phonetic", 1, FLAGS, fuzzy_phonetic, fn_code},
    {"fuzzy_soundex", 1, FLAGS, fuzzy_soundex, fn_code},
    {"fuzzy_rsoundex", 1, FLAGS, fuzzy_rsoundex, fn_code},

    // transliteration
    {"fuzzy_translit", 1, FLAGS, 0, fn_translit},
    {"fuzzy_script", 1, FLAGS, 0, fn_script},

    // aliases
    {"dlevenshtein", 2, FLAGS, fuzzy_damlev, fn_distance},
    {"edit_distance", 2, FLAGS, 0, fn_editdist},
    {"hamming", 2, FLAGS, fuzzy_hamming, fn_distance},
    {"jaro_winkler", 2, FLAGS, 0, fn_jarowin},
    {"levenshtein", 2, FLAGS, fuzzy_leven, fn_distance},
    {"osa_distance", 2, FLAGS, fuzzy_osadist, fn_distance},
    {"caverphone", 1, FLAGS, fuzzy_caver, fn_code},
    {"phonetic_hash", 1, FLAGS, fuzzy_phonetic, fn_code},
    {"soundex", 1, FLAGS, fuzzy_soundex, fn_code},
    {"rsoundex", 1, FLAGS, fuzzy_rsoundex, fn_code},
    {"translit", 1, FLAGS, 0, fn_translit},
    {"script_code", 1, FLAGS, 0, fn_script},
};

int fuzzy_init(sqlite3* db) {
    for (size_t i = 0; i < sizeof(functions) / sizeof(functions[0]); i++) {
        const Function* f = &functions[i];
        int rc =
            sqlite3_create_function(db, f->name, f->nargs, f->flags, f->user_data, f->fn, 0, 0);
        if (rc != SQLITE_OK) {
            return rc;
        }
    }
    return SQLITE_OK;
}

#pragma endregion
