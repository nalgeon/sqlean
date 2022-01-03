// Copyright (c) 2022 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Generic array list implementation.
// Serializes to/from byte array for persistence.

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"

// Do not use growth / shrink coefficients, because they are useless
// when the whole array is serialized after each operation.
static const double GROWTH_FACTOR = 1.0;
static const double SHRINK_FACTOR = 0.0;

// Returns the default element size for a given type.
size_t infer_size(VALUE_TYPE type) {
    switch (type) {
        case TYPE_INT:
            return sizeof(int64_t);
        case TYPE_REAL:
            return sizeof(double);
        case TYPE_TEXT:
            return sizeof(uint8_t);
        default:
            return sizeof(uint8_t);
    }
}

// Right-pads the string with zeros to match the specified width.
static uint8_t* fix_width(const uint8_t* str, size_t width) {
    uint8_t* fixed = malloc(width + 1);
    if (fixed == NULL) {
        return NULL;
    }
    size_t idx = 0;
    uint8_t chr;
    for (; chr = str[idx], chr != 0 && idx < width; idx++) {
        fixed[idx] = chr;
    }
    for (size_t i = idx; i <= width; i++) {
        fixed[idx] = 0;
    }
    return fixed;
}

static Array* array_create(VALUE_TYPE type, size_t el_size) {
    Array* arr = malloc(sizeof(Array));
    if (arr == NULL) {
        return NULL;
    }
    arr->type = type;
    arr->el_size = el_size;
    arr->length = 0;
    arr->capacity = 0;
    arr->data = NULL;
    return arr;
}

static Array* array_create_cap(VALUE_TYPE type, size_t el_size, size_t capacity) {
    assert(capacity > 0);
    Array* arr = malloc(sizeof(Array));
    if (arr == NULL) {
        return NULL;
    }
    arr->type = type;
    arr->el_size = el_size;
    arr->length = 0;
    arr->capacity = capacity;
    arr->data = malloc(capacity * el_size);
    return arr;
}

static Array* array_clone(Array* arr) {
    assert(arr != NULL);

    Array* clone = malloc(sizeof(Array));
    if (clone == NULL) {
        return NULL;
    }

    clone->type = arr->type;
    clone->el_size = arr->el_size;
    clone->length = arr->length;
    clone->capacity = arr->length;

    if (arr->data == NULL) {
        clone->data = NULL;
        return clone;
    }

    clone->data = malloc(clone->length * clone->el_size);
    if (clone->data == NULL) {
        return NULL;
    }
    memcpy(clone->data, arr->data, clone->length * clone->el_size);
    return clone;
}

static void array_print(Array* arr) {
    assert(arr != NULL);

    printf("[ ");
    for (size_t i = 0; i < arr->length * arr->el_size; i++) {
        printf("%02x ", arr->data[i]);
    }
    printf("]\n");
}

static void array_free(Array* arr) {
    assert(arr != NULL);

    if (arr->data != NULL) {
        free(arr->data);
    }
    free(arr);
}

static bool array_grow(Array* arr) {
    assert(arr != NULL);

    size_t new_cap = (size_t)(arr->capacity * GROWTH_FACTOR) + 1;
    uint8_t* data = realloc(arr->data, new_cap * arr->el_size);
    if (data == NULL) {
        return false;
    }
    arr->data = data;
    arr->capacity = new_cap;
    return true;
}

static bool array_shrink(Array* arr) {
    assert(arr != NULL);

    size_t new_cap = (size_t)(arr->capacity * SHRINK_FACTOR) + 1;
    uint8_t* data = realloc(arr->data, new_cap * arr->el_size);
    if (data == NULL) {
        return false;
    }
    arr->data = data;
    arr->capacity = new_cap;
    return true;
}

static size_t array_size(Array* arr) {
    assert(arr != NULL);
    return sizeof(arr->type) + sizeof(arr->el_size) + sizeof(arr->length) +
           arr->length * arr->el_size;
}

static uint8_t* array_at(Array* arr, size_t idx) {
    assert(arr != NULL);
    if (idx < 0 || idx >= arr->length) {
        return NULL;
    };
    uint8_t* at = arr->data + idx * arr->el_size;
    return at;
}

static int array_index(Array* arr, void* el) {
    assert(arr != NULL);
    if (el == NULL) {
        return -1;
    }
    uint8_t* at = arr->data;
    for (size_t i = 0; i < arr->length; i++) {
        if (memcmp(at, el, arr->el_size) == 0) {
            return (int)i;
        }
        at += arr->el_size;
    }
    return -1;
}

static bool array_contains(Array* arr, void* el) {
    return array_index(arr, el) != -1;
}

static bool array_equals(Array* arr, Array* other) {
    assert(arr != NULL && other != NULL);
    if (arr->type != other->type) {
        return false;
    }
    if (arr->el_size != other->el_size) {
        return false;
    }
    if (arr->length != other->length) {
        return false;
    }

    if (memcmp(arr->data, other->data, arr->length * arr->el_size) == 0) {
        return true;
    }

    return false;
}

static STATUS array_append(Array* arr, void* el) {
    assert(arr != NULL);
    if (el == NULL) {
        return STATUS_INVALID_ARG;
    }

    if (arr->length == arr->capacity) {
        bool ok = array_grow(arr);
        if (!ok) {
            return STATUS_NOMEM;
        }
    }

    uint8_t* at = arr->data + arr->length * arr->el_size;
    memcpy(at, el, arr->el_size);
    arr->length += 1;
    return STATUS_OK;
}

