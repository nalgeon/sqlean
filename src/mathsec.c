// Created by Keith Medcalf, Public Domain
// http://www.dessus.com/files/sqlmath.c

#include <ctype.h>
#include <math.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

/*
** Hooks into the MSVC/GCC Runtime
** Return Constants, call Runtime Functions
*/

typedef double (*dfdPtr)(double);
typedef double (*dfddPtr)(double, double);
typedef double (*dfidPtr)(int, double);
typedef double (*dfdiPtr)(double, int);

#ifndef LONGDOUBLE_TYPE
#define LONGDOUBLE_TYPE long double
#endif

static LONGDOUBLE_TYPE m_deg2rad, m_rad2deg, m_pi, m_pi_2, m_pi_4, m_e, m_ln2, m_ln10, m_log2e,
    m_log10e;
static LONGDOUBLE_TYPE m_1_pi, m_2_pi, m_2_sqrtpi, m_sqrt2, m_sqrt1_2;

/*
** Return Mathematical Constants
*/

static void _dfc(sqlite3_context* context, int argc, sqlite3_value** argv) {
    sqlite3_result_double(context, *((LONGDOUBLE_TYPE*)sqlite3_user_data(context)));
}

/*
** Dispatch to Math Library (Runtime) Functions
*/

static void _dfd(sqlite3_context* context, int argc, sqlite3_value** argv) {
    sqlite3_result_double(context,
                          (*(dfdPtr)sqlite3_user_data(context))(sqlite3_value_double(argv[0])));
}

static void _dfdd(sqlite3_context* context, int argc, sqlite3_value** argv) {
    sqlite3_result_double(context,
                          (*(dfddPtr)sqlite3_user_data(context))(sqlite3_value_double(argv[0]),
                                                                 sqlite3_value_double(argv[1])));
}

static void _dfid(sqlite3_context* context, int argc, sqlite3_value** argv) {
    sqlite3_result_double(context, (*(dfidPtr)sqlite3_user_data(context))(
                                       sqlite3_value_int(argv[0]), sqlite3_value_double(argv[1])));
}

static void _dfdi(sqlite3_context* context, int argc, sqlite3_value** argv) {
    sqlite3_result_double(context, (*(dfdiPtr)sqlite3_user_data(context))(
                                       sqlite3_value_double(argv[0]), sqlite3_value_int(argv[1])));
}

/*
** Convert Text Argument which is hex string into decimal
*/

static void _FromHex(sqlite3_context* context, int argc, sqlite3_value** argv) {
    const char* src;
    unsigned long long value = 0;

    if (sqlite3_value_type(argv[0]) == SQLITE_TEXT) {
        src = (const char*)sqlite3_value_text(argv[0]);
        if (sqlite3_value_bytes(argv[0]) > 16)
            return;
        while (*src != 0) {
            if (!isxdigit(*src))
                return;
            value = value << 4;
            value += (*src > '9' ? ((*src & 0x0f) + 9) : (*src & 0x0f));
            src++;
        }
        sqlite3_result_int64(context, (sqlite_int64)value);
    }
}

/*
** Specific Mathematical Functions
*/

static void _TestNull(sqlite3_context* context, int argc, sqlite3_value** argv) {
    if (sqlite3_value_type(argv[0]) != SQLITE_NULL)
        sqlite3_result_value(context, argv[0]);
    else
        sqlite3_result_error(context, "ErrorIfNull value is NULL", -1);
}

#if !defined(SQLITE_ENABLE_MATH_FUNCTIONS)
static void _sign(sqlite3_context* context, int argc, sqlite3_value** argv) {
    double x = sqlite3_value_double(argv[0]);
    int res = 0;
    if (x < 0)
        res = -1;
    else if (x > 0)
        res = 1;
    sqlite3_result_int(context, res);
}
#endif

static void _mantissa(sqlite3_context* context, int argc, sqlite3_value** argv) {
    int exponent;
    sqlite3_result_double(context, frexp(sqlite3_value_double(argv[0]), &exponent));
}

static void _exponent(sqlite3_context* context, int argc, sqlite3_value** argv) {
    int exponent;
    double mantissa = frexp(sqlite3_value_double(argv[0]), &exponent);
    sqlite3_result_int(context, exponent);
}

static void _fracpart(sqlite3_context* context, int argc, sqlite3_value** argv) {
    double ipart, fpart;

    fpart = modf(sqlite3_value_double(argv[0]), &ipart);
    sqlite3_result_double(context, fpart);
}

static void _intpart(sqlite3_context* context, int argc, sqlite3_value** argv) {
    double ipart, fpart;

    fpart = modf(sqlite3_value_double(argv[0]), &ipart);
    sqlite3_result_double(context, ipart);
}

