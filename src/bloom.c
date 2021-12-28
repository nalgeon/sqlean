/*
Copyright 2018 Shawn Wagner

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/* Bloom filter virtual table */

#include <math.h>
#include <stdlib.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

/*
   SipHash reference C implementation

   Copyright (c) 2012-2016 Jean-Philippe Aumasson
   <jeanphilippe.aumasson@gmail.com>
   Copyright (c) 2012-2014 Daniel J. Bernstein <djb@cr.yp.to>

   To the extent possible under law, the author(s) have dedicated all copyright
   and related and neighboring rights to this software to the public domain
   worldwide. This software is distributed without any warranty.

   You should have received a copy of the CC0 Public Domain Dedication along
   with
   this software. If not, see
   <http://creativecommons.org/publicdomain/zero/1.0/>.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* default: SipHash-2-4 */
#define cROUNDS 2
#define dROUNDS 4

#define ROTL(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))

#define U32TO8_LE(p, v)            \
    (p)[0] = (uint8_t)((v));       \
    (p)[1] = (uint8_t)((v) >> 8);  \
    (p)[2] = (uint8_t)((v) >> 16); \
    (p)[3] = (uint8_t)((v) >> 24);

#define U64TO8_LE(p, v)              \
    U32TO8_LE((p), (uint32_t)((v))); \
    U32TO8_LE((p) + 4, (uint32_t)((v) >> 32));

#define U8TO64_LE(p)                                                                        \
    (((uint64_t)((p)[0])) | ((uint64_t)((p)[1]) << 8) | ((uint64_t)((p)[2]) << 16) |        \
     ((uint64_t)((p)[3]) << 24) | ((uint64_t)((p)[4]) << 32) | ((uint64_t)((p)[5]) << 40) | \
     ((uint64_t)((p)[6]) << 48) | ((uint64_t)((p)[7]) << 56))

#define SIPROUND           \
    do {                   \
        v0 += v1;          \
        v1 = ROTL(v1, 13); \
        v1 ^= v0;          \
        v0 = ROTL(v0, 32); \
        v2 += v3;          \
        v3 = ROTL(v3, 16); \
        v3 ^= v2;          \
        v0 += v3;          \
        v3 = ROTL(v3, 21); \
        v3 ^= v0;          \
        v2 += v1;          \
        v1 = ROTL(v1, 17); \
        v1 ^= v2;          \
        v2 = ROTL(v2, 32); \
    } while (0)

#ifdef DEBUG
#define TRACE                                                                           \
    do {                                                                                \
        printf("(%3d) v0 %08x %08x\n", (int)inlen, (uint32_t)(v0 >> 32), (uint32_t)v0); \
        printf("(%3d) v1 %08x %08x\n", (int)inlen, (uint32_t)(v1 >> 32), (uint32_t)v1); \
        printf("(%3d) v2 %08x %08x\n", (int)inlen, (uint32_t)(v2 >> 32), (uint32_t)v2); \
        printf("(%3d) v3 %08x %08x\n", (int)inlen, (uint32_t)(v3 >> 32), (uint32_t)v3); \
    } while (0)
#else
#define TRACE
#endif

