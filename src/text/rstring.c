// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Rune (UTF-8) string data structure.

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "text.h"

static size_t utf8_length(const char* str) {
    size_t length = 0;

    while (*str != '\0') {
        if (0xf0 == (0xf8 & *str)) {
            // 4-byte utf8 code point (began with 0b11110xxx)
            str += 4;
        } else if (0xe0 == (0xf0 & *str)) {
            // 3-byte utf8 code point (began with 0b1110xxxx)
            str += 3;
        } else if (0xc0 == (0xe0 & *str)) {
            // 2-byte utf8 code point (began with 0b110xxxxx)
            str += 2;
        } else {  // if (0x00 == (0x80 & *s)) {
            // 1-byte ascii (began with 0b0xxxxxxx)
            str += 1;
        }

        // no matter the bytes we marched s forward by, it was
        // only 1 utf8 codepoint
        length++;
    }

    return length;
}

static RuneString string_new(void) {
    RuneString str = {.runes = NULL, .length = 0, .size = 0, .owning = true};
    return str;
}

static RuneString string_from_runes(const int32_t* const runes, size_t length, bool owning) {
    RuneString str = {
        .runes = runes, .length = length, .size = length * sizeof(int32_t), .owning = owning};
    return str;
}

static RuneString string_from_cstring(const char* const utf8str) {
    size_t length = utf8_length(utf8str);
    int32_t* runes = length > 0 ? runes_from_cstring(utf8str, length) : NULL;
    return string_from_runes(runes, length, true);
}

static char* string_to_cstring(RuneString str) {
    return runes_to_cstring(str.runes, str.length);
}

static void string_free(RuneString str) {
    if (str.owning && str.runes != NULL) {
        free((void*)str.runes);
    }
}

static int32_t string_at(RuneString str, size_t idx) {
    if (str.length == 0) {
        return 0;
    }
    if (idx < 0 || idx >= str.length) {
        return 0;
    };
    return str.runes[idx];
}

static RuneString string_slice(RuneString str, int start, int end) {
    if (str.length == 0) {
        return string_new();
    }
    start = start < 0 ? str.length + start : start;
    end = end < 0 ? str.length + end : end;
    if (start < 0 || start >= (int)str.length) {
        return string_new();
    }
    if (end < 0 || end > (int)str.length) {
        return string_new();
    }
    if (start >= end) {
        return string_new();
    }

    int32_t* at = (int32_t*)str.runes + start;
    size_t length = end - start;
    RuneString slice = string_from_runes(at, length, false);
    return slice;
}

static RuneString string_substring(RuneString str, size_t start, size_t length) {
    if (length > str.length - start) {
        length = str.length - start;
    }
    return string_slice(str, start, start + length);
}

static bool string_contains_after(RuneString str, RuneString other, size_t start) {
    if (start + other.length > str.length) {
        return false;
    }
    for (size_t idx = 0; idx < other.length; idx++) {
        if (str.runes[start + idx] != other.runes[idx]) {
            return false;
        }
    }
    return true;
}

static int string_index_char(RuneString str, int32_t rune, size_t start) {
    for (size_t idx = start; idx < str.length; idx++) {
        if (str.runes[idx] == rune) {
            return idx;
        }
    }
    return -1;
}

static int string_last_index_char(RuneString str, int32_t rune, size_t end) {
    if (end >= str.length) {
        return -1;
    }
    for (int idx = end; idx >= 0; idx--) {
        if (str.runes[idx] == rune) {
            return idx;
        }
    }
    return -1;
}

static int string_index_after(RuneString str, RuneString other, size_t start) {
    if (other.length == 0) {
        return start;
    }
    if (str.length == 0 || other.length > str.length) {
        return -1;
    }

    size_t cur_idx = start;
    while (cur_idx < str.length) {
        int match_idx = string_index_char(str, other.runes[0], cur_idx);
        if (match_idx == -1) {
            return match_idx;
        }
        if (string_contains_after(str, other, match_idx)) {
            return match_idx;
        }
        cur_idx = match_idx + 1;
    }
    return -1;
}