#if !defined(SQLITE_ENABLE_MATH_FUNCTIONS)
static void _radians(sqlite3_context* context, int argc, sqlite3_value** argv) {
    sqlite3_result_double(context, sqlite3_value_double(argv[0]) * m_deg2rad);
}

static void _degrees(sqlite3_context* context, int argc, sqlite3_value** argv) {
    sqlite3_result_double(context, sqlite3_value_double(argv[0]) * m_rad2deg);
}
#endif

/*
** Test (Set/Clear) Set/Clear bits
*/

/*
 * isset(value, bit, bit, bit ...)
 * return true if all bits are set in value
 */
static void _isset(sqlite3_context* context, int argc, sqlite3_value** argv) {
    sqlite_uint64 value = sqlite3_value_int64(argv[0]);
    sqlite_uint64 mask = 0;
    int bit, i;
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL)
        return;
    for (i = 1; i < argc; i++) {
        if (sqlite3_value_type(argv[i]) == SQLITE_NULL)
            continue;
        bit = sqlite3_value_int(argv[i]);
        if ((bit >= 0) && (bit <= 63))
            mask |= (1ULL << bit);
    }
    sqlite3_result_int(context, (value & mask) == mask);
}

/*
 * isclr(value, bit, bit, bit ...)
 * return true if all bits are clr in value
 */
static void _isclr(sqlite3_context* context, int argc, sqlite3_value** argv) {
    sqlite_uint64 value = sqlite3_value_int64(argv[0]);
    sqlite_uint64 mask = 0;
    int bit, i;
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL)
        return;
    for (i = 1; i < argc; i++) {
        if (sqlite3_value_type(argv[i]) == SQLITE_NULL)
            continue;
        bit = sqlite3_value_int(argv[i]);
        if ((bit >= 0) && (bit <= 63))
            mask |= (1ULL << bit);
    }
    sqlite3_result_int(context, (value & mask) == 0ULL);
}

/*
 * ismaskset(value, mask)
 * return true if all set bits in mask set in value
 */
static void _ismaskset(sqlite3_context* context, int argc, sqlite3_value** argv) {
    sqlite_uint64 value = sqlite3_value_int64(argv[0]);
    sqlite_uint64 mask = sqlite3_value_int64(argv[1]);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL)
        return;
    if (sqlite3_value_type(argv[1]) == SQLITE_NULL)
        return;
    sqlite3_result_int(context, (value & mask) == mask);
}

/*
 * ismaskclr(value, mask)
 * return true if all set bits set in mask are clr in value
 */
static void _ismaskclr(sqlite3_context* context, int argc, sqlite3_value** argv) {
    sqlite_uint64 value = sqlite3_value_int64(argv[0]);
    sqlite_uint64 mask = sqlite3_value_int64(argv[1]);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL)
        return;
    if (sqlite3_value_type(argv[1]) == SQLITE_NULL)
        return;
    sqlite3_result_int(context, (value & mask) == 0ULL);
}

/*
 * bitmask(bit, bit, bit ...)
 * return value of bitmask with bits set
 */
static void _bitmask(sqlite3_context* context, int argc, sqlite3_value** argv) {
    sqlite_uint64 mask = 0;
    int bit, i;
    for (i = 0; i < argc; i++) {
        if (sqlite3_value_type(argv[i]) == SQLITE_NULL)
            continue;
        bit = sqlite3_value_int(argv[i]);
        if ((bit >= 0) && (bit <= 63))
            mask |= (1ULL << bit);
    }
    sqlite3_result_int64(context, (sqlite_int64)mask);
}

/* setbits(value, bit, bit, ...)
 * return value with bits set
 */
static void _setbits(sqlite3_context* context, int argc, sqlite3_value** argv) {
    sqlite_uint64 value = sqlite3_value_int64(argv[0]);
    int bit, i;
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL)
        return;
    for (i = 1; i < argc; i++) {
        if (sqlite3_value_type(argv[i]) == SQLITE_NULL)
            continue;
        bit = sqlite3_value_int(argv[i]);
        if ((bit >= 0) && (bit <= 63))
            value |= (1ULL << bit);
    }
    sqlite3_result_int64(context, value);
}

/* clrbits(value, bit, bit, ...)
 * return value with bits cleared
 */
