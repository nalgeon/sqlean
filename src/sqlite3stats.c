//#include "config.h"

#define COMPILE_SQLITE_EXTENSIONS_AS_LOADABLE_MODULE 1

#ifdef COMPILE_SQLITE_EXTENSIONS_AS_LOADABLE_MODULE
#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#else
#include "sqlite3.h"
#endif

#include <ctype.h>
/* relicoder */
#include <assert.h>
#include <errno.h> /* LMH 2007-03-25 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _MAP_H_
#define _MAP_H_

#include <stdint.h>

/*
** Simple binary tree implementation to use in median, mode and quartile calculations
** Tree is not necessarily balanced. That would require something like red&black trees of AVL
*/

typedef int (*cmp_func)(const void *, const void *);
typedef void (*map_iterator)(void *, int64_t, void *);

typedef struct node {
    struct node *l;
    struct node *r;
    void *data;
    int64_t count;
} node;

typedef struct map {
    node *base;
    cmp_func cmp;
    short free;
} map;

/*
** creates a map given a comparison function
*/
map map_make(cmp_func cmp);

/*
** inserts the element e into map m
*/
void map_insert(map *m, void *e);

/*
** executes function iter over all elements in the map, in key increasing order
*/
void map_iterate(map *m, map_iterator iter, void *p);

/*
** frees all memory used by a map
*/
void map_destroy(map *m);

/*
** compares 2 integers
** to use with map_make
*/
int int_cmp(const void *a, const void *b);

/*
** compares 2 doubles
** to use with map_make
*/
int double_cmp(const void *a, const void *b);

#endif /* _MAP_H_ */

typedef uint8_t u8;
typedef uint16_t u16;
typedef int64_t i64;

static char *sqlite3StrDup(const char *z) {
    char *res = sqlite3_malloc(strlen(z) + 1);
    return strcpy(res, z);
}

/*
** These are copied verbatim from fun.c so as to not have the names exported
*/

/* LMH from sqlite3 3.3.13 */
/*
** This table maps from the first byte of a UTF-8 character to the number
** of trailing bytes expected. A value '4' indicates that the table key
** is not a legal first byte for a UTF-8 character.
*/
static const u8 xtra_utf8_bytes[256] = {
    /* 0xxxxxxx */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,

    /* 10wwwwww */
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,

    /* 110yyyyy */
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,

    /* 1110zzzz */
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,

    /* 11110yyy */
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
};

/*
** This table maps from the number of trailing bytes in a UTF-8 character
** to an integer constant that is effectively calculated for each character
** read by a naive implementation of a UTF-8 character reader. The code
** in the READ_UTF8 macro explains things best.
*/
static const int xtra_utf8_bits[] = {
    0,
    12416,   /* (0xC0 << 6) + (0x80) */
    925824,  /* (0xE0 << 12) + (0x80 << 6) + (0x80) */
    63447168 /* (0xF0 << 18) + (0x80 << 12) + (0x80 << 6) + 0x80 */
};

/*
** If a UTF-8 character contains N bytes extra bytes (N bytes follow
** the initial byte so that the total character length is N+1) then
** masking the character with utf8_mask[N] must produce a non-zero
** result.  Otherwise, we have an (illegal) overlong encoding.
*/
static const int utf_mask[] = {
    0x00000000,
    0xffffff80,
    0xfffff800,
    0xffff0000,
};

/* LMH salvaged from sqlite3 3.3.13 source code src/utf.c */
#define READ_UTF8(zIn, c)                                                                                    \
    {                                                                                                        \
        int xtra;                                                                                            \
        c = *(zIn)++;                                                                                        \
        xtra = xtra_utf8_bytes[c];                                                                           \
        switch (xtra) {                                                                                      \
            case 4:                                                                                          \
                c = (int)0xFFFD;                                                                             \
                break;                                                                                       \
            case 3:                                                                                          \
                c = (c << 6) + *(zIn)++;                                                                     \
            case 2:                                                                                          \
                c = (c << 6) + *(zIn)++;                                                                     \
            case 1:                                                                                          \
                c = (c << 6) + *(zIn)++;                                                                     \
                c -= xtra_utf8_bits[xtra];                                                                   \
                if ((utf_mask[xtra] & c) == 0 || (c & 0xFFFFF800) == 0xD800 || (c & 0xFFFFFFFE) == 0xFFFE) { \
                    c = 0xFFFD;                                                                              \
                }                                                                                            \
        }                                                                                                    \
    }

static int sqlite3ReadUtf8(const unsigned char *z) {
    int c;
    READ_UTF8(z, c);
    return c;
}

