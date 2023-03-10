// Standard deviation and variance by Liam Healy, Public Domain
// extension-functions.c at https://sqlite.org/contrib/

// Percentile and generate series by D. Richard Hipp, Public Domain
// https://sqlite.org/src/file/ext/misc/percentile.c
// https://sqlite.org/src/file/ext/misc/series.c

// Refactored by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Statistical functions for SQLite.

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sqlean.h"
#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

#pragma region Standard deviation and variance

/*
** An instance of the following structure holds the context of a
** stddev() or variance() aggregate computation.
** implementaion of http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Algorithm_II
** less prone to rounding errors
*/
typedef struct StddevCtx StddevCtx;
struct StddevCtx {
    double rM;
    double rS;
    int64_t cnt; /* number of elements */
};

/*
** called for each value received during a calculation of stddev or variance
*/
static void varianceStep(sqlite3_context* context, int argc, sqlite3_value** argv) {
    StddevCtx* p;

    double delta;
    double x;

    assert(argc == 1);
    p = sqlite3_aggregate_context(context, sizeof(*p));
    /* only consider non-null values */
    if (SQLITE_NULL != sqlite3_value_numeric_type(argv[0])) {
        p->cnt++;
        x = sqlite3_value_double(argv[0]);
        delta = (x - p->rM);
        p->rM += delta / p->cnt;
        p->rS += delta * (x - p->rM);
    }
}

/*
** Returns the sample standard deviation value
*/
static void stddevFinalize(sqlite3_context* context) {
    StddevCtx* p;
    p = sqlite3_aggregate_context(context, 0);
    if (p && p->cnt > 1) {
        sqlite3_result_double(context, sqrt(p->rS / (p->cnt - 1)));
    } else {
        sqlite3_result_double(context, 0.0);
    }
}

/*
** Returns the population standard deviation value
*/
static void stddevpopFinalize(sqlite3_context* context) {
    StddevCtx* p;
    p = sqlite3_aggregate_context(context, 0);
    if (p && p->cnt > 1) {
        sqlite3_result_double(context, sqrt(p->rS / p->cnt));
    } else {
        sqlite3_result_double(context, 0.0);
    }
}

/*
** Returns the sample variance value
*/
static void varianceFinalize(sqlite3_context* context) {
    StddevCtx* p;
    p = sqlite3_aggregate_context(context, 0);
    if (p && p->cnt > 1) {
        sqlite3_result_double(context, p->rS / (p->cnt - 1));
    } else {
        sqlite3_result_double(context, 0.0);
    }
}

/*
** Returns the population variance value
*/
static void variancepopFinalize(sqlite3_context* context) {
    StddevCtx* p;
    p = sqlite3_aggregate_context(context, 0);
    if (p && p->cnt > 1) {
        sqlite3_result_double(context, p->rS / p->cnt);
    } else {
        sqlite3_result_double(context, 0.0);
    }
}

#pragma endregion

#pragma region Percentile

/* The following object is the session context for a single percentile()
** function.  We have to remember all input Y values until the very end.
** Those values are accumulated in the Percentile.a[] array.
*/
typedef struct Percentile Percentile;
struct Percentile {
    unsigned nAlloc; /* Number of slots allocated for a[] */
    unsigned nUsed;  /* Number of slots actually used in a[] */
    double rPct;     /* 1.0 more than the value for P */
    double* a;       /* Array of Y values */
};

/*
** Return TRUE if the input floating-point number is an infinity.
*/
static int isInfinity(double r) {
    sqlite3_uint64 u;
    assert(sizeof(u) == sizeof(r));
    memcpy(&u, &r, sizeof(u));
    return ((u >> 52) & 0x7ff) == 0x7ff;
}

/*
** Return TRUE if two doubles differ by 0.001 or less
*/
static int sameValue(double a, double b) {
    a -= b;
    return a >= -0.001 && a <= 0.001;
}

