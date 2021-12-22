/*
 * Implements ToBestType SQL Function
 *
 * Takes one value parameter and returns the Best Type value
 *
 * NULL returns NULL
 * TEXT/BLOB that "looks like a number" returns the number as Integer or Real as appropriate
 * TEXT/BLOB that is zero-length returns NULL
 * Otherwise returns what was given
 *
 * Written 2020 by Keith Medcalf
 * Given to the Public Domain.  If you break it, you own all the pieces.
 */

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

/*
**
** Determine affinity of field
**
** ignore leading space
** then may have + or -
** then may have digits or . (if . found then type=real)
** then may have digits (if another . then not number)
** then may have e (if found then type=real)
** then may have + or -
** then may have digits
** then may have trailing space
*/
static int _isValidNumber(char* arg) {
    char* start;
    char* stop;
    int isValid = 0;
    int hasDigit = 0;

    start = arg;
    stop = arg + strlen(arg) - 1;
    while (start <= stop && *start == ' ')  // strip spaces from begining
    {
        start++;
    }
    while (start <= stop && *stop == ' ')  // strip spaces from end
    {
        stop--;
    }
    if (start > stop) {
        goto _end_isValidNumber;
    }
    if (start <= stop && (*start == '+' || *start == '-'))  // may have + or -
    {
        start++;
    }
    if (start <= stop && isdigit(*start))  // must have a digit to be valid
    {
        hasDigit = 1;
        isValid = 1;
    }
    while (start <= stop && isdigit(*start))  // bunch of digits
    {
        start++;
    }
    if (start <= stop && *start == '.')  // may have .
    {
        isValid = 2;
        start++;
    }
    if (start <= stop && isdigit(*start)) {
        hasDigit = 1;
    }
    while (start <= stop && isdigit(*start))  // bunch of digits
    {
        start++;
    }
    if (!hasDigit)  // no digits then invalid
    {
        isValid = 0;
        goto _end_isValidNumber;
    }
    if (start <= stop && (*start == 'e' || *start == 'E'))  // may have 'e' or 'E'
    {
        isValid = 3;
        start++;
    }
    if (start <= stop && isValid == 3 && (*start == '+' || *start == '-')) {
        start++;
    }
    if (start <= stop && isValid == 3 && isdigit(*start)) {
        isValid = 2;
    }
    while (start <= stop && isdigit(*start))  // bunch of digits
    {
        start++;
    }
    if (isValid == 3) {
        isValid = 0;
    }
_end_isValidNumber:
    if (start <= stop) {
        isValid = 0;
    }
    return isValid;
}

static void _ToBestType(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    char* pCur;

    switch (sqlite3_value_type(argv[0])) {
        case SQLITE_NULL: {
            return;
        }
        case SQLITE_BLOB: {
            pCur = (char*)sqlite3_value_blob(argv[0]);
            break;
        }
        default: {
            pCur = (char*)sqlite3_value_text(argv[0]);
            break;
        }
    }
    if (sqlite3_value_bytes(argv[0]) == 0) {
        return;
    }
    switch (_isValidNumber(pCur)) {
        case 1: {
            sqlite3_result_int64(ctx, strtoll(pCur, 0, 10));
            break;
        }
        case 2: {
            long double dv, fp, ip;

            dv = strtold(pCur, 0);
            fp = modfl(dv, &ip);
            if (sizeof(long double) > sizeof(double)) {
                if (fp == 0.0L && dv >= -9223372036854775808.0L && dv <= 9223372036854775807.0L) {
                    sqlite3_result_int64(ctx, (long long)dv);
                } else {
                    sqlite3_result_double(ctx, (double)dv);
                }
            } else {
                // Only convert if it will fit in a 6-byte varint
                if (fp == 0.0L && dv >= -140737488355328.0L && dv <= 140737488355328.0L) {
                    sqlite3_result_int64(ctx, (long long)dv);
                } else {
                    sqlite3_result_double(ctx, (double)dv);
                }
            }
            break;
        }
        default: {
            sqlite3_result_value(ctx, argv[0]);
            break;
        }
    }
    return;
}

#ifdef _WIN32
#ifndef SQLITE_CORE
__declspec(dllexport)
#endif
#endif
    int sqlite3_numeric_init(sqlite3* db, char** pzErrMsg, const sqlite3_api_routines* pApi) {
    SQLITE_EXTENSION_INIT2(pApi);
    return sqlite3_create_function(db, "ToBestType", 1, SQLITE_UTF8 | SQLITE_INNOCUOUS, 0,
                                   _ToBestType, 0, 0);
}
