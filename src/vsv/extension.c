// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Based on vsv.c by Keith Medcalf and SQLite's csv.c, Public Domain
// https://github.com/ncruces/kmedcalf-sqlite/blob/main/vsv.c
// https://sqlite.org/src/file/ext/misc/csv.c

// SQLite binding for the vsv virtual table: CSV files as tables.

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT3

#include "vsv/extension.h"
#include "vsv/vsv.h"

#pragma region Parameters

// Params are the parameters of create virtual table, dequoted.
typedef struct {
    char* filename;
    char* data;
    char* schema;
    char* fsep;
    bool header;
} Params;

static void params_free(Params* p) {
    sqlite3_free(p->filename);
    sqlite3_free(p->data);
    sqlite3_free(p->schema);
    sqlite3_free(p->fsep);
}

// is_space reports whether c is whitespace, as SQLite's tokenizer sees it.
static bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\f' || c == '\r';
}

// trim returns s without leading and trailing whitespace,
// and updates n (the length of s) to match.
static const char* trim(const char* s, size_t* n) {
    while (*n > 0 && is_space(s[0])) {
        s++;
        (*n)--;
    }
    while (*n > 0 && is_space(s[*n - 1])) {
        (*n)--;
    }
    return s;
}

// dequote returns a copy of the n bytes at s without the enclosing quotes
// (' or "), with doubled quotes inside undoubled. As in SQLite's
// sqlite3Dequote, the string ends at the first quote that isn't doubled.
// A string that isn't quoted is copied as is. Returns NULL if out of memory.
static char* dequote(const char* s, size_t n) {
    char* z = sqlite3_mprintf("%.*s", (int)n, s);
    if (z == NULL) {
        return NULL;
    }
    char quote = z[0];
    if ((quote != '\'' && quote != '"') || n < 2 || z[n - 1] != quote) {
        return z;
    }
    size_t j = 0;
    for (size_t i = 1; i < n - 1; i++) {
        if (z[i] == quote) {
            if (z[i + 1] != quote) {
                break;
            }
            i++;
        }
        z[j++] = z[i];
    }
    z[j] = '\0';
    return z;
}

// parse_bool parses a boolean value: yes, no, on, off, true, false, 1 or 0,
// in any case.
static bool parse_bool(const char* s, bool* value) {
    static const char* const yes[] = {"yes", "on", "true", "1"};
    static const char* const no[] = {"no", "off", "false", "0"};
    for (size_t i = 0; i < sizeof(yes) / sizeof(yes[0]); i++) {
        if (sqlite3_stricmp(s, yes[i]) == 0) {
            *value = true;
            return true;
        }
        if (sqlite3_stricmp(s, no[i]) == 0) {
            *value = false;
            return true;
        }
    }
    return false;
}

// parse_fsep parses the field separator: a single character, \t for a tab,
// or \xHH for an ASCII character in hex (most aren't easy to type in SQL).
static bool parse_fsep(const char* s, int32_t* comma) {
    char buf[1];
    size_t n = strlen(s);
    if (n == 2 && s[0] == '\\' && s[1] == 't') {
        buf[0] = '\t';
        s = buf;
        n = 1;
    } else if (n == 4 && s[0] == '\\' && s[1] == 'x') {
        int value = 0;
        for (size_t i = 2; i < 4; i++) {
            char c = s[i];
            int digit = c >= '0' && c <= '9'   ? c - '0'
                        : c >= 'a' && c <= 'f' ? c - 'a' + 10
                        : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                               : -1;
            if (digit < 0) {
                return false;
            }
            value = value * 16 + digit;
        }
        if (value >= 0x80) {
            return false;
        }
        buf[0] = (char)value;
        s = buf;
        n = 1;
    }
    return vsv_parse_comma(s, n, comma);
}

// is_name reports whether the n bytes at s are the parameter name, in any case.
static bool is_name(const char* s, size_t n, const char* name) {
    return n == strlen(name) && sqlite3_strnicmp(s, name, (int)n) == 0;
}

