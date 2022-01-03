// Copyright (c) 2022 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

#ifndef ARRAY_EXTENSION_H
#define ARRAY_EXTENSION_H

#include "../sqlite3ext.h"

#define ERR_INVALID_ELEMENT_TYPE "array element should be int, real or text"
#define ERR_INVALID_INDEX_TYPE "array index should be integer"

int arrayagg_init(sqlite3* db);
int arrayscalar_init(sqlite3* db);
int unnest_init(sqlite3* db);

#endif /* ARRAY_EXTENSION_H */
