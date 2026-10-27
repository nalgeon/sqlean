// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Based on fileio.c by D. Richard Hipp, Public Domain
// https://www.sqlite.org/src/file/ext/misc/fileio.c

// Creating and listing directories.

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif

#include "fileio/common.h"
#include "fileio/fileio.h"

#pragma region Platform

// stat_dir reports whether the file at path exists and whether it's
// a directory, following symbolic links.
static int stat_dir(const char* path, bool* exists, bool* is_dir) {
#ifdef _WIN32
    wchar_t* wpath;
    int err = win_path(path, &wpath);
    if (err != 0) {
        return err;
    }
    DWORD attrs = GetFileAttributesW(wpath);
    free(wpath);
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        err = win_errno(GetLastError());
        if (err != ENOENT) {
            return err;
        }
        *exists = false;
        *is_dir = false;
        return 0;
    }
    *exists = true;
    *is_dir = (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
    return 0;
#else
    struct stat st;
    if (stat(path, &st) != 0) {
        if (errno != ENOENT) {
            return errno;
        }
        *exists = false;
        *is_dir = false;
        return 0;
    }
    *exists = true;
    *is_dir = S_ISDIR(st.st_mode);
    return 0;
#endif
}

// make_dir creates a directory at path with permissions perm, minus the umask.
static int make_dir(const char* path, uint32_t perm) {
#ifdef _WIN32
    (void)perm;
    wchar_t* wpath;
    int err = win_path(path, &wpath);
    if (err != 0) {
        return err;
    }
    err = _wmkdir(wpath) == 0 ? 0 : errno;
    free(wpath);
    return err;
#else
    return mkdir(path, (mode_t)(perm & 07777)) == 0 ? 0 : errno;
#endif
}

#pragma endregion

#pragma region Creating directories

// fs_mkdir creates a directory at path with permissions perm, minus the
// umask (the bits above 07777 are ignored). Succeeds if the directory exists,
// and sets its permissions to perm if they differ. Doesn't create the parent
// directories. Windows ignores the permissions.
int fs_mkdir(const char* path, uint32_t perm) {
    int err = make_dir(path, perm);
    if (err != EEXIST) {
        return err;
    }
    // Something exists at the path, which is fine if it's a directory.
#ifdef _WIN32
    bool exists, is_dir;
    err = stat_dir(path, &exists, &is_dir);
    if (err != 0) {
        return err;
    }
    return is_dir ? 0 : EEXIST;
#else
    struct stat st;
    if (stat(path, &st) != 0) {
        return errno;
    }
    if (!S_ISDIR(st.st_mode)) {
        return EEXIST;
    }
    if ((st.st_mode & 07777) != (perm & 07777) && chmod(path, (mode_t)(perm & 07777)) != 0) {
        return errno;
    }
    return 0;
#endif
}

// fs_mkdir_parents creates the missing parent directories of path,
// with the default permissions (0777 minus the umask).
int fs_mkdir_parents(const char* path) {
    size_t len = strlen(path);
    char* dir = malloc(len + 1);
    if (dir == NULL) {
        return ENOMEM;
    }
    memcpy(dir, path, len + 1);

    // Check each prefix that ends before a separator, from the top down.
    // A leading separator is the root, which always exists.
    int err = 0;
    for (size_t i = 1; i < len && err == 0; i++) {
        if (!is_sep(dir[i])) {
            continue;
        }
        dir[i] = '\0';
        bool exists, is_dir;
        err = stat_dir(dir, &exists, &is_dir);
        if (err == 0 && !exists) {
            err = make_dir(dir, 0777);
            // Someone else could have created it in the meantime.
            if (err == EEXIST) {
                err = 0;
            }
        } else if (err == 0 && !is_dir) {
            err = ENOTDIR;
        }
        dir[i] = path[i];
    }

    free(dir);
    return err;
}

#pragma endregion

#pragma region Listing directories

#ifdef _WIN32

struct FsDir {
    HANDLE handle;
    // true if data holds an entry not yet returned
    bool pending;
    WIN32_FIND_DATAW data;
    // the UTF-8 entry name: cFileName has MAX_PATH UTF-16 units,
    // each taking at most 3 bytes in UTF-8
    char name[MAX_PATH * 3 + 1];
};

