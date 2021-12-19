// Created by Keith Medcalf, Public Domain
// http://www.dessus.com/files/sqlfcmp.c

/*
** 2015-11-09
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
******************************************************************************
**
** This SQLite extension implements functions for comparison of floating point
** double precision numbers x and y where x is within a specified number of
** ULPs (units of least precision) of y.
**
** The default number of ULPs if not specified is 5.
** The size of an ULP is based on the scale of x.
** If only one value is specified it will be a y value and x will be 0.0
**
**   ulp(x)     -> ULP precision of x (increment to next representable number)
**   epsilon(x) -> ULP precision of x (alias of ulp)
**   ulps(x, y) -> number of ULP of x in (x - y)
**
**   flt(x[, y[, u]])   -> x less than y
**   fle(x[, y[, u]])   -> x less or equal y
**   feq(x[, y[, u]])   -> x equal y
**   fge(x[, y[, u]])   -> x greater or equal y
**   fgt(x[, y[, u]])   -> x greater than y
**   fne(x[, y[, u]])   -> x not equal y
**
**
** It also implements a Gaussian / Statistical / Bankers rounding algorithm
** to replace the builtin round function.  This also rounds according to the
** IEEE 754 recommendation to round to nearest, ties to even:
**
**  > 0.5 rounds away from zero
**  = 0.5 rounds to even
**  < 0.5 rounds towards zero
**
*/

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

/*
 * Compute the Epsilon (Distance to Next Representable Number)
 * for double precision floating point numbers of scale value
 */

static __inline double epsilon(double value, int significand) {
    int exponent;
    double mantissa = frexp(value, &exponent);
    return ldexp(1.0, exponent - significand);
}

/*
 * Implements if(a, b, c) which tests a returning either b or c
 *  if a is a non-empty string/blob return b else c
 *  if a is a non-zero float/integer return b else c
 *  if a is null return c
 */

static void _ifFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
    register int which = 0;
    switch (sqlite3_value_type(argv[0])) {
        case SQLITE_NULL: {
            which = 2;
            break;
        }
        case SQLITE_INTEGER: {
            which = sqlite3_value_int64(argv[0]) ? 1 : 2;
            break;
        }
        case SQLITE_FLOAT: {
            which = sqlite3_value_double(argv[0]) ? 1 : 2;
            break;
        }
        case SQLITE_TEXT:
        case SQLITE_BLOB: {
            which = sqlite3_value_bytes(argv[0]) ? 1 : 2;
            break;
        }
    }
    if (which > 0) {
        sqlite3_result_value(context, argv[which]);
    }
    return;
}

static void _rounddigitsFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
    double x, scale;
    int digits = 14;

    if ((argc == 0) || (argc > 2)) {
        return;
    }
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        return;
    }

    x = sqlite3_value_double(argv[0]);
    if (argc == 2) {
        digits = sqlite3_value_int(argv[1]);
    }

    digits = fmax(fmin(digits, 14), 1) - ceil(log10(fabs(x)));
    scale = pow(10.0, digits);
    x *= scale;
    x -= remainder(x, 1.0);
    x /= scale;

    sqlite3_result_double(context, x);
    return;
}

/*
 * Implement a fast Nearest Half-to-Even Rounder function
 * Theoretically IEEE precise
 * Use for roundhe and money rounding
 */

static void _fastroundingFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
    double x, scale;
    int digits = 0;

    if ((argc == 0) || (argc > 2)) {
        return;
    }
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        return;
    }
    x = sqlite3_value_double(argv[0]);
    if ((intptr_t)sqlite3_user_data(context) == 1) {
        digits = 4;
    } else if (argc == 2) {
        digits = sqlite3_value_int(argv[1]);
    }
    digits = fmin(digits, 15);

    scale = pow(10.0, digits);
    x *= scale;
    x -= remainder(x, 1.0);
    x /= scale;

    sqlite3_result_double(context, x);
    return;
}

