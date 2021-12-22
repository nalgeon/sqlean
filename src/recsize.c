/*
** This extension estimates the cell size for a record in schema version 4.  The
** result is more or less accurate although the various optimizations actually
** used (for 1, 0, and double as integer) depend on the column declarations
** of the table.  Since we do not have access to this data (or even the schema
** version) this UDF assumes schema type 4 and that the data record storage
** is using the optimal layout.  This means our returned record size may not
** be entirely accurate, but it will be pretty close.
**
** Results validated on SQLite 3.31.1
**
** Copyright (C) 2020 by Keith Medcalf
**
** Released into the Public Domain.  You may do with this code as you wish.
**
** example use:
** create table x(id integer primary key, x real, z text);
** insert ...
** select id, recsize(id, x, z) from x;
** will return the size consumed by each cell (row) no matter what type of
** data is actually stored -- and will work for both utf-8 and utf-16
** database encodings.
*/

#include <math.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

static sqlite_int64 _varIntSize_(sqlite_int64 v) {
    sqlite_int64 uu;

    if (v < 0)
        uu = ~v;
    else
        uu = v;
    if (uu < 128)
        return 1;
    else if (uu < 32768)
        return 2;
    else if (uu < 8388608)
        return 3;
    else if (uu < 2147483648)
        return 4;
    else if (uu < 140737488355328LL)
        return 6;
    else
        return 8;
}

static void _recSizeFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
    sqlite_int64 hdrsize = 0;
    sqlite_int64 datsize = 0;
    sqlite_int64 sz;
    double v;
    int i;

    for (i = 0; i < argc; i++) {
        switch (sqlite3_value_type(argv[i])) {
            case SQLITE_NULL:
                hdrsize += 1;
                break;
            case SQLITE_INTEGER:
                hdrsize += 1;
                sz = sqlite3_value_int64(argv[i]);
                if ((sz < 0) || (sz > 1))
                    datsize += _varIntSize_(sz);
                break;
            case SQLITE_FLOAT:
                hdrsize += 1;
                v = sqlite3_value_double(argv[i]);
                if ((fabs(v) <= 140737488355327.0) && (trunc(v) == v)) {
                    if ((v < 0) || (v > 1))
                        datsize += _varIntSize_((sqlite_int64)v);
                } else
                    datsize += 8;
                break;
            case SQLITE_TEXT:
                sqlite3_value_blob(argv[i]);
                sz = sqlite3_value_bytes(argv[i]);
                hdrsize += _varIntSize_(sz * 2 + 12);
                datsize += sz;
                break;
            case SQLITE_BLOB:
                sz = sqlite3_value_bytes(argv[i]);
                hdrsize += _varIntSize_(sz * 2 + 13);
                datsize += sz;
                break;
        }
    }
    sqlite3_result_int64(context,
                         _varIntSize_(hdrsize + _varIntSize_(hdrsize)) + hdrsize + datsize);
}

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_recsize_init(sqlite3* db, char** pzErrMsg, const sqlite3_api_routines* pApi) {
    SQLITE_EXTENSION_INIT2(pApi);

    return sqlite3_create_function(db, "recsize", -1,
                                   SQLITE_ANY | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS, 0,
                                   _recSizeFunc, 0, 0);
}