// parse_param parses one parameter ("name = value", or just "name" for
// a boolean) into p. Returns SQLITE_OK, or an error code and sets err.
static int parse_param(const char* arg, Params* p, char** err) {
    // Parameters that vsv used to support, which would now give
    // different results if ignored.
    static const char* const removed[] = {"rsep", "dsep",         "nulls",
                                          "skip", "validatetext", "affinity"};

    const char* eq = strchr(arg, '=');
    size_t name_len = eq != NULL ? (size_t)(eq - arg) : strlen(arg);
    const char* name = trim(arg, &name_len);
    const char* value = NULL;
    size_t value_len = 0;
    if (eq != NULL) {
        value_len = strlen(eq + 1);
        value = trim(eq + 1, &value_len);
    }

    char** str = NULL;
    if (is_name(name, name_len, "filename")) {
        str = &p->filename;
    } else if (is_name(name, name_len, "data")) {
        str = &p->data;
    } else if (is_name(name, name_len, "schema")) {
        str = &p->schema;
    } else if (is_name(name, name_len, "fsep")) {
        str = &p->fsep;
    }
    if (str != NULL) {
        if (value == NULL) {
            *err = sqlite3_mprintf("missing value for %.*s", (int)name_len, name);
            return SQLITE_ERROR;
        }
        if (*str != NULL) {
            *err = sqlite3_mprintf("more than one %.*s parameter", (int)name_len, name);
            return SQLITE_ERROR;
        }
        *str = dequote(value, value_len);
        return *str == NULL ? SQLITE_NOMEM : SQLITE_OK;
    }

    if (is_name(name, name_len, "header")) {
        if (value == NULL) {
            p->header = true;
            return SQLITE_OK;
        }
        char* z = dequote(value, value_len);
        if (z == NULL) {
            return SQLITE_NOMEM;
        }
        bool ok = parse_bool(z, &p->header);
        if (!ok) {
            *err = sqlite3_mprintf("invalid value for header: '%s'", z);
        }
        sqlite3_free(z);
        return ok ? SQLITE_OK : SQLITE_ERROR;
    }

    // The column count comes from the schema or the input, but earlier
    // versions required it with a schema, so it's accepted and ignored.
    if (is_name(name, name_len, "columns")) {
        return SQLITE_OK;
    }

    for (size_t i = 0; i < sizeof(removed) / sizeof(removed[0]); i++) {
        if (is_name(name, name_len, removed[i])) {
            *err = sqlite3_mprintf("unsupported parameter: %s", removed[i]);
            return SQLITE_ERROR;
        }
    }
    *err = sqlite3_mprintf("unknown parameter: %.*s", (int)name_len, name);
    return SQLITE_ERROR;
}

#pragma endregion

#pragma region Table

typedef struct {
    sqlite3_vtab base;
    sqlite3* db;
    // the input: a file name, or the data itself
    char* filename;
    char* data;
    int32_t comma;
    // whether the first record is a header
    bool header;
    // the affinity of each column
    VsvAffinity* affs;
    int ncols;
    // "select ?1" in a private connection, for converting fields to numbers;
    // NULL unless a column has a numeric affinity
    sqlite3* conv_db;
    sqlite3_stmt* conv;
} Table;

// Input is an open input of a table.
typedef struct {
    FILE* file;
    VsvBuffer buf;
    VsvReader reader;
    bool open;
} Input;

// input_open opens the table's input and starts reading it.
// Returns SQLITE_OK, or an error code and sets err.
static int input_open(Input* in, const Table* table, char** err) {
    memset(in, 0, sizeof(*in));
    VsvReadFunc read;
    void* src;
    if (table->filename != NULL) {
        int errnum = vsv_open_file(table->filename, &in->file);
        if (errnum == ENOMEM) {
            return SQLITE_NOMEM;
        }
        if (errnum != 0) {
            *err = sqlite3_mprintf("cannot open file: '%s'", table->filename);
            return SQLITE_ERROR;
        }
        read = vsv_read_file;
        src = in->file;
    } else {
        in->buf.data = table->data;
        in->buf.len = strlen(table->data);
        read = vsv_read_buffer;
        src = &in->buf;
    }
    size_t max = (size_t)sqlite3_limit(table->db, SQLITE_LIMIT_LENGTH, -1);
    vsv_reader_init(&in->reader, read, src, table->comma, max);
    in->open = true;
    return SQLITE_OK;
}

