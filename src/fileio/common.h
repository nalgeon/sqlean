// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Internal helpers shared by the file system functions.

#ifndef FILEIO_COMMON_H
#define FILEIO_COMMON_H

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

// win_errno converts a Windows error code to an errno value.
static inline int win_errno(DWORD err) {
    switch (err) {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
        case ERROR_INVALID_NAME:
            return ENOENT;
        case ERROR_ACCESS_DENIED:
        case ERROR_SHARING_VIOLATION:
            return EACCES;
        case ERROR_ALREADY_EXISTS:
        case ERROR_FILE_EXISTS:
            return EEXIST;
        case ERROR_DIRECTORY:
            return ENOTDIR;
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_OUTOFMEMORY:
            return ENOMEM;
        default:
            return EIO;
    }
}

// win_path converts a UTF-8 path to UTF-16, as the Windows API expects.
// The caller frees the result with free(). Fails with EINVAL if the path
// is not valid UTF-8.
static inline int win_path(const char* path, wchar_t** out) {
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, NULL, 0);
    if (n <= 0) {
        return EINVAL;
    }
    wchar_t* wpath = malloc((size_t)n * sizeof(wchar_t));
    if (wpath == NULL) {
        return ENOMEM;
    }
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wpath, n) <= 0) {
        free(wpath);
        return EINVAL;
    }
    *out = wpath;
    return 0;
}

#endif /* _WIN32 */

// is_sep reports whether c separates path components.
static inline bool is_sep(char c) {
#ifdef _WIN32
    return c == '/' || c == '\\';
#else
    return c == '/';
#endif
}

#endif /* FILEIO_COMMON_H */
