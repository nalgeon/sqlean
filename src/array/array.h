// Copyright (c) 2022 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

#ifndef ARRAY_H
#define ARRAY_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef enum { STATUS_OK = 0, STATUS_NOMEM = 1, STATUS_INVALID_ARG = 2 } STATUS;
typedef enum { TYPE_INT = 1, TYPE_REAL = 2, TYPE_TEXT = 3, TYPE_NULL = 5 } VALUE_TYPE;

typedef struct {
    uint8_t type;
    uint32_t el_size;
    uint32_t length;
    uint32_t capacity;
    uint8_t* data;
} Array;

struct array_ns {
    Array* (*create)(VALUE_TYPE type, size_t el_size);
    Array* (*create_cap)(VALUE_TYPE type, size_t el_size, size_t capacity);
    Array* (*clone)(Array* arr);
    void (*free)(Array* arr);
    size_t (*size)(Array* arr);
    uint8_t* (*at)(Array* arr, size_t idx);
    int (*index)(Array* arr, void* el);
    bool (*contains)(Array* arr, void* el);
    bool (*equals)(Array* arr, Array* other);
    STATUS (*append)(Array* arr, void* el);
    STATUS (*extend)(Array* arr, Array* other);
    STATUS (*insert)(Array* arr, size_t idx, void* el);
    STATUS (*remove_at)(Array* arr, size_t idx);
    STATUS (*remove)(Array* arr, void* el);
    STATUS (*slice)(Array* arr, size_t start, size_t end);
    STATUS (*clear)(Array* arr);
    uint8_t* (*to_blob)(Array* arr);
    Array* (*from_blob)(uint8_t* blob);
    void (*print)(Array* arr);

    uint8_t* (*fix_width)(const uint8_t* str, size_t width);
    size_t (*infer_size)(VALUE_TYPE type);
};

extern struct array_ns array;

#endif /* ARRAY_H */