static void _clrbits(sqlite3_context* context, int argc, sqlite3_value** argv) {
    sqlite_uint64 value = sqlite3_value_int(argv[0]);
    int bit, i;
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL)
        return;
    for (i = 1; i < argc; i++) {
        if (sqlite3_value_type(argv[i]) == SQLITE_NULL)
            continue;
        bit = sqlite3_value_int(argv[i]);
        if ((bit >= 0) && (bit <= 63))
            value &= ~(1ULL << bit);
    }
    sqlite3_result_int64(context, value);
}

/*
** bitmask aggregate -- set bits and return resulting mask
*/

static void _bitmaskFinal(sqlite3_context* context) {
    sqlite_uint64* mask = sqlite3_aggregate_context(context, sizeof(sqlite_uint64));
    sqlite3_result_int64(context, *mask);
}

static void _bitmaskStep(sqlite3_context* context, int argc, sqlite3_value** argv) {
    sqlite_uint64* mask = sqlite3_aggregate_context(context, sizeof(sqlite_uint64));
    int bit, i;
    for (i = 0; i < argc; i++) {
        if (sqlite3_value_type(argv[i]) == SQLITE_NULL)
            continue;
        bit = sqlite3_value_int(argv[i]);
        if ((bit >= 0) && (bit <= 63))
            *mask |= (1ULL << bit);
    }
}

static void _bitmaskInverse(sqlite3_context* context, int argc, sqlite3_value** argv) {
    sqlite_uint64* mask = sqlite3_aggregate_context(context, sizeof(sqlite_uint64));
    int bit, i;
    for (i = 0; i < argc; i++) {
        if (sqlite3_value_type(argv[i]) == SQLITE_NULL)
            continue;
        bit = sqlite3_value_int(argv[i]);
        if ((bit >= 0) && (bit <= 63))
            *mask &= ~(1ULL << bit);
    }
}

/*
** If compiling as a builtin extension, don't export the initializer -- make it static
** Change name of initializer to sqlite3_init_<nameOfExtension>
*/

/* SQLite invokes this routine once when it loads the extension.
** Create new functions, collating sequences, and virtual table
** modules here.  This is usually the only exported symbol in
** the shared library.
*/