/*
 * Define a Rounding function
 *
 * user data indicates rounding method to use
 *
 *  0 : Nearest Half to Odd
 *  1 : Nearest Half to Even
 *  2 : Nearest Half Away from 0
 *  3 : Nearest Half Towards 0
 *  4 : Directed Down
 *  5 : Directed Up
 *  6 : Directed Towards 0
 *  7 : Directed Away from 0
 *  8 : Nearest Half Towards -Inf
 *  9 : Nearest Half Towards +Inf
 * 10 : Money Special (Nearest Half to Even, 4 digits)
 *
 * Inherently less accurate than the function above
 *
 */

static void _heroundingFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
    intptr_t flag = (intptr_t)sqlite3_user_data(context);
    double sgn = 1.0;
    double x, scale, scaledx, ipart, fpart;
    int p = 0;

    if ((argc == 0) || (argc > 2)) {
        return;
    }
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        return;
    }
    x = sqlite3_value_double(argv[0]);
    if (flag == 10) /* Money is 4 Digits Half to Even  */
    {
        p = 4;
        flag = 1;
    }
    if (argc == 2) {
        if (sqlite3_value_type(argv[1]) == SQLITE_NULL) {
            return;
        }
        p = sqlite3_value_int(argv[1]);
        p = p > 15 ? 15 : (p < 0 ? 0 : p);
    }
    if (fabs(x) > 4503599627370496.0)  // Short Circuit if no fractional part
    {
        sqlite3_result_double(context, x);
        return;
    }
    sgn = 1.0;
    scale = pow(10.0, p);
    scaledx = x * scale;
    if (fabs(scaledx) > 4503599627370496.0)  // Short Circuit if no fractional part
    {
        sqlite3_result_double(context, x);
        return;
    }
    if ((flag & 0x04) == 0x04)  // Directed Rounding
    {
        if ((flag & 0x02) == 0x02)  // Up/Down or Toward/Away 0
        {
            sgn = (x < 0.0) ? -1.0 : 1.0;
        }
        if (flag & 0x01) {
            ipart = ceil(sgn * scaledx);  // Up or Away from 0
        } else {
            ipart = floor(sgn * scaledx);  // Down or Towards 0
        }
    } else  // Round Nearest with Half
    {
        char zBuf[8];
        //      if (fabs(x) > 35184372088832.0) // Insufficient bits to do half-rounding
        //      {
        //          sqlite3_result_double(context, x);
        //          return;
        //      }
        sgn = (x < 0.0) ? -1.0 : 1.0;
        fpart = modf(sgn * scaledx, &ipart);
        //      snprintf(zBuf, 8, "%.2f", fpart);
        //      fpart = atof(zBuf);
        {
            if (((flag == 0) &&
                 ((fpart > 0.5) || ((fpart == 0.5) && (fmod(ipart, 2.0) == 0.0)))) ||  // to odd
                ((flag == 1) &&
                 ((fpart > 0.5) || ((fpart == 0.5) && (fmod(ipart, 2.0) == 1.0)))) ||  // to even
                ((flag == 2) && (fpart >= 0.5)) ||                                     // away 0
                ((flag == 3) && (fpart > 0.5)) ||                                      // towards 0
                ((flag == 8) && (sgn > 0) && (fpart > 0.5)) ||   // towards -Inf
                ((flag == 8) && (sgn < 0) && (fpart >= 0.5)) ||  // towards -Inf
                ((flag == 9) && (sgn > 0) && (fpart >= 0.5)) ||  // towards +Inf
                ((flag == 9) && (sgn < 0) && (fpart > 0.5)))     // towards +Inf
            {
                ipart += 1.0;
            }
        }
    }
    sqlite3_result_double(context, sgn * ipart / scale);
}

/*
 * Define a floating point truncation function
 * Retains 1 ulp precision
 */

static void _fptruncateFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
    int p = 0;
    double x, scale, ipart, fpart;

    if ((sqlite3_value_type(argv[0]) == SQLITE_NULL) ||
        (sqlite3_value_type(argv[1]) == SQLITE_NULL)) {
        return;
    }
    x = sqlite3_value_double(argv[0]);
    p = sqlite3_value_int(argv[1]);
    scale = pow(10.0, p);
    fpart = modf(x * scale, &ipart);
    sqlite3_result_double(context, ipart / scale);
}

