// Copyright (c) 2022 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Scalar array functions.

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "extension.h"

#include "../sqlite3ext.h"
SQLITE_EXTENSION_INIT3

// Returns the maximum size among provided strings.
static size_t max_text_size(int argc, sqlite3_value** argv) {
    size_t maxsize = 0;
    for (int i = 0; i < argc; i++) {
        size_t size = sqlite3_value_bytes(argv[i]);
        if (size > maxsize) {
            maxsize = size;
        }
    }
    return maxsize;
}

// Extracts an array from the sqlite value.
static Array* extract_array(sqlite3_value* val) {
    if (sqlite3_value_type(val) != SQLITE_BLOB) {
        return NULL;
    }
    uint8_t* blob = (uint8_t*)sqlite3_value_blob(val);
    if (blob == NULL) {
        return NULL;
    }
    Array* arr = array.from_blob(blob);
    return arr;
}

// intarray()
// Creates an empty 64-bit integer array.
static void intarray(sqlite3_context* context, int argc, sqlite3_value** argv) {
    (void)argv;
    assert(argc == 0);

    size_t el_size = array.infer_size(TYPE_INT);
    Array* arr = array.create(TYPE_INT, el_size);
    if (arr == NULL) {
        sqlite3_result_error_nomem(context);
        return;
    }

    sqlite3_result_blob(context, array.to_blob(arr), array.size(arr), SQLITE_TRANSIENT);
    array.free(arr);
}

// realarray()
// Creates an empty 64-bit real array.
static void realarray(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 0);
    (void)argv;
    size_t el_size = array.infer_size(TYPE_INT);
    Array* arr = array.create(TYPE_REAL, el_size);
    if (arr == NULL) {
        sqlite3_result_error_nomem(context);
        return;
    }
    sqlite3_result_blob(context, array.to_blob(arr), array.size(arr), SQLITE_TRANSIENT);
    array.free(arr);
}

// textarray(width)
// Creates an empty text array. Each string has a maximum size of `width` bytes.
// Shorter strings are fine, but longer ones will be truncated to `width` bytes.
static void textarray(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);
    if (sqlite3_value_type(argv[0]) != SQLITE_INTEGER) {
        return;
    }
    size_t width = sqlite3_value_int(argv[0]);

    // +1 for '\0' char
    Array* arr = array.create(TYPE_TEXT, width + 1);
    if (arr == NULL) {
        sqlite3_result_error_nomem(context);
        return;
    }
    sqlite3_result_blob(context, array.to_blob(arr), array.size(arr), SQLITE_TRANSIENT);
    array.free(arr);
}

// array(value, ...)
// Creates an array filled with provided values.
// Infers array type from the first value. If the value is a string,
// sets max width to the maximum size among provided strings.
static void array_func(sqlite3_context* context, int argc, sqlite3_value** argv) {
    if (argc == 0) {
        sqlite3_result_error(context, "array() expects 1 or more aguments", -1);
        return;
    }

    Array* arr;
    size_t length = 0;
    if (sqlite3_value_type(argv[0]) == SQLITE_INTEGER) {
        size_t el_size = array.infer_size(TYPE_INT);
        arr = array.create_cap(TYPE_INT, el_size, argc);
        if (arr == NULL) {
            sqlite3_result_error_nomem(context);
            return;
        }
        for (int i = 0; i < argc; i++) {
            if (sqlite3_value_type(argv[i]) != arr->type) {
                continue;
            }
            int64_t val = sqlite3_value_int64(argv[i]);
            array.append(arr, &val);
            length += 1;
        }
    } else if (sqlite3_value_type(argv[0]) == SQLITE_FLOAT) {
        size_t el_size = array.infer_size(TYPE_INT);
        arr = array.create_cap(TYPE_REAL, el_size, argc);
        if (arr == NULL) {
            sqlite3_result_error_nomem(context);
            return;
        }
        for (int i = 0; i < argc; i++) {
            if (sqlite3_value_type(argv[i]) != arr->type) {
                continue;
            }
            double val = sqlite3_value_double(argv[i]);
            array.append(arr, &val);
            length += 1;
        }
    } else if (sqlite3_value_type(argv[0]) == SQLITE_TEXT) {
        size_t max_width = max_text_size(argc, argv);
        arr = array.create_cap(TYPE_TEXT, max_width + 1, argc);
        if (arr == NULL) {
            sqlite3_result_error_nomem(context);
            return;
        }
        for (int i = 0; i < argc; i++) {
            if (sqlite3_value_type(argv[i]) != arr->type) {
                continue;
            }
            uint8_t* val = array.fix_width(sqlite3_value_text(argv[i]), max_width);
            array.append(arr, val);
            free(val);
            length += 1;
        }
    } else {
        sqlite3_result_error(context, ERR_INVALID_ELEMENT_TYPE, -1);
        return;
    }

    if (arr->length != length) {
        // at least some appends failed
        array.free(arr);
        sqlite3_result_error_nomem(context);
        return;
    }

    sqlite3_result_blob(context, array.to_blob(arr), array.size(arr), SQLITE_TRANSIENT);
    array.free(arr);
}

