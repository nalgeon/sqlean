// Created by Keith Medcalf, Public Domain
// http://www.dessus.com/files/sqlfunc.c

// Even more math statistics functions.

#include <math.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

#ifndef LONGDOUBLE_TYPE
#define LONGDOUBLE_TYPE long double
#endif

/*
** Running Statistical Computations
*/

typedef struct momentCtx momentCtx;
struct momentCtx {
    LONGDOUBLE_TYPE m1;
    LONGDOUBLE_TYPE m2;
    LONGDOUBLE_TYPE m3;
    LONGDOUBLE_TYPE m4;
    sqlite_int64 n;
};

static void momentStep(sqlite3_context* context, int argc, sqlite3_value** argv) {
    momentCtx* p = sqlite3_aggregate_context(context, sizeof(momentCtx));
    if (p && sqlite3_value_numeric_type(argv[0]) != SQLITE_NULL) {
        register LONGDOUBLE_TYPE nx, y;
        register LONGDOUBLE_TYPE x = sqlite3_value_double(argv[0]);
        p->n++;
        nx = (LONGDOUBLE_TYPE)p->n;
        if (p->n == 1) {
            y = x;
            p->m1 = y;
            y *= x;
            p->m2 = y;
            y *= x;
            p->m3 = y;
            y *= x;
            p->m4 = y;
        } else {
            y = x;
            p->m1 += (x - p->m1) / nx;
            y *= x;
            p->m2 += (y - p->m2) / nx;
            y *= x;
            p->m3 += (y - p->m3) / nx;
            y *= x;
            p->m4 += (y - p->m4) / nx;
        }
    }
}

static void momentUnStep(sqlite3_context* context, int argc, sqlite3_value** argv) {
    momentCtx* p = sqlite3_aggregate_context(context, sizeof(momentCtx));
    if (p && sqlite3_value_numeric_type(argv[0]) != SQLITE_NULL) {
        register LONGDOUBLE_TYPE nx, y;
        register LONGDOUBLE_TYPE x = sqlite3_value_double(argv[0]);
        p->n--;
        nx = (LONGDOUBLE_TYPE)p->n;
        y = x;
        p->m1 -= (x - p->m1) / nx;
        y *= x;
        p->m2 -= (y - p->m2) / nx;
        y *= x;
        p->m3 -= (y - p->m3) / nx;
        y *= x;
        p->m4 -= (y - p->m4) / nx;
    }
}

static void skewPFinal(sqlite3_context* context) {
    register LONGDOUBLE_TYPE k2, k3, num, den;
    momentCtx* p = sqlite3_aggregate_context(context, 0);
    if (!p || p->n < 3)
        return;

    //  k1 = p->m1;
    k2 = p->m2 - powl(p->m1, 2.0);
    k3 = 2. * powl(p->m1, 3.) - 3. * p->m1 * p->m2 + p->m3;
    //  k4 = -6. * powl(p->m1, 4.) + 12. * powl(p->m1, 2.) * p->m2 - 3. * powl(p->m2, 2.) - 4. *
    //  p->m1 * p->m3 + p->m4;

    num = k3;
    den = powl(k2, 1.5);

    if (den != 0.0)
        sqlite3_result_double(context, num / den);
}

static void skewFinal(sqlite3_context* context) {
    register LONGDOUBLE_TYPE k2, k3, nx, num, den;
    momentCtx* p = sqlite3_aggregate_context(context, 0);
    if (!p || p->n < 3)
        return;

    nx = (LONGDOUBLE_TYPE)p->n;
    //  k1 = p->m1;
    k2 = p->m2 - powl(p->m1, 2.0);
    k3 = 2. * powl(p->m1, 3.) - 3. * p->m1 * p->m2 + p->m3;
    //  k4 = -6. * powl(p->m1, 4.) + 12. * powl(p->m1, 2.) * p->m2 - 3. * powl(p->m2, 2.) - 4. *
    //  p->m1 * p->m3 + p->m4;

    num = sqrtl(nx * (nx - 1.0)) / (nx - 2.0) * k3;
    den = powl(k2, 1.5);

    if (den != 0.0)
        sqlite3_result_double(context, num / den);
}