/*
** The "step" function for percentile(Y,P) is called once for each
** input row.
*/
static void percentStep(sqlite3_context* pCtx, double rPct, int argc, sqlite3_value** argv) {
    Percentile* p;
    int eType;
    double y;

    /* Allocate the session context. */
    p = (Percentile*)sqlite3_aggregate_context(pCtx, sizeof(*p));
    if (p == 0)
        return;

    /* Remember the P value.  Throw an error if the P value is different
    ** from any prior row, per Requirement (2). */
    if (p->rPct == 0.0) {
        p->rPct = rPct + 1.0;
    } else if (!sameValue(p->rPct, rPct + 1.0)) {
        sqlite3_result_error(pCtx,
                             "2nd argument to percentile() is not the "
                             "same for all input rows",
                             -1);
        return;
    }

    /* Ignore rows for which Y is NULL */
    eType = sqlite3_value_type(argv[0]);
    if (eType == SQLITE_NULL)
        return;

    /* If not NULL, then Y must be numeric.  Otherwise throw an error.
    ** Requirement 4 */
    if (eType != SQLITE_INTEGER && eType != SQLITE_FLOAT) {
        sqlite3_result_error(pCtx,
                             "1st argument to percentile() is not "
                             "numeric",
                             -1);
        return;
    }

    /* Throw an error if the Y value is infinity or NaN */
    y = sqlite3_value_double(argv[0]);
    if (isInfinity(y)) {
        sqlite3_result_error(pCtx, "Inf input to percentile()", -1);
        return;
    }

    /* Allocate and store the Y */
    if (p->nUsed >= p->nAlloc) {
        unsigned n = p->nAlloc * 2 + 250;
        double* a = sqlite3_realloc64(p->a, sizeof(double) * n);
        if (a == 0) {
            sqlite3_free(p->a);
            memset(p, 0, sizeof(*p));
            sqlite3_result_error_nomem(pCtx);
            return;
        }
        p->nAlloc = n;
        p->a = a;
    }
    p->a[p->nUsed++] = y;
}

static void percentStepCustom(sqlite3_context* pCtx, int argc, sqlite3_value** argv) {
    assert(argc == 2);
    /* Requirement 3:  P must be a number between 0 and 100 */
    int eType = sqlite3_value_numeric_type(argv[1]);
    double rPct = sqlite3_value_double(argv[1]);
    if ((eType != SQLITE_INTEGER && eType != SQLITE_FLOAT) || rPct < 0.0 || rPct > 100.0) {
        sqlite3_result_error(pCtx,
                             "2nd argument to percentile() should be "
                             "a number between 0.0 and 100.0",
                             -1);
        return;
    }
    percentStep(pCtx, rPct, argc, argv);
}

static void percentStep25(sqlite3_context* pCtx, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    percentStep(pCtx, 25, argc, argv);
}

static void percentStep50(sqlite3_context* pCtx, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    percentStep(pCtx, 50, argc, argv);
}

static void percentStep75(sqlite3_context* pCtx, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    percentStep(pCtx, 75, argc, argv);
}

static void percentStep90(sqlite3_context* pCtx, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    percentStep(pCtx, 90, argc, argv);
}

static void percentStep95(sqlite3_context* pCtx, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    percentStep(pCtx, 95, argc, argv);
}

static void percentStep99(sqlite3_context* pCtx, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    percentStep(pCtx, 99, argc, argv);
}

/*
** Compare to doubles for sorting using qsort()
*/
static int SQLITE_CDECL doubleCmp(const void* pA, const void* pB) {
    double a = *(double*)pA;
    double b = *(double*)pB;
    if (a == b)
        return 0;
    if (a < b)
        return -1;
    return +1;
}

/*
** Called to compute the final output of percentile() and to clean
** up all allocated memory.
*/
static void percentFinal(sqlite3_context* pCtx) {
    Percentile* p;
    unsigned i1, i2;
    double v1, v2;
    double ix, vx;
    p = (Percentile*)sqlite3_aggregate_context(pCtx, 0);
    if (p == 0)
        return;
    if (p->a == 0)
        return;
    if (p->nUsed) {
        qsort(p->a, p->nUsed, sizeof(double), doubleCmp);
        ix = (p->rPct - 1.0) * (p->nUsed - 1) * 0.01;
        i1 = (unsigned)ix;
        i2 = ix == (double)i1 || i1 == p->nUsed - 1 ? i1 : i1 + 1;
        v1 = p->a[i1];
        v2 = p->a[i2];
        vx = v1 + (v2 - v1) * (ix - i1);
        sqlite3_result_double(pCtx, vx);
    }
    sqlite3_free(p->a);
    memset(p, 0, sizeof(*p));
}