// input_close frees the input. Does nothing if it's not open.
static void input_close(Input* in) {
    if (!in->open) {
        return;
    }
    vsv_reader_free(&in->reader);
    if (in->file != NULL) {
        fclose(in->file);
    }
    memset(in, 0, sizeof(*in));
}

// read_error returns the error code for status, the result of a failed read,
// and sets err to its message.
static int read_error(const Table* table, const VsvReader* r, VsvStatus status, char** err) {
    sqlite3_free(*err);
    *err = NULL;
    const char* reason;
    switch (status) {
        case VSV_BARE_QUOTE:
            reason = "bare \" in non-quoted-field";
            break;
        case VSV_QUOTE:
            reason = "extraneous or missing \" in quoted-field";
            break;
        case VSV_NOMEM:
            return SQLITE_NOMEM;
        case VSV_TOOBIG:
            return SQLITE_TOOBIG;
        default:
            if (table->filename != NULL) {
                *err = sqlite3_mprintf("cannot read file: '%s'", table->filename);
            }
            return SQLITE_IOERR;
    }
    // The same messages as Go's csv.ParseError.
    if (r->err_start_line != r->err_pos.line) {
        *err = sqlite3_mprintf("record on line %lld; parse error on line %lld, column %lld: %s",
                               (long long)r->err_start_line, (long long)r->err_pos.line,
                               (long long)r->err_pos.col, reason);
    } else {
        *err = sqlite3_mprintf("parse error on line %lld, column %lld: %s",
                               (long long)r->err_pos.line, (long long)r->err_pos.col, reason);
    }
    return SQLITE_ERROR;
}

// read_names reads the first record and builds a schema from it:
// the fields are the column names with a header, otherwise the columns
// are named c0, c1, ... Empty input has one column, c0.
static int read_names(Table* table, char** schema, char** err) {
    Input in;
    int rc = input_open(&in, table, err);
    if (rc != SQLITE_OK) {
        return rc;
    }
    VsvStatus status = vsv_reader_next(&in.reader);
    if (status != VSV_OK && status != VSV_EOF) {
        rc = read_error(table, &in.reader, status, err);
        input_close(&in);
        return rc;
    }
    bool named = table->header && status == VSV_OK;
    size_t count = status == VSV_OK ? vsv_reader_count(&in.reader) : 1;

    sqlite3_str* s = sqlite3_str_new(table->db);
    sqlite3_str_appendall(s, "create table x(");
    for (size_t i = 0; i < count; i++) {
        if (i > 0) {
            sqlite3_str_appendchar(s, 1, ',');
        }
        if (named) {
            size_t len;
            const char* name = vsv_reader_field(&in.reader, i, &len);
            sqlite3_str_appendf(s, "\"%.*w\"", (int)len, name);
        } else {
            sqlite3_str_appendf(s, "c%llu", (unsigned long long)i);
        }
    }
    sqlite3_str_appendchar(s, 1, ')');
    input_close(&in);
    rc = sqlite3_str_errcode(s);
    *schema = sqlite3_str_finish(s);
    if (rc == SQLITE_OK && *schema == NULL) {
        rc = SQLITE_NOMEM;
    }
    return rc;
}

