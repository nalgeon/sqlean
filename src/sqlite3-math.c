// Originally from SQLite 3.35.4 source code (func.c), Public Domain
// Modified by Anton Zhiyanov, https://github.com/nalgeon/sqlean/, MIT License

/*
 * SQLite math functions.
 */
#include <assert.h>
#include <math.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

#if defined(HAVE_STDINT_H) /* Use this case if we have ANSI headers */
#define SQLITE_INT_TO_PTR(X) ((void*)(intptr_t)(X))
#define SQLITE_PTR_TO_INT(X) ((int)(intptr_t)(X))
#elif defined(__PTRDIFF_TYPE__) /* This case should work for GCC */
#define SQLITE_INT_TO_PTR(X) ((void*)(__PTRDIFF_TYPE__)(X))
#define SQLITE_PTR_TO_INT(X) ((int)(__PTRDIFF_TYPE__)(X))
#elif !defined(__GNUC__) /* Works for compilers other than LLVM */
#define SQLITE_INT_TO_PTR(X) ((void*)&((char*)0)[X])
#define SQLITE_PTR_TO_INT(X) ((int)(((char*)X) - (char*)0))
#else /* Generates a warning - but it always works */
#define SQLITE_INT_TO_PTR(X) ((void*)(X))
#define SQLITE_PTR_TO_INT(X) ((int)(X))
#endif

/* Mathematical Constants */
#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884
#endif
#ifndef M_LN10
#define M_LN10 2.302585092994045684017991454684364208
#endif
#ifndef M_LN2
#define M_LN2 0.693147180559945309417232121458176568
#endif

/*
** Implementation SQL functions:
**
**   ceil(X)
**   ceiling(X)
**   floor(X)
**
** The sqlite3_user_data() pointer is a pointer to the libm implementation
** of the underlying C function.
*/
static void ceilingFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    switch (sqlite3_value_numeric_type(argv[0])) {
        case SQLITE_INTEGER: {
            sqlite3_result_int64(context, sqlite3_value_int64(argv[0]));
            break;
        }
        case SQLITE_FLOAT: {
            double (*x)(double) = (double (*)(double))sqlite3_user_data(context);
            sqlite3_result_double(context, x(sqlite3_value_double(argv[0])));
            break;
        }
        default: {
            break;
        }
    }
}

/*
** On some systems, ceil() and floor() are intrinsic function.  You are
** unable to take a pointer to these functions.  Hence, we here wrap them
** in our own actual functions.
*/
static double xCeil(double x) {
    return ceil(x);
}
static double xFloor(double x) {
    return floor(x);
}

/*
** Implementation of SQL functions:
**
**   ln(X)       - natural logarithm
**   log(X)      - log X base 10
**   log10(X)    - log X base 10
*/
static void log1Func(sqlite3_context* context, int argc, sqlite3_value** argv) {
    double x, ans;
    assert(argc == 1);
    switch (sqlite3_value_numeric_type(argv[0])) {
        case SQLITE_INTEGER:
        case SQLITE_FLOAT:
            x = sqlite3_value_double(argv[0]);
            if (x <= 0.0)
                return;
            break;
        default:
            return;
    }
    ans = log(x);
    switch (SQLITE_PTR_TO_INT(sqlite3_user_data(context))) {
        case 1:
            /* Convert from natural logarithm to log base 10 */
            ans *= 1.0 / M_LN10;
            break;
        case 2:
            /* Convert from natural logarithm to log base 2 */
            ans *= 1.0 / M_LN2;
            break;
        default:
            break;
    }
    sqlite3_result_double(context, ans);
}

/*
** Implementation of SQL functions:
**
**   log(B,X)    - log X base B
*/
static void log2Func(sqlite3_context* context, int argc, sqlite3_value** argv) {
    double x, b, ans;
    assert(argc == 2);
    switch (sqlite3_value_numeric_type(argv[0])) {
        case SQLITE_INTEGER:
        case SQLITE_FLOAT:
            x = sqlite3_value_double(argv[0]);
            if (x <= 0.0)
                return;
            break;
        default:
            return;
    }
    switch (sqlite3_value_numeric_type(argv[0])) {
        case SQLITE_INTEGER:
        case SQLITE_FLOAT:
            b = log(x);
            if (b <= 0.0)
                return;
            x = sqlite3_value_double(argv[1]);
            if (x <= 0.0)
                return;
            break;
        default:
            return;
    }
    ans = log(x) / b;
    sqlite3_result_double(context, ans);
}

/*
** Functions to converts degrees to radians and radians to degrees.
*/
static double degToRad(double x) {
    return x * (M_PI / 180.0);
}
static double radToDeg(double x) {
    return x * (180.0 / M_PI);
}

