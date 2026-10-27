// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// File system access: reading, writing and listing files.

#ifndef FILEIO_FILEIO_H
#define FILEIO_FILEIO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// Every function that touches the file system returns 0 on success, or an
// errno value on failure: ENOMEM if out of memory, and whatever the system
// call reported otherwise. Paths are UTF-8 strings.

// File information.

// FileInfo describes a file, as stat does.
typedef struct {
    uint32_t mode;  // file type and permission bits, as in st_mode
    int64_t size;   // size in bytes
    int64_t mtime;  // modification time in seconds since the Unix epoch
} FileInfo;

// fs_stat gets information about the file at path. It doesn't follow
// a symbolic link, but describes the link itself.
int fs_stat(const char* path, FileInfo* info);

// FS_MODE_SIZE is the size of the fs_mode_string buffer:
// 10 characters ("drwxr-xr-x") and the terminating NUL.
#define FS_MODE_SIZE 11

// fs_mode_string writes the file mode as text in the style of "ls -l".
// The bits above the file type (0170000) are ignored.
void fs_mode_string(uint32_t mode, char buf[FS_MODE_SIZE]);

// Reading.

// fs_open opens the file at path for reading. A directory can't be opened
// (EISDIR). The caller closes the file with fclose.
int fs_open(const char* path, FILE** file);

// fs_size gets the size of an open regular file. Fails with EINVAL if the
// file is not a regular file, so its size is meaningless.
int fs_size(FILE* file, int64_t* size);

// fs_read reads up to n bytes starting at offset into buf, and sets nread to
// the number of bytes read. It reads less than n bytes only at the end of the
// file. Fails with EINVAL if the offset is negative or not supported.
int fs_read(FILE* file, int64_t offset, void* buf, size_t n, size_t* nread);

// fs_read_line reads bytes into buf until it reads a newline (which is kept),
// fills the buffer or reaches the end of the file. Sets n to the number of
// bytes read, and done to true if the line is complete (at a newline or the
// end of the file). An empty complete line (n == 0) means the end of the file.
// Reads nothing unless size > 0.
int fs_read_line(FILE* file, char* buf, size_t size, size_t* n, bool* done);

// fs_trim_eol returns the length of the line without its end-of-line marker:
// a trailing \n, \r\n, or \r (as Go's bufio.ScanLines does).
size_t fs_trim_eol(const char* line, size_t len);

// Writing.

// fs_write writes n bytes from data to the file at path, replacing its contents
// and creating the file and its parent directories if needed. Sets the
// permissions to perm (the bits above 07777 are ignored), and the modification
// time to *mtime, unless mtime is NULL. Fails with EOVERFLOW before writing
// anything if the system can't represent mtime.
int fs_write(const char* path, const void* data, size_t n, uint32_t perm, const int64_t* mtime);

// fs_open_append opens the file at path for appending, creating the file and
// its parent directories if needed. The caller closes the file with fclose.
int fs_open_append(const char* path, FILE** file);

// fs_append writes n bytes from data to an open file.
int fs_append(FILE* file, const void* data, size_t n);

// fs_symlink creates a symbolic link at path, pointing to target. Fails with
// ENOSYS on Windows.
int fs_symlink(const char* target, const char* path);

// Directories.

// fs_mkdir creates a directory at path with permissions perm, minus the
// umask (the bits above 07777 are ignored). Succeeds if the directory exists,
// and sets its permissions to perm if they differ. Doesn't create the parent
// directories. Windows ignores the permissions.
int fs_mkdir(const char* path, uint32_t perm);

// fs_mkdir_parents creates the missing parent directories of path,
// with the default permissions (0777 minus the umask).
int fs_mkdir_parents(const char* path);

// FsDir is an open directory, read entry by entry.
typedef struct FsDir FsDir;

// fs_dir_open opens the directory at path for reading.
// The caller closes it with fs_dir_close.
int fs_dir_open(const char* path, FsDir** dir);

// fs_dir_next reads the next entry name of the directory, skipping "."
// and "..". Sets name to NULL after the last entry. The name stays valid
// until the next call.
int fs_dir_next(FsDir* dir, const char** name);

// fs_dir_close closes the directory. Does nothing if dir is NULL.
void fs_dir_close(FsDir* dir);

// fs_join writes the path of the entry name in the directory dir into buf, as
// "dir/name", without doubling a trailing separator in dir. On Windows, the
// separator is the last one used in dir (/ if there is none).
// Has snprintf semantics: writes at most size bytes including the terminating
// NUL, and returns the full length of the path.
size_t fs_join(char* buf, size_t size, const char* dir, const char* name);

#endif /* FILEIO_FILEIO_H */