// fs_dir_open opens the directory at path for reading.
// The caller closes it with fs_dir_close.
int fs_dir_open(const char* path, FsDir** out) {
    wchar_t* wpath;
    int err = win_path(path, &wpath);
    if (err != 0) {
        return err;
    }
    // FindFirstFileW takes a pattern, so search for "path\*".
    size_t len = wcslen(wpath);
    wchar_t* pattern = malloc((len + 3) * sizeof(wchar_t));
    if (pattern == NULL) {
        free(wpath);
        return ENOMEM;
    }
    memcpy(pattern, wpath, len * sizeof(wchar_t));
    free(wpath);
    if (len > 0 && pattern[len - 1] != L'/' && pattern[len - 1] != L'\\') {
        pattern[len++] = L'\\';
    }
    pattern[len++] = L'*';
    pattern[len] = L'\0';

    FsDir* dir = malloc(sizeof(*dir));
    if (dir == NULL) {
        free(pattern);
        return ENOMEM;
    }
    dir->handle = FindFirstFileW(pattern, &dir->data);
    free(pattern);
    if (dir->handle == INVALID_HANDLE_VALUE) {
        err = win_errno(GetLastError());
        free(dir);
        return err;
    }
    dir->pending = true;
    *out = dir;
    return 0;
}

// fs_dir_next reads the next entry name of the directory, skipping "."
// and "..". Sets name to NULL after the last entry. The name stays valid
// until the next call.
int fs_dir_next(FsDir* dir, const char** name) {
    for (;;) {
        if (dir->pending) {
            dir->pending = false;
        } else if (!FindNextFileW(dir->handle, &dir->data)) {
            DWORD code = GetLastError();
            if (code == ERROR_NO_MORE_FILES) {
                *name = NULL;
                return 0;
            }
            return win_errno(code);
        }
        const wchar_t* wname = dir->data.cFileName;
        if (wcscmp(wname, L".") == 0 || wcscmp(wname, L"..") == 0) {
            continue;
        }
        int n =
            WideCharToMultiByte(CP_UTF8, 0, wname, -1, dir->name, sizeof(dir->name), NULL, NULL);
        if (n <= 0) {
            return EILSEQ;
        }
        *name = dir->name;
        return 0;
    }
}

// fs_dir_close closes the directory. Does nothing if dir is NULL.
void fs_dir_close(FsDir* dir) {
    if (dir == NULL) {
        return;
    }
    FindClose(dir->handle);
    free(dir);
}

#else

struct FsDir {
    DIR* dir;
};

// fs_dir_open opens the directory at path for reading.
// The caller closes it with fs_dir_close.
int fs_dir_open(const char* path, FsDir** out) {
    FsDir* dir = malloc(sizeof(*dir));
    if (dir == NULL) {
        return ENOMEM;
    }
    dir->dir = opendir(path);
    if (dir->dir == NULL) {
        int err = errno;
        free(dir);
        return err;
    }
    *out = dir;
    return 0;
}

// fs_dir_next reads the next entry name of the directory, skipping "."
// and "..". Sets name to NULL after the last entry. The name stays valid
// until the next call.
int fs_dir_next(FsDir* dir, const char** name) {
    for (;;) {
        // readdir returns NULL both at the end and on error,
        // which only errno tells apart.
        errno = 0;
        struct dirent* entry = readdir(dir->dir);
        if (entry == NULL) {
            if (errno != 0) {
                return errno;
            }
            *name = NULL;
            return 0;
        }
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        *name = entry->d_name;
        return 0;
    }
}

// fs_dir_close closes the directory. Does nothing if dir is NULL.
void fs_dir_close(FsDir* dir) {
    if (dir == NULL) {
        return;
    }
    closedir(dir->dir);
    free(dir);
}

#endif /* _WIN32 */

// append copies src to buf at position pos, as far as it fits into size-1
// bytes, and returns the position after src (even if it didn't fit).
static size_t append(char* buf, size_t size, size_t pos, const char* src, size_t len) {
    if (pos + 1 < size) {
        size_t room = size - 1 - pos;
        memcpy(buf + pos, src, len < room ? len : room);
    }
    return pos + len;
}

// fs_join writes the path of the entry name in the directory dir into buf, as
// "dir/name", without doubling a trailing separator in dir. On Windows, the
// separator is the last one used in dir, so "a\b" and "x" give "a\b\x".
// Has snprintf semantics: writes at most size bytes including the terminating
// NUL, and returns the full length of the path.
size_t fs_join(char* buf, size_t size, const char* dir, const char* name) {
    size_t dir_len = strlen(dir);
    size_t pos = append(buf, size, 0, dir, dir_len);
    if (dir_len == 0 || !is_sep(dir[dir_len - 1])) {
        char sep = '/';
        for (size_t i = dir_len; i > 0; i--) {
            if (is_sep(dir[i - 1])) {
                sep = dir[i - 1];
                break;
            }
        }
        pos = append(buf, size, pos, &sep, 1);
    }
    pos = append(buf, size, pos, name, strlen(name));
    if (size > 0) {
        buf[pos < size ? pos : size - 1] = '\0';
    }
    return pos;
}

#pragma endregion