#define SKIP_UTF8(zIn)                            \
    {                                             \
        zIn += (xtra_utf8_bytes[*(u8 *)zIn] + 1); \
    }

/*
** pZ is a UTF-8 encoded unicode string. If nByte is less than zero,
** return the number of unicode characters in pZ up to (but not including)
** the first 0x00 byte. If nByte is not less than zero, return the
** number of unicode characters in the first nByte of pZ (or up to
** the first 0x00, whichever comes first).
*/
static int sqlite3Utf8CharLen(const char *z, int nByte) {
    int r = 0;
    const char *zTerm;
    if (nByte >= 0) {
        zTerm = &z[nByte];
    } else {
        zTerm = (const char *)(-1);
    }
    assert(z <= zTerm);
    while (*z != 0 && z < zTerm) {
        SKIP_UTF8(z);
        r++;
    }
    return r;
}

/*
** X is a pointer to the first byte of a UTF-8 character.  Increment
** X so that it points to the next character.  This only works right
** if X points to a well-formed UTF-8 string.
*/
#define sqliteNextChar(X)             \
    while ((0xc0 & *++(X)) == 0x80) { \
    }
#define sqliteCharVal(X) sqlite3ReadUtf8(X)

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
    i64 cnt; /* number of elements */
};

/*
** An instance of the following structure holds the context of a
** mode() or median() aggregate computation.
** Depends on structures defined in map.c (see map & map)
** These aggregate functions only work for integers and floats although
** they could be made to work for strings. This is usually considered meaningless.
** Only usuall order (for median), no use of collation functions (would this even make sense?)
*/
typedef struct ModeCtx ModeCtx;
struct ModeCtx {
    i64 riM;       /* integer value found so far */
    double rdM;    /* double value found so far */
    i64 cnt;       /* number of elements so far */
    double pcnt;   /* number of elements smaller than a percentile */
    i64 mcnt;      /* maximum number of occurrences (for mode) */
    i64 mn;        /* number of occurrences (for mode and percentiles) */
    i64 is_double; /* whether the computation is being done for doubles (>0) or integers (=0) */
    map *m;        /* map structure used for the computation */
    int done;      /* whether the answer has been found */
};