#pragma endregion

#pragma region Generate series

#ifndef SQLITE_OMIT_VIRTUALTABLE

/* series_cursor is a subclass of sqlite3_vtab_cursor which will
** serve as the underlying representation of a cursor that scans
** over rows of the result
*/
typedef struct series_cursor series_cursor;
struct series_cursor {
    sqlite3_vtab_cursor base; /* Base class - must be first */
    int isDesc;               /* True to count down rather than up */
    sqlite3_int64 iRowid;     /* The rowid */
    sqlite3_int64 iValue;     /* Current value ("value") */
    sqlite3_int64 mnValue;    /* Mimimum value ("start") */
    sqlite3_int64 mxValue;    /* Maximum value ("stop") */
    sqlite3_int64 iStep;      /* Increment ("step") */
};

/*
** The seriesConnect() method is invoked to create a new
** series_vtab that describes the generate_series virtual table.
**
** Think of this routine as the constructor for series_vtab objects.
**
** All this routine needs to do is:
**
**    (1) Allocate the series_vtab object and initialize all fields.
**
**    (2) Tell SQLite (via the sqlite3_declare_vtab() interface) what the
**        result set of queries against generate_series will look like.
*/
static int seriesConnect(sqlite3* db,
                         void* pUnused,
                         int argcUnused,
                         const char* const* argvUnused,
                         sqlite3_vtab** ppVtab,
                         char** pzErrUnused) {
    sqlite3_vtab* pNew;
    int rc;

/* Column numbers */
#define SERIES_COLUMN_VALUE 0
#define SERIES_COLUMN_START 1
#define SERIES_COLUMN_STOP 2
#define SERIES_COLUMN_STEP 3

    (void)pUnused;
    (void)argcUnused;
    (void)argvUnused;
    (void)pzErrUnused;
    rc = sqlite3_declare_vtab(db, "CREATE TABLE x(value,start hidden,stop hidden,step hidden)");
    if (rc == SQLITE_OK) {
        pNew = *ppVtab = sqlite3_malloc(sizeof(*pNew));
        if (pNew == 0)
            return SQLITE_NOMEM;
        memset(pNew, 0, sizeof(*pNew));
        sqlite3_vtab_config(db, SQLITE_VTAB_INNOCUOUS);
    }
    return rc;
}

/*
** This method is the destructor for series_cursor objects.
*/
static int seriesDisconnect(sqlite3_vtab* pVtab) {
    sqlite3_free(pVtab);
    return SQLITE_OK;
}

/*
** Constructor for a new series_cursor object.
*/
static int seriesOpen(sqlite3_vtab* pUnused, sqlite3_vtab_cursor** ppCursor) {
    series_cursor* pCur;
    (void)pUnused;
    pCur = sqlite3_malloc(sizeof(*pCur));
    if (pCur == 0)
        return SQLITE_NOMEM;
    memset(pCur, 0, sizeof(*pCur));
    *ppCursor = &pCur->base;
    return SQLITE_OK;
}

/*
** Destructor for a series_cursor.
*/
static int seriesClose(sqlite3_vtab_cursor* cur) {
    sqlite3_free(cur);
    return SQLITE_OK;
}

/*
** Advance a series_cursor to its next row of output.
*/
static int seriesNext(sqlite3_vtab_cursor* cur) {
    series_cursor* pCur = (series_cursor*)cur;
    if (pCur->isDesc) {
        pCur->iValue -= pCur->iStep;
    } else {
        pCur->iValue += pCur->iStep;
    }
    pCur->iRowid++;
    return SQLITE_OK;
}