static void kurtPFinal(sqlite3_context* context) {
    register LONGDOUBLE_TYPE k2, k4, num, den, nx;
    momentCtx* p = sqlite3_aggregate_context(context, 0);
    if (!p || p->n < 3)
        return;

    nx = (LONGDOUBLE_TYPE)p->n;

    //  k1 = p->m1;
    k2 = p->m2 - powl(p->m1, 2.0);
    //  k3 = 2. * powl(p->m1, 3.) - 3. * p->m1 * p->m2 + p->m3;
    k4 = -6. * powl(p->m1, 4.) + 12. * powl(p->m1, 2.) * p->m2 - 3. * powl(p->m2, 2.) -
         4. * p->m1 * p->m3 + p->m4;

    num = k4;
    den = powl(k2, 2.);

    if (den != 0.0)
        sqlite3_result_double(context, num / den);
}

static void kurtFinal(sqlite3_context* context) {
    momentCtx* p = sqlite3_aggregate_context(context, 0);
    register LONGDOUBLE_TYPE k2, k4, num, den, nx;
    if (!p || p->n < 3)
        return;

    nx = (LONGDOUBLE_TYPE)p->n;

    //  k1 = p->m1;
    k2 = p->m2 - powl(p->m1, 2.0);
    //  k3 = 2. * powl(p->m1, 3.) - 3. * p->m1 * p->m2 + p->m3;
    k4 = -6. * powl(p->m1, 4.) + 12. * powl(p->m1, 2.) * p->m2 - 3. * powl(p->m2, 2.) -
         4. * p->m1 * p->m3 + p->m4;

    num = k4;
    den = powl(k2, 2.0);

    if (den == 0.0)
        return;

    sqlite3_result_double(context,
                          (nx - 1.) / ((nx - 2.) * (nx - 3.)) * ((nx + 1.) * (num / den) + 6.));
}

typedef struct stdevCtx stdevCtx;
struct stdevCtx {
    LONGDOUBLE_TYPE oldM, newM, oldS, newS;
    sqlite_int64 n;
};

static void stdevStep(sqlite3_context* context, int argc, sqlite3_value** argv) {
    stdevCtx* p = sqlite3_aggregate_context(context, sizeof(stdevCtx));
    if (p && sqlite3_value_numeric_type(argv[0]) != SQLITE_NULL) {
        register LONGDOUBLE_TYPE x = sqlite3_value_double(argv[0]);
        p->n++;
        if (p->n == 1) {
            p->oldM = p->newM = x;
            p->oldS = p->oldS = 0.0;
        } else {
            p->newM = p->oldM + (x - p->oldM) / (LONGDOUBLE_TYPE)p->n;
            p->newS = p->oldS + (x - p->oldM) * (x - p->newM);
            p->oldM = p->newM;
            p->oldS = p->newS;
        }
    }
}

static void stdevUnStep(sqlite3_context* context, int argc, sqlite3_value** argv) {
    stdevCtx* p = sqlite3_aggregate_context(context, sizeof(stdevCtx));
    if (p && sqlite3_value_numeric_type(argv[0]) != SQLITE_NULL) {
        register LONGDOUBLE_TYPE x = sqlite3_value_double(argv[0]);
        p->n--;
        p->newM = p->oldM - (x - p->oldM) / (LONGDOUBLE_TYPE)p->n;
        p->newS = p->oldS - (x - p->oldM) * (x - p->newM);
        p->oldM = p->newM;
        p->oldS = p->newS;
    }
}

static void varianceFinal(sqlite3_context* context) {
    stdevCtx* p = sqlite3_aggregate_context(context, 0);
    if (p && p->n > 0)
        sqlite3_result_double(context, p->n > 1 ? p->newS / (LONGDOUBLE_TYPE)(p->n - 1) : 0.0);
}

