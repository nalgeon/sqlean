// Originally by D. Richard Hipp, Public Domain
// https://www.sqlite.org/src/file/ext/misc/fileio.c

// Modified by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean/

// lsdir(path[, recursive])
// Lists files and directories with the specified path.
// Implemented as a table-valued function.

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#if !defined(_WIN32) && !defined(WIN32)
#include <dirent.h>
#include <sys/time.h>
#include <unistd.h>
#include <utime.h>

#else

#define _MSC_VER 1929
#define FILEIO_WIN32_DLL
#include <direct.h>
#include <io.h>
#include "../test_windirent.h"
#include "windows.h"
#define dirent DIRENT

#ifndef chmod
#define chmod _chmod
#endif

#ifndef stat
#define stat _stat
#endif

#define mkdir(path, mode) _mkdir(path)
#define lstat(path, buf) stat(path, buf)

#endif  // !defined(_WIN32) && !defined(WIN32)

#include <errno.h>
#include <time.h>

#include "extension.h"

#include "../sqlite3ext.h"
SQLITE_EXTENSION_INIT3

/*
** This function is used in place of lstat().  On Windows, special handling
** is required in order for the included time to be returned as UTC.  On all
** other systems, this function simply calls lstat().
*/
static int fileLinkStat(const char* zPath, struct stat* pStatBuf) {
#if defined(_WIN32)
    int rc = lstat(zPath, pStatBuf);
    if (rc == 0)
        statTimesToUtc(zPath, pStatBuf);
    return rc;
#else
    return lstat(zPath, pStatBuf);
#endif
}

/*
** Structure of the fsdir() table-valued function
*/
/*    0    1    2     3    4           5             */
#define FSDIR_SCHEMA "(name,mode,mtime,size,path HIDDEN,dir HIDDEN)"
#define FSDIR_COLUMN_NAME 0  /* Name of the file */
#define FSDIR_COLUMN_MODE 1  /* Access mode */
#define FSDIR_COLUMN_MTIME 2 /* Last modification time */
#define FSDIR_COLUMN_SIZE 3  /* File size */
#define FSDIR_COLUMN_PATH 4  /* Path to top of search */
#define FSDIR_COLUMN_REC 5   /* Recursive flag */

/*
** Cursor type for recursively iterating through a directory structure.
*/
typedef struct fsdir_cursor fsdir_cursor;
typedef struct FsdirLevel FsdirLevel;

struct FsdirLevel {
    DIR* pDir;  /* From opendir() */
    char* zDir; /* Name of directory (nul-terminated) */
};

struct fsdir_cursor {
    sqlite3_vtab_cursor base; /* Base class - must be first */

    bool recursive; /* true to traverse dirs recursively, false otherwise */

    int nLvl;         /* Number of entries in aLvl[] array */
    int iLvl;         /* Index of current entry */
    FsdirLevel* aLvl; /* Hierarchy of directories being traversed */

    struct stat sStat;    /* Current lstat() results */
    char* zPath;          /* Path to current entry */
    sqlite3_int64 iRowid; /* Current rowid */
};

typedef struct fsdir_tab fsdir_tab;
struct fsdir_tab {
    sqlite3_vtab base; /* Base class - must be first */
};

/*
** Construct a new fsdir virtual table object.
*/
static int fsdirConnect(sqlite3* db,
                        void* pAux,
                        int argc,
                        const char* const* argv,
                        sqlite3_vtab** ppVtab,
                        char** pzErr) {
    fsdir_tab* pNew = 0;
    int rc;
    (void)pAux;
    (void)argc;
    (void)argv;
    (void)pzErr;
    rc = sqlite3_declare_vtab(db, "CREATE TABLE x" FSDIR_SCHEMA);
    if (rc == SQLITE_OK) {
        pNew = (fsdir_tab*)sqlite3_malloc(sizeof(*pNew));
        if (pNew == 0)
            return SQLITE_NOMEM;
        memset(pNew, 0, sizeof(*pNew));
        sqlite3_vtab_config(db, SQLITE_VTAB_DIRECTONLY);
    }
    *ppVtab = (sqlite3_vtab*)pNew;
    return rc;
}