#ifdef _WIN32
#ifndef SQLITE_CORE
__declspec(dllexport)
#endif
#endif
    int sqlite3_mathsec_init(sqlite3* db, char** pzErrMsg, const sqlite3_api_routines* pApi) {
    int nErr = 0;

    SQLITE_EXTENSION_INIT2(pApi);

    int flags = SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS;
    nErr += sqlite3_create_function(db, "m_e", 0, flags, &m_e, _dfc, 0, 0);
    nErr += sqlite3_create_function(db, "m_log2e", 0, flags, &m_log2e, _dfc, 0, 0);
    nErr += sqlite3_create_function(db, "m_log10e", 0, flags, &m_log10e, _dfc, 0, 0);
    nErr += sqlite3_create_function(db, "m_ln2", 0, flags, &m_ln2, _dfc, 0, 0);
    nErr += sqlite3_create_function(db, "m_ln10", 0, flags, &m_ln10, _dfc, 0, 0);
    nErr += sqlite3_create_function(db, "m_pi", 0, flags, &m_pi, _dfc, 0, 0);
    nErr += sqlite3_create_function(db, "m_pi_2", 0, flags, &m_pi_2, _dfc, 0, 0);
    nErr += sqlite3_create_function(db, "m_pi_4", 0, flags, &m_pi_4, _dfc, 0, 0);
    nErr += sqlite3_create_function(db, "m_1_pi", 0, flags, &m_1_pi, _dfc, 0, 0);
    nErr += sqlite3_create_function(db, "m_2_pi", 0, flags, &m_2_pi, _dfc, 0, 0);
    nErr += sqlite3_create_function(db, "m_2_sqrtpi", 0, flags, &m_2_sqrtpi, _dfc, 0, 0);
    nErr += sqlite3_create_function(db, "m_sqrt2", 0, flags, &m_sqrt2, _dfc, 0, 0);
    nErr += sqlite3_create_function(db, "m_sqrt1_2", 0, flags, &m_sqrt1_2, _dfc, 0, 0);
    nErr += sqlite3_create_function(db, "m_deg2rad", 0, flags, &m_deg2rad, _dfc, 0, 0);
    nErr += sqlite3_create_function(db, "m_rad2deg", 0, flags, &m_rad2deg, _dfc, 0, 0);

    nErr += sqlite3_create_function(db, "fabs", 1, flags, fabs, _dfd, 0, 0);
    nErr += sqlite3_create_function(db, "ldexp", 2, flags, ldexp, _dfdi, 0, 0);
    nErr += sqlite3_create_function(db, "mantissa", 1, flags, 0, _mantissa, 0, 0);
    nErr += sqlite3_create_function(db, "exponent", 1, flags, 0, _exponent, 0, 0);

    nErr += sqlite3_create_function(db, "trunc", 1, flags, 0, _intpart, 0, 0);
    nErr += sqlite3_create_function(db, "frac", 1, flags, 0, _fracpart, 0, 0);

    nErr += sqlite3_create_function(db, "fromhex", 1, flags, 0, _FromHex, 0, 0);

    nErr += sqlite3_create_function(db, "isset", -1, flags, 0, _isset, 0, 0);
    nErr += sqlite3_create_function(db, "isclr", -1, flags, 0, _isclr, 0, 0);
    nErr += sqlite3_create_function(db, "setbits", -1, flags, 0, _setbits, 0, 0);
    nErr += sqlite3_create_function(db, "clrbits", -1, flags, 0, _clrbits, 0, 0);
    nErr += sqlite3_create_function(db, "bitmask", -1, flags, 0, _bitmask, 0, 0);
    nErr += sqlite3_create_function(db, "ismaskset", -1, flags, 0, _ismaskset, 0, 0);
    nErr += sqlite3_create_function(db, "ismaskclr", -1, flags, 0, _ismaskclr, 0, 0);
    nErr += sqlite3_create_function(db, "ErrorIfNull", 1, flags, 0, _TestNull, 0, 0);
    nErr += sqlite3_create_window_function(db, "aggbitmask", -1, flags, 0, _bitmaskStep,
                                           _bitmaskFinal, _bitmaskFinal, _bitmaskInverse, 0);

#if !defined(SQLITE_ENABLE_MATH_FUNCTIONS)
    sqlite3_create_function(db, "ceil", 1, flags, ceil, _dfd, 0, 0);
    sqlite3_create_function(db, "floor", 1, flags, floor, _dfd, 0, 0);
    sqlite3_create_function(db, "ln", 1, flags, log, _dfd, 0, 0);
    sqlite3_create_function(db, "log", 1, flags, log10, _dfd, 0, 0);
    sqlite3_create_function(db, "exp", 1, flags, exp, _dfd, 0, 0);
    sqlite3_create_function(db, "pow", 2, flags, pow, _dfdd, 0, 0);
    sqlite3_create_function(db, "fmod", 2, flags, fmod, _dfdd, 0, 0);
    sqlite3_create_function(db, "acos", 1, flags, acos, _dfd, 0, 0);
    sqlite3_create_function(db, "asin", 1, flags, asin, _dfd, 0, 0);
    sqlite3_create_function(db, "atan", 1, flags, atan, _dfd, 0, 0);
    sqlite3_create_function(db, "atan2", 2, flags, atan2, _dfdd, 0, 0);
    sqlite3_create_function(db, "cos", 1, flags, cos, _dfd, 0, 0);
    sqlite3_create_function(db, "sin", 1, flags, sin, _dfd, 0, 0);
    sqlite3_create_function(db, "tan", 1, flags, tan, _dfd, 0, 0);
    sqlite3_create_function(db, "cosh", 1, flags, cosh, _dfd, 0, 0);
    sqlite3_create_function(db, "sinh", 1, flags, sinh, _dfd, 0, 0);
    sqlite3_create_function(db, "tanh", 1, flags, tanh, _dfd, 0, 0);
    sqlite3_create_function(db, "sqrt", 1, flags, sqrt, _dfd, 0, 0);
    sqlite3_create_function(db, "radians", 1, flags, 0, _radians, 0, 0);
    sqlite3_create_function(db, "degrees", 1, flags, 0, _degrees, 0, 0);
    sqlite3_create_function(db, "sign", 1, flags, 0, _sign, 0, 0);
#endif

    {
        register LONGDOUBLE_TYPE x;
        x = asinl(1.0L);
        m_pi_2 = x;
        m_pi_4 = x / 2.0L;
        m_pi = x * 2.0L;
        m_1_pi = 0.5L / x;
        m_2_pi = 1.0L / x;
        m_2_sqrtpi = 2.0L / sqrtl(x * 2.0L);
        m_deg2rad = x / 90.0L;
        m_rad2deg = 90.0L / x;
    }
    m_e = expl(1.0L);
    m_log10e = log10l(expl(1.0L));
    m_sqrt2 = sqrtl(2.0L);
    m_ln2 = logl(2.0L);
    m_ln10 = logl(10.0L);
    m_log2e = 1.0L / logl(2.0);
    m_sqrt2 = sqrtl(2.0L);
    m_sqrt1_2 = sqrtl(0.5L);

    return nErr ? SQLITE_ERROR : SQLITE_OK;
}