/*
** Return values of columns for the row at which the series_cursor
** is currently pointing.
*/
static int seriesColumn(sqlite3_vtab_cursor* cur, /* The cursor */
                        sqlite3_context* ctx,     /* First argument to sqlite3_result_...() */
                        int i                     /* Which column to return */
) {
    series_cursor* pCur = (series_cursor*)cur;
    sqlite3_int64 x = 0;
    switch (i) {
        case SERIES_COLUMN_START:
            x = pCur->mnValue;
            break;
        case SERIES_COLUMN_STOP:
            x = pCur->mxValue;
            break;
        case SERIES_COLUMN_STEP:
            x = pCur->iStep;
            break;
        default:
            x = pCur->iValue;
            break;
    }
    sqlite3_result_int64(ctx, x);
    return SQLITE_OK;
}

/*
** Return the rowid for the current row. In this implementation, the
** first row returned is assigned rowid value 1, and each subsequent
** row a value 1 more than that of the previous.
*/
static int seriesRowid(sqlite3_vtab_cursor* cur, sqlite_int64* pRowid) {
    series_cursor* pCur = (series_cursor*)cur;
    *pRowid = pCur->iRowid;
    return SQLITE_OK;
}

/*
** Return TRUE if the cursor has been moved off of the last
** row of output.
*/
static int seriesEof(sqlite3_vtab_cursor* cur) {
    series_cursor* pCur = (series_cursor*)cur;
    if (pCur->isDesc) {
        return pCur->iValue < pCur->mnValue;
    } else {
        return pCur->iValue > pCur->mxValue;
    }
}

/* True to cause run-time checking of the start=, stop=, and/or step=
** parameters.  The only reason to do this is for testing the
** constraint checking logic for virtual tables in the SQLite core.
*/
#ifndef SQLITE_SERIES_CONSTRAINT_VERIFY
#define SQLITE_SERIES_CONSTRAINT_VERIFY 0
#endif

/*
** This method is called to "rewind" the series_cursor object back
** to the first row of output.  This method is always called at least
** once prior to any call to seriesColumn() or seriesRowid() or
** seriesEof().
**
** The query plan selected by seriesBestIndex is passed in the idxNum
** parameter.  (idxStr is not used in this implementation.)  idxNum
** is a bitmask showing which constraints are available:
**
**    1:    start=VALUE
**    2:    stop=VALUE
**    4:    step=VALUE
**
** Also, if bit 8 is set, that means that the series should be output
** in descending order rather than in ascending order.  If bit 16 is
** set, then output must appear in ascending order.
**
** This routine should initialize the cursor and position it so that it
** is pointing at the first row, or pointing off the end of the table
** (so that seriesEof() will return true) if the table is empty.
*/
static int seriesFilter(sqlite3_vtab_cursor* pVtabCursor,
                        int idxNum,
                        const char* idxStrUnused,
                        int argc,
                        sqlite3_value** argv) {
    series_cursor* pCur = (series_cursor*)pVtabCursor;
    int i = 0;
    (void)idxStrUnused;
    if (idxNum & 1) {
        pCur->mnValue = sqlite3_value_int64(argv[i++]);
    } else {
        pCur->mnValue = 0;
    }
    if (idxNum & 2) {
        pCur->mxValue = sqlite3_value_int64(argv[i++]);
    } else {
        pCur->mxValue = 0xffffffff;
    }
    if (idxNum & 4) {
        pCur->iStep = sqlite3_value_int64(argv[i++]);
        if (pCur->iStep == 0) {
            pCur->iStep = 1;
        } else if (pCur->iStep < 0) {
            pCur->iStep = -pCur->iStep;
            if ((idxNum & 16) == 0)
                idxNum |= 8;
        }
    } else {
        pCur->iStep = 1;
    }
    for (i = 0; i < argc; i++) {
        if (sqlite3_value_type(argv[i]) == SQLITE_NULL) {
            /* If any of the constraints have a NULL value, then return no rows.
            ** See ticket https://www.sqlite.org/src/info/fac496b61722daf2 */
            pCur->mnValue = 1;
            pCur->mxValue = 0;
            break;
        }
    }
    if (idxNum & 8) {
        pCur->isDesc = 1;
        pCur->iValue = pCur->mxValue;
        if (pCur->iStep > 0) {
            pCur->iValue -= (pCur->mxValue - pCur->mnValue) % pCur->iStep;
        }
    } else {
        pCur->isDesc = 0;
        pCur->iValue = pCur->mnValue;
    }
    pCur->iRowid = 1;
    return SQLITE_OK;
}