static int siphash(const uint8_t* in,
                   const size_t inlen,
                   const uint8_t* k,
                   uint8_t* out,
                   const size_t outlen) {
    assert((outlen == 8) || (outlen == 16));
    uint64_t v0 = 0x736f6d6570736575ULL;
    uint64_t v1 = 0x646f72616e646f6dULL;
    uint64_t v2 = 0x6c7967656e657261ULL;
    uint64_t v3 = 0x7465646279746573ULL;
    uint64_t k0 = U8TO64_LE(k);
    uint64_t k1 = U8TO64_LE(k + 8);
    uint64_t m;
    int i;
    const uint8_t* end = in + inlen - (inlen % sizeof(uint64_t));
    const int left = inlen & 7;
    uint64_t b = ((uint64_t)inlen) << 56;
    v3 ^= k1;
    v2 ^= k0;
    v1 ^= k1;
    v0 ^= k0;

    if (outlen == 16)
        v1 ^= 0xee;

    for (; in != end; in += 8) {
        m = U8TO64_LE(in);
        v3 ^= m;

        TRACE;
        for (i = 0; i < cROUNDS; ++i)
            SIPROUND;

        v0 ^= m;
    }

    switch (left) {
        case 7:
            b |= ((uint64_t)in[6]) << 48;
        case 6:
            b |= ((uint64_t)in[5]) << 40;
        case 5:
            b |= ((uint64_t)in[4]) << 32;
        case 4:
            b |= ((uint64_t)in[3]) << 24;
        case 3:
            b |= ((uint64_t)in[2]) << 16;
        case 2:
            b |= ((uint64_t)in[1]) << 8;
        case 1:
            b |= ((uint64_t)in[0]);
            break;
        case 0:
            break;
    }

    v3 ^= b;

    TRACE;
    for (i = 0; i < cROUNDS; ++i)
        SIPROUND;

    v0 ^= b;

    if (outlen == 16)
        v2 ^= 0xee;
    else
        v2 ^= 0xff;

    TRACE;
    for (i = 0; i < dROUNDS; ++i)
        SIPROUND;

    b = v0 ^ v1 ^ v2 ^ v3;
    U64TO8_LE(out, b);

    if (outlen == 8)
        return 0;

    v1 ^= 0xdd;

    TRACE;
    for (i = 0; i < dROUNDS; ++i)
        SIPROUND;

    b = v0 ^ v1 ^ v2 ^ v3;
    U64TO8_LE(out + 8, b);

    return 0;
}

static inline sqlite3_uint64 compute_bits(sqlite3_uint64 n, double p) {
    return ceil(-((n * log(p)) / (log(2.0) * log(2.0))));
}

static inline int compute_k(double p) {
    return round(-log2(p));
}

static inline int compute_bytes(sqlite3_uint64 bits) {
    int quo = bits / 8;
    int rem = bits % 8;
    if (rem) {
        quo += 1;
    }
    return quo;
}

static inline int bf_size(int nElements, double p) {
    return compute_bytes(compute_bits(nElements, p));
}

static int bf_create(sqlite3*, void*, int, const char* const*, sqlite3_vtab**, char**);
static int bf_connect(sqlite3*, void*, int, const char* const*, sqlite3_vtab**, char**);
static int bf_bestindex(sqlite3_vtab*, sqlite3_index_info*);
static int bf_disconnect(sqlite3_vtab*);
static int bf_destroy(sqlite3_vtab*);
static int bf_open(sqlite3_vtab*, sqlite3_vtab_cursor**);
static int bf_close(sqlite3_vtab_cursor*);
static int bf_filter(sqlite3_vtab_cursor*, int, const char*, int argc, sqlite3_value** argv);
static int bf_next(sqlite3_vtab_cursor*);
static int bf_eof(sqlite3_vtab_cursor*);
static int bf_column(sqlite3_vtab_cursor*, sqlite3_context*, int);
static int bf_rowid(sqlite3_vtab_cursor*, sqlite3_int64*);
static int bf_update(sqlite3_vtab*, int, sqlite3_value**, sqlite3_int64*);
static int bf_rename(sqlite3_vtab*, const char*);

struct sqlite3_module bf_module = {
    1,         bf_create, bf_connect, bf_bestindex, bf_disconnect, bf_destroy, bf_open, bf_close,
    bf_filter, bf_next,   bf_eof,     bf_column,    bf_rowid,      bf_update,  NULL,    NULL,
    NULL,      NULL,      NULL,       bf_rename,    NULL,          NULL,       NULL,
#if SQLITE_VERSION_NUMBER >= 3026000
    NULL,  // xShadowName
#endif
};

struct bf_vtab {
    const sqlite3_module* pModule;
    int nRef;
    char* zErrMsg;
    char* zDBName;
    char* zName;
    char* zStorage;
    int nFilter;
    double p;
    int k;
    sqlite3* db;
};

