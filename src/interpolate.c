// Created by Steinar Midtskogen, Public Domain
// http://voksenlia.net/sqlite3/interpolate.c

#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* The "interpolate" virtual table for SQLite3

   Written by Steinar Midtskogen <steinar@latinitas.org>

   Revision 1: 2012-03-06.  Not yet extensively tested or verified.
   Revision 2: 2012-04-13.  Bug fixes.  Optimisations.  Filter added.
                            Support for multiple averages.
   Revision 3: 2012-10-11.  __min/__max bugfix.
   Revision 4: 2012-10-21.  Support timestamps in descending order.
                            Disable max() optimisation assuming too
                            much.  Added first() and last() aggregate
                            funtions.  Fixed missing ORDER BY.
   Revision 5: 2012-11-03.  Fixed constraint bug.
   Revision 6: 2012-11-25.  Added rnd() function.

   This is public domain as SQLite itself.  Copy, change and use as
   you wish at your own risk, but if you find bugs, the author would
   appreciate to hear about it or even better get a fix.

   The module creates a read-only virtual table based on a natural
   left join between several tables with a common timestamp column and
   a union of all timestamps using the timestamp.  That is, several
   tables will be combined into one, like natural full outer join if
   SQLite had supported it.  The table has the following extra
   features:

   * NULL values are filled in with interpolated values where possible.

   * Non-existing rows can be looked up (timestamps not in any table),
     and column values will be interpolated if possible.

   * Support for calculating the moving average, and max and min
     values withing the moving window.

   * Support for interpolation and average of angles/bearings.

   * Basic filtering capabilities.

   * Only INTEGER and REAL columns are supported.  Everything else
     becomes NULL.

   This module was developed to provide a convenient way to access and
   process sensor data (like weather data), but it might be useful for
   other purposes as well.

   To compile: gcc -std=c99 -O3 -g -fPIC -shared interpolate.c -o libsqliteinterpolate.so -lm
   To load into SQLite: SELECT load_extension("libsqliteinterpolate.so");

   Examples:

   Use this module to turn a table like:

   timestamp|value
    100      |20
    150      |
    300      |40

   into:

    timestamp|value
    100      |20
    150      |25
    300      |40

   "SELECT value FROM tab WHERE timestamp = 250" will give the value
   35.

   While all this is possible using regular SQL, it would be somewhat
   complex or very complex SQL expressions.  Besides, calculating a
   moving average with regular SQL would require a lot of summing of
   the same values, while this module will try to do this efficiently.

   Usage: CREATE VIRTUAL TABLE v USING
          interpolate(tab1, [tab2, ..., tabn][, key[, col1, ..., coln]][, period1 ..., periodn][,
   filter %][, subsampling /]);

   Requirements:

   * If a key is not given, all tables must have an INTEGER primary
     key with the same name.
   * If a key is given, it must be a column having the same name in
     all tables.  By explicitly naming the key, it is possible to use
     tables without an INTEGER primary key, but this will be slower.
   * col1, ..., coln are the columns to select for the virtual table.
     It's not necessary to include these.  If not, "*" will be assumed,
     but this can make queries run slower and much more memory hungry.
   * The input tables can't depend on a virtual table unless SQLite
     3.7.12 or later is used.  This is an SQLite design limitation.
   * The key is the axis used for interpolation, usually a timestamp.
   * The tables must not share other coulmn names.
   * The column names in the input tables must not cause aliasing issues
     (see below).  If this is a problem, use a view to work around it.
   * The order of the arguments are not important, except when listing
     column names.  Then the column to be used as the key must be the
     first one listed.  Also, if several of the listed columns are
     present in all the input tables, the first one will be used as
     a key.

   Note that it will be much more efficient to create a single virtual
   table from several tables than to create a virtual table from every
   table and then join the virtual tables.

   The optional "period" arguments state both the size for the moving
   average window as well as the minimum search range for
   interpolation.  Note that this is the minimum search range, and
   it's not strictly enforced, so the search range may be longer in
   reality.  If the "period" argument is not given, it will default to
   86400, corresponding to 24 hours for a timestamp key in second
   resolution.  The period and filter arguments can appear anywhere in
   the argument list.  A postfix can be used which will act as a
   multiplier.  The possible postfixes are "s", "m", "h", "d", "w",
   "M" and "y" which represent the multiplies 1, 60, 3600, 86400,
   604800, 2592000 and 31536000 respectively.  That is, corresponding
   to a second, a minute, an hour, a day, a week, a month (30 days)
   and a year (365 days) assuming that the number represents a second.
   So, "3600", "1 h", "3600 s" and "60 m" all give the same period.
   Note that there must be a space between the number and the postfix.
   State a period only once, and not both "1 d" and "24 h".

   The optional argument "filter", if non-zero, is used to create
   additional columns ending in __avg_filtered, which will give the
   moving average, just as __avg (see below), except that a filter
   removing the highest and lowest values will be applied before the
   averaging.  The "filter" argument is a percentage of how many
   samples that should be filtered away.  A percentage of 0 means no
   filtering, and the columns wont be created.  If the argument is not
   supplied, 0% will be assumed.  Note that the percentage must end in
   a "%", otherwise the number can be interpreted as a period value.
   The "filter" argument might be useful on noisy data.

   An integer followed by "/" can be used to apply subsampling on the
   input tables.  It's implemented as a "GROUP BY" statement involving
   the timestamp divided by the given number.  This can be useful to
   speed up operations on tables with many rows at the loss of some
   precision.

   One use for this module could be to combine data from columns in
   different tables.  For instance, to calculate the dew point we need
   to know the temperature and relative humidity, but if the
   temperature and relative humidity are stored in different tables
   and logged at different intervals, interpolation will be needed to
   find the dew point at a certain time.  Another example: Assume two
   tables:

   CREATE TABLE t1 ( temp_out REAL, time INTEGER, PRIMARY KEY (time);
   CREATE TABLE t2 ( temp_in REAL, time INTEGER, PRIMARY KEY (time);

   and a virtual table created by this module:

   CREATE TABLE v USING interpolate(t1, t2);

   We can now look up an arbitrary time in v even if that time doesn't
   exist in either two source tables:

   SELECT temp_in - temp_out FROM v WHERE time = 1234567890;

   The module will try to interpolate if a row with time = 1234567890
   doesn't exist in one or both tables.  Interpolation fails if no
   values to interpolate from are found within the period.  In that
   case, the result will be NULL.


   The module defines a few new functions, and it will also create
   some extra columns.

   New functions:

   * avg(value, timestamp): Like the regular avg(), but it will
     perform a weighted average.  It will sum the average of every
     adjacent samples multiplied by the difference of their
     timestamps.  The timestamp would usually be the primary key.
     This is very useful to calculate mean values when there are
     irregular gaps in the data.

   * avg_angle(column[, timestamp[, weight]]): As above, but the
     column is treated as an angle in degrees, and there is also an
     optional third argument which is an additional weight.  This
     ensures that the average of 350 and 10 degrees becomes 0, not
     180.  For instance: avg_angle(wind_dir, unix_time, wind_speed)
     would compute the average wind direction which is not only
     duration weighted, but also wind speed weighted.  The result will
     be between 0 and 360.

   * first(value, timestamp): Returns the first non-NULL value.  Useful
     when selecting multiple columns like
       SELECT first(col:1, timestamp), ... first(col_n, timestamp)
     when some columns have NULL values.

   * last(value, timestamp): Like above, but the last non-NULL value
     is returned.

   * hexagesimal(value[, decimals]): Convert a decimal number into its
     hexagesimal equivalence down to second (1/3600) fractions.  The
     optional second argument controls how many decimal points should
     be used for the seconds part.

   * rnd(value[, decimals]): As the built in round(), but using a
     rounding giving more intuitive results when the number can't be
     represented exactly in IEEE floating point.

   * havg(value1[, value2[, ..., valuen]]): "Horizontal average".
     Scalar function which will return the average of all non-NULL
     arguments.

   * havg_angle(value1[, value2[, ..., valuen]]): As above, but the
     arguments are assumed to be angles in degrees.

   New columns have the same name as the original colums with a postfix:

   * __avg: This column contains the centered moving average.

   * __avg_filtered: This column contains the centered moving average
     after a filter has been applied (see above).  The column will only
     exist if the filter percentage is not 0.

   * __angle: Use this column to make the interpolation work on angles
     in degrees.

   * __angle_avg: As __avg, but the value is assumed to be an angle in
     degrees.

   * __min: This column contains the lowest value in the moving
     average window.

   * __max: This column contains the highest value in the moving
     average window.

   Note that this can cause name aliasing.  If one of the source
   tables have a column "t" it cannot also have a column "t__avg".
   All new colums are HIDDEN, so they wont show up using "*" and must
   be accessed (and computed) explicitly.

   Important: The postfixes above assume that the default 24 hour
   average was used.  If the period was stated explicitly, the postfix
   will end in _6h if the argument was "6 h" or _81600 if the argument
   was "81600", etc.  So a virtual table declared "USING
   interpolate(tab, 1 w)" will give columns with postfix "__avg_1w",
   "__min_1w", etc.

   Examples (all assuming a 24 hour moving average window, a column
   "temp" a timestamp "ts" as the primary key):

   * To get the moving average:

     SELECT temp__avg FROM v WHERE ts = strftime("%s", "2012-01-31 18:00:00");

     which is equivalent to:

     SELECT avg(temp, ts) FROM v
     WHERE ts >= strftime("%s", "2012-01-31 06:00:00") AND ts <= strftime("%s", "2012-02-01
   06:00:00");

   * To get the highest and lowest value in the moving average window:

     SELECT temp__max, temp__min FROM v WHERE ts = strftime("%s", "2012-01-31 18:00:00");

   * To get the average daily low and high:

     SELECT avg(temp__min, ts), avg(temp__max, ts) FROM v WHERE ...;

   * To get the lowest high and highest low of any day:

     SELECT min(temp__max), max(temp__min) FROM v WHERE ...;

   * To get the biggest difference between daily low and high:

     SELECT max(temp__max - temp__min) FROM v WHERE ...;


   Note on angles:

   There are several ways to do interpolation and average of angles.
   It can be argued that there is no one single correct way to do
   this.  To find the average this module will convert all angles into
   their sinus and cosinus components, so it becomes a point on the
   unit circle, then add everything the regular way, except that the
   time weighting is adjusted so that the weight equals the path along
   the arc spanned by two angles, rather than along the shortest
   paths.  This ensures that the average of time weighted angles
   remains the same when interpolated angles are inserted in between
   (inaccuracy errors might accumulate slightly differently, though).
   Still, there might be unexpected results.  For instance, SELECT
   havg_angle(350, 10, 60) returns 19.3243480943937, not 20 as one
   perhaps would think.  But, again, any "correct" average of circular
   values depends on what your assumptations are.


   Limitations and final notes:

   Don't mix X and X__angle in the same SQL statement.  This
   will usually make no sense (either a value is an angle or it's
   not), so it shouldn't be a problem.  So a statement like SELECT
   avg(dir, ts), avg_angle(dir__angle, ts) FROM ... will give
   undefined results.  The reason is that the two average calculations
   might share data state information.
*/

static double myNAN = 0.0 / 0.0;

typedef enum {
    COL_BASE = 0,
    COL_AVG,
    COL_MIN,
    COL_MAX,
    COL_ANGLE,
    COL_ANGLE_AVG,
    COL_AVG_FILTERED
} coltype;

/* More intuitive rounding */
static void rnd_func(sqlite3_context* context, int argc, sqlite3_value** argv) {
    double x = argc == 1 ? 1 : exp(sqlite3_value_double(argv[1]) * log(10));
    sqlite3_result_double(context, round(sqlite3_value_double(argv[0]) * x) / x);
}

/* Horizontal average: Add non-NULL columns */
static void havg_func(sqlite3_context* context, int argc, sqlite3_value** argv) {
    double sum = 0;
    int count = 0;
    while (--argc >= 0)
        if (sqlite3_value_type(argv[argc]) != SQLITE_NULL) {
            sum += sqlite3_value_double(argv[argc]);
            count++;
        }

    count ? sqlite3_result_double(context, sum / count) : sqlite3_result_null(context);
}

static void havg_angle_func(sqlite3_context* context, int argc, sqlite3_value** argv) {
    double sumy = 0, sumx = 0;
    int count = 0;
    while (--argc >= 0)
        if (sqlite3_value_type(argv[argc]) != SQLITE_NULL) {
            double val = sqlite3_value_double(argv[argc]) * M_PI / 180;
            sumx += cos(val);
            sumy += sin(val);
            count++;
        }

    count ? sqlite3_result_double(context, fmod(atan2(sumy, sumx) * 180 / M_PI + 360, 360))
          : sqlite3_result_null(context);
}

static void hexagesimal_func(sqlite3_context* context, int argc, sqlite3_value** argv) {
    int decimals = argc == 1 ? 0 : sqlite3_value_int(argv[1]);
    int sign = sqlite3_value_double(argv[0]) < 0 ? -1 : 1;
    double angle = fabs(sqlite3_value_double(argv[0]));
    double deg = floor(angle);
    double min = floor((angle - deg) * 60);
    double sec = ((angle - deg) * 60 - min) * 60 - 0.5 / pow(10, decimals);
    if (min < 0)
        min = 0;
    if (min >= 60)
        min = 60 - DBL_MIN;
    if (sec < 0)
        sec = 0;
    if (sec >= 60)
        sec = 60 - DBL_MIN;

    char* s = sqlite3_mprintf("%s%%d%c%c %%02d\' %%0%d.%df\"", sign == -1 ? "-" : "", 0xc2, 0xb0,
                              decimals + 2 + !!decimals, decimals);
    char* buf = sqlite3_mprintf(s, (int)deg, (int)min, sec);
    sqlite3_free(s);
    sqlite3_result_text(context, buf, -1, SQLITE_TRANSIENT);
    sqlite3_free(buf);
}

typedef struct {
    double sumx;
    double sumy;
    double lastvalue;
    double lasttimestamp;
    int init;
} avg_angle_priv;

/* Calculate optionally time weighted and optionally speed weighted average of direction */
static void avg_angle_step(sqlite3_context* context, int argc, sqlite3_value** argv) {
    avg_angle_priv* p = sqlite3_aggregate_context(context, sizeof(avg_angle_priv));
    double val1 = sqlite3_value_double(argv[0]) * M_PI / 180;

    if ((argc == 1 || p->init) && sqlite3_value_type(argv[0]) != SQLITE_NULL &&
        (argc == 1 || sqlite3_value_type(argv[1]) != SQLITE_NULL)) {
        if (argc == 1) {
            p->sumx += cos(val1);
            p->sumy += sin(val1);
        } else {
            double val2 = sqlite3_value_double(argv[1]) - p->lasttimestamp;
            val2 *= (argc == 3 ? sqlite3_value_double(argv[2]) : 1);
            double x = cos(val1) + cos(p->lastvalue);
            double y = sin(val1) + sin(p->lastvalue);
            double len = sqrt(x * x + y * y);
            val2 /= len;
            if (val1 != p->lastvalue) {
                /* The weight is the distance along the arc */
                double a = val1;
                double b = p->lastvalue;
                if (a - b > M_PI)
                    b += 2 * M_PI;
                if (b - a > M_PI)
                    a += 2 * M_PI;
                double xx = cos(a) - cos(b);
                double yy = sin(a) - sin(b);
                val2 *= sqrt(xx * xx + yy * yy) / fabs(a - b);
            }

            p->sumx += x * val2;
            p->sumy += y * val2;
        }
    }
    if (!p->init)
        p->init = 1;

    p->lastvalue = val1;
    if (argc > 1)
        p->lasttimestamp = sqlite3_value_double(argv[1]);
}

static void avg_angle_finalize(sqlite3_context* context) {
    avg_angle_priv* p = sqlite3_aggregate_context(context, 0);

    p && p->init && p->init
        ? sqlite3_result_double(context, fmod(atan2(p->sumy, p->sumx) * 180 / M_PI + 360, 360))
        : sqlite3_result_null(context);
}

typedef struct {
    double sum;
    double sumx;
    double sumy;
    double total;
    double lastvalue;
    double lasttimestamp;
    int init;
} avg_priv;

/* Calculate time weighted average */
static void avg_step(sqlite3_context* context, int argc, sqlite3_value** argv) {
    avg_priv* p = sqlite3_aggregate_context(context, sizeof(avg_priv));

    if (!p->init)
        p->init = 1;
    else if (sqlite3_value_type(argv[0]) != SQLITE_NULL &&
             sqlite3_value_type(argv[1]) != SQLITE_NULL) {
        p->sum += (sqlite3_value_double(argv[0]) + p->lastvalue) / 2 *
                  (sqlite3_value_double(argv[1]) - p->lasttimestamp);
        p->total += sqlite3_value_double(argv[1]) - p->lasttimestamp;
    }

    p->lastvalue = sqlite3_value_double(argv[0]);
    p->lasttimestamp = sqlite3_value_double(argv[1]);
}

static void avg_finalize(sqlite3_context* context) {
    avg_priv* p = sqlite3_aggregate_context(context, 0);
    p && p->init && p->total > 0 ? sqlite3_result_double(context, p->sum / p->total)
                                 : sqlite3_result_null(context);
}

typedef struct {
    double val;
    double timestamp;
    int init;
} firstlast_priv;

/* Find the first non-NULL value */
static void first_step(sqlite3_context* context, int argc, sqlite3_value** argv) {
    firstlast_priv* p = sqlite3_aggregate_context(context, sizeof(firstlast_priv));

    if (sqlite3_value_type(argv[0]) != SQLITE_NULL &&
        (!p->init || sqlite3_value_double(argv[1]) < p->timestamp)) {
        p->val = sqlite3_value_double(argv[0]);
        p->timestamp = sqlite3_value_double(argv[1]);
        p->init = 1;
    }
}

/* Find the last non-NULL value */
static void last_step(sqlite3_context* context, int argc, sqlite3_value** argv) {
    firstlast_priv* p = sqlite3_aggregate_context(context, sizeof(firstlast_priv));

    if (sqlite3_value_type(argv[0]) != SQLITE_NULL &&
        (!p->init || sqlite3_value_double(argv[1]) > p->timestamp)) {
        p->val = sqlite3_value_double(argv[0]);
        p->timestamp = sqlite3_value_double(argv[1]);
        p->init = 1;
    }
}

static void firstlast_finalize(sqlite3_context* context) {
    firstlast_priv* p = sqlite3_aggregate_context(context, 0);
    p && p->init ? sqlite3_result_double(context, p->val) : sqlite3_result_null(context);
}

typedef struct {
    sqlite3_vtab vtab;  // Must come first
    sqlite3* db;        // Database

    const char* table;    // Name of virtual table
    char** tables;        // List of table names to join
    int* tablename_lens;  // Their lengths
    int tablenum;         // Number of tables

    sqlite_int64* table_start;  // Their lowest timestamp
    sqlite_int64* table_end;    // Their highest timestamp

    char* timestamp;    // Name of the timestamp column
    int timestamp_len;  // Its length
    int timestamp_idx;  // Its column index in joined table

    int cols;         // Number of columns in joined table
    int* coltypes;    // The type of each column
    char** colnames;  // Names of the columns
    char* selection;  // Columns to select

    int context;           // Maximum context range to allow calculations near ends
    int* contexts;         // List of context ranges
    char** context_names;  // List of context names
    int contexts_num;      // Number of context ranges

    int filter;  // Percentage to filter away.

    int subsample;  // How much to subsample
} interpolate_vtab;

typedef struct {
    sqlite3_vtab_cursor cur;  // Must come first
    sqlite3_stmt* stmt;       // SQL statement that the cursor works on
    int eof;                  // End flag
    double timestamp;         // Current timestamp
    int timestamp_order;      // Query timestamp order (1: ascending, -1: descending)

    double** context_values;  // Circular buffer holding rows before and after current row
    int context_index;        // Current position in buffer
    int context_end;          // Wrap position in buffer
    double* timestamp_min;    // Lowest timestamp for non-null value in source column
    struct {
        double v, t;
        double min, max;
        int sorted;
        int j[3];
    } * last;  // Used for moving averages

    int stmt_done;  // Flag telling whether there is more rows left in SQL query

    struct {
        int op;
        double rval;
    } * constraints;  // List of constraints on the timestamp

    int constraints_num;  // Number of contraints on the timestamp

    int temptable;  // Random number used to create a unique name for temporary table

    int cols;         // Number of cols in query, unknown until completion of first row
    int row_context;  // Context rows to allow calculations near ends

    double min, max;  // For filtering.
} interpolate_cursor;

/* Find the index of the primary key */
static int find_primary_key(sqlite3* db, const char* table, int* pk) {
    int j = 0;
    sqlite3_stmt* stmt;
    char* buf = sqlite3_mprintf("PRAGMA table_info(%s)", table);
    *pk = -1;
    int rc = sqlite3_prepare_v2(db, buf, -1, &stmt, NULL);
    sqlite3_free(buf);
    if (rc != SQLITE_OK)
        return rc;
    do {
        rc = sqlite3_step(stmt);
        if (rc == SQLITE_DONE)
            break;
        if (rc != SQLITE_ROW)
            return rc;

        if (sqlite3_column_int64(stmt, 5) &&
            !sqlite3_strnicmp((const char*)sqlite3_column_text(stmt, 2), "INTEGER", 7)) {
            *pk = j;
            break;
        }
        j++;
    } while (1);

    sqlite3_finalize(stmt);

    return *pk != -1 ? SQLITE_OK : SQLITE_MISUSE;
}

static int concat_strings(char** bufs, int j, char** dest) {
    int i, l = 0;
    for (i = 0; i < j; i++)
        l += strlen(bufs[i]);
    *dest = sqlite3_malloc(l + 1);
    **dest = 0;
    if (!*dest)
        return SQLITE_NOMEM;
    for (i = l = 0; i < j; i++) {
        strcpy(*dest + l, bufs[i]);
        l += strlen(bufs[i]);
        sqlite3_free(bufs[i]);
    }
    sqlite3_free(bufs);
    return SQLITE_OK;
}

/* Parse arguments and create virtual table */
static int interpolate_connect(sqlite3* db,
                               void* pAux,
                               int argc,
                               const char* const* argv,
                               sqlite3_vtab** ppVtab,
                               char** pzErr) {
    interpolate_vtab* v;
    sqlite3_stmt *stmt, *stmt2;
    int i, j, k;
    int cols;
    int* coltypes;
    int rc;
    int numbers[argc];
    int contexts[argc];
    const char* names[argc];

    /* Argument check */
    if (argc < 4)
        return SQLITE_MISUSE;

    *pzErr = 0;
    memset(numbers, 0, sizeof(numbers));
    memset(names, 0, sizeof(names));

    v = sqlite3_malloc(sizeof(*v));
    *ppVtab = (sqlite3_vtab*)v;
    if (!v)
        return SQLITE_NOMEM;
    memset(v, 0, sizeof(*v));

    v->context = 0;
    v->filter = 0;
    v->subsample = 0;
    v->table = argv[2];

    /* Filter or context arguments supplied? */
    for (i = 3, k = 0; i < argc; i++) {
        const char* p = argv[i];
        for (j = 1; *p; p++)
            j &= (*p <= '9' && *p >= '0') || *p <= ' ' || *p == '.' || *p == '+' || *p == '-' ||
                 ((*p == '%' || *p == 's' || *p == 'm' || *p == 'h' || *p == 'd' || *p == '/' ||
                   *p == 'w' || *p == 'M' || *p == 'y') &&
                  !p[1]);
        if (j) {
            if (p[-1] == '%') {
                v->filter = atol(argv[i]);
                if (v->filter < 0)
                    v->filter = 0;
                else if (v->filter > 100)
                    v->filter = 100;
            } else if (p[-1] == '/') {
                v->subsample = atol(argv[i]);
                if (v->subsample < 0)
                    v->subsample = 0;
            } else {
                int factor;
                switch (p[-1]) {
                    case 's':
                        factor = 1;
                        break;
                    case 'm':
                        factor = 60;
                        break;
                    case 'h':
                        factor = 3600;
                        break;
                    case 'd':
                        factor = 86400;
                        break;
                    case 'w':
                        factor = 604800;
                        break;
                    case 'M':
                        factor = 2592000;
                        break;
                    case 'y':
                        factor = 31536000;
                        break;
                    default:
                        factor = 1;
                }
                int c = atol(argv[i]) * factor;
                if (c < 1)
                    c = 1;
                if (c > v->context)
                    v->context = c;

                for (j = 0; j < k && contexts[j] != c; j++)
                    ;

                if (j == k) {
                    names[k] = argv[i];
                    contexts[k++] = c;
                }
            }
            numbers[i] = 1;
        }
    }

    int dfl = 0;
    if (!k) {
        contexts[0] = v->context = 60 * 60 * 24;
        k++;
        dfl = 1;
    }

    v->contexts_num = k;
    v->contexts = sqlite3_malloc(sizeof(int) * k);
    v->context_names = sqlite3_malloc(sizeof(char*) * k);
    if (!v->contexts || !v->context_names)
        return SQLITE_NOMEM;

    if (!dfl) {
        for (i = 0; i < k; i++) {
            int l = strlen(names[i]);
            int c = 1;
            v->contexts[i] = contexts[i];
            v->context_names[i] = sqlite3_malloc(l + 2);
            if (!v->context_names[i])
                return SQLITE_NOMEM;
            v->context_names[i][0] = '_';
            for (j = 0; j < l; j++)
                if (names[i][j] != ' ')
                    v->context_names[i][c++] = names[i][j];
            v->context_names[i][c] = 0;
        }
    } else {
        v->context_names[0] = sqlite3_malloc(1);
        if (!v->context_names[0])
            return SQLITE_NOMEM;
        v->context_names[0][0] = 0;
        v->contexts[0] = contexts[0];
    }

    /* Check whether arguments are tables or column names */
    int found[argc];
    int last;
    for (last = argc - 1; numbers[last] && last >= 0; last--)
        ;
    int al[argc];
    for (i = 0; i < argc; i++) {
        al[i] = strlen(argv[i]);
        found[i] = 0;
    }

    for (i = 3, k = 0; i < argc && i <= last; i++) {
        if (numbers[i])
            continue;
        char* buf = sqlite3_mprintf("SELECT * FROM %s", argv[i]);
        rc = sqlite3_prepare_v2(db, buf, -1, &stmt, NULL);
        sqlite3_free(buf);
        if (rc != SQLITE_OK) {
            sqlite3_finalize(stmt);
            continue;
        }
        k++;  // Identified as a table
        sqlite3_step(stmt);
        cols = sqlite3_column_count(stmt);
        if (!cols)
            return SQLITE_MISUSE;
        int p;
        for (p = 3; p < argc; p++) {
            for (j = 0; j < cols; j++) {
                const char* n = sqlite3_column_name(stmt, j);
                int nl = strlen(n);
                if (numbers[p] || p == i)
                    found[p] = -1;
                else if (!sqlite3_strnicmp(argv[p], n, al[p] > nl ? al[p] : nl)) {
                    found[p]++;
                    break;
                }
            }
        }
        sqlite3_finalize(stmt);
    }

    /* Arguments that are not table, nor column, nor number? */
    for (i = 3; i < argc; i++)
        if (!found[i])
            return SQLITE_MISUSE;

    int first = 0;
    /* Find column present in all tables */
    for (i = 0; i < argc; i++) {
        if (!first && found[i])
            first = i;
        if (found[i] == k) {
            last = i;
            break;
        }
    }
    if (first) {
        v->timestamp = sqlite3_malloc(al[last] + 1);
        if (!v->timestamp)
            return SQLITE_NOMEM;
        v->timestamp_len = al[last];
        strcpy(v->timestamp, argv[last]);
    } else
        k++;

    v->tablenum = k;
    if (!first || i == argc) {
        sqlite3_free(v->timestamp);
        v->timestamp = 0;
        /* Look up primary key */
        for (j = 3, i = 0; i < v->tablenum; i++, j++) {
            int pk;
            while (numbers[j] && j < argc)
                j++;
            rc = find_primary_key(db, argv[j], &pk);
            if (rc != SQLITE_OK)
                continue;

            char* buf = sqlite3_mprintf("SELECT * FROM %s", argv[j]);
            rc = sqlite3_prepare_v2(db, buf, -1, &stmt, NULL);
            sqlite3_free(buf);
            if (rc != SQLITE_OK)
                return rc;

            sqlite3_step(stmt);
            cols = sqlite3_column_count(stmt);
            if (!cols)
                return SQLITE_MISUSE;

            const char* pkname = sqlite3_column_name(stmt, pk);
            if (!v->timestamp) {
                v->timestamp_len = strlen(pkname);
                v->timestamp = sqlite3_malloc(v->timestamp_len + 1);
                if (!v->timestamp)
                    return SQLITE_NOMEM;
                strcpy(v->timestamp, pkname);
            } else {
                int len = strlen(v->timestamp);
                if (sqlite3_strnicmp(pkname, v->timestamp,
                                     v->timestamp_len > len ? v->timestamp_len : len))
                    return SQLITE_MISUSE;
            }
            sqlite3_finalize(stmt);
        }
    }
    if (!v->timestamp)
        return SQLITE_MISUSE;

    v->tables = sqlite3_malloc(v->tablenum * sizeof(char*));
    v->tablename_lens = sqlite3_malloc(v->tablenum * sizeof(int));
    v->table_start = sqlite3_malloc(v->tablenum * sizeof(double));
    v->table_end = sqlite3_malloc(v->tablenum * sizeof(double));
    if (!v->table_start || !v->table_end || !v->tables || !v->tablename_lens)
        return SQLITE_NOMEM;

    v->db = db;

    for (j = 3, i = 0; i < v->tablenum; i++, j++) {
        while (numbers[j] && j < argc)
            j++;
        v->tables[i] = sqlite3_malloc(strlen(argv[j]) + 1);
        if (!v->tables[i])
            return SQLITE_NOMEM;
        strcpy(v->tables[i], argv[j]);
        v->tablename_lens[i] = strlen(v->tables[i]);
    }

    for (i = j = 0; i < argc; i++)
        j += found[i] > 0;

    if (j < 2)
        v->selection = sqlite3_mprintf("*");
    else {
        for (i = j = 0; i < argc; i++)
            if (found[i] > 0)
                j += strlen(argv[i]) + 2;
        v->selection = sqlite3_malloc(j + 1);
        if (!v->selection)
            return SQLITE_NOMEM;
        for (i = j = 0; i < argc; i++)
            if (found[i] > 0) {
                strcpy(v->selection + j, argv[i]);
                j += al[i];
                strcpy(v->selection + j, ", ");
                j += 2;
            }
        v->selection[j - 2] = 0;
    }

    /* Copy description from source table.  We don't need a union of the
       timestamps since we're only interested in the column information */
    char** bufs = sqlite3_malloc((2 + v->tablenum) * sizeof(char*));
    if (!bufs)
        return SQLITE_NOMEM;
    j = 0;
    bufs[j++] = sqlite3_mprintf("SELECT %s FROM (SELECT %s FROM %s LIMIT 1) ", v->selection,
                                v->timestamp, v->tables[0]);
    for (i = 0; i < v->tablenum; i++)
        bufs[j++] = sqlite3_mprintf("NATURAL JOIN %s ", v->tables[i]);
    bufs[j++] = sqlite3_mprintf("\n");

    char* select;
    rc = concat_strings(bufs, j, &select);
    if (rc != SQLITE_OK)
        return rc;

    // printf("%s: %s", v->table, select);

    rc = sqlite3_prepare_v2(db, select, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
        return rc;
    sqlite3_free(select);
    cols = sqlite3_column_count(stmt);
    coltypes = sqlite3_malloc(sizeof(int) * cols);
    if (!coltypes) {
        sqlite3_finalize(stmt);
        return SQLITE_NOMEM;
    }

    /* Find smallest and largest timestamps in each table */
    for (i = 0; i < v->tablenum; i++) {
        /* For some reason it's much faster to select separately than ask for both in one query */
        char* buf = sqlite3_mprintf(
            "SELECT (SELECT %s FROM %s ORDER BY %s LIMIT 1), (SELECT %s FROM %s ORDER BY %s DESC "
            "LIMIT 1)",
            v->timestamp, v->tables[i], v->timestamp, v->timestamp, v->tables[i], v->timestamp);
        // printf("%s: %s\n", v->table, buf);
        rc = sqlite3_prepare_v2(db, buf, -1, &stmt2, NULL);
        sqlite3_free(buf);
        if (rc != SQLITE_OK) {
            sqlite3_finalize(stmt2);
            return rc;
        }
        rc = sqlite3_step(stmt2);
        if (rc != SQLITE_ROW) {
            sqlite3_finalize(stmt2);
            return rc;
        }
        v->table_start[i] = sqlite3_column_double(stmt2, 0);
        v->table_end[i] = sqlite3_column_double(stmt2, 1);

        sqlite3_finalize(stmt2);
    }

    int extras = v->filter ? 6 : 5;

    /* Fill out column names */
    v->colnames = sqlite3_malloc(cols * (1 + extras * v->contexts_num) * sizeof(char*));
    for (i = 0; i < cols; i++) {
        int l = strlen(sqlite3_column_name(stmt, i));
        const char* n = sqlite3_column_name(stmt, i);

        v->colnames[i] = sqlite3_malloc(l + 1);
        if (!v->colnames[i])
            return SQLITE_NOMEM;
        strcpy(v->colnames[i], n);

        for (j = 0; j < v->contexts_num; j++) {
            l += strlen(v->context_names[j]);
            v->colnames[i + cols * 1 + j * cols * extras] = sqlite3_malloc(l + 6);
            v->colnames[i + cols * 2 + j * cols * extras] = sqlite3_malloc(l + 6);
            v->colnames[i + cols * 3 + j * cols * extras] = sqlite3_malloc(l + 6);
            v->colnames[i + cols * 4 + j * cols * extras] = sqlite3_malloc(l + 8);
            v->colnames[i + cols * 5 + j * cols * extras] = sqlite3_malloc(l + 12);
            if (v->filter)
                v->colnames[i + cols * 6 + j * cols * extras] = sqlite3_malloc(l + 15);

            if (!v->colnames[i + cols * 1 + j * cols * extras] ||
                !v->colnames[i + cols * 2 + j * cols * extras] ||
                !v->colnames[i + cols * 3 + j * cols * extras] ||
                !v->colnames[i + cols * 4 + j * cols * extras] ||
                !v->colnames[i + cols * 5 + j * cols * extras] ||
                (v->filter && !v->colnames[i + cols * 6 + j * cols * extras]))
                return SQLITE_NOMEM;

            sprintf(v->colnames[i + cols * 1 + j * cols * extras], "%s__avg%s", n,
                    v->context_names[j]);
            sprintf(v->colnames[i + cols * 2 + j * cols * extras], "%s__min%s", n,
                    v->context_names[j]);
            sprintf(v->colnames[i + cols * 3 + j * cols * extras], "%s__max%s", n,
                    v->context_names[j]);
            sprintf(v->colnames[i + cols * 4 + j * cols * extras], "%s__angle%s", n,
                    v->context_names[j]);
            sprintf(v->colnames[i + cols * 5 + j * cols * extras], "%s__angle_avg%s", n,
                    v->context_names[j]);

            if (v->filter)
                sprintf(v->colnames[i + cols * 6 + j * cols * extras], "%s__avg_filtered%s", n,
                        v->context_names[j]);
        }
    }

    /* Create the vtab declaration */
    bufs = sqlite3_malloc((2 + cols * (1 + extras * v->contexts_num)) * sizeof(char*));
    if (!bufs)
        return SQLITE_NOMEM;
    j = 0;

    bufs[j++] = sqlite3_mprintf("CREATE TABLE %s_vtab(", argv[2]);
    for (i = 0; i < cols * (1 + extras * v->contexts_num); i++) {
        const char* type = sqlite3_column_decltype(stmt, i % cols);
        if (i < cols)
            coltypes[i] = type && strcmp(type, "REAL") ? SQLITE_INTEGER : SQLITE_FLOAT;
        bufs[j++] =
            sqlite3_mprintf("%s %s%s%s", v->colnames[i],
                            i < cols || (i / cols - 1) % extras == 1 || (i / cols - 1) % extras == 2
                                ? (type ? type : "")
                                : "REAL",
                            i / cols == 0 ? "" : " HIDDEN",
                            i + 1 == cols * (1 + extras * v->contexts_num) ? "" : ", ");
    }
    bufs[j++] = sqlite3_mprintf(")");

    char* stmtbuf;
    rc = concat_strings(bufs, j, &stmtbuf);
    if (rc != SQLITE_OK)
        return rc;

    /* Which one is the timestamp? */
    for (i = 0; i < cols; i++)
        if (!strcmp(v->timestamp, sqlite3_column_name(stmt, i)))
            break;
    v->timestamp_idx = i;

    v->cols = cols;
    v->coltypes = coltypes;

    sqlite3_finalize(stmt);

    // printf("%s: %s\n", v->table, stmtbuf);

    /* Declare vtab */
    rc = sqlite3_declare_vtab(db, stmtbuf);
    sqlite3_free(stmtbuf);
    if (rc != SQLITE_OK)
        return rc;

    (*ppVtab)->zErrMsg = 0;

    return SQLITE_OK;
}

static int interpolate_disconnect(sqlite3_vtab* vtab) {
    int i;
    interpolate_vtab* v = (interpolate_vtab*)vtab;
    for (i = 0; i < v->tablenum; i++)
        if (v->tables[i])
            sqlite3_free(v->tables[i]);
    if (v->tables)
        sqlite3_free(v->tables);
    if (v->selection)
        sqlite3_free(v->selection);
    if (v->contexts)
        sqlite3_free(v->contexts);
    if (v->tablename_lens)
        sqlite3_free(v->tablename_lens);
    if (v->table_start)
        sqlite3_free(v->table_start);
    if (v->table_end)
        sqlite3_free(v->table_end);
    if (v->timestamp)
        sqlite3_free(v->timestamp);
    if (v->coltypes)
        sqlite3_free(v->coltypes);
    for (i = 0; i < v->cols; i++) {
        if (v->colnames[i + v->cols * 0])
            sqlite3_free(v->colnames[i + v->cols * 0]);
        if (v->colnames[i + v->cols * 1])
            sqlite3_free(v->colnames[i + v->cols * 1]);
        if (v->colnames[i + v->cols * 2])
            sqlite3_free(v->colnames[i + v->cols * 2]);
        if (v->colnames[i + v->cols * 3])
            sqlite3_free(v->colnames[i + v->cols * 3]);
        if (v->colnames[i + v->cols * 4])
            sqlite3_free(v->colnames[i + v->cols * 4]);
        if (v->colnames[i + v->cols * 5])
            sqlite3_free(v->colnames[i + v->cols * 5]);
        if (v->filter)
            if (v->colnames[i + v->cols * 6])
                sqlite3_free(v->colnames[i + v->cols * 6]);
    }

    for (i = 0; i < v->contexts_num; i++)
        if (v->context_names[i])
            sqlite3_free(v->context_names[i]);

    if (v->colnames)
        sqlite3_free(v->colnames);
    if (v->context_names)
        sqlite3_free(v->context_names);

    sqlite3_free(vtab);
    return SQLITE_OK;
}

/* Open cursor */
static int interpolate_open(sqlite3_vtab* p, sqlite3_vtab_cursor** ppCursor) {
    interpolate_cursor* c = sqlite3_malloc(sizeof(*c));
    const interpolate_vtab* v = (interpolate_vtab*)c->cur.pVtab;
    if (!c)
        return SQLITE_NOMEM;
    memset(c, 0, sizeof(*c));
    *ppCursor = (sqlite3_vtab_cursor*)c;

    /* Remove temporary table if it exists */
    if (c->temptable) {
        /* It would have been better to do this in interpolate_close(),
           but the table seems always to be locked here. */
        char* buf = sqlite3_mprintf("DROP TABLE temp.t%d;", c->temptable);
        sqlite3_exec(v->db, buf, NULL, NULL, NULL);
        sqlite3_free(buf);
        c->temptable = 0;
    }

    return SQLITE_OK;
}

/* Close cursor */
static int interpolate_close(sqlite3_vtab_cursor* cur) {
    int i;
    interpolate_cursor* c = (interpolate_cursor*)cur;
    sqlite3_finalize(c->stmt);

    for (i = 0; i < c->row_context; i++)
        if (c->context_values[i])
            sqlite3_free(c->context_values[i]);

    if (c->last)
        sqlite3_free(c->last);
    if (c->context_values)
        sqlite3_free(c->context_values);
    if (c->timestamp_min)
        sqlite3_free(c->timestamp_min);

    return SQLITE_OK;
}

/* Make sure that we observe the constraints */
static int check_constraints(interpolate_cursor* c) {
    int i;
    int res = 1;
    if (isnan(c->timestamp))
        return 0;
    for (i = 0; i < c->constraints_num; i++)
        switch (c->constraints[i].op) {
            case SQLITE_INDEX_CONSTRAINT_EQ:
                res &= c->timestamp * c->timestamp_order == c->constraints[i].rval;
                break;
            case SQLITE_INDEX_CONSTRAINT_GT:
                res &= c->timestamp * c->timestamp_order > c->constraints[i].rval;
                break;
            case SQLITE_INDEX_CONSTRAINT_LE:
                res &= c->timestamp * c->timestamp_order <= c->constraints[i].rval;
                break;
            case SQLITE_INDEX_CONSTRAINT_LT:
                res &= c->timestamp * c->timestamp_order < c->constraints[i].rval;
                break;
            case SQLITE_INDEX_CONSTRAINT_GE:
                res &= c->timestamp * c->timestamp_order >= c->constraints[i].rval;
                break;
        }
    return res;
}

/* Double the size of the buffer, and remove unneccesary columns */
static int resize_buffer(interpolate_cursor* c) {
    int newsize = c->row_context * 2;
    int i, j, k;
    int context_index = 0, context_end;
    const interpolate_vtab* v = (interpolate_vtab*)c->cur.pVtab;

    double** context_values = sqlite3_malloc(sizeof(double*) * newsize);
    if (!context_values)
        return SQLITE_NOMEM;

    /* Allocate row context */
    for (i = 0; i < newsize; i++) {
        context_values[i] = sqlite3_malloc(sizeof(double) * c->cols);
        if (!context_values[i])
            return SQLITE_NOMEM;
    }

    /* Copy from old context */
    k = 0;
    j = (c->context_end + 1) % c->row_context;
    do {
        for (i = 0; i < c->cols; i++)
            context_values[k][i] = c->context_values[j][i];
        if (j == c->context_index)
            context_index = k;
        j = (j + 1) % c->row_context;
        k++;
    } while (j != c->context_end);

    for (i = 0; i < c->cols; i++)
        context_values[k][i] = c->context_values[j][i];
    context_end = c->row_context - 1;

    /* Free the old context */
    for (i = 0; i < c->row_context; i++)
        sqlite3_free(c->context_values[i]);
    sqlite3_free(c->context_values);

    /* Make the new context ready for use */
    c->row_context = newsize;
    c->context_values = context_values;
    c->context_index = context_index;
    c->context_end = context_end;

    /* Fill the new context */
    int rc;
    j = (c->context_end + 1) % c->row_context;
    do {
        int i;
        rc = sqlite3_step(c->stmt);
        if (rc == SQLITE_ROW) {
            c->context_end = (c->context_end + 1) % c->row_context;
            for (i = 0; i < v->cols; i++) {
                context_values[j][i] =
                    sqlite3_value_type(sqlite3_column_value(c->stmt, i)) != SQLITE_INTEGER &&
                            sqlite3_value_type(sqlite3_column_value(c->stmt, i)) != SQLITE_FLOAT
                        ? myNAN
                        : sqlite3_column_double(c->stmt, i);
                if (i == v->timestamp_idx)  // Internally timestamp must be in ascending order
                    c->context_values[j][i] *= c->timestamp_order;
                if (c->timestamp_min[i] == DBL_MAX && !isnan(c->context_values[j][i]))
                    c->timestamp_min[i] =
                        sqlite3_column_double(c->stmt, v->timestamp_idx) * c->timestamp_order;
            }
        } else if (rc == SQLITE_DONE)
            c->stmt_done = 1;
        else
            return rc;
    } while (++j != c->context_index + newsize / 2 && rc == SQLITE_ROW);

    for (i = j - 1; i < newsize; i++)
        for (j = 0; j < c->cols; j++)
            c->context_values[i][j] = myNAN;

    return SQLITE_OK;
}

/* Check whether the value is valid */
static inline int isvalid(interpolate_cursor* c, double value) {
    return !isnan(value) && value >= c->min && value <= c->max;
}

/* Return one new row */
static int interpolate_next(sqlite3_vtab_cursor* cur) {
    int rc = SQLITE_OK;
    interpolate_cursor* c = (interpolate_cursor*)cur;
    const interpolate_vtab* v = (interpolate_vtab*)c->cur.pVtab;

    /* Loop until we have one row matching the constraints */
    do {
        /* Have we reached the end? */
        if (c->context_index == c->context_end) {
            sqlite3_reset(c->stmt);
            c->eof = 1;
            if (c->constraints) {
                sqlite3_free(c->constraints);
                c->constraints = 0;
            }
            return SQLITE_OK;
        }

        c->context_index = (c->context_index + 1) % c->row_context;
        c->timestamp = c->context_values[c->context_index][v->timestamp_idx];

        /* Need to resize buffer? */
        while (
            !c->stmt_done &&
            c->row_context <=
                60 * 60 * 24 *
                    31 &&  // More than one month at second resolution might be a runaway queries
            (((!isnan(c->context_values[c->context_end % c->row_context][v->timestamp_idx]) &&
               c->context_values[c->context_end][v->timestamp_idx] - c->timestamp <= v->context) ||
              (!isnan(c->context_values[(c->context_end + 1) % c->row_context][v->timestamp_idx]) &&
               c->timestamp -
                       c->context_values[(c->context_end + 1) % c->row_context][v->timestamp_idx] <=
                   v->context)))) {
            int rc = resize_buffer(c);
            if (rc != SQLITE_OK)
                return rc;
        }

        /* Read from SQL query if there is more to get */
        if (!c->stmt_done) {
            int i;
            rc = sqlite3_step(c->stmt);
            if (rc == SQLITE_ROW) {
                c->context_end = (c->context_end + 1) % c->row_context;
                for (i = 0; i < v->cols; i++) {
                    c->context_values[c->context_end][i] =
                        sqlite3_value_type(sqlite3_column_value(c->stmt, i)) != SQLITE_INTEGER &&
                                sqlite3_value_type(sqlite3_column_value(c->stmt, i)) != SQLITE_FLOAT
                            ? myNAN
                            : sqlite3_column_double(c->stmt, i);
                    if (i == v->timestamp_idx)  // Internally timestamp must be in ascending order
                        c->context_values[c->context_end][i] *= c->timestamp_order;
                    if (c->timestamp_min[i] == DBL_MAX &&
                        !isnan(c->context_values[c->context_end][i]))
                        c->timestamp_min[i] =
                            sqlite3_column_double(c->stmt, v->timestamp_idx) * c->timestamp_order;
                }
            }
            if (rc == SQLITE_DONE)
                c->stmt_done = 1;
        }
    } while (!check_constraints(c));

    return rc == SQLITE_DONE || rc == SQLITE_ROW ? SQLITE_OK : rc;
}

/* Calculate mean for [timestamp-size/2, timestamp+size/2] */
static double calcmean(interpolate_cursor* c, double timestamp, double size, int k, int x) {
    double last_value = myNAN;
    double last_timestamp = myNAN;
    int last_j = -1;
    int j;
    const interpolate_vtab* v = (interpolate_vtab*)c->cur.pVtab;

    for (j = c->last[k].j[x] != -1
                 ? c->last[k].j[x]
                 : (x == 2 ? c->context_index : (c->context_end + 1) % c->row_context);
         j != c->context_end; j = (j + 1) % c->row_context) {
        /* Clip last value if filtering to track trends */
        if (c->context_values[j][v->timestamp_idx] - timestamp >= size / 2) {
            if (isnan(c->context_values[j][k % c->cols]))
                continue;
        } else if (!isvalid(c, c->context_values[j][k % c->cols]))
            continue;
        if (c->context_values[j][v->timestamp_idx] - timestamp >= size / 2)
            break;
        last_j = j;
        last_value = c->context_values[j][k % c->cols];
        last_timestamp = c->context_values[j][v->timestamp_idx];
    }
    if (c->context_values[j][v->timestamp_idx] - timestamp < size / 2)
        return myNAN;

    last_value = (last_value * (c->context_values[j][v->timestamp_idx] - timestamp - size / 2) +
                  c->context_values[j][k % c->cols] * (timestamp + size / 2 - last_timestamp)) /
                 (c->context_values[j][v->timestamp_idx] - last_timestamp);

    /* Retry with full scan if this didn't work */
    if (isnan(last_value) && c->last[k].j[x] != -1) {
        c->last[k].j[x] = -1;
        return calcmean(c, timestamp, size, k, x);
    }
    if (last_value > c->max)
        last_value = (c->min + c->max) / 2;
    if (last_value < c->min)
        last_value = (c->min + c->max) / 2;

    c->last[k].j[x] = last_j;
    last_timestamp = timestamp + size / 2;
    double valsum = 0;

    for (j = (j + c->row_context - 1) % c->row_context; j != c->context_end;
         j = (j + c->row_context - 1) % c->row_context) {
        /* Clip last value if filtering to track trends */
        if (timestamp - c->context_values[j][v->timestamp_idx] >= size / 2) {
            if (isnan(c->context_values[j][k % c->cols]))
                continue;
        } else if (!isvalid(c, c->context_values[j][k % c->cols]))
            continue;
        if (timestamp - c->context_values[j][v->timestamp_idx] >= size / 2)
            break;
        valsum += (c->context_values[j][k % c->cols] + last_value) / 2 *
                  (last_timestamp - c->context_values[j][v->timestamp_idx]);
        last_timestamp = c->context_values[j][v->timestamp_idx];
        last_value = c->context_values[j][k % c->cols];
    }

    if (last_value > c->max)
        last_value = (c->min + c->max) / 2;
    if (last_value < c->min)
        last_value = (c->min + c->max) / 2;

    if (timestamp - c->context_values[j][v->timestamp_idx] < size / 2)
        return myNAN;

    double last = (c->context_values[j][k % c->cols] * (last_timestamp - timestamp + size / 2) +
                   last_value * (timestamp - c->context_values[j][v->timestamp_idx] - size / 2)) /
                  (last_timestamp - c->context_values[j][v->timestamp_idx]);

    if (last > c->max)
        last = (c->min + c->max) / 2;
    if (last < c->min)
        last = (c->min + c->max) / 2;

    return valsum + (last + last_value) / 2 * (last_timestamp - timestamp + size / 2);
}

/* As above, but calculate mean of angles */
static double calcmean_angle(interpolate_cursor* c, double timestamp, double size, int k, int x) {
    double last_value = myNAN;
    double last_valuex, last_valuey;
    double last_timestamp = myNAN;
    double len;
    int last_j = -1;
    int j;
    const interpolate_vtab* v = (interpolate_vtab*)c->cur.pVtab;

    for (j = c->last[k].j[x] != -1
                 ? c->last[k].j[x]
                 : (x == 2 ? c->context_index : (c->context_end + 1) % c->row_context);
         j != c->context_end; j = (j + 1) % c->row_context) {
        if (isnan(c->context_values[j][k % c->cols]))
            continue;
        if (c->context_values[j][v->timestamp_idx] - timestamp >= size / 2)
            break;
        last_j = j;
        last_value = c->context_values[j][k % c->cols];
        last_timestamp = c->context_values[j][v->timestamp_idx];
    }
    if (c->context_values[j][v->timestamp_idx] - timestamp < size / 2)
        return myNAN;

    double this_value = c->context_values[j][k % c->cols];
    if (this_value - last_value > 180)
        last_value += 360;
    if (last_value - this_value > 180)
        this_value += 360;
    double val =
        fmod((last_value * (c->context_values[j][v->timestamp_idx] - timestamp - size / 2) +
              this_value * (timestamp + size / 2 - last_timestamp)) /
                     (c->context_values[j][v->timestamp_idx] - last_timestamp) +
                 360,
             360);
    last_valuex = cos(val * M_PI / 180);
    last_valuey = sin(val * M_PI / 180);

    /* Retry with full scan if this didn't work */
    if (isnan(last_value) && c->last[k].j[x] != -1) {
        c->last[k].j[x] = -1;
        return calcmean_angle(c, timestamp, size, k, x);
    }

    c->last[k].j[x] = last_j;
    last_timestamp = timestamp + size / 2;
    double valsumx = 0;
    double valsumy = 0;

    for (j = (j + c->row_context - 1) % c->row_context; j != c->context_end;
         j = (j + c->row_context - 1) % c->row_context) {
        if (isnan(c->context_values[j][k % c->cols]))
            continue;
        if (timestamp - c->context_values[j][v->timestamp_idx] >= size / 2)
            break;
        double xx = cos(c->context_values[j][k % c->cols] * M_PI / 180);
        double yy = sin(c->context_values[j][k % c->cols] * M_PI / 180);
        double xxx = xx + last_valuex;
        double yyy = yy + last_valuey;
        double len = sqrt(xxx * xxx + yyy * yyy);
        xxx /= len;
        yyy /= len;

        if (xx != last_valuex && yy != last_valuey) {
            /* The weight is the distance along the arc */
            double a = c->context_values[j][k % c->cols] * M_PI / 180;
            double b = atan2(last_valuey, last_valuex);
            if (a - b > M_PI)
                b += 2 * M_PI;
            if (b - a > M_PI)
                a += 2 * M_PI;
            double xx = cos(a) - cos(b);
            double yy = sin(a) - sin(b);
            double corr = sqrt(xx * xx + yy * yy) / fabs(a - b);
            xxx *= corr;
            yyy *= corr;
        }

        valsumx += xxx * (last_timestamp - c->context_values[j][v->timestamp_idx]);
        valsumy += yyy * (last_timestamp - c->context_values[j][v->timestamp_idx]);
        last_timestamp = c->context_values[j][v->timestamp_idx];
        last_valuex = xx;
        last_valuey = yy;
    }

    if (timestamp - c->context_values[j][v->timestamp_idx] < size / 2)
        return myNAN;

    last_value = fmod(atan2(last_valuey, last_valuex) * 180 / M_PI + 360, 360);
    this_value = c->context_values[j][k % c->cols];

    if (this_value - last_value > 180)
        last_value += 360;
    if (last_value - this_value > 180)
        this_value += 360;
    val = fmod((this_value * (last_timestamp - timestamp + size / 2) +
                last_value * (timestamp - c->context_values[j][v->timestamp_idx] - size / 2)) /
                       (last_timestamp - c->context_values[j][v->timestamp_idx]) +
                   360,
               360);

    double xx = cos(val * M_PI / 180) + last_valuex;
    double yy = sin(val * M_PI / 180) + last_valuey;
    len = sqrt(xx * xx + yy * yy);
    xx /= len;
    yy /= len;
    if (this_value != last_value) {
        /* The weight is the distance along the arc */
        double a = this_value * M_PI / 180;
        double b = last_value * M_PI / 180;
        if (a - b > M_PI)
            b += 2 * M_PI;
        if (b - a > M_PI)
            a += 2 * M_PI;
        double x = cos(a) - cos(b);
        double y = sin(a) - sin(b);
        double corr = sqrt(x * x + y * y) / fabs(a - b);
        xx *= corr;
        yy *= corr;
    }

    return fmod(atan2(valsumy + yy * (last_timestamp - timestamp + size / 2),
                      valsumx + xx * (last_timestamp - timestamp + size / 2)) *
                        180 / M_PI +
                    360,
                360);
}

const double precision = 1e+10;

static inline int eq(double a, double b) {
    return fabs(a - b) <= fabs(a / precision);
}

static inline int le(double a, double b) {
    return a <= b || fabs(a - b) <= fabs(a / precision);
}

static inline int ge(double a, double b) {
    return a >= b || fabs(a - b) <= fabs(a / precision);
}

/* Calculate min/max for [timestamp-size/2, timestamp+size/2] */
static double interpolate_minmax(interpolate_cursor* c,
                                 double timestamp,
                                 double size,
                                 int k,
                                 int x,
                                 int returnmin,
                                 int* sorted,
                                 double* res2) {
    double last_value = myNAN;
    double last_timestamp = myNAN;
    int last_j = -1;
    int j;
    int s = 1;
    const interpolate_vtab* v = (interpolate_vtab*)c->cur.pVtab;

    for (j = c->last[k].j[x] != -1
                 ? c->last[k].j[x]
                 : (x == 2 ? c->context_index : (c->context_end + 1) % c->row_context);
         j != c->context_end; j = (j + 1) % c->row_context) {
        if (!isvalid(c, c->context_values[j][k % c->cols]))
            continue;
        if (c->context_values[j][v->timestamp_idx] - timestamp >= size / 2)
            break;
        last_j = j;
        last_value = c->context_values[j][k % c->cols];
        last_timestamp = c->context_values[j][v->timestamp_idx];
    }
    if (c->context_values[j][v->timestamp_idx] - timestamp < size / 2) {
        *sorted = 0;
        return myNAN;
    }

    last_value = (last_value * (c->context_values[j][v->timestamp_idx] - timestamp - size / 2) +
                  c->context_values[j][k % c->cols] * (timestamp + size / 2 - last_timestamp)) /
                 (c->context_values[j][v->timestamp_idx] - last_timestamp);

    s &= j != c->context_end && !isnan(c->context_values[(j + 1) % c->row_context][k % c->cols]) &&
         (returnmin ? le(last_value, c->context_values[(j + 1) % c->row_context][k % c->cols])
                    : ge(last_value, c->context_values[(j + 1) % c->row_context][k % c->cols]));
    /* Retry with full scan if this didn't work */
    if (isnan(last_value) && c->last[k].j[x] != -1) {
        c->last[k].j[x] = -1;
        return interpolate_minmax(c, timestamp, size, k, x, returnmin, sorted, res2);
    }

    c->last[k].j[x] = last_j;
    last_timestamp = timestamp + size / 2;
    double min = last_value;
    double max = last_value;

    for (j = (j + c->row_context - 1) % c->row_context; j != c->context_end;
         j = (j + c->row_context - 1) % c->row_context) {
        if (!isvalid(c, c->context_values[j][k % c->cols]))
            continue;
        if (timestamp - c->context_values[j][v->timestamp_idx] >= size / 2)
            break;
        s &= returnmin ? ge(last_value, c->context_values[j][k % c->cols])
                       : le(last_value, c->context_values[j][k % c->cols]);
        if (min > c->context_values[j][k % c->cols])
            min = c->context_values[j][k % c->cols];
        if (max < c->context_values[j][k % c->cols])
            max = c->context_values[j][k % c->cols];
        last_timestamp = c->context_values[j][v->timestamp_idx];
        last_value = c->context_values[j][k % c->cols];
    }
    if (timestamp - c->context_values[j][v->timestamp_idx] < size / 2) {
        *sorted = 0;
        return myNAN;
    }

    double last = (c->context_values[j][k % c->cols] * (last_timestamp - timestamp + size / 2) +
                   last_value * (timestamp - c->context_values[j][v->timestamp_idx] - size / 2)) /
                  (last_timestamp - c->context_values[j][v->timestamp_idx]);

    s &=
        j != c->context_end && (j + c->row_context - 1) % c->row_context != c->context_end &&
        (returnmin ? ge(last_value,
                        c->context_values[(j + c->row_context - 1) % c->row_context][k % c->cols])
                   : le(last_value,
                        c->context_values[(j + c->row_context - 1) % c->row_context][k % c->cols]));
    if (min > last)
        min = last;
    if (max < last)
        max = last;

    *sorted = s;
    if (res2)
        *res2 = returnmin ? max : min;
    return returnmin ? min : max;
}

/* Sort an array */
static void quicksort(double* vals, int length) {
    int i, j;
    double v, t;

    if (length <= 1)
        return;

    i = 0;
    j = length;
    v = vals[0];
    while (1) {
        while (vals[++i] < v && i < length)
            ;
        while (vals[--j] > v)
            ;
        if (i >= j)
            break;
        t = vals[i];
        vals[i] = vals[j];
        vals[j] = t;
    }
    t = vals[i - 1];
    vals[i - 1] = vals[0];
    vals[0] = t;
    quicksort(vals, i - 1);
    quicksort(vals + i, length - i);
}

static coltype get_coltype(const interpolate_cursor* c, int col) {
    const interpolate_vtab* v = (interpolate_vtab*)c->cur.pVtab;

    if (col < c->cols)
        return COL_BASE;
    col -= c->cols;
    return (col / c->cols) % (v->filter ? 6 : 5) + 1;
}

static int get_context(const interpolate_cursor* c, int col) {
    const interpolate_vtab* v = (interpolate_vtab*)c->cur.pVtab;
    if (col < c->cols)
        return 0;
    col -= c->cols;
    return (col / c->cols) / (v->filter ? 6 : 5);
}

/* Calculate column values */
static int interpolate_column(sqlite3_vtab_cursor* cur, sqlite3_context* ctx, int i) {
    interpolate_cursor* c = (interpolate_cursor*)cur;
    const interpolate_vtab* v = (interpolate_vtab*)c->cur.pVtab;
    double last_value = 0.0 / 0.0;
    double last_timestamp = 0.0 / 0.0;
    int j;
    int ctype = get_coltype(c, i);
    int ccontext = get_context(c, i);

    // Internally timestamp must be in ascending order, so restore original value
    double sign = i != v->timestamp_idx ? 1 : c->timestamp_order;

    c->min = -DBL_MAX;
    c->max = DBL_MAX;
    if (ctype != COL_BASE && ctype != COL_ANGLE) {
        int q = i - c->cols;
        int k = i % c->cols;
        double val;

        /* To near ends to compute a value? */
        if (c->timestamp - v->contexts[ccontext] / 2 < c->timestamp_min[i % c->cols] ||
            c->timestamp + v->contexts[ccontext] / 2 >
                c->context_values[c->context_end][v->timestamp_idx]) {
            sqlite3_result_null(ctx);
            return SQLITE_OK;
        }

        if (ctype == COL_AVG || ctype == COL_ANGLE_AVG || ctype == COL_AVG_FILTERED) {
            /* Moving average */
            double (*mean)(interpolate_cursor*, double, double, int, int);
            mean = ctype == COL_ANGLE_AVG ? calcmean_angle : calcmean;

            if (ctype == COL_AVG_FILTERED) {
                /* Run filter to find min and max */
                double* f = sqlite3_malloc(c->row_context * sizeof(double));
                if (!f)
                    return SQLITE_NOMEM;
                int vals = 0;

                /* Copy all non-NULL values into a new array */
                for (j = c->context_index;
                     j != c->context_end && c->timestamp - c->context_values[j][v->timestamp_idx] <=
                                                v->contexts[ccontext] / 2;
                     j = (j + c->row_context - 1) % c->row_context)
                    if (!isnan(c->context_values[j][k]))
                        f[vals++] = c->context_values[j][k];
                for (j = c->context_index; j != (c->context_end + 1) % c->row_context &&
                                           c->context_values[j][v->timestamp_idx] - c->timestamp <=
                                               v->contexts[ccontext] / 2;
                     j = (j + 1) % c->row_context)
                    if (!isnan(c->context_values[j][k]))
                        f[vals++] = c->context_values[j][k];

                if (vals) {
                    quicksort(f, vals);
                    c->min = f[v->filter * vals / 200];
                    int max = vals - v->filter * vals / 200;
                    c->max = f[max >= vals ? vals - 1 : max];
                    if (c->max < c->min)
                        c->max = c->min;
                }
                sqlite3_free(f);
            }

            // Reuse last result if this column has been computed before
            if (ctype == COL_AVG && !isnan(c->last[q].v) && c->timestamp == c->last[q].t)
                val = c->last[q].v;
            /* Reuse calculations from last time if the overlap is big */
            else if (ctype == COL_AVG && !isnan(c->last[q].v) &&
                     c->timestamp - c->last[q].t < v->contexts[ccontext] / 4) {
                /* Not for __angle, accuracy losses would accumulate */
                /* Nor for __avg_filtered, since the filter needs to be rerun */
                val = c->last[q].v +
                      mean(c, (c->last[q].t + c->timestamp) / 2 + v->contexts[ccontext] / 2,
                           c->timestamp - c->last[q].t, q, 0) -
                      mean(c, (c->last[q].t + c->timestamp) / 2 - v->contexts[ccontext] / 2,
                           c->timestamp - c->last[q].t, q, 1);
            } else  // Full scan
                val = mean(c, c->timestamp, v->contexts[ccontext], q, 2);

            c->last[q].v = val;
            c->last[q].t = c->timestamp;
            if (isnan(val))
                sqlite3_result_null(ctx);
            else
                sqlite3_result_double(
                    ctx, sign *
                             round(precision * (val / (ctype == COL_AVG || ctype == COL_AVG_FILTERED
                                                           ? v->contexts[ccontext]
                                                           : 1))) /
                             precision);
            return SQLITE_OK;
        } else {
            /* Min or max */
            int returnmin = ctype == COL_MIN;
            double last = returnmin ? c->last[q].min : c->last[q].max;
            int sorted_prev, sorted_next;
            double res2;

            /* Does the area leaving the window have the min/max value? */
            /* If yes, is the window sorted? */
            double prev = interpolate_minmax(
                c, (c->last[q].t + c->timestamp) / 2 - v->contexts[ccontext] / 2,
                c->timestamp - c->last[q].t, q, 1, returnmin, &sorted_prev, &res2);
            double next =
                interpolate_minmax(c, (c->last[q].t + c->timestamp) / 2 + v->contexts[ccontext] / 2,
                                   c->timestamp - c->last[q].t, q, 0, returnmin, &sorted_next, 0);
            if (!isnan(last) && c->timestamp - c->last[q].t < v->contexts[ccontext] / 4 &&
                (!eq(last, prev) || (sorted_prev && sorted_next && c->last[q].sorted &&
                                     (returnmin ? le(prev, next) : ge(prev, next))))) {
                /* No, reuse last result */
                val = sorted_prev && sorted_next && c->last[q].sorted &&
                              (returnmin ? le(prev, next) : ge(prev, next))
                          ? res2
                          : (returnmin ? (next < last ? next : last) : (next > last ? next : last));
                sorted_next &= sorted_prev && c->last[q].sorted;
            } else  // Yes, full scan
                val = interpolate_minmax(c, c->timestamp, v->contexts[ccontext], q, 2, returnmin,
                                         &sorted_next, 0);
            c->last[q].sorted = sorted_next;
            c->last[q].t = c->timestamp;
            if (returnmin)
                c->last[q].min = val;
            else
                c->last[q].max = val;
            if (isnan(val))
                sqlite3_result_null(ctx);
            else {
                sqlite3_result_double(ctx, sign * round(precision * val) / precision);
            }
            return SQLITE_OK;
        }

        sqlite3_result_double(ctx, 0);
        return SQLITE_OK;
    }

    int dir = ctype != COL_BASE;
    if (dir)
        i %= c->cols;

    /* Search backwards for previous value */
    j = c->context_index;
    do {
        if (isvalid(c, c->context_values[j][i])) {
            last_value = c->context_values[j][i];
            last_timestamp = c->context_values[j][v->timestamp_idx];
            break;
        }
        j = (j + c->row_context - 1) % c->row_context;
    } while (c->timestamp > c->context_values[j][v->timestamp_idx]);

    /* If there is a value for this row, return that one */
    if (c->timestamp == last_timestamp) {
        if (v->coltypes[i] == SQLITE_INTEGER)
            sqlite3_result_int64(ctx, sign * last_value);
        else if (v->coltypes[i] == SQLITE_FLOAT)
            sqlite3_result_double(ctx, sign * round(precision * last_value) / precision);
        return SQLITE_OK;
    } else
        /* Return null if no previous value was found */
        if (isnan(last_value)) {
        sqlite3_result_null(ctx);
        return SQLITE_OK;
    } else
        /* Look forward for the next non-null value */
        for (j = (c->context_index + 1) % c->row_context;
             j != (c->context_end + 1) % c->row_context; j = (j + 1) % c->row_context) {
            /* If found, we can now interpolate */
            if (isvalid(c, c->context_values[j][i])) {
                double this_value = c->context_values[j][i];
                double val;
                if (dir) {
                    if (this_value - last_value > 180)
                        last_value += 360;
                    if (last_value - this_value > 180)
                        this_value += 360;
                    val = fmod(
                        (this_value * (c->timestamp - last_timestamp) +
                         last_value * (c->context_values[j][v->timestamp_idx] - c->timestamp)) /
                            (c->context_values[j][v->timestamp_idx] - last_timestamp),
                        360);
                } else
                    val = (this_value * (c->timestamp - last_timestamp) +
                           last_value * (c->context_values[j][v->timestamp_idx] - c->timestamp)) /
                          (c->context_values[j][v->timestamp_idx] - last_timestamp);
                if (v->coltypes[i] == SQLITE_INTEGER)
                    sqlite3_result_int64(ctx, sign * val);
                else if (v->coltypes[i] == SQLITE_FLOAT)
                    sqlite3_result_double(ctx, sign * round(precision * val) / precision);
                else
                    sqlite3_result_null(ctx);  // Only interpolate integers and floats
                return SQLITE_OK;
            }
        }

    /* Return null if no next non-null value was found */
    sqlite3_result_null(ctx);
    return SQLITE_OK;
}

/* In: list of columns and table.  Out: list of columns actually in table. */
int strip_selection(sqlite3* db, char* buf, const char* selection, const char* table) {
    int c = 1, i, last;
    int l = strlen(selection);
    for (i = 0; i < l; i++)
        c += selection[i] == ',';

    char** bufs = sqlite3_malloc(c * sizeof(char*));
    if (!bufs)
        return SQLITE_NOMEM;
    strcpy(buf, selection);
    for (last = i = 0; i < l + 1; i++) {
        if (buf[i] != ',' && i != l)
            continue;
        buf[i] = 0;
        sqlite3_stmt* stmt;
        char* stmtbuf = sqlite3_mprintf("SELECT %s from %s", buf + last, table);
        int rc = sqlite3_prepare_v2(db, stmtbuf, -1, &stmt, NULL);
        sqlite3_free(stmtbuf);
        if (rc != SQLITE_OK)
            memset(buf + last - 1, ' ', i - last + 1);
        sqlite3_finalize(stmt);
        buf[i] = selection[i];
        last = i + 1;
    }
    buf[l] = 0;
    return SQLITE_OK;
}

/* Prepare lookup in the virtual table */
static int interpolate_filter(sqlite3_vtab_cursor* cur,
                              int idxNum,
                              const char* idxStr,
                              int argc,
                              sqlite3_value** argv) {
    interpolate_cursor* c = (interpolate_cursor*)cur;
    const interpolate_vtab* v = (interpolate_vtab*)c->cur.pVtab;
    int rc, i, j = 0, l;

    /* Count equalities in the string passed to us from interpolation_bestindex */
    if (argc) {
        l = strlen(idxStr);
        for (j = 0, i = 1; i < l; i++)
            j += idxStr[i] == '=';
    }

    double eq_ts[j ? j : 1];
    int eq = 0;

    c->cols = v->cols;

    struct {
        int op;
        double rval;
    } constraints[argc + j ? argc + j : 1];

    c->constraints = argc ? sqlite3_malloc(argc * sizeof(*c->constraints)) : 0;
    c->constraints_num = argc;

    /* Add constraints to allow context for interpolation of first and last rows */
    /* Equality constraints need to be converted to <= and >= constraints */
    for (i = j = 0; i < argc; i++) {
        double val1, val2;
        while (*idxStr != 0 && *idxStr != '<' && *idxStr != '>' && *idxStr != '=')
            idxStr++;
        switch (*idxStr) {
            case '>':
                val1 = sqlite3_value_double(argv[i]);
                val2 = val1 - v->context;
                c->constraints[i].rval = val1;
                c->constraints[i].op =
                    idxStr[1] == '=' ? SQLITE_INDEX_CONSTRAINT_GE : SQLITE_INDEX_CONSTRAINT_GT;
                if (idxStr[1])
                    eq_ts[eq++] = c->constraints[j].rval;
                constraints[j].op =
                    idxStr[1] == '=' ? SQLITE_INDEX_CONSTRAINT_GE : SQLITE_INDEX_CONSTRAINT_GT;
                constraints[j++].rval = val2;
                break;
            case '<':
                val1 = sqlite3_value_double(argv[i]);
                val2 = val1 + v->context;
                c->constraints[i].rval = val1;
                c->constraints[i].op =
                    idxStr[1] == '=' ? SQLITE_INDEX_CONSTRAINT_LE : SQLITE_INDEX_CONSTRAINT_LT;
                if (idxStr[1])
                    eq_ts[eq++] = c->constraints[j].rval;
                constraints[j].op =
                    idxStr[1] == '=' ? SQLITE_INDEX_CONSTRAINT_LE : SQLITE_INDEX_CONSTRAINT_LT;
                constraints[j++].rval = val2;
                break;
            default:
                val1 = sqlite3_value_double(argv[i]);
                c->constraints[i].op = SQLITE_INDEX_CONSTRAINT_EQ;
                eq_ts[eq++] = c->constraints[i].rval = val1;
                val2 = val1 - v->context;
                if (val1 != val2) {
                    constraints[j].op = SQLITE_INDEX_CONSTRAINT_GE;
                    constraints[j++].rval = val2;
                }
                val2 = val1 + v->context;
                constraints[j].op = SQLITE_INDEX_CONSTRAINT_LE;
                constraints[j++].rval = val2;
                break;
        }
        idxStr += 2;
    }

    int cn = j;

    /* Open the full range */
    char** bufs = sqlite3_malloc((7 + !!eq + 5 * v->tablenum) * sizeof(char*));
    char** bufs2 = sqlite3_malloc((1 + cn * v->tablenum) * sizeof(char*));
    if (!bufs || !bufs2)
        return SQLITE_NOMEM;
    j = 0;
    bufs2[0] = 0;

    int k;
    for (k = 0; k < cn; k++) {
        char* op;
        switch (constraints[k].op) {
            case SQLITE_INDEX_CONSTRAINT_EQ:
                op = "=";
                break;
            case SQLITE_INDEX_CONSTRAINT_GT:
                op = ">";
                break;
            case SQLITE_INDEX_CONSTRAINT_LE:
                op = "<=";
                break;
            case SQLITE_INDEX_CONSTRAINT_LT:
                op = "<";
                break;
            case SQLITE_INDEX_CONSTRAINT_GE:
                op = ">=";
                break;
            case SQLITE_INDEX_CONSTRAINT_MATCH:
                op = "=";
                break;
            default:
                op = "?";
                break;
        }
        bufs2[j++] = sqlite3_mprintf("%s%s %s %f%s", k ? "" : "WHERE ", v->timestamp, op,
                                     constraints[k].rval, k != cn - 1 ? " AND " : "");
    }

    char* where;
    rc = concat_strings(bufs2, j, &where);
    if (rc != SQLITE_OK)
        return rc;

    /* Insert new rows for all equality contraints, so we can get interpolated values for them */
    if (eq || v->subsample > 0) {
        c->temptable = rand() | 1;
        char* buf =
            sqlite3_mprintf("CREATE TEMP TABLE temp.t%d ( %s %s )", c->temptable, v->timestamp,
                            v->coltypes[v->timestamp_idx] == SQLITE_INTEGER ? "INTEGER" : "FLOAT");
        rc = sqlite3_exec(v->db, buf, NULL, NULL, NULL);
        sqlite3_free(buf);
        if (rc != SQLITE_OK)
            return rc;

        for (i = 0; i < eq; i++) {
            if (v->coltypes[v->timestamp_idx] == SQLITE_INTEGER)
                buf = sqlite3_mprintf("INSERT INTO temp.t%d VALUES ( %ld )", c->temptable,
                                      (sqlite_int64)eq_ts[i]);
            else
                buf = sqlite3_mprintf("INSERT INTO temp.t%d VALUES ( %f )", c->temptable, eq_ts[i]);
            rc = sqlite3_exec(v->db, buf, NULL, NULL, NULL);
            if (rc != SQLITE_OK)
                return rc;
        }

        /* Insert timestamps for every subsample interval */
        if (v->subsample > 0) {
            int j = 0;
            /* First find range */
            sqlite3_stmt* stmt;
            char** bufs = sqlite3_malloc((3 + 2 * v->tablenum) * sizeof(char*));

            if (!bufs)
                return SQLITE_NOMEM;

            bufs[j++] =
                sqlite3_mprintf("SELECT min(%s), max(%s) FROM (\n", v->timestamp, v->timestamp);
            for (i = 0; i < v->tablenum; i++) {
                bufs[j++] = sqlite3_mprintf("  SELECT %s FROM (SELECT %s FROM %s %s)", v->timestamp,
                                            v->timestamp, v->tables[i], where);

                if (i != v->tablenum - 1)
                    bufs[j++] = sqlite3_mprintf(" UNION\n");
            }
            bufs[j++] = sqlite3_mprintf(")\n");

            char* stmtbuf;
            rc = concat_strings(bufs, j, &stmtbuf);
            if (rc != SQLITE_OK)
                return rc;
            // printf("%s: %s", v->table, stmtbuf);

            rc = sqlite3_prepare_v2(v->db, stmtbuf, -1, &stmt, NULL);
            sqlite3_free(stmtbuf);
            if (rc != SQLITE_OK) {
                sqlite3_finalize(stmt);
                return rc;
            }

            rc = sqlite3_step(stmt);
            if (rc != SQLITE_ROW) {
                sqlite3_finalize(stmt);
                return rc;
            }

            double start = sqlite3_column_double(stmt, 0);
            double end = sqlite3_column_double(stmt, 1);

            sqlite3_finalize(stmt);

            for (double i = start; i < end; i += v->subsample) {
                if (v->coltypes[v->timestamp_idx] == SQLITE_INTEGER)
                    buf = sqlite3_mprintf("INSERT INTO temp.t%d VALUES ( %ld )", c->temptable,
                                          (sqlite_int64)i);
                else
                    buf = sqlite3_mprintf("INSERT INTO temp.t%d VALUES ( %f )", c->temptable, i);
                rc = sqlite3_exec(v->db, buf, NULL, NULL, NULL);
                if (rc != SQLITE_OK)
                    return rc;
            }
        }
    }

    /* Find required context size for the first row */
    double start = DBL_MAX;
    for (i = 0; i < v->tablenum; i++)
        if (start > v->table_start[i])
            start = v->table_start[i];
    for (i = 0; i < argc; i++) {
        if (c->constraints[i].rval > start && (c->constraints[i].op == SQLITE_INDEX_CONSTRAINT_GE ||
                                               c->constraints[i].op == SQLITE_INDEX_CONSTRAINT_GT ||
                                               c->constraints[i].op == SQLITE_INDEX_CONSTRAINT_EQ))
            start = c->constraints[i].rval;
    }

    char** bufs3 = sqlite3_malloc((3 + !!eq + v->tablenum) * sizeof(char*));
    if (!bufs3)
        return SQLITE_NOMEM;
    j = 0;

    bufs3[j++] = sqlite3_mprintf("SELECT count(%s) FROM (\n", v->timestamp);
    if (eq)
        bufs3[j++] =
            sqlite3_mprintf("  SELECT %s FROM temp.t%d UNION\n", v->timestamp, c->temptable);
    for (i = 0; i < v->tablenum; i++) {
        bufs3[j++] =
            sqlite3_mprintf("  SELECT %s FROM %s WHERE %s >= %f AND %s <= %f %s", v->timestamp,
                            v->tables[i], v->timestamp, start - v->context, v->timestamp,
                            start + v->context, i == v->tablenum - 1 ? "" : "UNION\n");
    }
    if (v->subsample > 0)
        bufs3[j++] = sqlite3_mprintf("GROUP BY %s / %d ", v->timestamp, v->subsample);
    bufs3[j++] = sqlite3_mprintf("ORDER BY %s)\n", v->timestamp);
    char* stmtbuf;
    rc = concat_strings(bufs3, j, &stmtbuf);
    if (rc != SQLITE_OK)
        return rc;
    // printf("%s: %s", v->table, stmtbuf);

    rc = sqlite3_prepare_v2(v->db, stmtbuf, -1, &c->stmt, NULL);
    sqlite3_free(stmtbuf);
    if (rc != SQLITE_OK) {
        sqlite3_finalize(c->stmt);
        return rc;
    }

    rc = sqlite3_step(c->stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(c->stmt);
        return rc;
    }

    int res = sqlite3_column_double(c->stmt, 0);
    for (i = 8; i < res; i *= 2)
        ;
    c->row_context = i;

    c->context_values = sqlite3_malloc(sizeof(double*) * c->row_context);
    if (!c->context_values)
        return SQLITE_NOMEM;
    c->last = sqlite3_malloc(sizeof(*c->last) * c->cols * (v->filter ? 6 : 5) * v->contexts_num);
    if (!c->last)
        return SQLITE_NOMEM;

    for (i = 0; i < c->cols * (v->filter ? 6 : 5) * v->contexts_num; i++) {
        int j;
        c->last[i].v = c->last[i].t = c->last[i].min = c->last[i].max = myNAN;
        c->last[i].sorted = 0;
        for (j = 0; j < sizeof(c->last[i].j) / sizeof(int); j++)
            c->last[i].j[j] = -1;
    }

    /* Allocate row context */
    for (i = 0; i < c->row_context; i++) {
        c->context_values[i] = sqlite3_malloc(sizeof(double) * v->cols);
        if (!c->context_values[i])
            return SQLITE_NOMEM;
    }

    sqlite3_finalize(c->stmt);

    j = 0;
    if (!idxStr)
        idxStr = "";
    i = strlen(idxStr);
    for (k = 0; sqlite3_strnicmp(idxStr + k, "ORDER BY", 8) && k < i - 8; k++)
        ;
    if (k != i - 8)
        idxStr += k;

    for (k = 0; sqlite3_strnicmp(idxStr + k, " DESC", 5) && k < i - 5; k++)
        ;

    c->timestamp_order = k && k != i - 5 ? -1 : 1;

    if (k && k != i - 5 && !*where) {
        // Possibly optimised query, try to preserve and
        // selects that will cause a full scan, so that max()
        // might run optimised.

        // When SQLite supports NATURAL FULL OUTER JOIN, that should
        // replace the UNIONs and NATURAL LEFT OUTER JOIN.

        // TODO: improve performance of max(timestamp) if possible
#if 0  // Hopefully, query is only asking for max(timestamp), so we don't need all rows
    bufs[j++] = sqlite3_mprintf("SELECT %s FROM (\n", v->selection);
    for (i = 0; i < v->tablenum; i++) {
      bufs[j++] = sqlite3_mprintf("  SELECT %s FROM (SELECT %s FROM %s %s LIMIT 1)",
				  v->timestamp, v->timestamp, v->tables[i], idxStr);
      
      if (i != v->tablenum - 1)
	bufs[j++] = sqlite3_mprintf(" UNION\n");
    }
    bufs[j++] = sqlite3_mprintf(")\n");
#else  // max(timestamp) will require a full scan and the above assumes too much, is a fix possible?
        bufs[j++] = sqlite3_mprintf("SELECT %s FROM (\n", v->selection);
        if (eq)
            bufs[j++] =
                sqlite3_mprintf("  SELECT %s FROM temp.t%d UNION\n", v->timestamp, c->temptable);
        for (i = 0; i < v->tablenum; i++) {
            bufs[j++] =
                sqlite3_mprintf("  SELECT %s FROM %s %s", v->timestamp, v->tables[i], where);

            if (i != v->tablenum - 1)
                bufs[j++] = sqlite3_mprintf(" UNION\n");
        }

        if (v->subsample > 0)
            bufs[j++] = sqlite3_mprintf(" GROUP BY %s / %d\n", v->timestamp, v->subsample);
        bufs[j++] = sqlite3_mprintf(")\n");

#endif
        for (i = 0; i < v->tablenum; i++)
            bufs[j++] = sqlite3_mprintf("NATURAL LEFT OUTER JOIN %s ", v->tables[i]);

        bufs[j++] = sqlite3_mprintf("%s ", where);

        if (v->subsample > 0)
            bufs[j++] = sqlite3_mprintf("GROUP BY %s / %d ", v->timestamp, v->subsample);
        bufs[j++] = sqlite3_mprintf("%s\n", idxStr);
    } else {
        // No hints of special optimisation, limit the selection as much
        // as possible.

        bufs[j++] = sqlite3_mprintf("SELECT %s FROM (\n", v->selection);
        if (eq)
            bufs[j++] =
                sqlite3_mprintf("  SELECT %s FROM temp.t%d UNION\n", v->timestamp, c->temptable);
        for (i = 0; i < v->tablenum; i++) {
            bufs[j++] =
                sqlite3_mprintf("  SELECT %s FROM %s %s", v->timestamp, v->tables[i], where);

            if (i != v->tablenum - 1)
                bufs[j++] = sqlite3_mprintf(" UNION\n");
        }

        if (v->subsample > 0)
            bufs[j++] = sqlite3_mprintf(" GROUP BY %s / %d\n", v->timestamp, v->subsample);
        bufs[j++] = sqlite3_mprintf(" %s)\n", idxStr);
        for (i = 0; i < v->tablenum; i++) {
            char* buf = sqlite3_malloc(strlen(v->selection) + 1);
            if (!buf)
                return SQLITE_NOMEM;
            rc = strip_selection(v->db, buf, v->selection, v->tables[i]);
            if (rc != SQLITE_OK)
                return rc;
            bufs[j++] =
                sqlite3_mprintf("LEFT JOIN (SELECT %s FROM %s %s", buf, v->tables[i], where);
            if (v->subsample > 0)
                bufs[j++] = sqlite3_mprintf(" GROUP BY %s / %d", v->timestamp, v->subsample);
            bufs[j++] =
                sqlite3_mprintf(") USING(%s)%s", v->timestamp, i == v->tablenum - 1 ? "" : "\n");
            sqlite3_free(buf);
        }
        bufs[j++] = sqlite3_mprintf(" %s\n", idxStr);
    }

    sqlite3_free(where);

    rc = concat_strings(bufs, j, &stmtbuf);
    if (rc != SQLITE_OK)
        return rc;

    // printf("%s: %s", v->table, stmtbuf);
    rc = sqlite3_prepare_v2(v->db, stmtbuf, -1, &c->stmt, NULL);
    sqlite3_free(stmtbuf);
    if (rc != SQLITE_OK) {
        sqlite3_finalize(c->stmt);
        return rc;
    }

    /* Fill the context with NANs */
    for (i = 0; i < v->cols; i++)
        for (j = 0; j < c->row_context; j++)
            c->context_values[j][i] = myNAN;
    c->context_index = 0;
    c->context_end = -1;
    c->stmt_done = 0;
    rc = sqlite3_reset(c->stmt);
    if (rc != SQLITE_OK) {
        sqlite3_finalize(c->stmt);
        return rc;
    }
    c->eof = 0;

    c->timestamp_min = sqlite3_malloc(c->cols * sizeof(double));
    if (!c->timestamp_min)
        return SQLITE_NOMEM;

    for (i = 0; i < v->cols; i++)
        c->timestamp_min[i] = DBL_MAX;

    /* Fill the context */
    j = 0;
    do {
        int i;
        rc = sqlite3_step(c->stmt);
        if (rc == SQLITE_ROW) {
            for (i = 0; i < v->cols; i++) {
                c->context_values[j][i] =
                    sqlite3_value_type(sqlite3_column_value(c->stmt, i)) != SQLITE_INTEGER &&
                            sqlite3_value_type(sqlite3_column_value(c->stmt, i)) != SQLITE_FLOAT
                        ? myNAN
                        : sqlite3_column_double(c->stmt, i);
                if (i == v->timestamp_idx)  // Internally timestamp must be in ascending order
                    c->context_values[j][i] *= c->timestamp_order;
                if (c->timestamp_min[i] == DBL_MAX && !isnan(c->context_values[j][i]))
                    c->timestamp_min[i] =
                        sqlite3_column_double(c->stmt, v->timestamp_idx) * c->timestamp_order;
                c->context_end = j;
            }
        }
    } while (++j < c->row_context / 2 && rc == SQLITE_ROW);

    c->timestamp = c->context_values[0][v->timestamp_idx];

    if (rc == SQLITE_DONE) {
        c->stmt_done = 1;

        /* Did statememt return an empty table? */
        if (j == 1) {
            c->eof = 1;
            if (c->constraints) {
                sqlite3_free(c->constraints);
                c->constraints = 0;
            }
            return SQLITE_OK;
        }
    }

    if (rc == SQLITE_DONE || rc == SQLITE_ROW) {
        /* Get the first row */
        if (!check_constraints(c))
            return interpolate_next(cur);
    }
    return rc == SQLITE_DONE || rc == SQLITE_ROW ? SQLITE_OK : rc;
}

/* Use the timestamp as ROWID for the virtual table */
static int interpolate_rowid(sqlite3_vtab_cursor* cur, sqlite_int64* rowid) {
    *rowid = (sqlite_int64)((interpolate_cursor*)cur)->timestamp;
    return SQLITE_OK;
}

static int interpolate_eof(sqlite3_vtab_cursor* cur) {
    return ((interpolate_cursor*)cur)->eof;
}

/* Decide what constraints to pass on to interpolate_filter() */
static int interpolate_bestindex(sqlite3_vtab* tab, sqlite3_index_info* info) {
    interpolate_vtab* v = (interpolate_vtab*)tab;
    int i;
    int j = 0;
    char** bufs;
    char* str;

    if (info->nConstraint || info->nOrderBy) {
        bufs = sqlite3_malloc((3 + info->nConstraint + info->nOrderBy) * sizeof(char*));
        if (!bufs)
            return SQLITE_NOMEM;
    } else {
        info->needToFreeIdxStr = 1;
        info->idxStr = sqlite3_mprintf(" ");
        // info->idxStr = sqlite3_mprintf("ORDER BY %s", v->timestamp);
        return SQLITE_OK;
    }

    if (info->nConstraint) {
        int totcollen = 0;
        int constraints = info->nConstraint;

        /* Find the total length of the column names with constraints that we care about */
        for (i = 0; i < info->nConstraint; i++) {
            if (!info->aConstraint[i].usable || info->aConstraint[i].iColumn < 0) {
                constraints--;
                continue;
            }

            totcollen += strlen(v->colnames[info->aConstraint[i].iColumn]);
        }

        bufs[j++] = sqlite3_mprintf("WHERE ");

        int first = 0, ii = 0;
        /* Pass on all usable contraints on the timestamp to xFilter */
        for (i = 0; i < info->nConstraint; i++) {
            const char* op;
            int len = strlen(v->colnames[info->aConstraint[i].iColumn]);
            if (!info->aConstraint[i].usable || info->aConstraint[i].iColumn < 0 ||
                sqlite3_strnicmp(v->colnames[info->aConstraint[i].iColumn], v->timestamp,
                                 len > v->timestamp_len ? len : v->timestamp_len))
                continue;
            switch (info->aConstraint[i].op) {
                case SQLITE_INDEX_CONSTRAINT_EQ:
                    op = "=";
                    break;
                case SQLITE_INDEX_CONSTRAINT_GT:
                    op = ">";
                    break;
                case SQLITE_INDEX_CONSTRAINT_LE:
                    op = "<=";
                    break;
                case SQLITE_INDEX_CONSTRAINT_LT:
                    op = "<";
                    break;
                case SQLITE_INDEX_CONSTRAINT_GE:
                    op = ">=";
                    break;
                case SQLITE_INDEX_CONSTRAINT_MATCH:
                    op = "=";
                    break;
                default:
                    op = "?";
                    break;
            }
            bufs[j++] = sqlite3_mprintf("%s%s %s %%", first ? " AND " : "",
                                        v->colnames[info->aConstraint[i].iColumn], op);
            first = 1;
            info->aConstraintUsage[i].argvIndex = ++ii;
            /* Since we need the context, tell sqlite not to observe constraints on the timestamp */
            /* We will take care of that ourselves */
            info->aConstraintUsage[i].omit = 1;
        }
    }

    /* Add orderings */
    bufs[j++] = sqlite3_mprintf(" ORDER BY ");

    /* No ordering other than by the timestamp first allowed */
    info->orderByConsumed = !!info->nOrderBy && (info->aOrderBy[0].iColumn == -1 ||
                                                 info->aOrderBy[0].iColumn == v->timestamp_idx);

    if (info->orderByConsumed)
        for (i = 0; i < info->nOrderBy; i++) {
            const char* col =
                info->aOrderBy[i].iColumn == -1 ? "rowid" : v->colnames[info->aOrderBy[i].iColumn];
            bufs[j++] = sqlite3_mprintf("%s%s%s", col, info->aOrderBy[i].desc ? " DESC" : "",
                                        i + 1 == info->nOrderBy ? "" : ", ");
        }
    else
        bufs[j++] = sqlite3_mprintf("%s", v->timestamp);

    int rc = concat_strings(bufs, j, &str);
    if (rc != SQLITE_OK)
        return rc;
    // printf("%s: %s\n", v->table, str);
    info->needToFreeIdxStr = 1;
    info->idxStr = str;

    // If there are constraints, this is good, so choose this method
    info->estimatedCost = info->nConstraint ? 1 : 1000;

    return SQLITE_OK;
}

static int interpolate_rename(sqlite3_vtab* tab, const char* new_name) {
    return SQLITE_OK;
}

/* Register fucntions and modules.  This function must have external linkage */
int sqlite3_interpolate_init(sqlite3* db, char** pzErrMsg, const sqlite3_api_routines* pApi) {
    SQLITE_EXTENSION_INIT2(pApi);

    /* Scalar functions */
    static const struct {
        char* zFunctionName;
        int nArg;
        void (*xFunc)(sqlite3_context*, int, sqlite3_value**);
    } functions[] = {
        {"rnd", 1, rnd_func},
        {"rnd", 2, rnd_func},
        {"havg", -1, havg_func},
        {"havg_angle", -1, havg_angle_func},
        {"hexagesimal", 1, hexagesimal_func},
        {"hexagesimal", 2, hexagesimal_func},
    };

    /* Aggregate functions */
    static const struct {
        char* zFunctionName;
        int nArg;
        void (*xStep)(sqlite3_context*, int, sqlite3_value**);
        void (*xFinalize)(sqlite3_context*);
    } aggregates[] = {
        {"avg", 2, avg_step, avg_finalize},
        {"first", 2, first_step, firstlast_finalize},
        {"last", 2, last_step, firstlast_finalize},
        {"avg_angle", 1, avg_angle_step, avg_angle_finalize},
        {"avg_angle", 2, avg_angle_step, avg_angle_finalize},
        {"avg_angle", 3, avg_angle_step, avg_angle_finalize},
    };

    /* Modules */
    static const struct {
        const char* name;
        sqlite3_module module;
    } modules[] = {
        {"interpolate",
         {0,  // Version
          interpolate_connect,
          interpolate_connect,
          interpolate_bestindex,
          interpolate_disconnect,
          interpolate_disconnect,
          interpolate_open,
          interpolate_close,
          interpolate_filter,
          interpolate_next,
          interpolate_eof,
          interpolate_column,
          interpolate_rowid,
          0,  // no xUpdate
          0,  // no xBegin
          0,  // no xSync
          0,  // no xCommit
          0,  // no xRollback
          0,  // no xFindMethod
          interpolate_rename}},
    };

    if (!isnan(myNAN))
        myNAN = atof("nan");

    /* Register functions and modules */
    int i;
    for (i = 0; i < sizeof(functions) / sizeof(*functions); i++)
        sqlite3_create_function(db, functions[i].zFunctionName, functions[i].nArg, SQLITE_UTF8, db,
                                functions[i].xFunc, 0, 0);

    for (i = 0; i < sizeof(aggregates) / sizeof(*aggregates); i++)
        sqlite3_create_function(db, aggregates[i].zFunctionName, aggregates[i].nArg, SQLITE_UTF8,
                                db, 0, aggregates[i].xStep, aggregates[i].xFinalize);

    for (i = 0; i < sizeof(modules) / sizeof(*modules); i++)
        sqlite3_create_module(db, modules[i].name, &modules[i].module, 0);

    return 0;
}
