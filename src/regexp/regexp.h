// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

#ifndef REGEXP_H
#define REGEXP_H

struct regexp_ns {
    int (*like)(const char* pattern, const char* source);
    int (*substr)(const char* pattern, const char* source, char** substr);
    int (*replace)(const char* pattern, const char* source, const char* repl, char** dest);
};

extern struct regexp_ns regexp;

#endif /* REGEXP_H */