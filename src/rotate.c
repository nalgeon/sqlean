// ROT13 by Richard Hipp, Public Domain
// https://sqlite.org/src/file/ext/misc/dbdump.c

// ROT47 Copyright (c) 2021 by CompuRoot, BSD 0-Clause License
// https://github.com/CompuRoot/static-sqlite3

// Implements a rot13() function and a rot13 collating sequence.
// Implements a rot47() function and a rot47 collating sequence.

#include <assert.h>
#include <string.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

// Perform rot13 encoding on a single ASCII character.
static unsigned char rot13(unsigned char c) {
    if (c >= 'a' && c <= 'z') {
        c += 13;
        if (c > 'z')
            c -= 26;
    } else if (c >= 'A' && c <= 'Z') {
        c += 13;
        if (c > 'Z')
            c -= 26;
    }
    return c;
}

// Perform rot47 monoalphabetic substitution on a single ASCII character.
//
// Encode all visible English ASCII 94 characters set in range
// ASCII DEC 33 to ASCII DEC 126 (regex [!-~]) using substitution cipher,
// similar to Caesar's cipher, that's why named as rot47 (94/2=47).
// Algorithm can be explained by modular arithmetic as: 33+((ASCII_char+14)%94)
// where ASCII_char is a single byte in range: 33-126
// Implementation don't use 33+((ASCII_char+14)%94) formula but faster analog.
//
// Lookup table for rot47:
//   !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~
//   PQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~!"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNO
static unsigned char rot47(unsigned char c) {
    if (c >= '!' && c <= 'O') {
        c += 47;
    } else if (c >= 'P' && c <= '~') {
        c -= 47;
    }
    return c;
}

// rot13(X)
// Rotate ASCII alphabetic characters by 13 character positions.
// Non-ASCII characters are unchanged.
// rot13(rot13(X)) = X
static void rot13func(sqlite3_context* context, int argc, sqlite3_value** argv) {
    const unsigned char* zIn;
    int nIn;
    unsigned char* zOut;
    unsigned char* zToFree = 0;
    int i;
    unsigned char zTemp[100];
    assert(argc == 1);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL)
        return;
    zIn = (const unsigned char*)sqlite3_value_text(argv[0]);
    nIn = sqlite3_value_bytes(argv[0]);
    if (nIn < sizeof(zTemp) - 1) {
        zOut = zTemp;
    } else {
        zOut = zToFree = (unsigned char*)sqlite3_malloc64(nIn + 1);
        if (zOut == 0) {
            sqlite3_result_error_nomem(context);
            return;
        }
    }
    for (i = 0; i < nIn; i++)
        zOut[i] = rot13(zIn[i]);
    zOut[i] = 0;
    sqlite3_result_text(context, (char*)zOut, i, SQLITE_TRANSIENT);
    sqlite3_free(zToFree);
}

// rot13 collating sequence
// x=y COLLATE rot13 <=> rot13(x)=rot13(y) COLLATE binary
static int rot13CollFunc(void* notUsed,
                         int nKey1,
                         const void* pKey1,
                         int nKey2,
                         const void* pKey2) {
    const char* zA = (const char*)pKey1;
    const char* zB = (const char*)pKey2;
    int i, x;
    for (i = 0; i < nKey1 && i < nKey2; i++) {
        x = (int)rot13(zA[i]) - (int)rot13(zB[i]);
        if (x != 0)
            return x;
    }
    return nKey1 - nKey2;
}

// rot47(X)
// Rotate ASCII alphabetic characters by 47 character positions.
// Non-ASCII characters are unchanged.
// rot47(rot47(X)) = X
static void rot47func(sqlite3_context* context, int argc, sqlite3_value** argv) {
    const unsigned char* zIn;
    int nIn;
    unsigned char* zOut;
    unsigned char* zToFree = 0;
    int i;
    unsigned char zTemp[127];
    assert(argc == 1);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL)
        return;
    zIn = (const unsigned char*)sqlite3_value_text(argv[0]);
    nIn = sqlite3_value_bytes(argv[0]);
    if (nIn < sizeof(zTemp) - 1) {
        zOut = zTemp;
    } else {
        zOut = zToFree = (unsigned char*)sqlite3_malloc64(nIn + 1);
        if (zOut == 0) {
            sqlite3_result_error_nomem(context);
            return;
        }
    }
    for (i = 0; i < nIn; i++)
        zOut[i] = rot47(zIn[i]);
    zOut[i] = 0;
    sqlite3_result_text(context, (char*)zOut, i, SQLITE_TRANSIENT);
    sqlite3_free(zToFree);
}

// rot47 collating sequence
// x=y COLLATE rot47 <=> rot47(x)=rot47(y) COLLATE binary
static int rot47CollFunc(void* notUsed,
                         int nKey1,
                         const void* pKey1,
                         int nKey2,
                         const void* pKey2) {
    const char* zA = (const char*)pKey1;
    const char* zB = (const char*)pKey2;
    int i, x;
    for (i = 0; i < nKey1 && i < nKey2; i++) {
        x = (int)rot47(zA[i]) - (int)rot47(zB[i]);
        if (x != 0)
            return x;
    }
    return nKey1 - nKey2;
}

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_rotate_init(sqlite3* db, char** pzErrMsg, const sqlite3_api_routines* pApi) {
    SQLITE_EXTENSION_INIT2(pApi);
    (void)pzErrMsg; /* Unused parameter */
    const int flags = SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC;
    sqlite3_create_function(db, "rot13", 1, flags, 0, rot13func, 0, 0);
    sqlite3_create_function(db, "rot47", 1, flags, 0, rot47func, 0, 0);
    sqlite3_create_collation(db, "rot13", SQLITE_UTF8, 0, rot13CollFunc);
    sqlite3_create_collation(db, "rot47", SQLITE_UTF8, 0, rot47CollFunc);
    return SQLITE_OK;
}