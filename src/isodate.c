// ISO weekday, week and year by Harald Hanche-Olsen
// https://sqlite.org/forum/forumpost/cb8923cb4db72f2e?t=h
// Unix timestamp by Richard Hipp
// https://sqlite.org/src/artifact/d0f09f7924a27e0d?ln=968-984

// Additional date and time functions:
// - Extract date parts according to ISO 8601: week day, week of a year, year
// - Convert ISO 8601 date to unix timestamp

#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

#pragma region Extracted from date.c

typedef unsigned short int u16;
typedef uint8_t u8;
typedef sqlite_int64 i64;

#define sqlite3Toupper(x) toupper((unsigned char)(x))
#define sqlite3Isspace(x) isspace((unsigned char)(x))
#define sqlite3Isalnum(x) isalnum((unsigned char)(x))
#define sqlite3Isalpha(x) isalpha((unsigned char)(x))
#define sqlite3Isdigit(x) isdigit((unsigned char)(x))
#define sqlite3Isxdigit(x) isxdigit((unsigned char)(x))
#define sqlite3Tolower(x) tolower((unsigned char)(x))
#define sqlite3Isquote(x) ((x) == '"' || (x) == '\'' || (x) == '[' || (x) == '`')

/*
** A structure for holding a single date and time.
*/
typedef struct DateTime DateTime;
struct DateTime {
    sqlite3_int64 iJD; /* The julian day number times 86400000 */
    int Y, M, D;       /* Year, month, and day */
    int h, m;          /* Hour and minutes */
    int tz;            /* Timezone offset in minutes */
    double s;          /* Seconds */
    char validJD;      /* True (1) if iJD is valid */
    char rawS;         /* Raw numeric value stored in s */
    char validYMD;     /* True (1) if Y,M,D are valid */
    char validHMS;     /* True (1) if h,m,s are valid */
    char validTZ;      /* True (1) if tz is valid */
    char tzSet;        /* Timezone was set explicitly */
    char isError;      /* An overflow has occurred */
};

/*
** Convert zDate into one or more integers according to the conversion
** specifier zFormat.
**
** zFormat[] contains 4 characters for each integer converted, except for
** the last integer which is specified by three characters.  The meaning
** of a four-character format specifiers ABCD is:
**
**    A:   number of digits to convert.  Always "2" or "4".
**    B:   minimum value.  Always "0" or "1".
**    C:   maximum value, decoded as:
**           a:  12
**           b:  14
**           c:  24
**           d:  31
**           e:  59
**           f:  9999
**    D:   the separator character, or \000 to indicate this is the
**         last number to convert.
**
** Example:  To translate an ISO-8601 date YYYY-MM-DD, the format would
** be "40f-21a-20c".  The "40f-" indicates the 4-digit year followed by "-".
** The "21a-" indicates the 2-digit month followed by "-".  The "20c" indicates
** the 2-digit day which is the last integer in the set.
**
** The function returns the number of successful conversions.
*/
static int getDigits(const char* zDate, const char* zFormat, ...) {
    /* The aMx[] array translates the 3rd character of each format
    ** spec into a max size:    a   b   c   d   e     f */
    static const u16 aMx[] = {12, 14, 24, 31, 59, 9999};
    va_list ap;
    int cnt = 0;
    char nextC;
    va_start(ap, zFormat);
    do {
        char N = zFormat[0] - '0';
        char min = zFormat[1] - '0';
        int val = 0;
        u16 max;

        assert(zFormat[2] >= 'a' && zFormat[2] <= 'f');
        max = aMx[zFormat[2] - 'a'];
        nextC = zFormat[3];
        val = 0;
        while (N--) {
            if (!sqlite3Isdigit(*zDate)) {
                goto end_getDigits;
            }
            val = val * 10 + *zDate - '0';
            zDate++;
        }
        if (val < (int)min || val > (int)max || (nextC != 0 && nextC != *zDate)) {
            goto end_getDigits;
        }
        *va_arg(ap, int*) = val;
        zDate++;
        cnt++;
        zFormat += 4;
    } while (nextC);
end_getDigits:
    va_end(ap);
    return cnt;
}