// CREATE VIRTUAL TABLE foo USING bloom_filter(nElemens, falseProb, kHashes)
static int bf_create(sqlite3* db,
                     void* pAux __attribute__((unused)),
                     int argc,
                     char const* const* argv,
                     sqlite3_vtab** ppVTab,
                     char** pzErr) {
    struct bf_vtab* vtab = sqlite3_malloc(sizeof *vtab);
    if (!vtab) {
        return SQLITE_NOMEM;
    }

    vtab->db = db;

    int nElems = 100;
    if (argc >= 4) {
        nElems = strtoull(argv[3], NULL, 0);
        if (nElems <= 0) {
            *pzErr = sqlite3_mprintf("Number of elements in filter must be positive");
            sqlite3_free(vtab);
            return SQLITE_ERROR;
        }
    }
    if (argc >= 5) {
        vtab->p = strtod(argv[4], NULL);
        if (vtab->p >= 1.0 || vtab->p <= 0.0) {
            *pzErr = sqlite3_mprintf("Probability must be in the range (0,1)");
            sqlite3_free(vtab);
            return SQLITE_ERROR;
        }
    } else {
        vtab->p = 0.01;
    }
    vtab->nFilter = bf_size(nElems, vtab->p);
    if (argc == 6) {
        vtab->k = strtol(argv[5], NULL, 0);
        if (vtab->k <= 0) {
            *pzErr = sqlite3_mprintf("Number of hash functions must be positive.");
            sqlite3_free(vtab);
            return SQLITE_ERROR;
        }
    } else {
        vtab->k = compute_k(vtab->p);
    }

    char* storage = sqlite3_mprintf(
        "CREATE TABLE \"%s\".\"%s_storage\"(data "
        "BLOB, p REAL, n INTEGER, m INTEGER, k INTEGER)",
        argv[1], argv[2]);
    int rc = sqlite3_exec(db, storage, NULL, NULL, pzErr);
    sqlite3_free(storage);
    if (rc != SQLITE_OK) {
        sqlite3_free(vtab);
        return rc;
    }

    storage = sqlite3_mprintf(
        "INSERT INTO \"%s\".\"%s_storage\"(rowid, data, p, n, m, "
        "k) VALUES (1, ?, %f, %d, %llu, %d)",
        argv[1], argv[2], vtab->p, nElems, (sqlite3_uint64)vtab->nFilter * 8, vtab->k);

    sqlite3_stmt* inserter;
    rc = sqlite3_prepare_v2(db, storage, -1, &inserter, NULL);
    sqlite3_free(storage);
    if (rc != SQLITE_OK) {
        sqlite3_free(vtab);
        return rc;
    }

    sqlite3_bind_zeroblob(inserter, 1, vtab->nFilter);
    rc = sqlite3_step(inserter);
    sqlite3_finalize(inserter);
    if (rc != SQLITE_DONE) {
        sqlite3_free(vtab);
        return rc;
    }

    rc = sqlite3_declare_vtab(db,
                              "CREATE TABLE x(present, word HIDDEN "
                              "NOT NULL PRIMARY KEY) WITHOUT ROWID");
    if (rc != SQLITE_OK) {
        storage = sqlite3_mprintf("DROP TABLE \"%s\".\"%s_storage\"", argv[1], argv[2]);
        sqlite3_exec(db, storage, NULL, NULL, NULL);
        sqlite3_free(storage);
        sqlite3_free(vtab);
        return rc;
    }

    vtab->zDBName = sqlite3_mprintf("%s", argv[1]);
    if (!vtab->zDBName) {
        sqlite3_free(vtab);
        return SQLITE_NOMEM;
    }
    vtab->zName = sqlite3_mprintf("%s", argv[2]);
    if (!vtab->zName) {
        sqlite3_free(vtab->zDBName);
        sqlite3_free(vtab);
        return SQLITE_NOMEM;
    }
    vtab->zStorage = sqlite3_mprintf("%s_storage", argv[2]);
    if (!vtab->zName) {
        sqlite3_free(vtab->zName);
        sqlite3_free(vtab->zDBName);
        sqlite3_free(vtab);
        return SQLITE_NOMEM;
    }

    *ppVTab = (struct sqlite3_vtab*)vtab;
    return SQLITE_OK;
}