static void variancePFinal(sqlite3_context* context) {
    stdevCtx* p = sqlite3_aggregate_context(context, 0);
    if (p && p->n > 0)
        sqlite3_result_double(context, p->n > 0 ? p->newS / (LONGDOUBLE_TYPE)(p->n) : 0.0);
}

static void stdevFinal(sqlite3_context* context) {
    stdevCtx* p = sqlite3_aggregate_context(context, 0);
    if (p && p->n > 0)
        sqlite3_result_double(context,
                              p->n > 1 ? sqrtl(p->newS / (LONGDOUBLE_TYPE)(p->n - 1)) : 0.0);
}

static void stdevPFinal(sqlite3_context* context) {
    stdevCtx* p = sqlite3_aggregate_context(context, 0);
    if (p && p->n > 0)
        sqlite3_result_double(context, p->n > 0 ? sqrtl(p->newS / (LONGDOUBLE_TYPE)(p->n)) : 0.0);
}

static void semFinal(sqlite3_context* context) {
    stdevCtx* p = sqlite3_aggregate_context(context, 0);
    if (p && p->n > 0)
        sqlite3_result_double(
            context, p->n > 1 ? sqrtl(p->newS / (LONGDOUBLE_TYPE)(p->n - 1)) / sqrtl(p->n) : 0.0);
}

static void ciFinal(sqlite3_context* context) {
    stdevCtx* p = sqlite3_aggregate_context(context, 0);
    if (p && p->n > 0)
        sqlite3_result_double(
            context,
            p->n > 1 ? sqrtl(p->newS / (LONGDOUBLE_TYPE)(p->n - 1)) / sqrtl(p->n) * 1.96 : 0.0);
}

typedef struct rangeCtx rangeCtx;
struct rangeCtx {
    LONGDOUBLE_TYPE minx;
    LONGDOUBLE_TYPE maxx;
    int flag;
};

static void rangeStep(sqlite3_context* context, int argc, sqlite3_value** argv) {
    rangeCtx* p = sqlite3_aggregate_context(context, sizeof(rangeCtx));
    if (p && sqlite3_value_numeric_type(argv[0]) != SQLITE_NULL) {
        register LONGDOUBLE_TYPE x = (LONGDOUBLE_TYPE)sqlite3_value_double(argv[0]);
        if (p->flag == 0) {
            p->minx = x;
            p->maxx = x;
            p->flag = 1;
        } else {
            if (x < p->minx)
                p->minx = x;
            if (x > p->maxx)
                p->maxx = x;
        }
    }
}

static void rangeFinal(sqlite3_context* context) {
    rangeCtx* p = sqlite3_aggregate_context(context, 0);
    if (p)
        sqlite3_result_double(context, (p->maxx - p->minx) / 2.0L);
}

static void medianFinal(sqlite3_context* context) {
    rangeCtx* p = sqlite3_aggregate_context(context, 0);
    if (p)
        sqlite3_result_double(context, (p->maxx + p->minx) / 2.0L);
}

typedef struct meanCtx meanCtx;
struct meanCtx {
    LONGDOUBLE_TYPE m;
    sqlite_int64 n;
};

static void meanStep(sqlite3_context* context, int argc, sqlite3_value** argv) {
    meanCtx* p = sqlite3_aggregate_context(context, sizeof(meanCtx));
    if (p && sqlite3_value_numeric_type(argv[0]) != SQLITE_NULL) {
        p->n++;
        if (p->n == 1)
            p->m = (LONGDOUBLE_TYPE)sqlite3_value_double(argv[0]);
        else
            p->m += ((LONGDOUBLE_TYPE)sqlite3_value_double(argv[0]) - p->m) / (LONGDOUBLE_TYPE)p->n;
    }
}

static void meanUnStep(sqlite3_context* context, int argc, sqlite3_value** argv) {
    meanCtx* p = sqlite3_aggregate_context(context, sizeof(meanCtx));
    if (p && sqlite3_value_numeric_type(argv[0]) != SQLITE_NULL) {
        p->n--;
        p->m -= ((LONGDOUBLE_TYPE)sqlite3_value_double(argv[0]) - p->m) / (LONGDOUBLE_TYPE)p->n;
    }
}