/*
 * Compute the distance of X from Y in units of the Epsilon of X
 */

static __inline double distance(double x, double y) {
    return (x - y) / epsilon(x, 53);
}

/*
 * Return the distance to the next representable number
 */

static void _ulp(sqlite3_context* context, int argc, sqlite3_value** argv) {
    double x = 0.0;
    int y = 0;

    if ((argc > 2) || (argc < 1)) {
        return;
    }
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        return;
    } else {
        x = sqlite3_value_double(argv[0]);
    }
    if ((argc == 2) && (sqlite3_value_type(argv[1]) != SQLITE_NULL)) {
        y = sqlite3_value_int(argv[1]);
    }
    if (y == 0) {
        y = 53;
    }
    sqlite3_result_double(context, epsilon(x, y));
}

/*
 * Return the number of representable numbers between x and y based on the ULP of x
 * if only one parameter, it is y and x is 0.0
 */

static void _ulps(sqlite3_context* context, int argc, sqlite3_value** argv) {
    double x = 0.0;
    double y = 0.0;
    if ((argc == 0) || (argc > 2)) {
        return;
    }
    if (argc == 1) {
        if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
            return;
        }
        y = sqlite3_value_double(argv[0]);
    } else {
        if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
            return;
        }
        if (sqlite3_value_type(argv[1]) == SQLITE_NULL) {
            return;
        }
        x = sqlite3_value_double(argv[0]);
        y = sqlite3_value_double(argv[1]);
    }
    sqlite3_result_double(context, distance(x, y));
}

/*
 * Perform Floating Point (Double) Comparisons using Epsilon of first parameter
 *   if only one value provided, it will be Y and X will be 0.0
 * User Context defines operations
 *  flag & 1        x < y
 *  flag & 2        x = y
 *  flag & 4        x > y
 * Flag values are additive
 *  flag & 3        x <= y
 *  flag & 6        x >= y
 *  flag & 5        x <> y
 * optional third parameter is distance in Epsilon of x to consider equal, defaults to 5
 * if only one parameter then test if x within 5 ULPs of 0.0 to machine epsilon
 */

static void _fpc(sqlite3_context* context, int argc, sqlite3_value** argv) {
    double x = 0.0;
    double y = 0.0;
    double ulps = 0.0;
    double delta = 5.0;
    uintptr_t flag = (uintptr_t)sqlite3_user_data(context);
    if ((argc == 0) || (argc > 3)) {
        return;
    }
    if (argc == 1) {
        if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
            return;
        }
        y = sqlite3_value_double(argv[0]);
    } else {
        if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
            return;
        }
        if (sqlite3_value_type(argv[1]) == SQLITE_NULL) {
            return;
        }
        x = sqlite3_value_double(argv[0]);
        y = sqlite3_value_double(argv[1]);
    }
    if (argc > 2) {
        if (sqlite3_value_type(argv[2]) == SQLITE_NULL) {
            return;
        }
        delta = fabs(sqlite3_value_double(argv[2]));
    }
    ulps = distance(x, y);
    if (((flag & 2) && (fabs(ulps) <= delta)) /* flag & 2 means test equal */
        || ((flag & 1) && (ulps < -delta))    /* flag & 1 means test less that */
        || ((flag & 4) && (ulps > delta)))    /* flag & 4 means test greater than */
    {
        sqlite3_result_int(context, 1);
    } else {
        sqlite3_result_int(context, 0);
    }
}

/*
 * SQLite function randomV
 *
 * using the standard source of randomness (PRNG) this function returns a random
 * floating point 0 <= value <= 1
 *
 * if an optional argument is provided (which must be an integer) representing the
 * number of buckets in which to divide the random value range is provided then
 * the return value is the integer bucket number 0 <= bucket < buckets
 */