static void free_bf_vtab(struct bf_vtab* vtab) {
    sqlite3_free(vtab->zErrMsg);
    sqlite3_free(vtab->zDBName);
    sqlite3_free(vtab->zName);
    sqlite3_free(vtab->zStorage);
    sqlite3_free(vtab);
}

static int bf_connect(sqlite3* db,
                      void* pAux __attribute__((unused)),
                      int argc __attribute__((unused)),
                      const char* const* argv,
                      sqlite3_vtab** ppVTab,
                      char** pzErr __attribute__((unused))) {
    struct bf_vtab* vtab = sqlite3_malloc(sizeof *vtab);
    if (!vtab) {
        return SQLITE_NOMEM;
    }
    vtab->db = db;

    int rc = sqlite3_declare_vtab(db,
                                  "CREATE TABLE x(present, word HIDDEN  "
                                  "NOT NULL PRIMARY KEY) WITHOUT ROWID");
    if (rc != SQLITE_OK) {
        sqlite3_free(vtab);
        return rc;
    }

    char* load_query = sqlite3_mprintf(
        "SELECT m/8, p, k FROM \"%s\".\"%s_storage\" WHERE rowid = 1", argv[1], argv[2]);
    if (!load_query) {
        sqlite3_free(vtab);
        return SQLITE_NOMEM;
    }

    sqlite3_stmt* loader;
    rc = sqlite3_prepare_v2(db, load_query, -1, &loader, NULL);
    sqlite3_free(load_query);
    if (rc != SQLITE_OK) {
        sqlite3_free(vtab);
        return rc;
    }

    rc = sqlite3_step(loader);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(loader);
        sqlite3_free(vtab);
        if (rc == SQLITE_DONE) {
            rc = SQLITE_CORRUPT_VTAB;
        }
        return rc;
    }

    vtab->nFilter = sqlite3_column_int(loader, 0);
    vtab->p = sqlite3_column_double(loader, 1);
    vtab->k = sqlite3_column_int(loader, 2);
    sqlite3_finalize(loader);

    vtab->zDBName = sqlite3_mprintf("%s", argv[1]);
    if (!vtab->zDBName) {
        sqlite3_free(vtab);
        return SQLITE_NOMEM;
    }
    vtab->zName = sqlite3_mprintf("%s", argv[2]);
    if (!vtab->zName) {
        sqlite3_free(vtab->zDBName);
        sqlite3_free(vtab);
        return SQLITE_NOMEM;
    }
    vtab->zStorage = sqlite3_mprintf("%s_storage", argv[2]);
    if (!vtab->zStorage) {
        sqlite3_free(vtab->zName);
        sqlite3_free(vtab->zDBName);
        sqlite3_free(vtab);
        return SQLITE_NOMEM;
    }

    *ppVTab = (struct sqlite3_vtab*)vtab;
    return SQLITE_OK;
}

static int bf_bestindex(struct sqlite3_vtab* pVTab, sqlite3_index_info* idx) {
    struct bf_vtab* vtab = (struct bf_vtab*)pVTab;

    for (int n = 0; n < idx->nConstraint; n += 1) {
        if (!idx->aConstraint[n].usable) {
            continue;
        }
        if (idx->aConstraint[n].iColumn == 1 &&
            idx->aConstraint[n].op == SQLITE_INDEX_CONSTRAINT_EQ) {
            idx->aConstraintUsage[n].argvIndex = 1;
        }
    }
    idx->orderByConsumed = 1;
    if (sqlite3_libversion_number() >= 3008002) {
        idx->estimatedRows = 1;
    }
    if (sqlite3_libversion_number() >= 3009000) {
        idx->idxFlags = SQLITE_INDEX_SCAN_UNIQUE;
    }
    idx->estimatedCost = vtab->k;
    return SQLITE_OK;
}

