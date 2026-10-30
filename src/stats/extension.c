// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Statistical functions for SQLite.

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT3

#include "stats/binding.h"
#include "stats/stats.h"

#pragma region Helpers

// has_null reports whether any of the arguments is NULL.
static bool has_null(int argc, sqlite3_value** argv) {
    for (int i = 0; i < argc; i++) {
        if (sqlite3_value_type(argv[i]) == SQLITE_NULL) {
            return true;
        }
    }
    return false;
}

// arg_real decodes a real argument into out, converting integers and numeric text.
// Returns false if the value is not a number.
static bool arg_real(sqlite3_value* value, double* out) {
    int type = sqlite3_value_numeric_type(value);
    if (type != SQLITE_INTEGER && type != SQLITE_FLOAT) {
        return false;
    }
    *out = sqlite3_value_double(value);
    return true;
}

#pragma endregion

#pragma region Percentiles

// PercContext is the state of a percentile aggregate.
typedef struct {
    Sample sample;
    // the percentile, set by the first row
    double p;
    bool has_p;
} PercContext;

// The percentiles of the fixed-percentile functions, passed as user data.
static const double P25 = 25;
static const double P50 = 50;
static const double P75 = 75;
static const double P90 = 90;
static const double P95 = 95;
static const double P99 = 99;

// arg_perc decodes the percentile: the second argument, or the user data
// for the fixed-percentile functions. The percentile is a spec argument,
// so an invalid one is an error. Sets the error and returns false
// if the value is not a valid percentile.
static bool arg_perc(sqlite3_context* context, int argc, sqlite3_value** argv, double* out) {
    if (argc == 1) {
        *out = *(const double*)sqlite3_user_data(context);
        return true;
    }
    if (!arg_real(argv[1], out) || !perc_valid(*out)) {
        sqlite3_result_error(context, "percentile should be a number between 0 and 100", -1);
        return false;
    }
    return true;
}

// stats_perc(x, p)
// stats_median(x), stats_p25(x), stats_p75(x), stats_p90(x), stats_p95(x), stats_p99(x)
static void fn_perc_step(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1 || argc == 2);
    if (has_null(argc, argv)) {
        return;
    }
    double p;
    if (!arg_perc(context, argc, argv, &p)) {
        return;
    }
    double x;
    if (!arg_real(argv[0], &x)) {
        // Bad data is skipped, as NULL is.
        return;
    }
    PercContext* ctx = sqlite3_aggregate_context(context, sizeof(*ctx));
    if (ctx == NULL) {
        sqlite3_result_error_nomem(context);
        return;
    }
    if (!ctx->has_p) {
        ctx->p = p;
        ctx->has_p = true;
    } else if (!perc_same(ctx->p, p)) {
        sqlite3_result_error(context, "percentile should be the same for all rows", -1);
        return;
    }
    if (!sample_add(&ctx->sample, x)) {
        sqlite3_result_error_nomem(context);
    }
}

// fn_perc_inverse removes a row that left the window frame.
static void fn_perc_inverse(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1 || argc == 2);
    // The row was skipped the same way when it was added.
    double x;
    if (has_null(argc, argv) || !arg_real(argv[0], &x)) {
        return;
    }
    PercContext* ctx = sqlite3_aggregate_context(context, 0);
    if (ctx == NULL) {
        return;
    }
    sample_remove(&ctx->sample, x);
}

// fn_perc_value returns the percentile of the rows so far.
static void fn_perc_value(sqlite3_context* context) {
    PercContext* ctx = sqlite3_aggregate_context(context, 0);
    double result;
    if (ctx == NULL || !sample_perc(&ctx->sample, ctx->p, &result)) {
        sqlite3_result_null(context);
        return;
    }
    sqlite3_result_double(context, result);
}

// fn_perc_final returns the percentile and releases the sample.
static void fn_perc_final(sqlite3_context* context) {
    fn_perc_value(context);
    PercContext* ctx = sqlite3_aggregate_context(context, 0);
    if (ctx != NULL) {
        sample_free(&ctx->sample);
    }
}

#pragma endregion

#pragma region Variance

// VarianceFunc is a function that computes a result from a Variance,
// passed as user data.
typedef bool (*VarianceFunc)(Variance v, double* out);