static int string_index(RuneString str, RuneString other) {
    return string_index_after(str, other, 0);
}

static int string_last_index(RuneString str, RuneString other) {
    if (other.length == 0) {
        return str.length - 1;
    }
    if (str.length == 0 || other.length > str.length) {
        return -1;
    }

    int cur_idx = str.length - 1;
    while (cur_idx >= 0) {
        int match_idx = string_last_index_char(str, other.runes[0], cur_idx);
        if (match_idx == -1) {
            return match_idx;
        }
        if (string_contains_after(str, other, match_idx)) {
            return match_idx;
        }
        cur_idx = match_idx - 1;
    }

    return -1;
}

static RuneString string_reverse(RuneString str) {
    int32_t* runes = (int32_t*)str.runes;
    for (size_t i = 0; i < str.length / 2; i++) {
        int32_t r = runes[i];
        runes[i] = runes[str.length - 1 - i];
        runes[str.length - 1 - i] = r;
    }
    RuneString res = string_from_runes(runes, str.length, false);
    return res;
}

// return string_from_runes(str.runes, str.length, false);

RuneString string_pad_left(RuneString str, size_t length, RuneString fill) {
    if (str.length >= length) {
        // If the string is already longer than length, return a truncated version of the string
        return string_substring(str, 0, length);
    }

    if (fill.length == 0) {
        // If the fill string is empty, return the original string
        return string_from_runes(str.runes, str.length, false);
    }

    // Calculate the number of characters to pad
    size_t pad_langth = length - str.length;

    // Allocate memory for the padded string
    size_t new_size = (str.length + pad_langth) * sizeof(int32_t);
    int32_t* new_runes = malloc(new_size);
    if (new_runes == NULL) {
        return string_new();
    }

    // Copy the fill characters to the beginning of the new string
    for (size_t i = 0; i < pad_langth; i++) {
        new_runes[i] = fill.runes[i % fill.length];
    }

    // Copy the original string to the end of the new string
    memcpy(&new_runes[pad_langth], str.runes, str.size);

    // Return the new string
    RuneString new_str = string_from_runes(new_runes, length, true);
    return new_str;
}

RuneString string_pad_right(RuneString str, size_t length, RuneString fill) {
    if (str.length >= length) {
        // If the string is already longer than length, return a truncated version of the string
        return string_substring(str, 0, length);
    }

    if (fill.length == 0) {
        // If the fill string is empty, return the original string
        return string_from_runes(str.runes, str.length, false);
    }

    // Calculate the number of characters to pad
    size_t pad_length = length - str.length;

    // Allocate memory for the padded string
    size_t new_size = (str.length + pad_length) * sizeof(int32_t);
    int32_t* new_runes = malloc(new_size);
    if (new_runes == NULL) {
        return string_new();
    }

    // Copy the original string to the beginning of the new string
    memcpy(new_runes, str.runes, str.size);

    // Copy the fill characters to the end of the new string
    for (size_t i = str.length; i < length; i++) {
        new_runes[i] = fill.runes[(i - str.length) % fill.length];
    }

    // Return the new string
    RuneString new_str = string_from_runes(new_runes, length, true);
    return new_str;
}

static void string_print(RuneString str) {
    if (str.length == 0) {
        printf("'' (len=0)\n");
        return;
    }
    printf("'");
    for (size_t i = 0; i < str.length; i++) {
        printf("%08x ", str.runes[i]);
    }
    printf("' (len=%zu)", str.length);
    printf("\n");
}

struct rstring_ns rstring = {.new = string_new,
                             .from_cstring = string_from_cstring,
                             .to_cstring = string_to_cstring,
                             .free = string_free,
                             .at = string_at,
                             .index = string_index,
                             .last_index = string_last_index,
                             .slice = string_slice,
                             .substring = string_substring,
                             .reverse = string_reverse,
                             .pad_left = string_pad_left,
                             .pad_right = string_pad_right,
                             .print = string_print};