static void ameanStep(sqlite3_context* context, int argc, sqlite3_value** argv) {
    meanCtx* p = sqlite3_aggregate_context(context, sizeof(meanCtx));
    if (p && sqlite3_value_numeric_type(argv[0]) != SQLITE_NULL) {
        p->n++;
        if (p->n == 1)
            p->m = (LONGDOUBLE_TYPE)fabs(sqlite3_value_double(argv[0]));
        else
            p->m += ((LONGDOUBLE_TYPE)fabs(sqlite3_value_double(argv[0])) - p->m) /
                    (LONGDOUBLE_TYPE)p->n;
    }
}

static void ameanUnStep(sqlite3_context* context, int argc, sqlite3_value** argv) {
    meanCtx* p = sqlite3_aggregate_context(context, sizeof(meanCtx));
    if (p && sqlite3_value_numeric_type(argv[0]) != SQLITE_NULL) {
        p->n--;
        p->m -=
            ((LONGDOUBLE_TYPE)fabs(sqlite3_value_double(argv[0])) - p->m) / (LONGDOUBLE_TYPE)p->n;
    }
}

static void gmeanStep(sqlite3_context* context, int argc, sqlite3_value** argv) {
    meanCtx* p = sqlite3_aggregate_context(context, sizeof(meanCtx));
    if (p && sqlite3_value_numeric_type(argv[0]) != SQLITE_NULL) {
        p->n++;
        if (p->n == 1)
            p->m = (LONGDOUBLE_TYPE)logl(sqlite3_value_double(argv[0]));
        else
            p->m += ((LONGDOUBLE_TYPE)logl(sqlite3_value_double(argv[0])) - p->m) /
                    (LONGDOUBLE_TYPE)p->n;
    }
}

static void gmeanUnStep(sqlite3_context* context, int argc, sqlite3_value** argv) {
    meanCtx* p = sqlite3_aggregate_context(context, sizeof(meanCtx));
    if (p && sqlite3_value_numeric_type(argv[0]) != SQLITE_NULL) {
        p->n--;
        p->m -=
            ((LONGDOUBLE_TYPE)logl(sqlite3_value_double(argv[0])) - p->m) / (LONGDOUBLE_TYPE)p->n;
    }
}

static void meanFinal(sqlite3_context* context) {
    meanCtx* p = sqlite3_aggregate_context(context, 0);
    if (p && p->n > 0)
        sqlite3_result_double(context, p->m);
}

static void gmeanFinal(sqlite3_context* context) {
    meanCtx* p = sqlite3_aggregate_context(context, 0);
    if (p && p->n > 0)
        sqlite3_result_double(context, expl(p->m));
}

static void rmsStep(sqlite3_context* context, int argc, sqlite3_value** argv) {
    meanCtx* p = sqlite3_aggregate_context(context, sizeof(meanCtx));
    if (p && sqlite3_value_numeric_type(argv[0]) != SQLITE_NULL) {
        register LONGDOUBLE_TYPE x = sqlite3_value_double(argv[0]);
        p->n++;
        if (p->n == 1)
            p->m = x * x;
        else
            p->m += (x * x - p->m) / (LONGDOUBLE_TYPE)p->n;
    }
}

static void rmsUnStep(sqlite3_context* context, int argc, sqlite3_value** argv) {
    meanCtx* p = sqlite3_aggregate_context(context, sizeof(meanCtx));
    if (p && sqlite3_value_numeric_type(argv[0]) != SQLITE_NULL) {
        register LONGDOUBLE_TYPE x = sqlite3_value_double(argv[0]);
        p->n--;
        p->m -= (x * x - p->m) / (LONGDOUBLE_TYPE)p->n;
    }
}

static void rmsFinal(sqlite3_context* context) {
    meanCtx* p = sqlite3_aggregate_context(context, 0);
    if (p && p->n > 0)
        sqlite3_result_double(context, sqrtl(p->m));
}