/*
** Parse a timezone extension on the end of a date-time.
** The extension is of the form:
**
**        (+/-)HH:MM
**
** Or the "zulu" notation:
**
**        Z
**
** If the parse is successful, write the number of minutes
** of change in p->tz and return 0.  If a parser error occurs,
** return non-zero.
**
** A missing specifier is not considered an error.
*/
static int parseTimezone(const char* zDate, DateTime* p) {
    int sgn = 0;
    int nHr, nMn;
    int c;
    while (sqlite3Isspace(*zDate)) {
        zDate++;
    }
    p->tz = 0;
    c = *zDate;
    if (c == '-') {
        sgn = -1;
    } else if (c == '+') {
        sgn = +1;
    } else if (c == 'Z' || c == 'z') {
        zDate++;
        goto zulu_time;
    } else {
        return c != 0;
    }
    zDate++;
    if (getDigits(zDate, "20b:20e", &nHr, &nMn) != 2) {
        return 1;
    }
    zDate += 5;
    p->tz = sgn * (nMn + nHr * 60);
zulu_time:
    while (sqlite3Isspace(*zDate)) {
        zDate++;
    }
    p->tzSet = 1;
    return *zDate != 0;
}

/*
** Parse times of the form HH:MM or HH:MM:SS or HH:MM:SS.FFFF.
** The HH, MM, and SS must each be exactly 2 digits.  The
** fractional seconds FFFF can be one or more digits.
**
** Return 1 if there is a parsing error and 0 on success.
*/
static int parseHhMmSs(const char* zDate, DateTime* p) {
    int h, m, s;
    double ms = 0.0;
    if (getDigits(zDate, "20c:20e", &h, &m) != 2) {
        return 1;
    }
    zDate += 5;
    if (*zDate == ':') {
        zDate++;
        if (getDigits(zDate, "20e", &s) != 1) {
            return 1;
        }
        zDate += 2;
        if (*zDate == '.' && sqlite3Isdigit(zDate[1])) {
            double rScale = 1.0;
            zDate++;
            while (sqlite3Isdigit(*zDate)) {
                ms = ms * 10.0 + *zDate - '0';
                rScale *= 10.0;
                zDate++;
            }
            ms /= rScale;
        }
    } else {
        s = 0;
    }
    p->validJD = 0;
    p->rawS = 0;
    p->validHMS = 1;
    p->h = h;
    p->m = m;
    p->s = s + ms;
    if (parseTimezone(zDate, p))
        return 1;
    p->validTZ = (p->tz != 0) ? 1 : 0;
    return 0;
}

/*
** Put the DateTime object into its error state.
*/
static void datetimeError(DateTime* p) {
    memset(p, 0, sizeof(*p));
    p->isError = 1;
}

/*
** Convert from YYYY-MM-DD HH:MM:SS to julian day.  We always assume
** that the YYYY-MM-DD is according to the Gregorian calendar.
**
** Reference:  Meeus page 61
*/
static void computeJD(DateTime* p) {
    int Y, M, D, A, B, X1, X2;

    if (p->validJD)
        return;
    if (p->validYMD) {
        Y = p->Y;
        M = p->M;
        D = p->D;
    } else {
        Y = 2000; /* If no YMD specified, assume 2000-Jan-01 */
        M = 1;
        D = 1;
    }
    if (Y < -4713 || Y > 9999 || p->rawS) {
        datetimeError(p);
        return;
    }
    if (M <= 2) {
        Y--;
        M += 12;
    }
    A = Y / 100;
    B = 2 - A + (A / 4);
    X1 = 36525 * (Y + 4716) / 100;
    X2 = 306001 * (M + 1) / 10000;
    p->iJD = (sqlite3_int64)((X1 + X2 + D + B - 1524.5) * 86400000);
    p->validJD = 1;
    if (p->validHMS) {
        p->iJD += p->h * 3600000 + p->m * 60000 + (sqlite3_int64)(p->s * 1000);
        if (p->validTZ) {
            p->iJD -= p->tz * 60000;
            p->validYMD = 0;
            p->validHMS = 0;
            p->validTZ = 0;
        }
    }
}

