// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// File mode formatting.

#include <stdint.h>

#include "fileio/fileio.h"

// fs_mode_string writes the file mode as text in the style of "ls -l".
// The bits above the file type (0170000) are ignored.
void fs_mode_string(uint32_t mode, char buf[FS_MODE_SIZE]) {
    // The file type values are the same on every system that defines them,
    // so they are spelled out instead of using S_ISDIR and friends, which
    // Windows lacks.
    switch (mode & 0170000) {
        case 0100000:
            buf[0] = '-';
            break;
        case 0040000:
            buf[0] = 'd';
            break;
        case 0120000:
            buf[0] = 'l';
            break;
        case 0010000:
            buf[0] = 'p';
            break;
        case 0020000:
            buf[0] = 'c';
            break;
        case 0060000:
            buf[0] = 'b';
            break;
        case 0140000:
            buf[0] = 's';
            break;
        default:
            buf[0] = '?';
            break;
    }
    // owner, group and others, from the highest bits down
    static const char letters[3] = {'r', 'w', 'x'};
    for (int i = 0; i < 9; i++) {
        uint32_t bit = (uint32_t)1 << (8 - i);
        buf[1 + i] = (mode & bit) ? letters[i % 3] : '-';
    }
    buf[10] = '\0';
}