/*
** This method is the destructor for fsdir vtab objects.
*/
static int fsdirDisconnect(sqlite3_vtab* pVtab) {
    sqlite3_free(pVtab);
    return SQLITE_OK;
}

/*
** Constructor for a new fsdir_cursor object.
*/
static int fsdirOpen(sqlite3_vtab* p, sqlite3_vtab_cursor** ppCursor) {
    fsdir_cursor* pCur;
    (void)p;
    pCur = sqlite3_malloc(sizeof(*pCur));
    if (pCur == 0)
        return SQLITE_NOMEM;
    memset(pCur, 0, sizeof(*pCur));
    pCur->iLvl = -1;
    *ppCursor = &pCur->base;
    return SQLITE_OK;
}

/*
** Reset a cursor back to the state it was in when first returned
** by fsdirOpen().
*/
static void fsdirResetCursor(fsdir_cursor* pCur) {
    int i;
    for (i = 0; i <= pCur->iLvl; i++) {
        FsdirLevel* pLvl = &pCur->aLvl[i];
        if (pLvl->pDir)
            closedir(pLvl->pDir);
        sqlite3_free(pLvl->zDir);
    }
    sqlite3_free(pCur->zPath);
    sqlite3_free(pCur->aLvl);
    pCur->aLvl = 0;
    pCur->zPath = 0;
    pCur->nLvl = 0;
    pCur->iLvl = -1;
    pCur->iRowid = 1;
}

/*
** Destructor for an fsdir_cursor.
*/
static int fsdirClose(sqlite3_vtab_cursor* cur) {
    fsdir_cursor* pCur = (fsdir_cursor*)cur;

    fsdirResetCursor(pCur);
    sqlite3_free(pCur);
    return SQLITE_OK;
}

/*
** Set the error message for the virtual table associated with cursor
** pCur to the results of vprintf(zFmt, ...).
*/
static void fsdirSetErrmsg(fsdir_cursor* pCur, const char* zFmt, ...) {
    va_list ap;
    va_start(ap, zFmt);
    pCur->base.pVtab->zErrMsg = sqlite3_vmprintf(zFmt, ap);
    va_end(ap);
}

/*
** Advance an fsdir_cursor to its next row of output.
*/
static int fsdirNext(sqlite3_vtab_cursor* cur) {
    fsdir_cursor* pCur = (fsdir_cursor*)cur;
    mode_t m = pCur->sStat.st_mode;

    pCur->iRowid++;
    if (S_ISDIR(m) && (pCur->iLvl == -1 || pCur->recursive)) {
        /* Descend into this directory */
        int iNew = pCur->iLvl + 1;
        FsdirLevel* pLvl;
        if (iNew >= pCur->nLvl) {
            int nNew = iNew + 1;
            sqlite3_int64 nByte = nNew * sizeof(FsdirLevel);
            FsdirLevel* aNew = (FsdirLevel*)sqlite3_realloc64(pCur->aLvl, nByte);
            if (aNew == 0)
                return SQLITE_NOMEM;
            memset(&aNew[pCur->nLvl], 0, sizeof(FsdirLevel) * (nNew - pCur->nLvl));
            pCur->aLvl = aNew;
            pCur->nLvl = nNew;
        }
        pCur->iLvl = iNew;
        pLvl = &pCur->aLvl[iNew];

        pLvl->zDir = pCur->zPath;
        pCur->zPath = 0;
        pLvl->pDir = opendir(pLvl->zDir);
        if (pLvl->pDir == 0) {
            fsdirSetErrmsg(pCur, "cannot read directory: %s", pCur->zPath);
            return SQLITE_ERROR;
        }
    }

    while (pCur->iLvl >= 0) {
        FsdirLevel* pLvl = &pCur->aLvl[pCur->iLvl];
        struct dirent* pEntry = readdir(pLvl->pDir);
        if (pEntry) {
            if (pEntry->d_name[0] == '.') {
                if (pEntry->d_name[1] == '.' && pEntry->d_name[2] == '\0')
                    continue;
                if (pEntry->d_name[1] == '\0')
                    continue;
            }
            sqlite3_free(pCur->zPath);
            pCur->zPath = sqlite3_mprintf("%s/%s", pLvl->zDir, pEntry->d_name);
            if (pCur->zPath == 0)
                return SQLITE_NOMEM;
            if (fileLinkStat(pCur->zPath, &pCur->sStat)) {
                fsdirSetErrmsg(pCur, "cannot stat file: %s", pCur->zPath);
                return SQLITE_ERROR;
            }
            return SQLITE_OK;
        }
        closedir(pLvl->pDir);
        sqlite3_free(pLvl->zDir);
        pLvl->pDir = 0;
        pLvl->zDir = 0;
        pCur->iLvl--;
    }

    /* EOF */
    sqlite3_free(pCur->zPath);
    pCur->zPath = 0;
    return SQLITE_OK;
}