// read_types sets the column affinities from the declared types in schema,
// which is a valid create table statement. Runs it in a private connection
// and asks SQLite for the types, and keeps the connection for converting
// values if any column is numeric.
static int read_types(Table* table, const char* schema, char** err) {
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_open_v2(":memory:", &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (rc != SQLITE_OK) {
        goto done;
    }
    // Only the first statement runs, and the connection has nothing attached,
    // so the schema can't touch anything outside it.
    rc = sqlite3_prepare_v2(db, schema, -1, &stmt, NULL);
    if (rc == SQLITE_OK && stmt != NULL) {
        rc = sqlite3_step(stmt);
        rc = rc == SQLITE_DONE ? SQLITE_OK : rc;
    }
    if (rc != SQLITE_OK) {
        *err = sqlite3_mprintf("bad schema: '%s' - %s", schema, sqlite3_errmsg(db));
        goto done;
    }
    sqlite3_finalize(stmt);

    rc = sqlite3_prepare_v2(db,
                            "select type from pragma_table_xinfo(("
                            "select name from sqlite_schema "
                            "union all select name from temp.sqlite_schema))",
                            -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        goto done;
    }
    bool numeric = false;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        VsvAffinity* affs =
            sqlite3_realloc64(table->affs, (sqlite3_uint64)(table->ncols + 1) * sizeof(*affs));
        if (affs == NULL) {
            rc = SQLITE_NOMEM;
            goto done;
        }
        const char* type = (const char*)sqlite3_column_text(stmt, 0);
        affs[table->ncols] = vsv_affinity(type != NULL ? type : "");
        numeric = numeric || affs[table->ncols] >= VSV_AFF_NUMERIC;
        table->affs = affs;
        table->ncols++;
    }
    if (rc != SQLITE_DONE) {
        goto done;
    }
    sqlite3_finalize(stmt);
    stmt = NULL;
    rc = SQLITE_OK;

    if (numeric) {
        rc = sqlite3_prepare_v2(db, "select ?1", -1, &table->conv, NULL);
        if (rc != SQLITE_OK) {
            goto done;
        }
        sqlite3_limit(db, SQLITE_LIMIT_LENGTH, sqlite3_limit(table->db, SQLITE_LIMIT_LENGTH, -1));
        table->conv_db = db;
        db = NULL;
    }

done:
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return rc;
}

static int xdisconnect(sqlite3_vtab* vtab) {
    Table* table = (Table*)vtab;
    sqlite3_free(table->filename);
    sqlite3_free(table->data);
    sqlite3_free(table->affs);
    sqlite3_finalize(table->conv);
    sqlite3_close(table->conv_db);
    sqlite3_free(table);
    return SQLITE_OK;
}

// xconnect creates the virtual table from its parameters (argv[3:]).
// Bad parameters and an input that can't be opened are errors.
static int xconnect(sqlite3* db,
                    void* aux,
                    int argc,
                    const char* const* argv,
                    sqlite3_vtab** vtabptr,
                    char** errptr) {
    (void)aux;
    Params params = {0};
    Table* table = NULL;
    char* schema = NULL;
    int rc = SQLITE_OK;

    for (int i = 3; i < argc; i++) {
        rc = parse_param(argv[i], &params, errptr);
        if (rc != SQLITE_OK) {
            goto fail;
        }
    }
    if ((params.filename == NULL) == (params.data == NULL)) {
        *errptr = sqlite3_mprintf("must specify either filename or data, but not both");
        rc = SQLITE_ERROR;
        goto fail;
    }
    int32_t comma = ',';
    if (params.fsep != NULL && !parse_fsep(params.fsep, &comma)) {
        *errptr = sqlite3_mprintf("invalid value for fsep: '%s'", params.fsep);
        rc = SQLITE_ERROR;
        goto fail;
    }

    table = sqlite3_malloc64(sizeof(*table));
    if (table == NULL) {
        rc = SQLITE_NOMEM;
        goto fail;
    }
    memset(table, 0, sizeof(*table));
    table->db = db;
    table->filename = params.filename;
    table->data = params.data;
    params.filename = NULL;
    params.data = NULL;
    table->comma = comma;
    table->header = params.header;

    if (params.schema != NULL) {
        rc = sqlite3_declare_vtab(db, params.schema);
        if (rc != SQLITE_OK) {
            *errptr = sqlite3_mprintf("bad schema: '%s' - %s", params.schema, sqlite3_errmsg(db));
            goto fail;
        }
        rc = read_types(table, params.schema, errptr);
        if (rc != SQLITE_OK) {
            goto fail;
        }
        // The input is part of the table definition, so check it now.
        Input in;
        rc = input_open(&in, table, errptr);
        if (rc != SQLITE_OK) {
            goto fail;
        }
        input_close(&in);
    } else {
        rc = read_names(table, &schema, errptr);
        if (rc != SQLITE_OK) {
            goto fail;
        }
        rc = sqlite3_declare_vtab(db, schema);
        if (rc != SQLITE_OK) {
            *errptr = sqlite3_mprintf("bad schema: '%s' - %s", schema, sqlite3_errmsg(db));
            goto fail;
        }
    }

    // An attacker who controls a database schema could use this vtab
    // to read other files in the file system. vsv tables are meant to be
    // created in temp, so they can still be used from temp triggers and views.
    sqlite3_vtab_config(db, SQLITE_VTAB_DIRECTONLY);
    params_free(&params);
    sqlite3_free(schema);
    *vtabptr = &table->base;
    return SQLITE_OK;

fail:
    params_free(&params);
    sqlite3_free(schema);
    if (table != NULL) {
        xdisconnect(&table->base);
    }
    rc = rc == SQLITE_OK ? SQLITE_ERROR : rc;
    // Without a message, SQLite reports "vtable constructor failed".
    if (*errptr == NULL && rc != SQLITE_NOMEM) {
        *errptr = sqlite3_mprintf("%s", sqlite3_errstr(rc));
    }
    return rc;
}

