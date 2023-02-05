// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

#ifndef FILEIO_EXTENSION_H
#define FILEIO_EXTENSION_H

#include "../sqlite3ext.h"

int fileioscalar_init(sqlite3* db);
int lsdir_init(sqlite3* db);
int scanfile_init(sqlite3* db);

#endif /* FILEIO_EXTENSION_H */