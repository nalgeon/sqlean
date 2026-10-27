// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Based on fileio.c by D. Richard Hipp, Public Domain
// https://www.sqlite.org/src/file/ext/misc/fileio.c

// Reading and writing files.

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#ifdef _WIN32
#include <io.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#include "fileio/common.h"
#include "fileio/fileio.h"

#pragma region Platform

#ifdef _WIN32

// WIN_EPOCH is the Unix epoch as a FILETIME: the number of 100-nanosecond
// intervals since 1601-01-01.
#define WIN_EPOCH 116444736000000000LL
#define WIN_TICKS_PER_SEC 10000000LL

// filetime_to_unix converts a FILETIME to seconds since the Unix epoch,
// rounding down.
static int64_t filetime_to_unix(FILETIME ft) {
    uint64_t ticks = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    // A FILETIME above INT64_MAX is invalid, and the conversion would overflow.
    if (ticks > INT64_MAX) {
        ticks = INT64_MAX;
    }
    int64_t since_epoch = (int64_t)ticks - WIN_EPOCH;
    int64_t sec = since_epoch / WIN_TICKS_PER_SEC;
    if (since_epoch % WIN_TICKS_PER_SEC < 0) {
        sec--;
    }
    return sec;
}

// unix_to_filetime converts seconds since the Unix epoch to a FILETIME.
// Returns false if the time is out of the FILETIME range.
static bool unix_to_filetime(int64_t sec, FILETIME* ft) {
    // Check the range before multiplying, so the multiplication can't overflow.
    const int64_t min = -(WIN_EPOCH / WIN_TICKS_PER_SEC);
    const int64_t max = (INT64_MAX - WIN_EPOCH) / WIN_TICKS_PER_SEC;
    if (sec < min || sec > max) {
        return false;
    }
    uint64_t ticks = (uint64_t)(sec * WIN_TICKS_PER_SEC + WIN_EPOCH);
    ft->dwLowDateTime = (DWORD)(ticks & 0xFFFFFFFF);
    ft->dwHighDateTime = (DWORD)(ticks >> 32);
    return true;
}

#endif /* _WIN32 */

// valid_mtime reports whether the system can set the modification time to sec.
static bool valid_mtime(int64_t sec) {
#ifdef _WIN32
    FILETIME ft;
    return unix_to_filetime(sec, &ft);
#else
    if (sizeof(time_t) < sizeof(int64_t)) {
        return sec >= INT32_MIN && sec <= INT32_MAX;
    }
    return true;
#endif
}

// set_mtime sets the modification time of the file at path to sec
// (which must be valid), and its access time to the current time.
static int set_mtime(const char* path, int64_t sec) {
#ifdef _WIN32
    FILETIME mtime;
    unix_to_filetime(sec, &mtime);
    FILETIME atime;
    GetSystemTimeAsFileTime(&atime);
    wchar_t* wpath;
    int err = win_path(path, &wpath);
    if (err != 0) {
        return err;
    }
    HANDLE file = CreateFileW(wpath, FILE_WRITE_ATTRIBUTES, 0, NULL, OPEN_EXISTING,
                              FILE_FLAG_BACKUP_SEMANTICS, NULL);
    free(wpath);
    if (file == INVALID_HANDLE_VALUE) {
        return win_errno(GetLastError());
    }
    BOOL ok = SetFileTime(file, NULL, &atime, &mtime);
    err = ok ? 0 : win_errno(GetLastError());
    CloseHandle(file);
    return err;
#else
    // utimensat would be more precise, but it's not available on older systems.
    struct timeval times[2];
    times[0].tv_sec = time(NULL);
    times[0].tv_usec = 0;
    times[1].tv_sec = (time_t)sec;
    times[1].tv_usec = 0;
    return utimes(path, times) == 0 ? 0 : errno;
#endif
}

// set_perm sets the permissions of the file at path. Windows only supports
// the read-only attribute, which is set when the owner can't write.
static int set_perm(const char* path, uint32_t perm) {
#ifdef _WIN32
    wchar_t* wpath;
    int err = win_path(path, &wpath);
    if (err != 0) {
        return err;
    }
    int mode = (perm & 0200) ? (_S_IREAD | _S_IWRITE) : _S_IREAD;
    err = _wchmod(wpath, mode) == 0 ? 0 : errno;
    free(wpath);
    return err;
#else
    return chmod(path, (mode_t)(perm & 07777)) == 0 ? 0 : errno;
#endif
}

