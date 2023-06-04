// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Byte string data structure.

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bstring.h"

// string_new creates an empty string.
static ByteString string_new(void) {
    char* bytes = "\0";
    ByteString str = {.bytes = bytes, .length = 0, .owning = false};
    return str;
}

// string_from_cstring creates a new string that wraps an existing C string.
static ByteString string_from_cstring(const char* const cstring, size_t length) {
    ByteString str = {.bytes = cstring, .length = length, .owning = false};
    return str;
}

// string_clone creates a new string by copying an existing C string.
static ByteString string_clone(const char* const cstring, size_t length) {
    char* bytes = calloc(length + 1, sizeof(char));
    if (bytes == NULL) {
        ByteString str = {NULL, 0, true};
        return str;
    }
    memcpy(bytes, cstring, length);
    ByteString str = {bytes, length, true};
    return str;
}

// string_to_cstring converts the string to a zero-terminated C string.
static const char* string_to_cstring(ByteString str) {
    if (str.bytes == NULL) {
        return NULL;
    }
    return str.bytes;
}

// string_free destroys the string, freeing resources if necessary.
static void string_free(ByteString str) {
    if (str.owning && str.bytes != NULL) {
        free((void*)str.bytes);
    }
}

// string_at returns a character by its index in the string.
static char string_at(ByteString str, size_t idx) {
    if (str.length == 0) {
        return 0;
    }
    if (idx < 0 || idx >= str.length) {
        return 0;
    };
    return str.bytes[idx];
}

// string_slice returns a slice of the string,
// from the `start` index (inclusive) to the `end` index (non-inclusive).
// Negative `start` and `end` values count from the end of the string.
static ByteString string_slice(ByteString str, int start, int end) {
    if (str.length == 0) {
        return string_new();
    }

    // adjusted start index
    start = start < 0 ? str.length + start : start;
    // python-compatible: treat negative start index larger than the length of the string as zero
    start = start < 0 ? 0 : start;
    // adjusted start index should be less the the length of the string
    if (start >= (int)str.length) {
        return string_new();
    }

    // adjusted end index
    end = end < 0 ? str.length + end : end;
    // python-compatible: treat end index larger than the length of the string
    // as equal to the length
    end = end > (int)str.length ? (int)str.length : end;
    // adjusted end index should be >= 0
    if (end < 0) {
        return string_new();
    }

    // adjusted start index should be less than adjusted end index
    if (start >= end) {
        return string_new();
    }

    char* at = (char*)str.bytes + start;
    size_t length = end - start;
    ByteString slice = string_clone(at, length);
    return slice;
}

// string_substring returns a substring of `length` characters,
// starting from the `start` index.
static ByteString string_substring(ByteString str, size_t start, size_t length) {
    if (length > str.length - start) {
        length = str.length - start;
    }
    return string_slice(str, start, start + length);
}

// string_contains_after checks if the other string is a substring of the original string,
// starting at the `start` index.
static bool string_contains_after(ByteString str, ByteString other, size_t start) {
    if (start + other.length > str.length) {
        return false;
    }
    for (size_t idx = 0; idx < other.length; idx++) {
        if (str.bytes[start + idx] != other.bytes[idx]) {
            return false;
        }
    }
    return true;
}

// string_index_char returns the first index of the character in the string
// after the `start` index, inclusive.
static int string_index_char(ByteString str, char chr, size_t start) {
    for (size_t idx = start; idx < str.length; idx++) {
        if (str.bytes[idx] == chr) {
            return idx;
        }
    }
    return -1;
}

// string_last_index_char returns the last index of the character in the string
// before the `end` index, inclusive.
static int string_last_index_char(ByteString str, char chr, size_t end) {
    if (end >= str.length) {
        return -1;
    }
    for (int idx = end; idx >= 0; idx--) {
        if (str.bytes[idx] == chr) {
            return idx;
        }
    }
    return -1;
}