static void _randomValue(sqlite3_context* context, int argc, sqlite3_value** argv) {
    sqlite_uint64 r;
    register sqlite_uint64 value;

    if ((argc > 1) || ((argc == 1) && (sqlite3_value_type(argv[0]) != SQLITE_INTEGER))) {
        return;
    }
    sqlite3_randomness(sizeof(r), &r);
    value = r / 18446744073709551615.0L;
    if (argc == 1) {
        sqlite3_result_int64(context, floor(sqlite3_value_int64(argv[0]) * value));
    } else {
        sqlite3_result_double(context, value);
    }
}

/*
 * If compiling as a builtin extension, don't export the initializer -- make it static
 */

/* SQLite invokes this routine once when it loads the extension.
 * Create new functions, collating sequences, and virtual table
 * modules here.  This is usually the only exported symbol in
 * the shared library.
 */

#ifdef _WIN32
#ifndef SQLITE_CORE
__declspec(dllexport)
#endif
#endif
    int sqlite3_fcmp_init(sqlite3* db, char** pzErrMsg, const sqlite3_api_routines* pApi) {
    int nErr = 0;

    SQLITE_EXTENSION_INIT2(pApi);
    int flags = SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS;
    int flagsUI = SQLITE_UTF8 | SQLITE_INNOCUOUS;
    sqlite3_create_function(db, "if", 3, flags, 0, _ifFunc, 0, 0);
    sqlite3_create_function(db, "ulp", -1, flags, 0, _ulp, 0, 0);
    sqlite3_create_function(db, "epsilon", -1, flags, 0, _ulp, 0, 0);
    sqlite3_create_function(db, "ulps", -1, flags, 0, _ulps, 0, 0);
    sqlite3_create_function(db, "flt", -1, flags, (void*)1, _fpc, 0, 0);
    sqlite3_create_function(db, "feq", -1, flags, (void*)2, _fpc, 0, 0);
    sqlite3_create_function(db, "fle", -1, flags, (void*)3, _fpc, 0, 0);
    sqlite3_create_function(db, "fgt", -1, flags, (void*)4, _fpc, 0, 0);
    sqlite3_create_function(db, "fne", -1, flags, (void*)5, _fpc, 0, 0);
    sqlite3_create_function(db, "fge", -1, flags, (void*)6, _fpc, 0, 0);
    sqlite3_create_function(db, "roundho", -1, flags, (void*)0, _heroundingFunc, 0, 0);
    // sqlite3_create_function(db, "roundhe", -1, flags, (void*)1, _heroundingFunc, 0, 0);
    sqlite3_create_function(db, "roundha", -1, flags, (void*)2, _heroundingFunc, 0, 0);
    sqlite3_create_function(db, "roundht", -1, flags, (void*)3, _heroundingFunc, 0, 0);
    sqlite3_create_function(db, "rounddd", -1, flags, (void*)4, _heroundingFunc, 0, 0);
    sqlite3_create_function(db, "rounddu", -1, flags, (void*)5, _heroundingFunc, 0, 0);
    sqlite3_create_function(db, "rounddt", -1, flags, (void*)6, _heroundingFunc, 0, 0);
    sqlite3_create_function(db, "roundda", -1, flags, (void*)7, _heroundingFunc, 0, 0);
    sqlite3_create_function(db, "roundhd", -1, flags, (void*)8, _heroundingFunc, 0, 0);
    sqlite3_create_function(db, "roundhu", -1, flags, (void*)9, _heroundingFunc, 0, 0);
    // sqlite3_create_function(db, "money", 1, flags, (void*)10, _heroundingFunc, 0, 0);
    // sqlite3_create_function(db, "trunc", 2, flags, 0, _fptruncateFunc, 0, 0);
    // sqlite3_create_function(db, "randomV", -1, flagsUI, 0, _randomValue, 0, 0);

    // Use this function for roundhe and money(4 places) rounding
    // because it has higher IEEE compliance
    sqlite3_create_function(db, "roundhe", -1, flags, 0, _fastroundingFunc, 0, 0);
    sqlite3_create_function(db, "money", 1, flags, (void*)1, _fastroundingFunc, 0, 0);
    sqlite3_create_function(db, "sigdigits", -1, flags, 0, _rounddigitsFunc, 0, 0);

    return SQLITE_OK;
}
