// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Internal declarations shared by the SQLite binding layer:
// the aggregate functions and the table-valued function.

#ifndef STATS_BINDING_H
#define STATS_BINDING_H

#include "sqlite3ext.h"

// stats_seq_init registers the stats_seq table-valued function.
int stats_seq_init(sqlite3* db);

#endif /* STATS_BINDING_H */
