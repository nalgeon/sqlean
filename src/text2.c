/*
Copyright 2018-2019 Shawn Wagner

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/* Additional string functions that don't involve unicode character
   properties or the like. */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

static char* do_append(sqlite3_context* context,
                       char* zOut,
                       sqlite3_uint64* nOut,
                       const char* zApp,
                       int nApp) {
    sqlite3_int64 newLen = *nOut + nApp;

    if (nApp == 0) {
        return zOut;
    }

    char* zNew = sqlite3_realloc64(zOut, newLen);
    if (!zNew) {
        sqlite3_free(zOut);
        sqlite3_result_error_nomem(context);
        return NULL;
    }

    memcpy(zNew + *nOut, zApp, nApp);
    *nOut = newLen;
    return zNew;
}

static void sf_concat(sqlite3_context* context, int argc, sqlite3_value** argv) {
    char* zOut = NULL;
    sqlite3_uint64 nOut = 0;
    _Bool empty = 0;
    // MySQL concat() returns NULL if given any NULL arguments, Postgres
    // just ignores NULLS.
    _Bool mysql_style = sqlite3_user_data(context);

    for (int n = 0; n < argc; n += 1) {
        if (sqlite3_value_type(argv[n]) == SQLITE_NULL) {
            if (mysql_style) {
                sqlite3_free(zOut);
                return;
            }
            continue;
        }

        const char* zArg = (const char*)sqlite3_value_text(argv[n]);
        int arglen = sqlite3_value_bytes(argv[n]);

        if (arglen == 0) {
            empty = 1;
            continue;
        }

        zOut = do_append(context, zOut, &nOut, zArg, arglen);
        if (!zOut) {
            return;
        }
    }

    if (zOut) {
        sqlite3_result_text64(context, zOut, nOut, sqlite3_free, SQLITE_UTF8);
    } else if (empty) {
        sqlite3_result_text(context, "", 0, SQLITE_STATIC);
    }
}

static void sf_concat_ws(sqlite3_context* context, int argc, sqlite3_value** argv) {
    if (argc <= 1) {
        return;
    }

    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        return;
    }

    const char* zSep = (const char*)sqlite3_value_text(argv[0]);
    int nSep = sqlite3_value_bytes(argv[0]);

    char* zOut = NULL;
    sqlite3_uint64 nOut = 0;

    for (int n = 1; n < argc; n += 1) {
        if (sqlite3_value_type(argv[n]) == SQLITE_NULL) {
            continue;
        }

        const char* zArg = (const char*)sqlite3_value_text(argv[n]);
        int arglen = sqlite3_value_bytes(argv[n]);

        if (zOut) {
            zOut = do_append(context, zOut, &nOut, zSep, nSep);
            if (!zOut) {
                return;
            }
            zOut = do_append(context, zOut, &nOut, zArg, arglen);
            if (!zOut) {
                return;
            }
        } else {
            if (arglen > 0) {
                zOut = sqlite3_malloc(arglen);
                if (!zOut) {
                    sqlite3_result_error_nomem(context);
                    return;
                }
                memcpy(zOut, zArg, arglen);
                nOut = arglen;
            } else {
                zOut = sqlite3_malloc(1);
                if (!zOut) {
                    sqlite3_result_error_nomem(context);
                    return;
                }
                *zOut = 0;
            }
        }
    }

    if (zOut) {
        sqlite3_result_text64(context, zOut, nOut, sqlite3_free, SQLITE_UTF8);
    }
}

static void sf_repeat8(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL || sqlite3_value_type(argv[1]) == SQLITE_NULL) {
        return;
    }

    const unsigned char* t = sqlite3_value_text(argv[0]);
    int tlen = sqlite3_value_bytes(argv[0]);
    int reps = sqlite3_value_int(argv[1]);

    if (tlen == 0) {
        sqlite3_result_text(context, "", -1, SQLITE_TRANSIENT);
        return;
    }

    if (reps <= 0) {
        sqlite3_result_null(context);
        return;
    }

    sqlite3_uint64 olen = (sqlite3_uint64)reps * tlen;
    unsigned char* output = sqlite3_malloc64(olen);
    if (!output) {
        sqlite3_result_error_nomem(context);
        return;
    }

    size_t off = 0;
    while (reps--) {
        memcpy(output + off, t, tlen);
        off += tlen;
    }
    sqlite3_result_text64(context, (const char*)output, olen, sqlite3_free, SQLITE_UTF8);
}

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_text_init(sqlite3* db, char** pzErrMsg, const sqlite3_api_routines* pApi) {
    SQLITE_EXTENSION_INIT2(pApi);
    const int flags = SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC;
    sqlite3_create_function(db, "concat", -1, flags, 0, sf_concat, 0, 0);
    sqlite3_create_function(db, "concat_ws", -1, flags, 0, sf_concat_ws, 0, 0);
    sqlite3_create_function(db, "repeat", 2, flags, 0, sf_repeat8, 0, 0);
    return SQLITE_OK;
}