/*
** Parse dates of the form
**
**     YYYY-MM-DD HH:MM:SS.FFF
**     YYYY-MM-DD HH:MM:SS
**     YYYY-MM-DD HH:MM
**     YYYY-MM-DD
**
** Write the result into the DateTime structure and return 0
** on success and 1 if the input string is not a well-formed
** date.
*/
static int parseYyyyMmDd(const char* zDate, DateTime* p) {
    int Y, M, D, neg;

    if (zDate[0] == '-') {
        zDate++;
        neg = 1;
    } else {
        neg = 0;
    }
    if (getDigits(zDate, "40f-21a-21d", &Y, &M, &D) != 3) {
        return 1;
    }
    zDate += 10;
    while (sqlite3Isspace(*zDate) || 'T' == *(u8*)zDate) {
        zDate++;
    }
    if (parseHhMmSs(zDate, p) == 0) {
        /* We got the time */
    } else if (*zDate == 0) {
        p->validHMS = 0;
    } else {
        return 1;
    }
    p->validJD = 0;
    p->validYMD = 1;
    p->Y = neg ? -Y : Y;
    p->M = M;
    p->D = D;
    if (p->validTZ) {
        computeJD(p);
    }
    return 0;
}

/*
** Input "r" is a numeric quantity which might be a julian day number,
** or the number of seconds since 1970.  If the value if r is within
** range of a julian day number, install it as such and set validJD.
** If the value is a valid unix timestamp, put it in p->s and set p->rawS.
*/
static void setRawDateNumber(DateTime* p, double r) {
    p->s = r;
    p->rawS = 1;
    if (r >= 0.0 && r < 5373484.5) {
        p->iJD = (sqlite3_int64)(r * 86400000.0 + 0.5);
        p->validJD = 1;
    }
}

/*
** Attempt to parse the given string into a julian day number.  Return
** the number of errors.
**
** The following are acceptable forms for the input string:
**
**      YYYY-MM-DD HH:MM:SS.FFF  +/-HH:MM
**      DDDD.DD
**      now
**
** In the first form, the +/-HH:MM is always optional.  The fractional
** seconds extension (the ".FFF") is optional.  The seconds portion
** (":SS.FFF") is option.  The year and date can be omitted as long
** as there is a time string.  The time string can be omitted as long
** as there is a year and date.
*/
static int parseDateOrTime(sqlite3_context* context, const char* zDate, DateTime* p) {
    double r;
    if (parseYyyyMmDd(zDate, p) == 0) {
        return 0;
    } else if (parseHhMmSs(zDate, p) == 0) {
        return 0;
    }
    return 1;
}

/* The julian day number for 9999-12-31 23:59:59.999 is 5373484.4999999.
** Multiplying this by 86400000 gives 464269060799999 as the maximum value
** for DateTime.iJD.
**
** But some older compilers (ex: gcc 4.2.1 on older Macs) cannot deal with
** such a large integer literal, so we have to encode it.
*/
#define INT_464269060799999 ((((i64)0x1a640) << 32) | 0x1072fdff)

/*
** Return TRUE if the given julian day number is within range.
**
** The input is the JulianDay times 86400000.
*/
static int validJulianDay(sqlite3_int64 iJD) {
    return iJD >= 0 && iJD <= INT_464269060799999;
}

/*
** Compute the Year, Month, and Day from the julian day number.
*/
static void computeYMD(DateTime* p) {
    int Z, A, B, C, D, E, X1;
    if (p->validYMD)
        return;
    if (!p->validJD) {
        p->Y = 2000;
        p->M = 1;
        p->D = 1;
    } else if (!validJulianDay(p->iJD)) {
        datetimeError(p);
        return;
    } else {
        Z = (int)((p->iJD + 43200000) / 86400000);
        A = (int)((Z - 1867216.25) / 36524.25);
        A = Z + 1 + A - (A / 4);
        B = A + 1524;
        C = (int)((B - 122.1) / 365.25);
        D = (36525 * (C & 32767)) / 100;
        E = (int)((B - D) / 30.6001);
        X1 = (int)(30.6001 * E);
        p->D = B - D - X1;
        p->M = E < 14 ? E - 1 : E - 13;
        p->Y = p->M > 2 ? C - 4716 : C - 4715;
    }
    p->validYMD = 1;
}

/*
** Compute the Hour, Minute, and Seconds from the julian day number.
*/
static void computeHMS(DateTime* p) {
    int s;
    if (p->validHMS)
        return;
    computeJD(p);
    s = (int)((p->iJD + 43200000) % 86400000);
    p->s = s / 1000.0;
    s = (int)p->s;
    p->s -= s;
    p->h = s / 3600;
    s -= p->h * 3600;
    p->m = s / 60;
    p->s += s - p->m * 60;
    p->rawS = 0;
    p->validHMS = 1;
}

/*
** Compute both YMD and HMS
*/
static void computeYMD_HMS(DateTime* p) {
    computeYMD(p);
    computeHMS(p);
}