// array_length(arr)
// Returns array elements count.
static void array_length(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);

    Array* arr = extract_array(argv[0]);
    if (arr == NULL) {
        return;
    }

    sqlite3_result_int64(context, arr->length);
    array.free(arr);
}

// array_at(arr, index)
// Returns the array element at the specified index (1-based).
static void array_at(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    Array* arr = extract_array(argv[0]);
    if (arr == NULL) {
        return;
    }

    if (sqlite3_value_type(argv[1]) != SQLITE_INTEGER) {
        sqlite3_result_error(context, ERR_INVALID_INDEX_TYPE, -1);
        return;
    }
    size_t index = sqlite3_value_int(argv[1]) - 1;

    uint8_t* valptr = array.at(arr, index);
    if (valptr == NULL) {
        array.free(arr);
        return;
    }

    switch (arr->type) {
        case TYPE_INT:
            sqlite3_result_int64(context, *((int64_t*)valptr));
            break;
        case TYPE_REAL:
            sqlite3_result_double(context, *((double*)valptr));
            break;
        case TYPE_TEXT:
            sqlite3_result_text(context, (const char*)valptr, -1, SQLITE_TRANSIENT);
            break;
        default:
            break;
    }
    array.free(arr);
}

// array_index(arr, value)
// Returns the index of the specified value, or 0 if there is no such value.
static void array_index(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    Array* arr = extract_array(argv[0]);
    if (arr == NULL) {
        return;
    }

    int index = -1;
    if (arr->type == TYPE_INT) {
        int64_t val = sqlite3_value_int64(argv[1]);
        index = array.index(arr, &val);
    } else if (arr->type == TYPE_REAL) {
        double val = sqlite3_value_double(argv[1]);
        index = array.index(arr, &val);
    } else if (arr->type == TYPE_TEXT) {
        const unsigned char* str = sqlite3_value_text(argv[1]);
        uint8_t* val = array.fix_width(str, arr->el_size - 1);
        index = array.index(arr, val);
        free(val);
    }

    sqlite3_result_int(context, index + 1);
    array.free(arr);
}

// array_contains(arr, value)
// Returns 1 if the value if found in the array, 0 otherwise.
static void array_contains(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    Array* arr = extract_array(argv[0]);
    if (arr == NULL) {
        return;
    }

    bool found = false;
    if (arr->type == TYPE_INT) {
        int64_t val = sqlite3_value_int64(argv[1]);
        found = array.contains(arr, &val);
    } else if (arr->type == TYPE_REAL) {
        double val = sqlite3_value_double(argv[1]);
        found = array.contains(arr, &val);
    } else if (arr->type == TYPE_TEXT) {
        const unsigned char* str = sqlite3_value_text(argv[1]);
        uint8_t* val = array.fix_width(str, arr->el_size - 1);
        found = array.contains(arr, val);
        free(val);
    }

    sqlite3_result_int(context, found);
    array.free(arr);
}

// array_append(arr, value)
// Appends the value to the end of the array.
static void array_append(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    Array* arr = extract_array(argv[0]);
    if (arr == NULL) {
        return;
    }

    if (sqlite3_value_type(argv[1]) != arr->type) {
        sqlite3_result_blob(context, array.to_blob(arr), array.size(arr), SQLITE_TRANSIENT);
        array.free(arr);
        return;
    }

    STATUS err = STATUS_OK;
    if (arr->type == TYPE_INT) {
        int64_t val = sqlite3_value_int64(argv[1]);
        err = array.append(arr, &val);
    } else if (arr->type == TYPE_REAL) {
        double val = sqlite3_value_double(argv[1]);
        err = array.append(arr, &val);
    } else if (arr->type == TYPE_TEXT) {
        const unsigned char* str = sqlite3_value_text(argv[1]);
        uint8_t* val = array.fix_width(str, arr->el_size - 1);
        err = array.append(arr, val);
        free(val);
    }

    if (err == STATUS_NOMEM) {
        array.free(arr);
        sqlite3_result_error_nomem(context);
        return;
    }

    sqlite3_result_blob(context, array.to_blob(arr), array.size(arr), SQLITE_TRANSIENT);
    array.free(arr);
}

