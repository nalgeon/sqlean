// Originally by D. Richard Hipp, Public Domain
// https://www.sqlite.org/src/file/ext/misc/fileio.c

// Modified by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean/

// Read and write files in SQLite.

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

#include "fileio/extension.h"

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_fileio_init(sqlite3* db, char** errmsg_ptr, const sqlite3_api_routines* api) {
    (void)errmsg_ptr;
    SQLITE_EXTENSION_INIT2(api);
    return fileio_init(db);
}
