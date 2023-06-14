// Originally by D. Richard Hipp, Public Domain
// https://sqlite.org/src/file/ext/misc/series.c

// Modified by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean/

// generate_series function.

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT3

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
static sqlite3_module series_module = {
    .xConnect = seriesConnect,
    .xBestIndex = seriesBestIndex,
    .xDisconnect = seriesDisconnect,
    .xOpen = seriesOpen,
    .xClose = seriesClose,
    .xFilter = seriesFilter,
    .xNext = seriesNext,
    .xEof = seriesEof,
    .xColumn = seriesColumn,
    .xRowid = seriesRowid,
};

int stats_series_init(sqlite3* db) {
    sqlite3_create_module(db, "generate_series", &series_module, 0);
    return SQLITE_OK;
}