// array_insert(arr, index, value)
// Inserts the value at the specified index, shifting following elements to the right.
static void array_insert(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 3);

    Array* arr = extract_array(argv[0]);
    if (arr == NULL) {
        return;
    }

    if (sqlite3_value_type(argv[1]) != SQLITE_INTEGER) {
        return;
    }
    size_t index = sqlite3_value_int(argv[1]) - 1;

    if (sqlite3_value_type(argv[2]) != arr->type) {
        sqlite3_result_blob(context, array.to_blob(arr), array.size(arr), SQLITE_TRANSIENT);
        array.free(arr);
        return;
    }

    STATUS err = STATUS_OK;
    if (arr->type == TYPE_INT) {
        int64_t val = sqlite3_value_int64(argv[2]);
        err = array.insert(arr, index, &val);
    } else if (arr->type == TYPE_REAL) {
        double val = sqlite3_value_double(argv[2]);
        err = array.insert(arr, index, &val);
    } else if (arr->type == TYPE_TEXT) {
        const unsigned char* str = sqlite3_value_text(argv[2]);
        uint8_t* val = array.fix_width(str, arr->el_size - 1);
        err = array.insert(arr, index, val);
        free(val);
    }

    if (err == STATUS_NOMEM) {
        array.free(arr);
        sqlite3_result_error_nomem(context);
        return;
    }

    sqlite3_result_blob(context, array.to_blob(arr), array.size(arr), SQLITE_TRANSIENT);
    array.free(arr);
}

// array_remove_at(arr, index)
// Removes the element at the specified index, shifting following elements to the left.
static void array_remove_at(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    Array* arr = extract_array(argv[0]);
    if (arr == NULL) {
        return;
    }

    if (sqlite3_value_type(argv[1]) != SQLITE_INTEGER) {
        sqlite3_result_error(context, ERR_INVALID_INDEX_TYPE, -1);
        return;
    }
    size_t index = sqlite3_value_int(argv[1]) - 1;

    STATUS err = array.remove_at(arr, index);
    if (err == STATUS_NOMEM) {
        array.free(arr);
        sqlite3_result_error_nomem(context);
        return;
    }

    sqlite3_result_blob(context, array.to_blob(arr), array.size(arr), SQLITE_TRANSIENT);
    array.free(arr);
}

// array_remove(arr, value)
// Removes the value from the array. If there are multiple such values,
// only removes the first one.
static void array_remove(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    Array* arr = extract_array(argv[0]);
    if (arr == NULL) {
        return;
    }

    if (sqlite3_value_type(argv[1]) != arr->type) {
        sqlite3_result_blob(context, array.to_blob(arr), array.size(arr), SQLITE_TRANSIENT);
        array.free(arr);
        return;
    }

    STATUS err = STATUS_OK;
    if (arr->type == TYPE_INT) {
        int64_t val = sqlite3_value_int64(argv[1]);
        err = array.remove(arr, &val);
    } else if (arr->type == TYPE_REAL) {
        double val = sqlite3_value_double(argv[1]);
        err = array.remove(arr, &val);
    } else if (arr->type == TYPE_TEXT) {
        const unsigned char* str = sqlite3_value_text(argv[1]);
        uint8_t* val = array.fix_width(str, arr->el_size - 1);
        err = array.remove(arr, val);
        free(val);
    }

    if (err == STATUS_NOMEM) {
        array.free(arr);
        sqlite3_result_error_nomem(context);
        return;
    }

    sqlite3_result_blob(context, array.to_blob(arr), array.size(arr), SQLITE_TRANSIENT);
    array.free(arr);
}

// array_clear(arr)
// Removes all elements from the array.
static void array_clear(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);

    Array* arr = extract_array(argv[0]);
    if (arr == NULL) {
        return;
    }

    array.clear(arr);

    sqlite3_result_blob(context, array.to_blob(arr), array.size(arr), SQLITE_TRANSIENT);
    array.free(arr);
}

// array_slice(arr, start)
// Returns a slice of the array from the `start` index inclusive
// to the end of the array.
static void array_slice_from(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    Array* arr = extract_array(argv[0]);
    if (arr == NULL) {
        return;
    }

    if (sqlite3_value_type(argv[1]) != SQLITE_INTEGER) {
        sqlite3_result_error(context, ERR_INVALID_INDEX_TYPE, -1);
        return;
    }
    size_t start = sqlite3_value_int(argv[1]) - 1;
    size_t end = arr->length;

    STATUS err = array.slice(arr, start, end);
    if (err == STATUS_NOMEM) {
        sqlite3_result_error_nomem(context);
    } else if (err == STATUS_INVALID_ARG) {
        sqlite3_result_null(context);
    } else {
        sqlite3_result_blob(context, array.to_blob(arr), array.size(arr), SQLITE_TRANSIENT);
    }
    array.free(arr);
}