typedef struct wmeanCtx wmeanCtx;
struct wmeanCtx {
    sqlite_int64 n;
    LONGDOUBLE_TYPE m;
    LONGDOUBLE_TYPE w;
    LONGDOUBLE_TYPE s;
};

static void wmeanStep(sqlite3_context* context, int argc, sqlite3_value** argv) {
    wmeanCtx* p = sqlite3_aggregate_context(context, sizeof(wmeanCtx));
    if (p && sqlite3_value_numeric_type(argv[0]) != SQLITE_NULL &&
        sqlite3_value_numeric_type(argv[1]) != SQLITE_NULL) {
        register LONGDOUBLE_TYPE x = sqlite3_value_double(argv[0]);
        register LONGDOUBLE_TYPE w = sqlite3_value_double(argv[1]);
        if (p->n) {
            register LONGDOUBLE_TYPE newW, delta, R;
            newW = w + p->w;
            delta = x - p->m;
            R = delta * w / newW;
            p->m += R;
            p->s += p->w * delta * R;
            p->w = newW;
        } else {
            p->m = x;
            p->w = w;
            p->s = 0.0L;
        }
        p->n++;
    }
}

static void wmeanUnStep(sqlite3_context* context, int argc, sqlite3_value** argv) {
    wmeanCtx* p = sqlite3_aggregate_context(context, sizeof(wmeanCtx));
    if (p && sqlite3_value_numeric_type(argv[0]) != SQLITE_NULL &&
        sqlite3_value_numeric_type(argv[1]) != SQLITE_NULL) {
        register LONGDOUBLE_TYPE x = sqlite3_value_double(argv[0]);
        register LONGDOUBLE_TYPE w = sqlite3_value_double(argv[1]);
        register LONGDOUBLE_TYPE newW, delta, R;
        newW = p->w - w;
        delta = x - p->m;
        R = delta * w / newW;
        p->m -= R;
        p->s -= p->w * delta * R;
        p->w = newW;
        p->n--;
    }
}

static void wmeanFinal(sqlite3_context* context) {
    wmeanCtx* p = sqlite3_aggregate_context(context, 0);
    if (p && p->n > 0)
        sqlite3_result_double(context, p->m);
}

static void wvarPFinal(sqlite3_context* context) {
    wmeanCtx* p = sqlite3_aggregate_context(context, 0);
    if (p && p->n > 0)
        sqlite3_result_double(context, p->s / p->w);
}

static void wvarFinal(sqlite3_context* context) {
    wmeanCtx* p = sqlite3_aggregate_context(context, 0);
    if (p && p->n > 0)
        sqlite3_result_double(context,
                              p->s / p->w * (LONGDOUBLE_TYPE)p->n / (LONGDOUBLE_TYPE)(p->n - 1));
}

static void wstdevPFinal(sqlite3_context* context) {
    wmeanCtx* p = sqlite3_aggregate_context(context, 0);
    if (p && p->n > 0)
        sqlite3_result_double(context, sqrtl(p->s / p->w));
}

static void wstdevFinal(sqlite3_context* context) {
    wmeanCtx* p = sqlite3_aggregate_context(context, 0);
    if (p && p->n > 0)
        sqlite3_result_double(
            context, sqrtl(p->s / p->w * (LONGDOUBLE_TYPE)p->n / (LONGDOUBLE_TYPE)(p->n - 1)));
}

static void wsemFinal(sqlite3_context* context) {
    wmeanCtx* p = sqlite3_aggregate_context(context, 0);
    if (p && p->n > 0)
        sqlite3_result_double(
            context,
            sqrtl(p->s / p->w * (LONGDOUBLE_TYPE)p->n / (LONGDOUBLE_TYPE)(p->n - 1)) / sqrtl(p->n));
}

static void wciFinal(sqlite3_context* context) {
    wmeanCtx* p = sqlite3_aggregate_context(context, 0);
    if (p && p->n > 0)
        sqlite3_result_double(
            context, sqrtl(p->s / p->w * (LONGDOUBLE_TYPE)p->n / (LONGDOUBLE_TYPE)(p->n - 1)) /
                         sqrtl(p->n) * 1.96L);
}