/*
** Return values of columns for the row at which the series_cursor
** is currently pointing.
*/
static int fsdirColumn(sqlite3_vtab_cursor* cur, /* The cursor */
                       sqlite3_context* ctx,     /* First argument to sqlite3_result_...() */
                       int i                     /* Which column to return */
) {
    fsdir_cursor* pCur = (fsdir_cursor*)cur;
    switch (i) {
        case FSDIR_COLUMN_NAME: {
            sqlite3_result_text(ctx, pCur->zPath, -1, SQLITE_TRANSIENT);
            break;
        }

        case FSDIR_COLUMN_MODE:
            sqlite3_result_int64(ctx, pCur->sStat.st_mode);
            break;

        case FSDIR_COLUMN_MTIME:
            sqlite3_result_int64(ctx, pCur->sStat.st_mtime);
            break;

        case FSDIR_COLUMN_SIZE: {
            sqlite3_result_int64(ctx, pCur->sStat.st_size);
            break;
        }
        case FSDIR_COLUMN_PATH:
        default: {
            /* The FSDIR_COLUMN_PATH and FSDIR_COLUMN_REC are input parameters.
            ** always return their values as NULL */
            break;
        }
    }
    return SQLITE_OK;
}

/*
** Return the rowid for the current row. In this implementation, the
** first row returned is assigned rowid value 1, and each subsequent
** row a value 1 more than that of the previous.
*/
static int fsdirRowid(sqlite3_vtab_cursor* cur, sqlite_int64* pRowid) {
    fsdir_cursor* pCur = (fsdir_cursor*)cur;
    *pRowid = pCur->iRowid;
    return SQLITE_OK;
}

/*
** Return TRUE if the cursor has been moved off of the last
** row of output.
*/
static int fsdirEof(sqlite3_vtab_cursor* cur) {
    fsdir_cursor* pCur = (fsdir_cursor*)cur;
    return (pCur->zPath == 0);
}