/*
** Implementation of 1-argument SQL math functions:
**
**   exp(X)  - Compute e to the X-th power
*/
static void math1Func(sqlite3_context* context, int argc, sqlite3_value** argv) {
    int type0;
    double v0, ans;
    double (*x)(double);
    assert(argc == 1);
    type0 = sqlite3_value_numeric_type(argv[0]);
    if (type0 != SQLITE_INTEGER && type0 != SQLITE_FLOAT)
        return;
    v0 = sqlite3_value_double(argv[0]);
    x = (double (*)(double))sqlite3_user_data(context);
    ans = x(v0);
    sqlite3_result_double(context, ans);
}

/*
** Implementation of 2-argument SQL math functions:
**
**   power(X,Y)  - Compute X to the Y-th power
*/
static void math2Func(sqlite3_context* context, int argc, sqlite3_value** argv) {
    int type0, type1;
    double v0, v1, ans;
    double (*x)(double, double);
    assert(argc == 2);
    type0 = sqlite3_value_numeric_type(argv[0]);
    if (type0 != SQLITE_INTEGER && type0 != SQLITE_FLOAT)
        return;
    type1 = sqlite3_value_numeric_type(argv[1]);
    if (type1 != SQLITE_INTEGER && type1 != SQLITE_FLOAT)
        return;
    v0 = sqlite3_value_double(argv[0]);
    v1 = sqlite3_value_double(argv[1]);
    x = (double (*)(double, double))sqlite3_user_data(context);
    ans = x(v0, v1);
    sqlite3_result_double(context, ans);
}

/*
** Implementation of pi() SQL math function
*/
static void piFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 0);
    sqlite3_result_double(context, M_PI);
}

/*
 * Registers the extension.
 */
#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_math_init(sqlite3* db, char** pzErrMsg, const sqlite3_api_routines* pApi) {
    static const int flags = SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC;
    SQLITE_EXTENSION_INIT2(pApi);
    sqlite3_create_function(db, "ceil", 1, flags, xCeil, ceilingFunc, 0, 0);
    sqlite3_create_function(db, "ceiling", 1, flags, xCeil, ceilingFunc, 0, 0);
    sqlite3_create_function(db, "floor", 1, flags, xFloor, ceilingFunc, 0, 0);
    sqlite3_create_function(db, "trunc", 1, flags, trunc, ceilingFunc, 0, 0);
    sqlite3_create_function(db, "ln", 1, flags, 0, log1Func, 0, 0);
    sqlite3_create_function(db, "log", 1, flags, (void*)(1), log1Func, 0, 0);
    sqlite3_create_function(db, "log10", 1, flags, (void*)(1), log1Func, 0, 0);
    sqlite3_create_function(db, "log2", 1, flags, (void*)(2), log1Func, 0, 0);
    sqlite3_create_function(db, "log", 2, flags, 0, log2Func, 0, 0);
    sqlite3_create_function(db, "exp", 1, flags, exp, math1Func, 0, 0);
    sqlite3_create_function(db, "pow", 2, flags, pow, math2Func, 0, 0);
    sqlite3_create_function(db, "power", 2, flags, pow, math2Func, 0, 0);
    sqlite3_create_function(db, "mod", 2, flags, fmod, math2Func, 0, 0);
    sqlite3_create_function(db, "acos", 1, flags, acos, math1Func, 0, 0);
    sqlite3_create_function(db, "asin", 1, flags, asin, math1Func, 0, 0);
    sqlite3_create_function(db, "atan", 1, flags, atan, math1Func, 0, 0);
    sqlite3_create_function(db, "atan2", 2, flags, atan2, math2Func, 0, 0);
    sqlite3_create_function(db, "cos", 1, flags, cos, math1Func, 0, 0);
    sqlite3_create_function(db, "sin", 1, flags, sin, math1Func, 0, 0);
    sqlite3_create_function(db, "tan", 1, flags, tan, math1Func, 0, 0);
    sqlite3_create_function(db, "cosh", 1, flags, cosh, math1Func, 0, 0);
    sqlite3_create_function(db, "sinh", 1, flags, sinh, math1Func, 0, 0);
    sqlite3_create_function(db, "tanh", 1, flags, tanh, math1Func, 0, 0);
    sqlite3_create_function(db, "acosh", 1, flags, acosh, math1Func, 0, 0);
    sqlite3_create_function(db, "asinh", 1, flags, asinh, math1Func, 0, 0);
    sqlite3_create_function(db, "atanh", 1, flags, atanh, math1Func, 0, 0);
    sqlite3_create_function(db, "sqrt", 1, flags, sqrt, math1Func, 0, 0);
    sqlite3_create_function(db, "radians", 1, flags, degToRad, math1Func, 0, 0);
    sqlite3_create_function(db, "degrees", 1, flags, radToDeg, math1Func, 0, 0);
    sqlite3_create_function(db, "pi", 0, flags, 0, piFunc, 0, 0);
    return SQLITE_OK;
}