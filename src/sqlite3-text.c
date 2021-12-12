// Originally by Liam Healy, Public Domain
// extension-functions.c at https://sqlite.org/contrib/
// Modified by Anton Zhiyanov, https://github.com/nalgeon/sqlean, MIT License

/*
 * SQLite text functions.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sqlite3ext.h"

SQLITE_EXTENSION_INIT1

/**
 * From sqlite3 utf.c
 */
static const unsigned char sqlite3Utf8Trans1[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x00, 0x00,
};

#define READ_UTF8(zIn, zTerm, c)                                                    \
    c = *(zIn++);                                                                   \
    if (c >= 0xc0) {                                                                \
        c = sqlite3Utf8Trans1[c - 0xc0];                                            \
        while (zIn != zTerm && (*zIn & 0xc0) == 0x80) {                             \
            c = (c << 6) + (0x3f & *(zIn++));                                       \
        }                                                                           \
        if (c < 0x80 || (c & 0xFFFFF800) == 0xD800 || (c & 0xFFFFFFFE) == 0xFFFE) { \
            c = 0xFFFD;                                                             \
        }                                                                           \
    }

/*
 * reverse() and friends extracted from
 * extension-functions.c (https://sqlite.org/contrib/)
 * by Liam Healy
 */
#define advance_char(X)               \
    while ((0xc0 & *++(X)) == 0x80) { \
    }

static int read_char(const unsigned char* str) {
    int c;
    READ_UTF8(str, 0, c);
    return c;
}

/*
 * Returns reversed string.
 * reverse("abcde") == "edcba"
 */
static char* reverse(const char* source) {
    int len = strlen(source);
    char* result = sqlite3_malloc(len + 1);
    char* rzt = result + len;
    *(rzt--) = '\0';

    const char* zt = source;
    while (read_char((unsigned char*)zt) != 0) {
        source = zt;
        advance_char(zt);
        for (int i = 1; zt - i >= source; ++i) {
            *(rzt--) = *(zt - i);
        }
    }
    return result;
}

static void sqlite3_reverse(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        sqlite3_result_null(context);
        return;
    }
    const char* source = (char*)sqlite3_value_text(argv[0]);
    char* result = reverse(source);
    sqlite3_result_text(context, result, -1, sqlite3_free);
}

/*
 * strsep() implementation, Windows doesn't have it
 * copied from https://unixpapa.com/incnote/string.html
 */
static char* str_sep(char** sp, const char* sep) {
    if (sp == NULL || *sp == NULL || **sp == '\0') {
        return NULL;
    }
    char* s = *sp;
    char* p = s + strcspn(s, sep);
    if (*p != '\0')
        *p++ = '\0';
    *sp = p;
    return s;
}

/*
 * Splits `source` string on `sep` and returns the given `part` (counting from one)
 * split_part("one;two;three", ";", 2) == "two"
 */
static char* split_part(char* source, const char* sep, int64_t part) {
    char* token;
    int64_t index = 1;
    while ((token = str_sep(&source, sep)) != NULL) {
        if (index == part) {
            break;
        }
        index++;
    }
    return token;
}

static void sqlite3_split_part(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 3);

    char* source = (char*)sqlite3_value_text(argv[0]);
    if (source == NULL) {
        sqlite3_result_null(context);
        return;
    }
    if (strcmp(source, "") == 0) {
        sqlite3_result_text(context, "", -1, SQLITE_TRANSIENT);
        return;
    }

    const char* sep = (const char*)sqlite3_value_text(argv[1]);
    if (!sep) {
        sqlite3_result_null(context);
        return;
    }

    if (sqlite3_value_type(argv[2]) != SQLITE_INTEGER) {
        sqlite3_result_error(context, "part parameter should be integer", -1);
        return;
    }
    int64_t part = sqlite3_value_int64(argv[2]);
    if (part <= 0) {
        sqlite3_result_error(context, "part parameter should be > 0", -1);
        return;
    }

    char* token = split_part(source, sep, part);

    if (token == NULL) {
        sqlite3_result_text(context, "", -1, SQLITE_TRANSIENT);
        return;
    }
    sqlite3_result_text(context, token, -1, SQLITE_TRANSIENT);
}

/*
 * Registers the extension.
 */
#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_text_init(sqlite3* db, char** pzErrMsg, const sqlite3_api_routines* pApi) {
    SQLITE_EXTENSION_INIT2(pApi);
    static const int flags = SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC;
    sqlite3_create_function(db, "reverse", 1, flags, 0, sqlite3_reverse, 0, 0);
    sqlite3_create_function(db, "split_part", 3, flags, 0, sqlite3_split_part, 0, 0);
    return SQLITE_OK;
}