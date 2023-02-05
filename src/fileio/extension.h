// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

#ifndef FILEIO_EXTENSION_H
#define FILEIO_EXTENSION_H

#include "../sqlite3ext.h"

#if defined(FILEIO_WIN32_DLL) && (defined(_WIN32) || defined(WIN32))
#/* To allow a standalone DLL, use this next replacement function: */
#undef sqlite3_win32_utf8_to_unicode
#define sqlite3_win32_utf8_to_unicode utf8_to_utf16
#
LPWSTR utf8_to_utf16(const char* z) {
    int nAllot = MultiByteToWideChar(CP_UTF8, 0, z, -1, NULL, 0);
    LPWSTR rv = sqlite3_malloc(nAllot * sizeof(WCHAR));
    if (rv != 0 && 0 < MultiByteToWideChar(CP_UTF8, 0, z, -1, rv, nAllot))
        return rv;
    sqlite3_free(rv);
    return 0;
}
#endif

int fileioscalar_init(sqlite3* db);
int lsdir_init(sqlite3* db);
int scanfile_init(sqlite3* db);

#endif /* FILEIO_EXTENSION_H */