static int bf_disconnect(struct sqlite3_vtab* pVTab) {
    free_bf_vtab((struct bf_vtab*)pVTab);
    return SQLITE_OK;
}

static int bf_destroy(struct sqlite3_vtab* pVTab) {
    struct bf_vtab* vtab = (struct bf_vtab*)pVTab;

    char* deleter = sqlite3_mprintf("DROP TABLE \"%s\".\"%s\"", vtab->zDBName, vtab->zStorage);
    int rc = sqlite3_exec(vtab->db, deleter, NULL, NULL, NULL);
    sqlite3_free(deleter);
    if (rc != SQLITE_OK) {
        return rc;
    }

    free_bf_vtab(vtab);
    return SQLITE_OK;
}

struct bf_cursor {
    struct bf_vtab* vtab;
    sqlite3_value* orig;
    _Bool found;
};

static int bf_open(sqlite3_vtab* pVTab __attribute__((unused)), sqlite3_vtab_cursor** ppCursor) {
    struct bf_cursor* c = sqlite3_malloc(sizeof *c);
    if (!c) {
        return SQLITE_NOMEM;
    }
    c->orig = NULL;
    c->found = 0;
    *ppCursor = (struct sqlite3_vtab_cursor*)c;
    return SQLITE_OK;
}

static int bf_close(sqlite3_vtab_cursor* pCursor) {
    struct bf_cursor* c = (struct bf_cursor*)pCursor;
    sqlite3_value_free(c->orig);
    sqlite3_free(c);
    return SQLITE_OK;
}

static int bf_eof(sqlite3_vtab_cursor* pCursor) {
    struct bf_cursor* c = (struct bf_cursor*)pCursor;
    return c->found == 0;
}

static inline uint64_t calc_hash(const uint8_t* data, size_t len, int k) {
    uint64_t seed[2] = {~((uint64_t)k), k};
    uint64_t hash;
    siphash(data, len, (const uint8_t*)seed, (uint8_t*)&hash, sizeof hash);
    return hash;
}

static int bf_filter(sqlite3_vtab_cursor* pCursor,
                     int idxNum __attribute__((unused)),
                     const char* idxStr __attribute__((unused)),
                     int argc,
                     sqlite3_value** argv) {
    struct bf_cursor* c = (struct bf_cursor*)pCursor;
    int rc;

    if (argc != 1) {
        return SQLITE_OK;
    }

    c->orig = sqlite3_value_dup(argv[0]);
    const unsigned char* blob = sqlite3_value_blob(argv[0]);
    int len = sqlite3_value_bytes(argv[0]);

    do {
        sqlite3_blob* b;
        rc = sqlite3_blob_open(c->vtab->db, c->vtab->zDBName, c->vtab->zStorage, "data", 1, 0, &b);
        if (rc != SQLITE_OK) {
            return rc;
        }

        for (int n = 0; n < c->vtab->k; n += 1) {
            uint64_t hash = calc_hash(blob, len, n);
            hash %= (uint64_t)c->vtab->nFilter * 8;
            int bytepos = hash / 8;
            int bitpos = hash % 8;
            unsigned char byte;
            rc = sqlite3_blob_read(b, &byte, 1, bytepos);
            if (rc == SQLITE_OK) {
                c->found = byte & (1 << bitpos);
                if (!c->found) {
                    sqlite3_blob_close(b);
                    return SQLITE_OK;
                }
            } else if (rc == SQLITE_ABORT) {
                break;
            } else {
                sqlite3_blob_close(b);
                return rc;
            }
        }
        sqlite3_blob_close(b);
    } while (rc == SQLITE_ABORT);
    return rc;
}

static int bf_next(sqlite3_vtab_cursor* pCursor) {
    struct bf_cursor* c = (struct bf_cursor*)pCursor;
    c->found = 0;
    return SQLITE_OK;
}