/*
** SQLite will invoke this method one or more times while planning a query
** that uses the generate_series virtual table.  This routine needs to create
** a query plan for each invocation and compute an estimated cost for that
** plan.
**
** In this implementation idxNum is used to represent the
** query plan.  idxStr is unused.
**
** The query plan is represented by bits in idxNum:
**
**  (1)  start = $value  -- constraint exists
**  (2)  stop = $value   -- constraint exists
**  (4)  step = $value   -- constraint exists
**  (8)  output in descending order
*/
static int seriesBestIndex(sqlite3_vtab* pVTab, sqlite3_index_info* pIdxInfo) {
    int i, j;             /* Loop over constraints */
    int idxNum = 0;       /* The query plan bitmask */
    int bStartSeen = 0;   /* EQ constraint seen on the START column */
    int unusableMask = 0; /* Mask of unusable constraints */
    int nArg = 0;         /* Number of arguments that seriesFilter() expects */
    int aIdx[3];          /* Constraints on start, stop, and step */
    const struct sqlite3_index_constraint* pConstraint;

    /* This implementation assumes that the start, stop, and step columns
    ** are the last three columns in the virtual table. */
    assert(SERIES_COLUMN_STOP == SERIES_COLUMN_START + 1);
    assert(SERIES_COLUMN_STEP == SERIES_COLUMN_START + 2);

    aIdx[0] = aIdx[1] = aIdx[2] = -1;
    pConstraint = pIdxInfo->aConstraint;
    for (i = 0; i < pIdxInfo->nConstraint; i++, pConstraint++) {
        int iCol;  /* 0 for start, 1 for stop, 2 for step */
        int iMask; /* bitmask for those column */
        if (pConstraint->iColumn < SERIES_COLUMN_START)
            continue;
        iCol = pConstraint->iColumn - SERIES_COLUMN_START;
        assert(iCol >= 0 && iCol <= 2);
        iMask = 1 << iCol;
        if (iCol == 0)
            bStartSeen = 1;
        if (pConstraint->usable == 0) {
            unusableMask |= iMask;
            continue;
        } else if (pConstraint->op == SQLITE_INDEX_CONSTRAINT_EQ) {
            idxNum |= iMask;
            aIdx[iCol] = i;
        }
    }
    for (i = 0; i < 3; i++) {
        if ((j = aIdx[i]) >= 0) {
            pIdxInfo->aConstraintUsage[j].argvIndex = ++nArg;
            pIdxInfo->aConstraintUsage[j].omit = !SQLITE_SERIES_CONSTRAINT_VERIFY;
        }
    }
    /* The current generate_column() implementation requires at least one
    ** argument (the START value).  Legacy versions assumed START=0 if the
    ** first argument was omitted.  Compile with -DZERO_ARGUMENT_GENERATE_SERIES
    ** to obtain the legacy behavior */
#ifndef ZERO_ARGUMENT_GENERATE_SERIES
    if (!bStartSeen) {
        sqlite3_free(pVTab->zErrMsg);
        pVTab->zErrMsg =
            sqlite3_mprintf("first argument to \"generate_series()\" missing or unusable");
        return SQLITE_ERROR;
    }
#endif
    if ((unusableMask & ~idxNum) != 0) {
        /* The start, stop, and step columns are inputs.  Therefore if there
        ** are unusable constraints on any of start, stop, or step then
        ** this plan is unusable */
        return SQLITE_CONSTRAINT;
    }
    if ((idxNum & 3) == 3) {
        /* Both start= and stop= boundaries are available.  This is the
        ** the preferred case */
        pIdxInfo->estimatedCost = (double)(2 - ((idxNum & 4) != 0));
        pIdxInfo->estimatedRows = 1000;
        if (pIdxInfo->nOrderBy == 1) {
            if (pIdxInfo->aOrderBy[0].desc) {
                idxNum |= 8;
            } else {
                idxNum |= 16;
            }
            pIdxInfo->orderByConsumed = 1;
        }
    } else {
        /* If either boundary is missing, we have to generate a huge span
        ** of numbers.  Make this case very expensive so that the query
        ** planner will work hard to avoid it. */
        pIdxInfo->estimatedRows = 2147483647;
    }
    pIdxInfo->idxNum = idxNum;
    return SQLITE_OK;
}