/*
** called for each value received during a calculation of stddev or variance
*/
static void varianceStep(sqlite3_context *context, int argc, sqlite3_value **argv) {
    StddevCtx *p;

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
** called for each value received during a calculation of mode of median
*/
static void modeStep(sqlite3_context *context, int argc, sqlite3_value **argv) {
    ModeCtx *p;
    i64 xi = 0;
    double xd = 0.0;
    i64 *iptr;
    double *dptr;
    int type;

    assert(argc == 1);
    type = sqlite3_value_numeric_type(argv[0]);

    if (type == SQLITE_NULL)
        return;

    p = sqlite3_aggregate_context(context, sizeof(*p));

    if (0 == (p->m)) {
        p->m = calloc(1, sizeof(map));
        if (type == SQLITE_INTEGER) {
            /* map will be used for integers */
            *(p->m) = map_make(int_cmp);
            p->is_double = 0;
        } else {
            p->is_double = 1;
            /* map will be used for doubles */
            *(p->m) = map_make(double_cmp);
        }
    }

    ++(p->cnt);

    if (0 == p->is_double) {
        xi = sqlite3_value_int64(argv[0]);
        iptr = (i64 *)calloc(1, sizeof(i64));
        *iptr = xi;
        map_insert(p->m, iptr);
    } else {
        xd = sqlite3_value_double(argv[0]);
        dptr = (double *)calloc(1, sizeof(double));
        *dptr = xd;
        map_insert(p->m, dptr);
    }
}

/*
**  Auxiliary function that iterates all elements in a map and finds the mode
**  (most frequent value)
*/
static void modeIterate(void *e, i64 c, void *pp) {
    i64 ei;
    double ed;
    ModeCtx *p = (ModeCtx *)pp;

    if (0 == p->is_double) {
        ei = *(int *)(e);

        if (p->mcnt == c) {
            ++p->mn;
        } else if (p->mcnt < c) {
            p->riM = ei;
            p->mcnt = c;
            p->mn = 1;
        }
    } else {
        ed = *(double *)(e);

        if (p->mcnt == c) {
            ++p->mn;
        } else if (p->mcnt < c) {
            p->rdM = ed;
            p->mcnt = c;
            p->mn = 1;
        }
    }
}

/*
**  Auxiliary function that iterates all elements in a map and finds the median
**  (the value such that the number of elements smaller is equal the the number of
**  elements larger)
*/
static void medianIterate(void *e, i64 c, void *pp) {
    i64 ei;
    double ed;
    double iL;
    double iR;
    int il;
    int ir;
    ModeCtx *p = (ModeCtx *)pp;

    if (p->done > 0)
        return;

    iL = p->pcnt;
    iR = p->cnt - p->pcnt;
    il = p->mcnt + c;
    ir = p->cnt - p->mcnt;

    if (il >= iL) {
        if (ir >= iR) {
            ++p->mn;
            if (0 == p->is_double) {
                ei = *(int *)(e);
                p->riM += ei;
            } else {
                ed = *(double *)(e);
                p->rdM += ed;
            }
        } else {
            p->done = 1;
        }
    }
    p->mcnt += c;
}

/*
** Returns the mode value
*/
static void modeFinalize(sqlite3_context *context) {
    ModeCtx *p;
    p = sqlite3_aggregate_context(context, 0);
    if (p && p->m) {
        map_iterate(p->m, modeIterate, p);
        map_destroy(p->m);
        free(p->m);

        if (1 == p->mn) {
            if (0 == p->is_double)
                sqlite3_result_int64(context, p->riM);
            else
                sqlite3_result_double(context, p->rdM);
        }
    }
}

/*
** auxiliary function for percentiles
*/
static void _medianFinalize(sqlite3_context *context) {
    ModeCtx *p;
    p = (ModeCtx *)sqlite3_aggregate_context(context, 0);
    if (p && p->m) {
        p->done = 0;
        map_iterate(p->m, medianIterate, p);
        map_destroy(p->m);
        free(p->m);

        if (0 == p->is_double)
            if (1 == p->mn)
                sqlite3_result_int64(context, p->riM);
            else
                sqlite3_result_double(context, p->riM * 1.0 / p->mn);
        else
            sqlite3_result_double(context, p->rdM / p->mn);
    }
}

/*
** Returns the median value
*/
static void medianFinalize(sqlite3_context *context) {
    ModeCtx *p;
    p = (ModeCtx *)sqlite3_aggregate_context(context, 0);
    if (p != 0) {
        p->pcnt = (p->cnt) / 2.0;
        _medianFinalize(context);
    }
}

/*
** Returns the percentile_25 value
*/
static void percentile_25Finalize(sqlite3_context *context) {
    ModeCtx *p;
    p = (ModeCtx *)sqlite3_aggregate_context(context, 0);
    if (p != 0) {
        p->pcnt = (p->cnt) / 4.0;
        _medianFinalize(context);
    }
}

/*
** Returns the percentile_75 value
*/
static void percentile_75Finalize(sqlite3_context *context) {
    ModeCtx *p;
    p = (ModeCtx *)sqlite3_aggregate_context(context, 0);
    if (p != 0) {
        p->pcnt = (p->cnt) * 3 / 4.0;
        _medianFinalize(context);
    }
}

/*
** Returns the percentile_90 value
*/
static void percentile_90Finalize(sqlite3_context *context) {
    ModeCtx *p;
    p = (ModeCtx *)sqlite3_aggregate_context(context, 0);
    if (p != 0) {
        p->pcnt = (p->cnt) * 9 / 10.0;
        _medianFinalize(context);
    }
}

/*
** Returns the percentile_95 value
*/
static void percentile_95Finalize(sqlite3_context *context) {
    ModeCtx *p;
    p = (ModeCtx *)sqlite3_aggregate_context(context, 0);
    if (p != 0) {
        p->pcnt = (p->cnt) * 95 / 100.0;
        _medianFinalize(context);
    }
}

/*
** Returns the percentile_99 value
*/
static void percentile_99Finalize(sqlite3_context *context) {
    ModeCtx *p;
    p = (ModeCtx *)sqlite3_aggregate_context(context, 0);
    if (p != 0) {
        p->pcnt = (p->cnt) * 99 / 100.0;
        _medianFinalize(context);
    }
}

/*
** Returns the sample standard deviation value
*/
static void stddevFinalize(sqlite3_context *context) {
    StddevCtx *p;
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
static void stddevpopFinalize(sqlite3_context *context) {
    StddevCtx *p;
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
static void varianceFinalize(sqlite3_context *context) {
    StddevCtx *p;
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
static void variancepopFinalize(sqlite3_context *context) {
    StddevCtx *p;
    p = sqlite3_aggregate_context(context, 0);
    if (p && p->cnt > 1) {
        sqlite3_result_double(context, p->rS / p->cnt);
    } else {
        sqlite3_result_double(context, 0.0);
    }
}

/*
** This function registered all of the above C functions as SQL
** functions.  This should be the only routine in this file with
** external linkage.
*/
int RegisterExtensionFunctions(sqlite3 *db) {
    static const struct FuncDefAgg {
        char *zName;
        signed char nArg;
        u8 argType;
        u8 needCollSeq;
        void (*xStep)(sqlite3_context *, int, sqlite3_value **);
        void (*xFinalize)(sqlite3_context *);
    } aAggs[] = {
        {"stddev", 1, 0, 0, varianceStep, stddevFinalize},
        {"stddev_samp", 1, 0, 0, varianceStep, stddevFinalize},
        {"stddev_pop", 1, 0, 0, varianceStep, stddevpopFinalize},
        {"variance", 1, 0, 0, varianceStep, varianceFinalize},
        {"var_samp", 1, 0, 0, varianceStep, varianceFinalize},
        {"var_pop", 1, 0, 0, varianceStep, variancepopFinalize},
        {"mode", 1, 0, 0, modeStep, modeFinalize},
        {"median", 1, 0, 0, modeStep, medianFinalize},
        {"percentile_25", 1, 0, 0, modeStep, percentile_25Finalize},
        {"percentile_75", 1, 0, 0, modeStep, percentile_75Finalize},
        {"percentile_90", 1, 0, 0, modeStep, percentile_90Finalize},
        {"percentile_95", 1, 0, 0, modeStep, percentile_95Finalize},
        {"percentile_99", 1, 0, 0, modeStep, percentile_99Finalize},
    };
    int i;

    for (i = 0; i < sizeof(aAggs) / sizeof(aAggs[0]); i++) {
        void *pArg = 0;
        switch (aAggs[i].argType) {
            case 1:
                pArg = db;
                break;
            case 2:
                pArg = (void *)(-1);
                break;
        }
        //sqlite3CreateFunc
        /* LMH no error checking */
        sqlite3_create_function(db, aAggs[i].zName, aAggs[i].nArg, SQLITE_UTF8,
                                pArg, 0, aAggs[i].xStep, aAggs[i].xFinalize);
    }
    return 0;
}

#ifdef COMPILE_SQLITE_EXTENSIONS_AS_LOADABLE_MODULE
int sqlite3_sqlitestats_init(
    sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi) {
    SQLITE_EXTENSION_INIT2(pApi);
    RegisterExtensionFunctions(db);
    return 0;
}
#endif /* COMPILE_SQLITE_EXTENSIONS_AS_LOADABLE_MODULE */

map map_make(cmp_func cmp) {
    map r;
    r.cmp = cmp;
    r.base = 0;

    return r;
}

void *xcalloc(size_t nmemb, size_t size, char *s) {
    void *ret = calloc(nmemb, size);
    return ret;
}

void xfree(void *p) {
    free(p);
}

void node_insert(node **n, cmp_func cmp, void *e) {
    int c;
    node *nn;
    if (*n == 0) {
        nn = (node *)xcalloc(1, sizeof(node), "for node");
        nn->data = e;
        nn->count = 1;
        *n = nn;
    } else {
        c = cmp((*n)->data, e);
        if (0 == c) {
            ++((*n)->count);
            xfree(e);
        } else if (c > 0) {
            /* put it right here */
            node_insert(&((*n)->l), cmp, e);
        } else {
            node_insert(&((*n)->r), cmp, e);
        }
    }
}

void map_insert(map *m, void *e) {
    node_insert(&(m->base), m->cmp, e);
}

void node_iterate(node *n, map_iterator iter, void *p) {
    if (n) {
        if (n->l)
            node_iterate(n->l, iter, p);
        iter(n->data, n->count, p);
        if (n->r)
            node_iterate(n->r, iter, p);
    }
}

void map_iterate(map *m, map_iterator iter, void *p) {
    node_iterate(m->base, iter, p);
}

void node_destroy(node *n) {
    if (0 != n) {
        xfree(n->data);
        if (n->l)
            node_destroy(n->l);
        if (n->r)
            node_destroy(n->r);

        xfree(n);
    }
}

void map_destroy(map *m) {
    node_destroy(m->base);
}

int int_cmp(const void *a, const void *b) {
    int64_t aa = *(int64_t *)(a);
    int64_t bb = *(int64_t *)(b);
    /* printf("cmp %d <=> %d\n",aa,bb); */
    if (aa == bb)
        return 0;
    else if (aa < bb)
        return -1;
    else
        return 1;
}

int double_cmp(const void *a, const void *b) {
    double aa = *(double *)(a);
    double bb = *(double *)(b);
    /* printf("cmp %d <=> %d\n",aa,bb); */
    if (aa == bb)
        return 0;
    else if (aa < bb)
        return -1;
    else
        return 1;
}

void print_elem(void *e, int64_t c, void *p) {
    int ee = *(int *)(e);
    printf("%d => %lld\n", ee, c);
}