// open_path opens the file at path with fopen's mode ("rb", "wb" or "ab").
static int open_path(const char* path, const char* mode, FILE** out) {
    FILE* file;
#ifdef _WIN32
    wchar_t* wpath;
    int err = win_path(path, &wpath);
    if (err != 0) {
        return err;
    }
    wchar_t wmode[4] = {0};
    for (size_t i = 0; i < 3 && mode[i] != '\0'; i++) {
        wmode[i] = (wchar_t)mode[i];
    }
    file = _wfopen(wpath, wmode);
    err = errno;
    free(wpath);
    if (file == NULL) {
        return err != 0 ? err : EIO;
    }
#else
    file = fopen(path, mode);
    if (file == NULL) {
        return errno != 0 ? errno : EIO;
    }
#endif
    *out = file;
    return 0;
}

// open_create opens the file at path for writing with fopen's mode ("wb" or "ab"),
// creating its parent directories if they don't exist.
static int open_create(const char* path, const char* mode, FILE** out) {
    int err = open_path(path, mode, out);
    if (err == ENOENT) {
        err = fs_mkdir_parents(path);
        if (err == 0) {
            err = open_path(path, mode, out);
        }
    }
    return err;
}

#pragma endregion

#pragma region File information

// fs_stat gets information about the file at path. It doesn't follow
// a symbolic link, but describes the link itself.
// On Windows, the mode is approximated from the file attributes:
// 040777 for a directory, and 0100666 (or 0100444 if read-only) for a file.
int fs_stat(const char* path, FileInfo* info) {
#ifdef _WIN32
    wchar_t* wpath;
    int err = win_path(path, &wpath);
    if (err != 0) {
        return err;
    }
    WIN32_FILE_ATTRIBUTE_DATA data;
    BOOL ok = GetFileAttributesExW(wpath, GetFileExInfoStandard, &data);
    err = ok ? 0 : win_errno(GetLastError());
    free(wpath);
    if (err != 0) {
        return err;
    }
    if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        info->mode = 040777;
    } else if (data.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
        info->mode = 0100444;
    } else {
        info->mode = 0100666;
    }
    info->size = (int64_t)(((uint64_t)data.nFileSizeHigh << 32) | data.nFileSizeLow);
    info->mtime = filetime_to_unix(data.ftLastWriteTime);
    return 0;
#else
    struct stat st;
    if (lstat(path, &st) != 0) {
        return errno;
    }
    info->mode = (uint32_t)st.st_mode;
    info->size = (int64_t)st.st_size;
    info->mtime = (int64_t)st.st_mtime;
    return 0;
#endif
}

#pragma endregion

#pragma region Reading