/*
** This following structure defines all the methods for the
** generate_series virtual table.
*/
static sqlite3_module seriesModule = {
    0,                /* iVersion */
    0,                /* xCreate */
    seriesConnect,    /* xConnect */
    seriesBestIndex,  /* xBestIndex */
    seriesDisconnect, /* xDisconnect */
    0,                /* xDestroy */
    seriesOpen,       /* xOpen - open a cursor */
    seriesClose,      /* xClose - close a cursor */
    seriesFilter,     /* xFilter - configure scan constraints */
    seriesNext,       /* xNext - advance a cursor */
    seriesEof,        /* xEof - check for end of scan */
    seriesColumn,     /* xColumn - read data */
    seriesRowid,      /* xRowid - read data */
    0,                /* xUpdate */
    0,                /* xBegin */
    0,                /* xSync */
    0,                /* xCommit */
    0,                /* xRollback */
    0,                /* xFindMethod */
    0,                /* xRename */
    0,                /* xSavepoint */
    0,                /* xRelease */
    0,                /* xRollbackTo */
    0                 /* xShadowName */
};

#endif /* SQLITE_OMIT_VIRTUALTABLE */

#pragma endregion

// Returns the current Sqlean version.
static void sqlean_version(sqlite3_context* context, int argc, sqlite3_value** argv) {
    sqlite3_result_text(context, SQLEAN_VERSION, -1, SQLITE_STATIC);
}

/*
 * Registers the extension.
 */
#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_stats_init(sqlite3* db, char** pzErrMsg, const sqlite3_api_routines* pApi) {
    SQLITE_EXTENSION_INIT2(pApi);
    static const int flags = SQLITE_UTF8 | SQLITE_INNOCUOUS;
    sqlite3_create_function(db, "stddev", 1, flags, 0, 0, varianceStep, stddevFinalize);
    sqlite3_create_function(db, "stddev_samp", 1, flags, 0, 0, varianceStep, stddevFinalize);
    sqlite3_create_function(db, "stddev_pop", 1, flags, 0, 0, varianceStep, stddevpopFinalize);
    sqlite3_create_function(db, "variance", 1, flags, 0, 0, varianceStep, varianceFinalize);
    sqlite3_create_function(db, "var_samp", 1, flags, 0, 0, varianceStep, varianceFinalize);
    sqlite3_create_function(db, "var_pop", 1, flags, 0, 0, varianceStep, variancepopFinalize);
    sqlite3_create_function(db, "median", 1, flags, 0, 0, percentStep50, percentFinal);
    sqlite3_create_function(db, "percentile", 2, flags, 0, 0, percentStepCustom, percentFinal);
    sqlite3_create_function(db, "percentile_25", 1, flags, 0, 0, percentStep25, percentFinal);
    sqlite3_create_function(db, "percentile_75", 1, flags, 0, 0, percentStep75, percentFinal);
    sqlite3_create_function(db, "percentile_90", 1, flags, 0, 0, percentStep90, percentFinal);
    sqlite3_create_function(db, "percentile_95", 1, flags, 0, 0, percentStep95, percentFinal);
    sqlite3_create_function(db, "percentile_99", 1, flags, 0, 0, percentStep99, percentFinal);
#ifndef SQLITE_OMIT_VIRTUALTABLE
    if (sqlite3_libversion_number() >= 3008012) {
        sqlite3_create_module(db, "generate_series", &seriesModule, 0);
    }
#endif
    sqlite3_create_function(db, "sqlean_version", 0, flags, 0, sqlean_version, 0, 0);
    return SQLITE_OK;
}