/*
** Process time function arguments.  argv[0] is a date-time stamp.
** argv[1] and following are modifiers.  Parse them all and write
** the resulting time into the DateTime structure p.  Return 0
** on success and 1 if there are any errors.
**
** If there are zero parameters (if even argv[0] is undefined)
** then assume a default value of "now" for argv[0].
*/

static int isDate(sqlite3_context* context, int argc, sqlite3_value** argv, DateTime* p) {
    int i, n;
    const unsigned char* z;
    int eType;
    memset(p, 0, sizeof(*p));
    if (argc == 0) {
        return 1;
    }
    if ((eType = sqlite3_value_type(argv[0])) == SQLITE_FLOAT || eType == SQLITE_INTEGER) {
        setRawDateNumber(p, sqlite3_value_double(argv[0]));
    } else {
        z = sqlite3_value_text(argv[0]);
        if (!z || parseDateOrTime(context, (char*)z, p)) {
            return 1;
        }
    }
    for (i = 1; i < argc; i++) {
        z = sqlite3_value_text(argv[i]);
        n = sqlite3_value_bytes(argv[i]);
        if (z == 0)
            return 1;
    }
    computeJD(p);
    if (p->isError || !validJulianDay(p->iJD))
        return 1;
    return 0;
}

#pragma endregion

// iso_weekday(date)
// Returns ISO 8601 weekday as a decimal number where 1 is Monday.
static void sqlite3_iso_weekday(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    DateTime x;

    if (isDate(context, argc, argv, &x))
        return;

    computeJD(&x);
    computeYMD_HMS(&x);
    int weekday = (((x.iJD + 43200000) / 86400000) % 7) + 1;

    sqlite3_result_int(context, weekday);
}

// iso_week(date)
// Returns ISO 8601 week as a decimal number with Monday as the first day of the week.
// Week 1 is the week containing Jan 4.
static void sqlite3_iso_week(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    DateTime x;

    if (isDate(context, argc, argv, &x))
        return;

    computeJD(&x);
    x.validYMD = 0;
    /* Adjust date to Thursday this week:
       The number in parentheses is 0 for Monday, 3 for Thursday */
    x.iJD += (3 - (((x.iJD + 43200000) / 86400000) % 7)) * 86400000;
    computeYMD(&x);

    /* borrowed code from case 'j' */
    sqlite3_int64 tJD = x.iJD;
    int nDay;
    x.validJD = 0;
    x.M = 1;
    x.D = 1;
    computeJD(&x);
    nDay = (int)((tJD - x.iJD + 43200000) / 86400000);
    int week = nDay / 7 + 1;
    sqlite3_result_int(context, week);
}

// iso_year(date)
// ISO 8601 year with century representing the year that contains the greater part of the ISO week.
static void sqlite3_iso_year(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    DateTime x;

    if (isDate(context, argc, argv, &x))
        return;

    computeJD(&x);
    x.validYMD = 0;
    // Adjust date to Thursday this week:
    // The number in parentheses is 0 for Monday, 3 for Thursday
    x.iJD += (3 - (((x.iJD + 43200000) / 86400000) % 7)) * 86400000;
    computeYMD(&x);

    int year = x.Y;
    sqlite3_result_int(context, year);
}

// unixepoch(date)
// unix timestamp - the number of seconds since 1970-01-01 00:00:00 UTC.
// Always returns an integer, even if the input time-value has millisecond precision.
static void sqlite3_unixepoch(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    DateTime x;

    if (isDate(context, argc, argv, &x))
        return;

    computeJD(&x);
    int time = x.iJD / 1000 - 21086676 * (i64)10000;
    sqlite3_result_int(context, time);
}

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_isodate_init(sqlite3* db, char** pzErrMsg, const sqlite3_api_routines* pApi) {
    SQLITE_EXTENSION_INIT2(pApi);
    static const int flags = SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC;
    sqlite3_create_function(db, "iso_weekday", 1, flags, 0, sqlite3_iso_weekday, 0, 0);
    sqlite3_create_function(db, "iso_week", 1, flags, 0, sqlite3_iso_week, 0, 0);
    sqlite3_create_function(db, "iso_year", 1, flags, 0, sqlite3_iso_year, 0, 0);
    sqlite3_create_function(db, "unixepoch", 1, flags, 0, sqlite3_unixepoch, 0, 0);
    return SQLITE_OK;
}