// xbest_index uses a full scan, the only plan there is.
static int xbest_index(sqlite3_vtab* vtab, sqlite3_index_info* info) {
    (void)vtab;
    info->estimatedCost = 1000000;
    return SQLITE_OK;
}

#pragma endregion

#pragma region Cursor

typedef struct {
    sqlite3_vtab_cursor base;
    Input in;
    bool eof;
    sqlite3_int64 rowid;
} Cursor;

static int xopen(sqlite3_vtab* vtab, sqlite3_vtab_cursor** curptr) {
    (void)vtab;
    Cursor* cursor = sqlite3_malloc64(sizeof(*cursor));
    if (cursor == NULL) {
        return SQLITE_NOMEM;
    }
    memset(cursor, 0, sizeof(*cursor));
    cursor->eof = true;
    *curptr = &cursor->base;
    return SQLITE_OK;
}

// reset frees the cursor resources and moves it to the end.
static void reset(Cursor* cursor) {
    input_close(&cursor->in);
    cursor->eof = true;
    cursor->rowid = 0;
}

static int xclose(sqlite3_vtab_cursor* cur) {
    Cursor* cursor = (Cursor*)cur;
    reset(cursor);
    sqlite3_free(cursor);
    return SQLITE_OK;
}

// read_record reads the next record. Sets eof at the end of the input.
static int read_record(Cursor* cursor) {
    Table* table = (Table*)cursor->base.pVtab;
    VsvStatus status = vsv_reader_next(&cursor->in.reader);
    if (status == VSV_EOF) {
        cursor->eof = true;
        return SQLITE_OK;
    }
    if (status != VSV_OK) {
        cursor->eof = true;
        return read_error(table, &cursor->in.reader, status, &table->base.zErrMsg);
    }
    return SQLITE_OK;
}

// xnext reads the next record. A malformed record is an error.
static int xnext(sqlite3_vtab_cursor* cur) {
    Cursor* cursor = (Cursor*)cur;
    cursor->rowid++;
    return read_record(cursor);
}

// xfilter opens the input, skips the header and reads the first record.
// Each scan reads the input from the start.
static int xfilter(sqlite3_vtab_cursor* cur,
                   int idx_num,
                   const char* idx_str,
                   int argc,
                   sqlite3_value** argv) {
    (void)idx_num;
    (void)idx_str;
    (void)argc;
    (void)argv;
    Cursor* cursor = (Cursor*)cur;
    Table* table = (Table*)cursor->base.pVtab;
    reset(cursor);
    sqlite3_free(table->base.zErrMsg);
    table->base.zErrMsg = NULL;
    int rc = input_open(&cursor->in, table, &table->base.zErrMsg);
    if (rc != SQLITE_OK) {
        return rc;
    }
    cursor->eof = false;
    if (table->header) {
        rc = read_record(cursor);
        if (rc != SQLITE_OK || cursor->eof) {
            return rc;
        }
    }
    return xnext(cur);
}

