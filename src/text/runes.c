// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// UTF-8 characters (runes) <-> C string conversions.

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// utf8_cat_rune prints the rune to the string.
static char* utf8_cat_rune(char* str, int32_t rune) {
    if (0 == ((int32_t)0xffffff80 & rune)) {
        // 1-byte/7-bit ascii
        // (0b0xxxxxxx)
        str[0] = (char)rune;
        str += 1;
    } else if (0 == ((int32_t)0xfffff800 & rune)) {
        // 2-byte/11-bit utf8 code point
        // (0b110xxxxx 0b10xxxxxx)
        str[0] = (char)(0xc0 | (char)((rune >> 6) & 0x1f));
        str[1] = (char)(0x80 | (char)(rune & 0x3f));
        str += 2;
    } else if (0 == ((int32_t)0xffff0000 & rune)) {
        // 3-byte/16-bit utf8 code point
        // (0b1110xxxx 0b10xxxxxx 0b10xxxxxx)
        str[0] = (char)(0xe0 | (char)((rune >> 12) & 0x0f));
        str[1] = (char)(0x80 | (char)((rune >> 6) & 0x3f));
        str[2] = (char)(0x80 | (char)(rune & 0x3f));
        str += 3;
    } else {  // if (0 == ((int)0xffe00000 & rune)) {
        // 4-byte/21-bit utf8 code point
        // (0b11110xxx 0b10xxxxxx 0b10xxxxxx 0b10xxxxxx)
        str[0] = (char)(0xf0 | (char)((rune >> 18) & 0x07));
        str[1] = (char)(0x80 | (char)((rune >> 12) & 0x3f));
        str[2] = (char)(0x80 | (char)((rune >> 6) & 0x3f));
        str[3] = (char)(0x80 | (char)(rune & 0x3f));
        str += 4;
    }
    return str;
}

// utf8iter iterates over the C string, producing runes.
typedef struct {
    const char* str;
    int32_t rune;
    size_t length;
    size_t index;
    bool eof;
} utf8iter;

// utf8iter_new creates a new iterator.
static utf8iter* utf8iter_new(const char* str, size_t length) {
    utf8iter* iter = malloc(sizeof(utf8iter));
    if (iter == NULL) {
        return NULL;
    }
    iter->str = str;
    iter->length = length;
    iter->index = 0;
    iter->eof = length == 0;
    return iter;
}

// utf8iter_next advances the iterator to the next rune and returns it.
static int32_t utf8iter_next(utf8iter* iter) {
    assert(iter != NULL);

    if (iter->eof) {
        return 0;
    }

    const char* str = iter->str;
    if (0xf0 == (0xf8 & str[0])) {
        // 4 byte utf8 codepoint
        iter->rune = ((0x07 & str[0]) << 18) | ((0x3f & str[1]) << 12) | ((0x3f & str[2]) << 6) |
                     (0x3f & str[3]);
        iter->str += 4;
    } else if (0xe0 == (0xf0 & str[0])) {
        // 3 byte utf8 codepoint
        iter->rune = ((0x0f & str[0]) << 12) | ((0x3f & str[1]) << 6) | (0x3f & str[2]);
        iter->str += 3;
    } else if (0xc0 == (0xe0 & str[0])) {
        // 2 byte utf8 codepoint
        iter->rune = ((0x1f & str[0]) << 6) | (0x3f & str[1]);
        iter->str += 2;
    } else {
        // 1 byte utf8 codepoint otherwise
        iter->rune = str[0];
        iter->str += 1;
    }

    iter->index += 1;

    if (iter->index == iter->length) {
        iter->eof = true;
    }

    return iter->rune;
}

// runes_from_cstring creates an array of runes from a C string.
int32_t* runes_from_cstring(const char* const str, size_t length) {
    assert(length > 0);
    int32_t* runes = calloc(length, sizeof(int32_t));
    if (runes == NULL) {
        return NULL;
    }
    utf8iter* iter = utf8iter_new(str, length);
    size_t idx = 0;
    while (!iter->eof) {
        int32_t rune = utf8iter_next(iter);
        runes[idx] = rune;
        idx += 1;
    }
    free(iter);
    return runes;
}

// runes_to_cstring creates a C string from an array of runes.
char* runes_to_cstring(const int32_t* runes, size_t length) {
    char* str;
    if (length == 0) {
        str = calloc(1, sizeof(char));
        return str;
    }

    size_t maxlen = length * sizeof(int32_t) + 1;
    str = malloc(maxlen);
    if (str == NULL) {
        return NULL;
    }

    char* at = str;
    for (size_t i = 0; i < length; i++) {
        at = utf8_cat_rune(at, runes[i]);
    }
    *at = '\0';
    at += 1;

    if ((size_t)(at - str) < maxlen) {
        // shrink to real size
        size_t size = at - str;
        str = realloc(str, size);
    }
    return str;
}