/*
** xFilter callback.
**
** idxNum==0   PATH was not supplied (invalid function call)
** idxNum==1   PATH was supplied
*/
static int fsdirFilter(sqlite3_vtab_cursor* cur,
                       int idxNum,
                       const char* idxStr,
                       int argc,
                       sqlite3_value** argv) {
    fsdir_cursor* pCur = (fsdir_cursor*)cur;
    (void)idxStr;
    fsdirResetCursor(pCur);

    if (idxNum == 0) {
        fsdirSetErrmsg(pCur, "table function lsdir requires an argument");
        return SQLITE_ERROR;
    }

    assert(idxNum == 1 && (argc == 1 || argc == 2));
    const char* zPath = (const char*)sqlite3_value_text(argv[0]);
    if (zPath == 0) {
        fsdirSetErrmsg(pCur, "table function lsdir requires a non-NULL argument");
        return SQLITE_ERROR;
    }
    pCur->zPath = sqlite3_mprintf("%s", zPath);

    bool recursive = false;
    if (argc == 2) {
        recursive = (bool)sqlite3_value_int(argv[1]);
    }
    pCur->recursive = recursive;

    if (pCur->zPath == 0) {
        return SQLITE_NOMEM;
    }
    if (fileLinkStat(pCur->zPath, &pCur->sStat)) {
        // file does not exist, terminate via subsequent call to fsdirEof
        sqlite3_free(pCur->zPath);
        pCur->zPath = 0;
    }

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
** The query plan is represented by values of idxNum:
**
**  (1)  The path value is supplied by argv[0]
*/
static int fsdirBestIndex(sqlite3_vtab* tab, sqlite3_index_info* pIdxInfo) {
    int i;            /* Loop over constraints */
    int idxPath = -1; /* Index in pIdxInfo->aConstraint of PATH= */
    int idxRec = -1;  /* Index in pIdxInfo->aConstraint of REC= */
    int seenPath = 0; /* True if an unusable PATH= constraint is seen */
    int seenRec = 0;  /* True if an unusable REC= constraint is seen */
    const struct sqlite3_index_constraint* pConstraint;

    (void)tab;
    pConstraint = pIdxInfo->aConstraint;
    for (i = 0; i < pIdxInfo->nConstraint; i++, pConstraint++) {
        if (pConstraint->op != SQLITE_INDEX_CONSTRAINT_EQ)
            continue;
        switch (pConstraint->iColumn) {
            case FSDIR_COLUMN_PATH: {
                if (pConstraint->usable) {
                    idxPath = i;
                    seenPath = 0;
                } else if (idxPath < 0) {
                    seenPath = 1;
                }
                break;
            }
            case FSDIR_COLUMN_REC: {
                if (pConstraint->usable) {
                    idxRec = i;
                    seenRec = 0;
                } else if (idxRec < 0) {
                    seenRec = 1;
                }
                break;
            }
        }
    }
    if (seenPath || seenRec) {
        /* If input parameters are unusable, disallow this plan */
        return SQLITE_CONSTRAINT;
    }

    if (idxPath < 0) {
        pIdxInfo->idxNum = 0;
        /* The pIdxInfo->estimatedCost should have been initialized to a huge
        ** number.  Leave it unchanged. */
        pIdxInfo->estimatedRows = 0x7fffffff;
    } else {
        pIdxInfo->aConstraintUsage[idxPath].omit = 1;
        pIdxInfo->aConstraintUsage[idxPath].argvIndex = 1;
        if (idxRec >= 0) {
            pIdxInfo->aConstraintUsage[idxRec].omit = 1;
            pIdxInfo->aConstraintUsage[idxRec].argvIndex = 2;
        }
        pIdxInfo->idxNum = 1;
        pIdxInfo->estimatedCost = 100.0;
    }

    return SQLITE_OK;
}

static sqlite3_module fsdirModule = {
    0,               /* iVersion */
    0,               /* xCreate */
    fsdirConnect,    /* xConnect */
    fsdirBestIndex,  /* xBestIndex */
    fsdirDisconnect, /* xDisconnect */
    0,               /* xDestroy */
    fsdirOpen,       /* xOpen - open a cursor */
    fsdirClose,      /* xClose - close a cursor */
    fsdirFilter,     /* xFilter - configure scan constraints */
    fsdirNext,       /* xNext - advance a cursor */
    fsdirEof,        /* xEof - check for end of scan */
    fsdirColumn,     /* xColumn - read data */
    fsdirRowid,      /* xRowid - read data */
    0,               /* xUpdate */
    0,               /* xBegin */
    0,               /* xSync */
    0,               /* xCommit */
    0,               /* xRollback */
    0,               /* xFindMethod */
    0,               /* xRename */
    0,               /* xSavepoint */
    0,               /* xRelease */
    0,               /* xRollbackTo */
    0,               /* xShadowName */
};

/*
** Register the "fsdir" virtual table.
*/
int lsdir_init(sqlite3* db) {
    sqlite3_create_module(db, "lsdir", &fsdirModule, 0);
    return SQLITE_OK;
}