// fs_open opens the file at path for reading. A directory can't be opened
// (EISDIR). The caller closes the file with fclose.
int fs_open(const char* path, FILE** file) {
    FILE* f;
    int err = open_path(path, "rb", &f);
    if (err != 0) {
        return err;
    }
    // Some systems open a directory as a file, which then fails on read.
#ifdef _WIN32
    struct _stat64 st;
    if (_fstat64(_fileno(f), &st) == 0 && (st.st_mode & _S_IFMT) == _S_IFDIR) {
#else
    struct stat st;
    if (fstat(fileno(f), &st) == 0 && S_ISDIR(st.st_mode)) {
#endif
        fclose(f);
        return EISDIR;
    }
    *file = f;
    return 0;
}

// fs_size gets the size of an open regular file. Fails with EINVAL if the
// file is not a regular file, so its size is meaningless.
int fs_size(FILE* file, int64_t* size) {
#ifdef _WIN32
    struct _stat64 st;
    if (_fstat64(_fileno(file), &st) != 0) {
        return errno;
    }
    if ((st.st_mode & _S_IFMT) != _S_IFREG) {
        return EINVAL;
    }
#else
    struct stat st;
    if (fstat(fileno(file), &st) != 0) {
        return errno;
    }
    if (!S_ISREG(st.st_mode)) {
        return EINVAL;
    }
#endif
    *size = (int64_t)st.st_size;
    return 0;
}

// fs_read reads up to n bytes starting at offset into buf, and sets nread to
// the number of bytes read. It reads less than n bytes only at the end of the
// file. Fails with EINVAL if the offset is negative or not supported.
int fs_read(FILE* file, int64_t offset, void* buf, size_t n, size_t* nread) {
    if (offset < 0) {
        return EINVAL;
    }
#ifdef _WIN32
    if (_fseeki64(file, offset, SEEK_SET) != 0) {
        return EINVAL;
    }
#else
    // off_t is 32-bit on some 32-bit systems, so a larger offset
    // can't be converted to it.
    if (sizeof(off_t) < sizeof(int64_t) && offset > INT32_MAX) {
        return EINVAL;
    }
    if (fseeko(file, (off_t)offset, SEEK_SET) != 0) {
        return EINVAL;
    }
#endif
    // fread with a NULL buffer is UB, even when n is 0.
    size_t got = n > 0 ? fread(buf, 1, n, file) : 0;
    if (got < n && ferror(file)) {
        return EIO;
    }
    *nread = got;
    return 0;
}

// fs_read_line reads bytes into buf until it reads a newline (which is kept),
// fills the buffer or reaches the end of the file. Sets n to the number of
// bytes read, and done to true if the line is complete (at a newline or the
// end of the file). An empty complete line (n == 0) means the end of the file.
// Reads nothing unless size > 0.
int fs_read_line(FILE* file, char* buf, size_t size, size_t* n, bool* done) {
    size_t i = 0;
    *done = false;
    while (i < size) {
        int c = getc(file);
        if (c == EOF) {
            if (ferror(file)) {
                return EIO;
            }
            *done = true;
            break;
        }
        buf[i++] = (char)c;
        if (c == '\n') {
            *done = true;
            break;
        }
    }
    *n = i;
    return 0;
}

// fs_trim_eol returns the length of the line without its end-of-line marker:
// a trailing \n, \r\n, or \r (as Go's bufio.ScanLines does).
size_t fs_trim_eol(const char* line, size_t len) {
    if (len > 0 && line[len - 1] == '\n') {
        len--;
    }
    if (len > 0 && line[len - 1] == '\r') {
        len--;
    }
    return len;
}

#pragma endregion

#pragma region Writing

// fs_write writes n bytes from data to the file at path, replacing its contents
// and creating the file and its parent directories if needed. Sets the
// permissions to perm (the bits above 07777 are ignored), and the modification
// time to *mtime, unless mtime is NULL. Fails with EOVERFLOW before writing
// anything if the system can't represent mtime.
int fs_write(const char* path, const void* data, size_t n, uint32_t perm, const int64_t* mtime) {
    if (mtime != NULL && !valid_mtime(*mtime)) {
        return EOVERFLOW;
    }
    FILE* file;
    int err = open_create(path, "wb", &file);
    if (err != 0) {
        return err;
    }
    err = fs_append(file, data, n);
    // fclose flushes the buffer, so it can fail to write too.
    if (fclose(file) != 0 && err == 0) {
        err = errno != 0 ? errno : EIO;
    }
    if (err != 0) {
        return err;
    }
    err = set_perm(path, perm);
    if (err != 0) {
        return err;
    }
    if (mtime != NULL) {
        return set_mtime(path, *mtime);
    }
    return 0;
}

// fs_open_append opens the file at path for appending, creating the file and
// its parent directories if needed. The caller closes the file with fclose.
int fs_open_append(const char* path, FILE** file) {
    return open_create(path, "ab", file);
}

// fs_append writes n bytes from data to an open file.
int fs_append(FILE* file, const void* data, size_t n) {
    // fwrite with a NULL buffer is UB, even when n is 0.
    if (n == 0) {
        return 0;
    }
    if (fwrite(data, 1, n, file) != n) {
        return errno != 0 ? errno : EIO;
    }
    return 0;
}

// fs_symlink creates a symbolic link at path, pointing to target. Fails with
// ENOSYS on Windows.
int fs_symlink(const char* target, const char* path) {
#ifdef _WIN32
    (void)target;
    (void)path;
    return ENOSYS;
#else
    return symlink(target, path) == 0 ? 0 : errno;
#endif
}

#pragma endregion