static STATUS array_extend(Array* arr, Array* other) {
    assert(arr != NULL && other != NULL);
    if (other->type != arr->type || other->el_size != arr->el_size) {
        return STATUS_INVALID_ARG;
    }

    // grow the array
    size_t new_length = arr->length + other->length;
    uint8_t* data = realloc(arr->data, new_length * arr->el_size);
    if (data == NULL) {
        return STATUS_NOMEM;
    }
    arr->data = data;

    // copy other's data to the array
    uint8_t* at = arr->data + arr->length * arr->el_size;
    memcpy(at, other->data, other->length * other->el_size);

    arr->length = new_length;
    arr->capacity = new_length;
    return STATUS_OK;
}

static STATUS array_insert(Array* arr, size_t idx, void* el) {
    assert(arr != NULL);
    if (el == NULL) {
        return STATUS_INVALID_ARG;
    }
    if (idx < 0 || idx > arr->length) {
        return STATUS_INVALID_ARG;
    }

    if (arr->length == arr->capacity) {
        bool ok = array_grow(arr);
        if (!ok) {
            return STATUS_NOMEM;
        }
    }

    // shift arr[idx:] to the right
    uint8_t* at = arr->data + idx * arr->el_size;
    memmove(at + arr->el_size, at, (arr->length - idx) * arr->el_size);
    // set arr[idx] = el
    memcpy(at, el, arr->el_size);

    arr->length += 1;
    return STATUS_OK;
}

static STATUS array_remove_at(Array* arr, size_t idx) {
    assert(arr != NULL);
    if (idx < 0 || idx >= arr->length) {
        return STATUS_INVALID_ARG;
    }

    // shift arr[idx+1:] to the left
    uint8_t* at = arr->data + (idx + 1) * arr->el_size;
    memmove(at - arr->el_size, at, (arr->length - idx - 1) * arr->el_size);
    arr->length -= 1;

    if (arr->length < arr->capacity * SHRINK_FACTOR) {
        bool ok = array_shrink(arr);
        if (!ok) {
            return STATUS_NOMEM;
        }
    }

    return STATUS_OK;
}

static STATUS array_remove(Array* arr, void* el) {
    assert(arr != NULL);

    int el_idx = array_index(arr, el);
    if (el_idx == -1) {
        return STATUS_OK;
    }
    return array_remove_at(arr, el_idx);
}

static STATUS array_slice(Array* arr, size_t start, size_t end) {
    assert(arr != NULL);
    if (start < 0 || start >= arr->length) {
        return STATUS_INVALID_ARG;
    }
    if (end < 0 || end > arr->length) {
        return STATUS_INVALID_ARG;
    }
    if (start >= end) {
        return STATUS_INVALID_ARG;
    }

    // move slice to the beginning
    uint8_t* at = arr->data + start * arr->el_size;
    size_t length = end - start;
    memmove(arr->data, at, length * arr->el_size);
    arr->length = length;

    if (arr->length < arr->capacity * SHRINK_FACTOR) {
        bool ok = array_shrink(arr);
        if (!ok) {
            return STATUS_NOMEM;
        }
    }

    return STATUS_OK;
}

static STATUS array_clear(Array* arr) {
    assert(arr != NULL);
    free(arr->data);
    arr->length = 0;
    arr->capacity = 0;
    arr->data = NULL;
    return STATUS_OK;
}

static uint8_t* array_to_blob(Array* arr) {
    assert(arr != NULL);
    uint8_t* blob = malloc(array_size(arr));
    if (blob == NULL) {
        return NULL;
    }
    uint8_t* at = blob;
    memcpy(at, &(arr->type), sizeof(arr->type));
    at += sizeof(arr->type);
    memcpy(at, &(arr->el_size), sizeof(arr->el_size));
    at += sizeof(arr->el_size);
    memcpy(at, &(arr->length), sizeof(arr->length));
    at += sizeof(arr->length);
    memcpy(at, arr->data, arr->length * arr->el_size);
    return blob;
}

static Array* array_from_blob(uint8_t* blob) {
    Array* arr = malloc(sizeof(Array));
    if (arr == NULL) {
        return NULL;
    }

    // set type and element size
    uint8_t* at = blob;
    memcpy(&(arr->type), at, sizeof(arr->type));
    at += sizeof(arr->type);
    memcpy(&(arr->el_size), at, sizeof(arr->el_size));
    at += sizeof(arr->el_size);

    // set length and capacity
    memcpy(&(arr->length), at, sizeof(arr->length));
    at += sizeof(arr->length);
    arr->capacity = arr->length;

    // set data
    if (arr->length == 0) {
        arr->data = NULL;
        return arr;
    }
    arr->data = malloc(arr->length * arr->el_size);
    if (arr->data == NULL) {
        free(arr);
        return NULL;
    }
    memcpy(arr->data, at, arr->length * arr->el_size);
    return arr;
}

struct array_ns array = {.append = array_append,
                         .at = array_at,
                         .clear = array_clear,
                         .clone = array_clone,
                         .contains = array_contains,
                         .create = array_create,
                         .create_cap = array_create_cap,
                         .equals = array_equals,
                         .extend = array_extend,
                         .free = array_free,
                         .index = array_index,
                         .insert = array_insert,
                         .print = array_print,
                         .remove = array_remove,
                         .remove_at = array_remove_at,
                         .size = array_size,
                         .slice = array_slice,
                         .to_blob = array_to_blob,
                         .from_blob = array_from_blob,

                         .fix_width = fix_width,
                         .infer_size = infer_size};

int main(void) {
    Array* arr = array_create(TYPE_INT, sizeof(int16_t));
    for (int i = 1; i < 5; i++) {
        array_append(arr, &i);
    }
    array_print(arr);
    printf("array length = %d, capacity = %d\n", arr->length, arr->capacity);
    array_free(arr);
}
