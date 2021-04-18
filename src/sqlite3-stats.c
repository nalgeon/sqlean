#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

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

/* end of binary tree implementation */

typedef uint8_t u8;
typedef uint16_t u16;
typedef int64_t i64;

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
 * Registers the extension.
 */
#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_stats_init(
        sqlite3 *db,
        char **pzErrMsg,
        const sqlite3_api_routines *pApi) {
    SQLITE_EXTENSION_INIT2(pApi);
    int err_count = 0;
    err_count += sqlite3_create_function(
        db, "stddev", 1, SQLITE_UTF8, 0, 0, varianceStep, stddevFinalize);
    err_count += sqlite3_create_function(
        db, "stddev_samp", 1, SQLITE_UTF8, 0, 0, varianceStep, stddevFinalize);
    err_count += sqlite3_create_function(
        db, "stddev_pop", 1, SQLITE_UTF8, 0, 0, varianceStep, stddevpopFinalize);
    err_count += sqlite3_create_function(
        db, "variance", 1, SQLITE_UTF8, 0, 0, varianceStep, varianceFinalize);
    err_count += sqlite3_create_function(
        db, "var_samp", 1, SQLITE_UTF8, 0, 0, varianceStep, varianceFinalize);
    err_count += sqlite3_create_function(
        db, "var_pop", 1, SQLITE_UTF8, 0, 0, varianceStep, variancepopFinalize);
    err_count += sqlite3_create_function(
        db, "mode", 1, SQLITE_UTF8, 0, 0, modeStep, modeFinalize);
    err_count += sqlite3_create_function(
        db, "median", 1, SQLITE_UTF8, 0, 0, modeStep, medianFinalize);
    err_count += sqlite3_create_function(
        db, "percentile_25", 1, SQLITE_UTF8, 0, 0, modeStep, percentile_25Finalize);
    err_count += sqlite3_create_function(
        db, "percentile_75", 1, SQLITE_UTF8, 0, 0, modeStep, percentile_75Finalize);
    err_count += sqlite3_create_function(
        db, "percentile_90", 1, SQLITE_UTF8, 0, 0, modeStep, percentile_90Finalize);
    err_count += sqlite3_create_function(
        db, "percentile_95", 1, SQLITE_UTF8, 0, 0, modeStep, percentile_95Finalize);
    err_count += sqlite3_create_function(
        db, "percentile_99", 1, SQLITE_UTF8, 0, 0, modeStep, percentile_99Finalize);
    return err_count ? SQLITE_ERROR : SQLITE_OK;
}