// array_slice(arr, start, end)
// Returns a slice of the array from the `start` index inclusive
// to the `end` index non-inclusive.
static void array_slice_from_to(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 3);

    Array* arr = extract_array(argv[0]);
    if (arr == NULL) {
        return;
    }

    if (sqlite3_value_type(argv[1]) != SQLITE_INTEGER) {
        sqlite3_result_error(context, ERR_INVALID_INDEX_TYPE, -1);
        return;
    }
    size_t start = sqlite3_value_int(argv[1]) - 1;

    if (sqlite3_value_type(argv[2]) != SQLITE_INTEGER) {
        sqlite3_result_error(context, ERR_INVALID_INDEX_TYPE, -1);
        return;
    }
    size_t end = sqlite3_value_int(argv[2]) - 1;

    STATUS err = array.slice(arr, start, end);
    if (err == STATUS_NOMEM) {
        sqlite3_result_error_nomem(context);
    } else if (err == STATUS_INVALID_ARG) {
        sqlite3_result_null(context);
    } else {
        sqlite3_result_blob(context, array.to_blob(arr), array.size(arr), SQLITE_TRANSIENT);
    }
    array.free(arr);
}

// array_concat(arr, other)
// Concatenates two arrays.
static void array_concat(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 2);

    Array* arr = extract_array(argv[0]);
    if (arr == NULL) {
        return;
    }

    Array* other = extract_array(argv[1]);
    if (other == NULL) {
        array.free(arr);
        return;
    }

    STATUS err = array.extend(arr, other);
    if (err == STATUS_NOMEM) {
        sqlite3_result_error_nomem(context);
    } else if (err == STATUS_INVALID_ARG) {
        sqlite3_result_error(context, "arrays are not compatible", -1);
    } else {
        sqlite3_result_blob(context, array.to_blob(arr), array.size(arr), SQLITE_TRANSIENT);
    }
    array.free(arr);
    array.free(other);
}

// array_to_json(arr)
// Returns the JSON string representation of the array.
static void array_to_json(sqlite3_context* context, int argc, sqlite3_value** argv) {
    assert(argc == 1);

    Array* arr = extract_array(argv[0]);
    if (arr == NULL) {
        return;
    }

    sqlite3_str* str = sqlite3_str_new(NULL);
    sqlite3_str_appendall(str, "[");
    for (size_t i = 0; i < arr->length; i++) {
        uint8_t* valptr = array.at(arr, i);
        switch (arr->type) {
            case TYPE_INT:
                sqlite3_str_appendf(str, "%d", *((int64_t*)valptr));
                break;
            case TYPE_REAL:
                sqlite3_str_appendf(str, "%g", *((double*)valptr));
                break;
            case TYPE_TEXT:
                sqlite3_str_appendf(str, "\"%s\"", (const char*)valptr);
                break;
            default:
                break;
        }
        if (i != arr->length - 1) {
            sqlite3_str_appendall(str, ",");
        }
    }
    sqlite3_str_appendall(str, "]");
    char* val = sqlite3_str_finish(str);
    sqlite3_result_text(context, (const char*)val, -1, sqlite3_free);
}

int arrayscalar_init(sqlite3* db) {
    const int flags = SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC;
    sqlite3_create_function(db, "intarray", 0, flags, 0, intarray, 0, 0);
    sqlite3_create_function(db, "realarray", 0, flags, 0, realarray, 0, 0);
    sqlite3_create_function(db, "textarray", 1, flags, 0, textarray, 0, 0);
    sqlite3_create_function(db, "array", -1, flags, 0, array_func, 0, 0);
    sqlite3_create_function(db, "array_length", 1, flags, 0, array_length, 0, 0);
    sqlite3_create_function(db, "array_at", 2, flags, 0, array_at, 0, 0);
    sqlite3_create_function(db, "array_index", 2, flags, 0, array_index, 0, 0);
    sqlite3_create_function(db, "array_contains", 2, flags, 0, array_contains, 0, 0);
    sqlite3_create_function(db, "array_append", 2, flags, 0, array_append, 0, 0);
    sqlite3_create_function(db, "array_insert", 3, flags, 0, array_insert, 0, 0);
    sqlite3_create_function(db, "array_remove_at", 2, flags, 0, array_remove_at, 0, 0);
    sqlite3_create_function(db, "array_remove", 2, flags, 0, array_remove, 0, 0);
    sqlite3_create_function(db, "array_clear", 1, flags, 0, array_clear, 0, 0);
    sqlite3_create_function(db, "array_slice", 2, flags, 0, array_slice_from, 0, 0);
    sqlite3_create_function(db, "array_slice", 3, flags, 0, array_slice_from_to, 0, 0);
    sqlite3_create_function(db, "array_concat", 2, flags, 0, array_concat, 0, 0);
    sqlite3_create_function(db, "array_to_json", 1, flags, 0, array_to_json, 0, 0);
    return SQLITE_OK;
}