// string_index_after returns the index of the substring in the original string
// after the `start` index, inclusive.
static int string_index_after(ByteString str, ByteString other, size_t start) {
    if (other.length == 0) {
        return start;
    }
    if (str.length == 0 || other.length > str.length) {
        return -1;
    }

    size_t cur_idx = start;
    while (cur_idx < str.length) {
        int match_idx = string_index_char(str, other.bytes[0], cur_idx);
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

// string_index returns the first index of the substring in the original string.
static int string_index(ByteString str, ByteString other) {
    return string_index_after(str, other, 0);
}

// string_last_index returns the last index of the substring in the original string.
static int string_last_index(ByteString str, ByteString other) {
    if (other.length == 0) {
        return str.length - 1;
    }
    if (str.length == 0 || other.length > str.length) {
        return -1;
    }

    int cur_idx = str.length - 1;
    while (cur_idx >= 0) {
        int match_idx = string_last_index_char(str, other.bytes[0], cur_idx);
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

// string_contains checks if the string contains the substring.
static bool string_contains(ByteString str, ByteString other) {
    return string_index(str, other) != -1;
}

// string_equals checks if two strings are equal character by character.
static bool string_equals(ByteString str, ByteString other) {
    if (str.bytes == NULL && other.bytes == NULL) {
        return true;
    }
    if (str.bytes == NULL || other.bytes == NULL) {
        return false;
    }
    if (str.length != other.length) {
        return false;
    }
    return string_contains_after(str, other, 0);
}

// string_has_prefix checks if the string starts with the `other` substring.
static bool string_has_prefix(ByteString str, ByteString other) {
    return string_index(str, other) == 0;
}

// string_has_suffix checks if the string ends with the `other` substring.
static bool string_has_suffix(ByteString str, ByteString other) {
    if (other.length == 0) {
        return true;
    }
    return string_last_index(str, other) == str.length - other.length;
}

// string_count counts how many times the `other` substring is contained in the original string.
static size_t string_count(ByteString str, ByteString other) {
    if (str.length == 0 || other.length == 0 || other.length > str.length) {
        return 0;
    }

    size_t count = 0;
    size_t char_idx = 0;
    while (char_idx < str.length) {
        int match_idx = string_index_after(str, other, char_idx);
        if (match_idx == -1) {
            break;
        }
        count += 1;
        char_idx = match_idx + other.length;
    }

    return count;
}

// string_split_part splits the string by the separator and returns the nth part (0-based).
static ByteString string_split_part(ByteString str, ByteString sep, size_t part) {
    if (str.length == 0 || sep.length > str.length) {
        return string_new();
    }
    if (sep.length == 0) {
        if (part == 0) {
            return string_slice(str, 0, str.length);
        } else {
            return string_new();
        }
    }

    size_t found = 0;
    size_t prev_idx = 0;
    size_t char_idx = 0;
    while (char_idx < str.length) {
        int match_idx = string_index_after(str, sep, char_idx);
        if (match_idx == -1) {
            break;
        }
        if (found == part) {
            return string_slice(str, prev_idx, match_idx);
        }
        found += 1;
        prev_idx = match_idx + sep.length;
        char_idx = match_idx + sep.length;
    }

    if (found == part) {
        return string_slice(str, prev_idx, str.length);
    }

    return string_new();
}

// string_join joins strings using the separator and returns the resulting string.
static ByteString string_join(ByteString* strings, size_t count, ByteString sep) {
    // calculate total string length
    size_t total_length = 0;
    for (size_t idx = 0; idx < count; idx++) {
        ByteString str = strings[idx];
        total_length += str.length;
        // no separator after the last one
        if (idx != count - 1) {
            total_length += sep.length;
        }
    }

    // allocate memory for the bytes
    size_t total_size = total_length * sizeof(char);
    char* bytes = malloc(total_size + 1);
    if (bytes == NULL) {
        ByteString str = {NULL, 0, false};
        return str;
    }

    // copy bytes from each string with separator in between
    char* at = bytes;
    for (size_t idx = 0; idx < count; idx++) {
        ByteString str = strings[idx];
        memcpy(at, str.bytes, str.length);
        at += str.length;
        if (idx != count - 1 && sep.length != 0) {
            memcpy(at, sep.bytes, sep.length);
            at += sep.length;
        }
    }

    bytes[total_length] = '\0';
    ByteString str = {bytes, total_length, true};
    return str;
}

// string_concat concatenates strings and returns the resulting string.
static ByteString string_concat(ByteString* strings, size_t count) {
    ByteString sep = string_new();
    return string_join(strings, count, sep);
}

// string_repeat concatenates the string to itself a given number of times
// and returns the resulting string.
static ByteString string_repeat(ByteString str, size_t count) {
    // calculate total string length
    size_t total_length = str.length * count;

    // allocate memory for the bytes
    size_t total_size = total_length * sizeof(char);
    char* bytes = malloc(total_size + 1);
    if (bytes == NULL) {
        ByteString res = {NULL, 0, false};
        return res;
    }

    // copy bytes
    char* at = bytes;
    for (size_t idx = 0; idx < count; idx++) {
        memcpy(at, str.bytes, str.length);
        at += str.length;
    }

    bytes[total_size] = '\0';
    ByteString res = {bytes, total_length, true};
    return res;
}

// string_replace replaces the `old` substring with the `new` substring in the original string,
// but not more than `max_count` times.
static ByteString string_replace(ByteString str, ByteString old, ByteString new, size_t max_count) {
    // count matches of the old string in the source string
    size_t count = string_count(str, old);
    if (count == 0) {
        return string_slice(str, 0, str.length);
    }

    // limit the number of replacements
    if (max_count >= 0 && count > max_count) {
        count = max_count;
    }

    // k matches split string into (k+1) parts
    // allocate an array for them
    size_t parts_count = count + 1;
    ByteString* strings = malloc(parts_count * sizeof(ByteString));
    if (strings == NULL) {
        ByteString res = {NULL, 0, false};
        return res;
    }

    // split the source string where it matches the old string
    // and fill the strings array with these parts
    size_t part_idx = 0;
    size_t char_idx = 0;
    while (char_idx < str.length && part_idx < count) {
        int match_idx = string_index_after(str, old, char_idx);
        if (match_idx == -1) {
            break;
        }
        // slice from the prevoius match to the current match
        strings[part_idx] = string_slice(str, char_idx, match_idx);
        part_idx += 1;
        char_idx = match_idx + old.length;
    }
    // "tail" from the last match to the end of the source string
    strings[part_idx] = string_slice(str, char_idx, str.length);

    // join all the parts using new string as a separator
    ByteString res = string_join(strings, parts_count, new);
    // free string parts
    for (size_t idx = 0; idx < parts_count; idx++) {
        string_free(strings[idx]);
    }
    free(strings);
    return res;
}

// string_replace_all replaces all `old` substrings with the `new` substrings
// in the original string.
static ByteString string_replace_all(ByteString str, ByteString old, ByteString new) {
    return string_replace(str, old, new, -1);
}

// string_reverse returns the reversed string.
static ByteString string_reverse(ByteString str) {
    ByteString res = string_clone(str.bytes, str.length);
    char* bytes = (char*)res.bytes;
    for (size_t i = 0; i < str.length / 2; i++) {
        char r = bytes[i];
        bytes[i] = bytes[str.length - 1 - i];
        bytes[str.length - 1 - i] = r;
    }
    return res;
}

// string_trim_left trims whitespaces from the beginning of the string.
static ByteString string_trim_left(ByteString str) {
    if (str.length == 0) {
        return string_new();
    }
    size_t idx = 0;
    for (; idx < str.length; idx++) {
        if (!isspace(str.bytes[idx])) {
            break;
        }
    }
    return string_slice(str, idx, str.length);
}

// string_trim_right trims whitespaces from the end of the string.
static ByteString string_trim_right(ByteString str) {
    if (str.length == 0) {
        return string_new();
    }
    size_t idx = str.length - 1;
    for (; idx >= 0; idx--) {
        if (!isspace(str.bytes[idx])) {
            break;
        }
    }
    return string_slice(str, 0, idx + 1);
}

// string_trim trims whitespaces from the beginning and end of the string.
static ByteString string_trim(ByteString str) {
    if (str.length == 0) {
        return string_new();
    }
    size_t left = 0;
    for (; left < str.length; left++) {
        if (!isspace(str.bytes[left])) {
            break;
        }
    }
    size_t right = str.length - 1;
    for (; right >= 0; right--) {
        if (!isspace(str.bytes[right])) {
            break;
        }
    }
    return string_slice(str, left, right + 1);
}

// string_print prints the string to stdout.
static void string_print(ByteString str) {
    if (str.bytes == NULL) {
        printf("<null>\n");
        return;
    }
    printf("'%s' (len=%zu)\n", str.bytes, str.length);
}

struct bstring_ns bstring = {.new = string_new,
                             .to_cstring = string_to_cstring,
                             .from_cstring = string_from_cstring,
                             .free = string_free,
                             .at = string_at,
                             .slice = string_slice,
                             .substring = string_substring,
                             .index = string_index,
                             .last_index = string_last_index,
                             .contains = string_contains,
                             .equals = string_equals,
                             .has_prefix = string_has_prefix,
                             .has_suffix = string_has_suffix,
                             .count = string_count,
                             .split_part = string_split_part,
                             .join = string_join,
                             .concat = string_concat,
                             .repeat = string_repeat,
                             .replace = string_replace,
                             .replace_all = string_replace_all,
                             .reverse = string_reverse,
                             .trim_left = string_trim_left,
                             .trim_right = string_trim_right,
                             .trim = string_trim,
                             .print = string_print};
