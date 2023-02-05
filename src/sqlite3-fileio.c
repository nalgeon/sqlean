// Originally by D. Richard Hipp, Public Domain
// https://www.sqlite.org/src/file/ext/misc/fileio.c

// Modified by Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean/

/*
 * This SQLite extension implements SQL functions
 * for reading, writing and listing files and folders.
 *
 *
 * Notes on building the extension for Windows:
 * Unless linked statically with the SQLite library, a preprocessor
 * symbol, FILEIO_WIN32_DLL, must be #define'd to create a stand-alone
 * DLL form of this extension for WIN32. See its use below for details.
 */

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

#include "fileio/extension.h"

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_fileio_init(sqlite3* db, char** errmsg_ptr, const sqlite3_api_routines* api) {
    (void)errmsg_ptr;
    SQLITE_EXTENSION_INIT2(api);
    fileioscalar_init(db);
    lsdir_init(db);
    scanfile_init(db);
    return SQLITE_OK;
}