typedef struct cvar2Ctx cvar2Ctx;
struct cvar2Ctx {
    sqlite_int64 n;
    LONGDOUBLE_TYPE mx;
    LONGDOUBLE_TYPE my;
    LONGDOUBLE_TYPE c1;
    LONGDOUBLE_TYPE c2;
};

static void covarStep(sqlite3_context* context, int argc, sqlite3_value** argv) {
    cvar2Ctx* p = sqlite3_aggregate_context(context, sizeof(cvar2Ctx));
    if (p && sqlite3_value_numeric_type(argv[0]) != SQLITE_NULL &&
        sqlite3_value_numeric_type(argv[1]) != SQLITE_NULL) {
        register LONGDOUBLE_TYPE x = sqlite3_value_double(argv[0]);
        register LONGDOUBLE_TYPE y = sqlite3_value_double(argv[1]);
        p->n++;
        if (p->n == 1) {
            p->mx = x;
            p->my = y;
            p->c1 = 0.0L;
            p->c2 = 0.0L;
        } else {
            register LONGDOUBLE_TYPE nx, ny, rn;
            rn = (LONGDOUBLE_TYPE)p->n;
            nx = p->mx + (x = p->mx) / rn;
            ny = p->my + (y - p->my) / rn;
            p->c1 += (x - nx) * (y - p->my);
            p->c2 += (x - p->mx) * (y - ny);
            p->mx = nx;
            p->my = ny;
        }
    }
}

static void covarFinal(sqlite3_context* context) {
    cvar2Ctx* p = sqlite3_aggregate_context(context, 0);
    if (p && p->n > 0)
        sqlite3_result_double(context, (p->c1 + p->c2) / 2.0L / (LONGDOUBLE_TYPE)p->n);
}

typedef struct fnnCtx fnnCtx;
struct fnnCtx {
    double fnn;
    int flag;
};

static void fnnStep(sqlite3_context* context, int argc, sqlite3_value** argv) {
    fnnCtx* p = sqlite3_aggregate_context(context, sizeof(fnnCtx));
    if (p && sqlite3_value_numeric_type(argv[0]) != SQLITE_NULL && p->flag == 0) {
        p->fnn = sqlite3_value_double(argv[0]);
        p->flag = 1;
    }
}

static void fnnUnstep(sqlite3_context* context, int argc, sqlite3_value** argv) {
    fnnCtx* p = sqlite3_aggregate_context(context, sizeof(fnnCtx));
    if (p && sqlite3_value_numeric_type(argv[0]) != SQLITE_NULL && p->flag == 1) {
        p->flag = 0;
    }
}

static void lnnStep(sqlite3_context* context, int argc, sqlite3_value** argv) {
    fnnCtx* p = sqlite3_aggregate_context(context, sizeof(fnnCtx));
    if (p && sqlite3_value_numeric_type(argv[0]) != SQLITE_NULL) {
        p->fnn = sqlite3_value_double(argv[0]);
        p->flag = 1;
    }
}

static void lnnUnstep(sqlite3_context* context, int argc, sqlite3_value** argv) {
    fnnCtx* p = sqlite3_aggregate_context(context, sizeof(fnnCtx));
    if (p && sqlite3_value_numeric_type(argv[0]) != SQLITE_NULL && p->flag == 1) {
        p->flag = 0;
    }
}

static void fnnFinal(sqlite3_context* context) {
    fnnCtx* p = sqlite3_aggregate_context(context, 0);
    if (p && p->flag == 1)
        sqlite3_result_double(context, p->fnn);
    else
        sqlite3_result_null(context);
}