// stats_var(x), stats_var_samp(x), stats_var_pop(x)
// stats_stddev(x), stats_stddev_samp(x), stats_stddev_pop(x)
static void fn_var_step(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    double x;
    if (!arg_real(argv[0], &x)) {
        // NULL and bad data are skipped.
        return;
    }
    Variance* v = sqlite3_aggregate_context(context, sizeof(*v));
    if (v == NULL) {
        sqlite3_result_error_nomem(context);
        return;
    }
    var_add(v, x);
}

// fn_var_inverse removes a row that left the window frame.
static void fn_var_inverse(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    // The row was skipped the same way when it was added.
    double x;
    if (!arg_real(argv[0], &x)) {
        return;
    }
    Variance* v = sqlite3_aggregate_context(context, 0);
    if (v == NULL) {
        return;
    }
    var_remove(v, x);
}

// fn_var_value returns the variance or deviation of the rows so far.
static void fn_var_value(sqlite3_context* context) {
    VarianceFunc fn = (VarianceFunc)sqlite3_user_data(context);
    Variance* v = sqlite3_aggregate_context(context, 0);
    double result;
    if (v == NULL || !fn(*v, &result)) {
        sqlite3_result_null(context);
        return;
    }
    sqlite3_result_double(context, result);
}

#pragma endregion

#define FLAGS (SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC)

typedef struct {
    const char* name;
    int nargs;
    void* user_data;
    void (*step)(sqlite3_context* context, int argc, sqlite3_value** argv);
    void (*final)(sqlite3_context* context);
    void (*value)(sqlite3_context* context);
    void (*inverse)(sqlite3_context* context, int argc, sqlite3_value** argv);
} Aggregate;

#define PERC fn_perc_step, fn_perc_final, fn_perc_value, fn_perc_inverse
#define VAR fn_var_step, fn_var_value, fn_var_value, fn_var_inverse

static const Aggregate aggregates[] = {
    // percentiles
    {"stats_median", 1, (void*)&P50, PERC},
    {"stats_p25", 1, (void*)&P25, PERC},
    {"stats_p75", 1, (void*)&P75, PERC},
    {"stats_p90", 1, (void*)&P90, PERC},
    {"stats_p95", 1, (void*)&P95, PERC},
    {"stats_p99", 1, (void*)&P99, PERC},
    {"stats_perc", 2, 0, PERC},

    // variance and standard deviation
    {"stats_stddev", 1, (void*)var_stddev_samp, VAR},
    {"stats_stddev_samp", 1, (void*)var_stddev_samp, VAR},
    {"stats_stddev_pop", 1, (void*)var_stddev_pop, VAR},
    {"stats_var", 1, (void*)var_samp, VAR},
    {"stats_var_samp", 1, (void*)var_samp, VAR},
    {"stats_var_pop", 1, (void*)var_pop, VAR},

    // aliases
    {"median", 1, (void*)&P50, PERC},
    {"percentile_25", 1, (void*)&P25, PERC},
    {"percentile_75", 1, (void*)&P75, PERC},
    {"percentile_90", 1, (void*)&P90, PERC},
    {"percentile_95", 1, (void*)&P95, PERC},
    {"percentile_99", 1, (void*)&P99, PERC},
    {"percentile", 2, 0, PERC},
    {"stddev", 1, (void*)var_stddev_samp, VAR},
    {"stddev_samp", 1, (void*)var_stddev_samp, VAR},
    {"stddev_pop", 1, (void*)var_stddev_pop, VAR},
    {"variance", 1, (void*)var_samp, VAR},
    {"var_samp", 1, (void*)var_samp, VAR},
    {"var_pop", 1, (void*)var_pop, VAR},
};

int stats_init(sqlite3* db) {
    for (size_t i = 0; i < sizeof(aggregates) / sizeof(aggregates[0]); i++) {
        const Aggregate* a = &aggregates[i];
        int rc = sqlite3_create_window_function(db, a->name, a->nargs, FLAGS, a->user_data, a->step,
                                                a->final, a->value, a->inverse, 0);
        if (rc != SQLITE_OK) {
            return rc;
        }
    }
    return stats_seq_init(db);
}