// convert sets the result to the n bytes at s converted to a number,
// as SQLite does for a column with the affinity. Text that doesn't look
// like a number stays text.
static int convert(sqlite3_context* ctx,
                   const Table* table,
                   VsvAffinity aff,
                   const char* s,
                   size_t n) {
    sqlite3_stmt* stmt = table->conv;
    int rc = sqlite3_bind_text64(stmt, 1, s, n, SQLITE_STATIC, SQLITE_UTF8);
    if (rc == SQLITE_TOOBIG) {
        // Too long for the private connection, too long to be a number.
        sqlite3_result_text64(ctx, s, n, SQLITE_TRANSIENT, SQLITE_UTF8);
        return SQLITE_OK;
    }
    if (rc == SQLITE_OK) {
        rc = sqlite3_step(stmt);
    }
    if (rc == SQLITE_ROW) {
        rc = SQLITE_OK;
        sqlite3_value* value = sqlite3_column_value(stmt, 0);
        // Applies SQLite's numeric affinity to the value in place.
        switch (sqlite3_value_numeric_type(value)) {
            case SQLITE_INTEGER: {
                sqlite3_int64 i = sqlite3_value_int64(value);
                if (aff == VSV_AFF_REAL) {
                    sqlite3_result_double(ctx, (double)i);
                } else {
                    sqlite3_result_int64(ctx, i);
                }
                break;
            }
            case SQLITE_FLOAT: {
                double r = sqlite3_value_double(value);
                int64_t i;
                if (aff != VSV_AFF_REAL && vsv_real_to_int(r, &i)) {
                    sqlite3_result_int64(ctx, i);
                } else {
                    sqlite3_result_double(ctx, r);
                }
                break;
            }
            default:
                sqlite3_result_text64(ctx, s, n, SQLITE_TRANSIENT, SQLITE_UTF8);
                break;
        }
    }
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
    return rc;
}

// xcolumn returns field col of the current record, or NULL if the record
// is shorter. Fields are converted with the column's affinity.
static int xcolumn(sqlite3_vtab_cursor* cur, sqlite3_context* ctx, int col) {
    Cursor* cursor = (Cursor*)cur;
    Table* table = (Table*)cursor->base.pVtab;
    if (col < 0 || (size_t)col >= vsv_reader_count(&cursor->in.reader)) {
        return SQLITE_OK;
    }
    size_t n;
    const char* s = vsv_reader_field(&cursor->in.reader, (size_t)col, &n);
    VsvAffinity aff = table->affs != NULL && col < table->ncols ? table->affs[col] : VSV_AFF_TEXT;
    if (aff >= VSV_AFF_NUMERIC && table->conv != NULL) {
        return convert(ctx, table, aff, s, n);
    }
    sqlite3_result_text64(ctx, s, n, SQLITE_TRANSIENT, SQLITE_UTF8);
    return SQLITE_OK;
}

// xrowid returns the number of the current record: 1 for the first one
// (after the header), and one more for each next one.
static int xrowid(sqlite3_vtab_cursor* cur, sqlite3_int64* rowid) {
    Cursor* cursor = (Cursor*)cur;
    *rowid = cursor->rowid;
    return SQLITE_OK;
}

static int xeof(sqlite3_vtab_cursor* cur) {
    Cursor* cursor = (Cursor*)cur;
    return cursor->eof;
}

#pragma endregion

static const sqlite3_module vsv_module = {
    .xCreate = xconnect,
    .xConnect = xconnect,
    .xBestIndex = xbest_index,
    .xDisconnect = xdisconnect,
    .xDestroy = xdisconnect,
    .xOpen = xopen,
    .xClose = xclose,
    .xFilter = xfilter,
    .xNext = xnext,
    .xEof = xeof,
    .xColumn = xcolumn,
    .xRowid = xrowid,
};

int vsv_init(sqlite3* db) {
    return sqlite3_create_module(db, "vsv", &vsv_module, 0);
}