#ifdef _WIN32
#ifndef SQLITE_CORE
__declspec(dllexport)
#endif
#endif
    int sqlite3_stats_init(sqlite3* db, char** pzErrMsg, const sqlite3_api_routines* pApi) {
    int flags = SQLITE_UTF8 | SQLITE_INNOCUOUS;

    SQLITE_EXTENSION_INIT2(pApi);

    sqlite3_create_window_function(db, "avg", 1, flags, 0, meanStep, meanFinal, meanFinal,
                                   meanUnStep, 0);
    sqlite3_create_window_function(db, "aavg", 1, flags, 0, ameanStep, meanFinal, meanFinal,
                                   ameanUnStep, 0);
    sqlite3_create_window_function(db, "gavg", 1, flags, 0, gmeanStep, gmeanFinal, gmeanFinal,
                                   gmeanUnStep, 0);
    sqlite3_create_window_function(db, "rms", 1, flags, 0, rmsStep, rmsFinal, rmsFinal, rmsUnStep,
                                   0);
    sqlite3_create_window_function(db, "stdev", 1, flags, 0, stdevStep, stdevFinal, stdevFinal,
                                   stdevUnStep, 0);
    sqlite3_create_window_function(db, "stdevP", 1, flags, 0, stdevStep, stdevPFinal, stdevPFinal,
                                   stdevUnStep, 0);
    sqlite3_create_window_function(db, "var", 1, flags, 0, stdevStep, varianceFinal, varianceFinal,
                                   stdevUnStep, 0);
    sqlite3_create_window_function(db, "varP", 1, flags, 0, stdevStep, variancePFinal,
                                   variancePFinal, stdevUnStep, 0);
    sqlite3_create_window_function(db, "sem", 1, flags, 0, stdevStep, semFinal, semFinal,
                                   stdevUnStep, 0);
    sqlite3_create_window_function(db, "ci", 1, flags, 0, stdevStep, ciFinal, ciFinal, stdevUnStep,
                                   0);
    sqlite3_create_window_function(db, "skewP", 1, flags, 0, momentStep, skewPFinal, skewPFinal,
                                   momentUnStep, 0);
    sqlite3_create_window_function(db, "skew", 1, flags, 0, momentStep, skewFinal, skewFinal,
                                   momentUnStep, 0);
    sqlite3_create_window_function(db, "kurtP", 1, flags, 0, momentStep, kurtPFinal, kurtPFinal,
                                   momentUnStep, 0);
    sqlite3_create_window_function(db, "kurt", 1, flags, 0, momentStep, kurtFinal, kurtFinal,
                                   momentUnStep, 0);

    sqlite3_create_window_function(db, "avg", 2, flags, 0, wmeanStep, wmeanFinal, wmeanFinal,
                                   wmeanUnStep, 0);
    sqlite3_create_window_function(db, "stdev", 2, flags, 0, wmeanStep, wstdevFinal, wstdevFinal,
                                   wmeanUnStep, 0);
    sqlite3_create_window_function(db, "stdevP", 2, flags, 0, wmeanStep, wstdevPFinal, wstdevPFinal,
                                   wmeanUnStep, 0);
    sqlite3_create_window_function(db, "var", 2, flags, 0, wmeanStep, wvarFinal, wvarFinal,
                                   wmeanUnStep, 0);
    sqlite3_create_window_function(db, "varP", 2, flags, 0, wmeanStep, wvarPFinal, wvarPFinal,
                                   wmeanUnStep, 0);
    sqlite3_create_window_function(db, "sem", 2, flags, 0, wmeanStep, wsemFinal, wsemFinal,
                                   wmeanUnStep, 0);
    sqlite3_create_window_function(db, "ci", 2, flags, 0, wmeanStep, wciFinal, wciFinal,
                                   wmeanUnStep, 0);

    sqlite3_create_window_function(db, "FirstNotNull", 1, flags, 0, fnnStep, fnnFinal, fnnFinal,
                                   fnnUnstep, 0);
    sqlite3_create_window_function(db, "LastNotNull", 1, flags, 0, lnnStep, fnnFinal, fnnFinal,
                                   lnnUnstep, 0);

    sqlite3_create_function(db, "range", 1, flags, 0, 0, rangeStep, rangeFinal);
    sqlite3_create_function(db, "median", 1, flags, 0, 0, rangeStep, medianFinal);
    sqlite3_create_function(db, "covar", 2, flags, 0, 0, covarStep, covarFinal);

    return SQLITE_OK;
}