static int bf_column(sqlite3_vtab_cursor* pCursor, sqlite3_context* ctx, int n) {
    struct bf_cursor* c = (struct bf_cursor*)pCursor;
    assert(n == 0 || n == 1);

    if (n == 0) {
        sqlite3_result_int(ctx, c->found);
    } else if (n == 1) {
        sqlite3_result_value(ctx, c->orig);
    }
    return SQLITE_OK;
}

static int bf_rowid(sqlite3_vtab_cursor* pCursor __attribute__((unused)),
                    sqlite_int64* pRowid __attribute__((unused))) {
    return SQLITE_OK;
}

static int bf_update(sqlite3_vtab* pVTab,
                     int argc,
                     sqlite3_value** argv,
                     sqlite_int64* pRowid __attribute__((unused))) {
    struct bf_vtab* vtab = (struct bf_vtab*)pVTab;

    if (sqlite3_value_type(argv[0]) != SQLITE_NULL) {
        if (vtab->zErrMsg) {
            sqlite3_free(vtab->zErrMsg);
        }
        if (argc == 1) {
            vtab->zErrMsg = sqlite3_mprintf("bloom_filter elements cannot be deleted.");
        } else {
            vtab->zErrMsg = sqlite3_mprintf("bloom_filter elements cannot be updated.");
        }
        return SQLITE_ERROR;
    }

    assert(argc == 4);

    const uint8_t* blob = sqlite3_value_blob(argv[2]);
    size_t len = sqlite3_value_bytes(argv[2]);

    int rc;
    do {
        sqlite3_blob* b;
        rc = sqlite3_blob_open(vtab->db, vtab->zDBName, vtab->zStorage, "data", 1, 1, &b);
        if (rc != SQLITE_OK) {
            return rc;
        }
        for (int n = 0; n < vtab->k; n += 1) {
            uint64_t hash = calc_hash(blob, len, n);
            hash %= (uint64_t)vtab->nFilter * 8;
            int bytepos = hash / 8;
            int bitpos = hash % 8;
            unsigned char byte;
            rc = sqlite3_blob_read(b, &byte, 1, bytepos);
            if (rc == SQLITE_OK) {
                byte |= 1 << bitpos;
                rc = sqlite3_blob_write(b, &byte, 1, bytepos);
                if (rc == SQLITE_ABORT) {
                    break;
                } else if (rc != SQLITE_OK) {
                    sqlite3_blob_close(b);
                    return rc;
                }
            } else if (rc == SQLITE_ABORT) {
                break;
            } else {
                sqlite3_blob_close(b);
                return rc;
            }
        }
        sqlite3_blob_close(b);
    } while (rc == SQLITE_ABORT);

    return rc;
}

static int bf_rename(sqlite3_vtab* pVTab, const char* zNew) {
    struct bf_vtab* vtab = (struct bf_vtab*)pVTab;

    char* renamer = sqlite3_mprintf("ALTER TABLE \"%s\".\"%s\" RENAME TO \"%s_storage\"",
                                    vtab->zDBName, vtab->zStorage, zNew);
    if (!renamer) {
        return SQLITE_NOMEM;
    }
    if (vtab->zErrMsg) {
        sqlite3_free(vtab->zErrMsg);
        vtab->zErrMsg = NULL;
    }
    int rc = sqlite3_exec(vtab->db, renamer, NULL, NULL, &(vtab->zErrMsg));
    sqlite3_free(renamer);
    if (rc == SQLITE_OK) {
        sqlite3_free(vtab->zName);
        vtab->zName = sqlite3_mprintf("%s", zNew);
        if (!vtab->zName) {
            return SQLITE_NOMEM;
        }
        sqlite3_free(vtab->zStorage);
        vtab->zStorage = sqlite3_mprintf("%s_storage", zNew);
        if (!vtab->zStorage) {
            return SQLITE_NOMEM;
        }
    }
    return rc;
}

#ifdef _WIN32
_declspec(dllexport)
#endif
    int sqlite3_bloom_init(sqlite3* db, char** pzErrMsg, const sqlite3_api_routines* pApi) {
    SQLITE_EXTENSION_INIT2(pApi);
    return sqlite3_create_module(db, "bloom_filter", &bf_module, NULL);
}
