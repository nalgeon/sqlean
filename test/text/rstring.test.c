// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

#include "text/rstring.h"

static bool eq(RuneString str, const char* expected) {
    char* got = rstring_to_cstring(str);
    bool eq = strcmp(got, expected) == 0;
    free(got);
    return eq;
}

static void test_cstring(void) {
    printf("test_cstring...");
    RuneString str = rstring_from_cstring("привет мир");
    assert(eq(str, "привет мир"));
    rstring_free(str);
    printf("OK\n");
}

static void test_at(void) {
    printf("test_at...");
    RuneString str = rstring_from_cstring("привет мир");
    int32_t rune = rstring_at(str, 2);
    assert(rune == 1080);
    rstring_free(str);
    printf("OK\n");
}

static void test_slice(void) {
    printf("test_slice...");
    RuneString str = rstring_from_cstring("привет мир");

    {
        RuneString slice = rstring_slice(str, 7, 10);
        assert(eq(slice, "мир"));
        rstring_free(slice);
    }

    {
        RuneString slice = rstring_slice(str, 0, 6);
        assert(eq(slice, "привет"));
        rstring_free(slice);
    }

    {
        RuneString slice = rstring_slice(str, -3, str.length);
        assert(eq(slice, "мир"));
        rstring_free(slice);
    }

    {
        RuneString slice = rstring_slice(str, 3, 3);
        assert(eq(slice, ""));
        rstring_free(slice);
    }

    rstring_free(str);
    printf("OK\n");
}

static void test_substring(void) {
    printf("test_substring...");
    RuneString str = rstring_from_cstring("привет мир");

    {
        RuneString slice = rstring_substring(str, 7, 3);
        assert(eq(slice, "мир"));
        rstring_free(slice);
    }

    {
        RuneString slice = rstring_substring(str, 0, 6);
        assert(eq(slice, "привет"));
        rstring_free(slice);
    }

    {
        RuneString slice = rstring_substring(str, 0, str.length);
        assert(eq(slice, "привет мир"));
        rstring_free(slice);
    }

    {
        RuneString slice = rstring_substring(str, 7, str.length);
        assert(eq(slice, "мир"));
        rstring_free(slice);
    }

    {
        RuneString slice = rstring_substring(str, 1, 1);
        assert(eq(slice, "р"));
        rstring_free(slice);
    }

    {
        RuneString slice = rstring_substring(str, 1, 0);
        assert(eq(slice, ""));
        rstring_free(slice);
    }

    rstring_free(str);
    printf("OK\n");
}

static void test_index(void) {
    printf("test_index...");
    RuneString str = rstring_from_cstring("привет мир");

    {
        RuneString other = rstring_from_cstring("пр");
        int index = rstring_index(str, other);
        assert(index == 0);
        rstring_free(other);
    }

    {
        RuneString other = rstring_from_cstring("и");
        int index = rstring_index(str, other);
        assert(index == 2);
        rstring_free(other);
    }

    {
        RuneString other = rstring_from_cstring("ми");
        int index = rstring_index(str, other);
        assert(index == 7);
        rstring_free(other);
    }

    {
        RuneString other = rstring_from_cstring("ир");
        int index = rstring_index(str, other);
        assert(index == 8);
        rstring_free(other);
    }

    {
        RuneString other = rstring_from_cstring("ирк");
        int index = rstring_index(str, other);
        assert(index == -1);
        rstring_free(other);
    }

    {
        RuneString str = rstring_from_cstring("привет миф");
        RuneString other = rstring_from_cstring("ф");
        int index = rstring_index(str, other);
        assert(index == 9);
        rstring_free(other);
    }

    {
        RuneString other = rstring_from_cstring("р ");
        int index = rstring_index(str, other);
        assert(index == -1);
        rstring_free(other);
    }

    rstring_free(str);
    printf("OK\n");
}

static void test_last_index(void) {
    printf("test_last_index...");
    RuneString str = rstring_from_cstring("привет мир");

    {
        RuneString other = rstring_from_cstring("и");
        int index = rstring_last_index(str, other);
        assert(index == 8);
        rstring_free(other);
    }

    {
        RuneString other = rstring_from_cstring("при");
        int index = rstring_last_index(str, other);
        assert(index == 0);
        rstring_free(other);
    }

    {
        RuneString other = rstring_from_cstring("ирк");
        int index = rstring_last_index(str, other);
        assert(index == -1);
        rstring_free(other);
    }

    rstring_free(str);
    printf("OK\n");
}

static void test_translate(void) {
    printf("test_translate...");
    RuneString str = rstring_from_cstring("привет мир");

    {
        RuneString from = rstring_from_cstring("ир");
        RuneString to = rstring_from_cstring("ИР");
        RuneString res = rstring_translate(str, from, to);
        assert(eq(res, "пРИвет мИР"));
        rstring_free(from);
        rstring_free(to);
        rstring_free(res);
    }

    {
        RuneString from = rstring_from_cstring("абв");
        RuneString to = rstring_from_cstring("АБВ");
        RuneString res = rstring_translate(str, from, to);
        assert(eq(res, "приВет мир"));
        rstring_free(from);
        rstring_free(to);
        rstring_free(res);
    }

    {
        RuneString from = rstring_from_cstring("мир");
        RuneString to = rstring_from_cstring("мир");
        RuneString res = rstring_translate(str, from, to);
        assert(eq(res, "привет мир"));
        rstring_free(from);
        rstring_free(to);
        rstring_free(res);
    }

    {
        RuneString from = rstring_from_cstring("ипр");
        RuneString to = rstring_from_cstring("И");
        RuneString res = rstring_translate(str, from, to);
        assert(eq(res, "Ивет мИ"));
        rstring_free(from);
        rstring_free(to);
        rstring_free(res);
    }

    {
        RuneString str = rstring_from_cstring("и");
        RuneString from = rstring_from_cstring("пир");
        RuneString to = rstring_from_cstring("ПИР");
        RuneString res = rstring_translate(str, from, to);
        assert(eq(res, "И"));
        rstring_free(str);
        rstring_free(from);
        rstring_free(to);
        rstring_free(res);
    }

    {
        RuneString str = rstring_from_cstring("о");
        RuneString from = rstring_from_cstring("пир");
        RuneString to = rstring_from_cstring("ПИР");
        RuneString res = rstring_translate(str, from, to);
        assert(eq(res, "о"));
        rstring_free(str);
        rstring_free(from);
        rstring_free(to);
        rstring_free(res);
    }

    {
        RuneString from = rstring_from_cstring("");
        RuneString to = rstring_from_cstring("ИР");
        RuneString res = rstring_translate(str, from, to);
        assert(eq(res, "привет мир"));
        rstring_free(from);
        rstring_free(to);
        rstring_free(res);
    }

    {
        RuneString from = rstring_from_cstring("ир");
        RuneString to = rstring_from_cstring("");
        RuneString res = rstring_translate(str, from, to);
        assert(eq(res, "пвет м"));
        rstring_free(from);
        rstring_free(to);
        rstring_free(res);
    }

    {
        RuneString from = rstring_from_cstring("");
        RuneString to = rstring_from_cstring("");
        RuneString res = rstring_translate(str, from, to);
        assert(eq(res, "привет мир"));
        rstring_free(from);
        rstring_free(to);
        rstring_free(res);
    }

    rstring_free(str);
    printf("OK\n");
}

static void test_reverse(void) {
    printf("test_reverse...");
    {
        RuneString str = rstring_from_cstring("привет");
        RuneString res = rstring_reverse(str);
        assert(eq(res, "тевирп"));
        rstring_free(str);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("привет мир");
        RuneString res = rstring_reverse(str);
        assert(eq(res, "рим тевирп"));
        rstring_free(str);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("𐌀𐌁𐌂");
        RuneString res = rstring_reverse(str);
        assert(eq(res, "𐌂𐌁𐌀"));
        rstring_free(str);
        rstring_free(res);
    }
    {
        RuneString str = rstring_new();
        RuneString res = rstring_reverse(str);
        assert(eq(res, ""));
        rstring_free(str);
        rstring_free(res);
    }
    printf("OK\n");
}

static void test_trim_left(void) {
    printf("test_trim_left...");
    {
        RuneString str = rstring_from_cstring("   привет");
        RuneString chars = rstring_from_cstring(" ");
        RuneString res = rstring_trim_left(str, chars);
        assert(eq(res, "привет"));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("273привет");
        RuneString chars = rstring_from_cstring("987654321");
        RuneString res = rstring_trim_left(str, chars);
        assert(eq(res, "привет"));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("273привет");
        RuneString chars = rstring_from_cstring("98765421");
        RuneString res = rstring_trim_left(str, chars);
        assert(eq(res, "3привет"));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("хохохпривет");
        RuneString chars = rstring_from_cstring("ох");
        RuneString res = rstring_trim_left(str, chars);
        assert(eq(res, "привет"));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("привет");
        RuneString chars = rstring_from_cstring(" ");
        RuneString res = rstring_trim_left(str, chars);
        assert(eq(res, "привет"));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("   ");
        RuneString chars = rstring_from_cstring(" ");
        RuneString res = rstring_trim_left(str, chars);
        assert(eq(res, ""));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("");
        RuneString chars = rstring_from_cstring(" ");
        RuneString res = rstring_trim_left(str, chars);
        assert(eq(res, ""));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    printf("OK\n");
}

static void test_trim_right(void) {
    printf("test_trim_right...");
    {
        RuneString str = rstring_from_cstring("привет   ");
        RuneString chars = rstring_from_cstring(" ");
        RuneString res = rstring_trim_right(str, chars);
        assert(eq(res, "привет"));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("привет372");
        RuneString chars = rstring_from_cstring("987654321");
        RuneString res = rstring_trim_right(str, chars);
        assert(eq(res, "привет"));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("привет372");
        RuneString chars = rstring_from_cstring("98765421");
        RuneString res = rstring_trim_right(str, chars);
        assert(eq(res, "привет3"));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("приветхохох");
        RuneString chars = rstring_from_cstring("ох");
        RuneString res = rstring_trim_right(str, chars);
        assert(eq(res, "привет"));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("привет");
        RuneString chars = rstring_from_cstring(" ");
        RuneString res = rstring_trim_right(str, chars);
        assert(eq(res, "привет"));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("   ");
        RuneString chars = rstring_from_cstring(" ");
        RuneString res = rstring_trim_right(str, chars);
        assert(eq(res, ""));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("");
        RuneString chars = rstring_from_cstring(" ");
        RuneString res = rstring_trim_right(str, chars);
        assert(eq(res, ""));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    printf("OK\n");
}

static void test_trim(void) {
    printf("test_trim...");
    {
        RuneString str = rstring_from_cstring("   привет");
        RuneString chars = rstring_from_cstring(" ");
        RuneString res = rstring_trim(str, chars);
        assert(eq(res, "привет"));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("273привет");
        RuneString chars = rstring_from_cstring("987654321");
        RuneString res = rstring_trim(str, chars);
        assert(eq(res, "привет"));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("273привет");
        RuneString chars = rstring_from_cstring("98765421");
        RuneString res = rstring_trim(str, chars);
        assert(eq(res, "3привет"));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("хохохпривет");
        RuneString chars = rstring_from_cstring("ох");
        RuneString res = rstring_trim(str, chars);
        assert(eq(res, "привет"));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("привет");
        RuneString chars = rstring_from_cstring(" ");
        RuneString res = rstring_trim(str, chars);
        assert(eq(res, "привет"));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("   ");
        RuneString chars = rstring_from_cstring(" ");
        RuneString res = rstring_trim(str, chars);
        assert(eq(res, ""));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("");
        RuneString chars = rstring_from_cstring(" ");
        RuneString res = rstring_trim(str, chars);
        assert(eq(res, ""));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("привет   ");
        RuneString chars = rstring_from_cstring(" ");
        RuneString res = rstring_trim(str, chars);
        assert(eq(res, "привет"));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("привет372");
        RuneString chars = rstring_from_cstring("987654321");
        RuneString res = rstring_trim(str, chars);
        assert(eq(res, "привет"));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("привет372");
        RuneString chars = rstring_from_cstring("98765421");
        RuneString res = rstring_trim(str, chars);
        assert(eq(res, "привет3"));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("приветхохох");
        RuneString chars = rstring_from_cstring("ох");
        RuneString res = rstring_trim(str, chars);
        assert(eq(res, "привет"));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("привет");
        RuneString chars = rstring_from_cstring(" ");
        RuneString res = rstring_trim(str, chars);
        assert(eq(res, "привет"));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("");
        RuneString chars = rstring_from_cstring(" ");
        RuneString res = rstring_trim(str, chars);
        assert(eq(res, ""));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("   привет  ");
        RuneString chars = rstring_from_cstring(" ");
        RuneString res = rstring_trim(str, chars);
        assert(eq(res, "привет"));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("19привет372");
        RuneString chars = rstring_from_cstring("987654321");
        RuneString res = rstring_trim(str, chars);
        assert(eq(res, "привет"));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("139привет372");
        RuneString chars = rstring_from_cstring("98765421");
        RuneString res = rstring_trim(str, chars);
        assert(eq(res, "39привет3"));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("хохохприветххх");
        RuneString chars = rstring_from_cstring("ох");
        RuneString res = rstring_trim(str, chars);
        assert(eq(res, "привет"));
        rstring_free(str);
        rstring_free(chars);
        rstring_free(res);
    }
    printf("OK\n");
}

static void test_pad_left(void) {
    printf("test_pad_left...");
    {
        RuneString str = rstring_from_cstring("hello");
        RuneString fill = rstring_from_cstring("0");
        RuneString res = rstring_pad_left(str, 8, fill);
        assert(eq(res, "000hello"));
        rstring_free(str);
        rstring_free(fill);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("hello");
        RuneString fill = rstring_from_cstring("xo");
        RuneString res = rstring_pad_left(str, 8, fill);
        assert(eq(res, "xoxhello"));
        rstring_free(str);
        rstring_free(fill);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("hello");
        RuneString fill = rstring_from_cstring("★");
        RuneString res = rstring_pad_left(str, 8, fill);
        assert(eq(res, "★★★hello"));
        rstring_free(str);
        rstring_free(fill);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("привет");
        RuneString fill = rstring_from_cstring(" ");
        RuneString res = rstring_pad_left(str, 8, fill);
        assert(eq(res, "  привет"));
        rstring_free(str);
        rstring_free(fill);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("привет");
        RuneString fill = rstring_from_cstring("★");
        RuneString res = rstring_pad_left(str, 8, fill);
        assert(eq(res, "★★привет"));
        rstring_free(str);
        rstring_free(fill);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("привет");
        RuneString fill = rstring_from_cstring("хо");
        RuneString res = rstring_pad_left(str, 9, fill);
        assert(eq(res, "хохпривет"));
        rstring_free(str);
        rstring_free(fill);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("привет");
        RuneString fill = rstring_from_cstring("★");
        RuneString res = rstring_pad_left(str, 6, fill);
        assert(eq(res, "привет"));
        rstring_free(str);
        rstring_free(fill);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("привет");
        RuneString fill = rstring_from_cstring("★");
        RuneString res = rstring_pad_left(str, 4, fill);
        assert(eq(res, "прив"));
        rstring_free(str);
        rstring_free(fill);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("привет");
        RuneString fill = rstring_from_cstring("★");
        RuneString res = rstring_pad_left(str, 0, fill);
        assert(eq(res, ""));
        rstring_free(str);
        rstring_free(fill);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("привет");
        RuneString fill = rstring_from_cstring("");
        RuneString res = rstring_pad_left(str, 8, fill);
        assert(eq(res, "привет"));
        rstring_free(str);
        rstring_free(fill);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("");
        RuneString fill = rstring_from_cstring("★");
        RuneString res = rstring_pad_left(str, 5, fill);
        assert(eq(res, "★★★★★"));
        rstring_free(str);
        rstring_free(fill);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("");
        RuneString fill = rstring_from_cstring("");
        RuneString res = rstring_pad_left(str, 5, fill);
        assert(eq(res, ""));
        rstring_free(str);
        rstring_free(fill);
        rstring_free(res);
    }

    printf("OK\n");
}

static void test_pad_right(void) {
    printf("test_pad_right...");
    {
        RuneString str = rstring_from_cstring("hello");
        RuneString fill = rstring_from_cstring("0");
        RuneString res = rstring_pad_right(str, 8, fill);
        assert(eq(res, "hello000"));
        rstring_free(str);
        rstring_free(fill);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("hello");
        RuneString fill = rstring_from_cstring("xo");
        RuneString res = rstring_pad_right(str, 8, fill);
        assert(eq(res, "helloxox"));
        rstring_free(str);
        rstring_free(fill);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("hello");
        RuneString fill = rstring_from_cstring("★");
        RuneString res = rstring_pad_right(str, 8, fill);
        assert(eq(res, "hello★★★"));
        rstring_free(str);
        rstring_free(fill);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("привет");
        RuneString fill = rstring_from_cstring(" ");
        RuneString res = rstring_pad_right(str, 8, fill);
        assert(eq(res, "привет  "));
        rstring_free(str);
        rstring_free(fill);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("привет");
        RuneString fill = rstring_from_cstring("★");
        RuneString res = rstring_pad_right(str, 8, fill);
        assert(eq(res, "привет★★"));
        rstring_free(str);
        rstring_free(fill);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("привет");
        RuneString fill = rstring_from_cstring("хо");
        RuneString res = rstring_pad_right(str, 9, fill);
        assert(eq(res, "приветхох"));
        rstring_free(str);
        rstring_free(fill);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("привет");
        RuneString fill = rstring_from_cstring("★");
        RuneString res = rstring_pad_right(str, 6, fill);
        assert(eq(res, "привет"));
        rstring_free(str);
        rstring_free(fill);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("привет");
        RuneString fill = rstring_from_cstring("★");
        RuneString res = rstring_pad_right(str, 4, fill);
        assert(eq(res, "прив"));
        rstring_free(str);
        rstring_free(fill);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("привет");
        RuneString fill = rstring_from_cstring("★");
        RuneString res = rstring_pad_right(str, 0, fill);
        assert(eq(res, ""));
        rstring_free(str);
        rstring_free(fill);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("привет");
        RuneString fill = rstring_from_cstring("");
        RuneString res = rstring_pad_right(str, 8, fill);
        assert(eq(res, "привет"));
        rstring_free(str);
        rstring_free(fill);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("");
        RuneString fill = rstring_from_cstring("★");
        RuneString res = rstring_pad_right(str, 5, fill);
        assert(eq(res, "★★★★★"));
        rstring_free(str);
        rstring_free(fill);
        rstring_free(res);
    }
    {
        RuneString str = rstring_from_cstring("");
        RuneString fill = rstring_from_cstring("");
        RuneString res = rstring_pad_right(str, 5, fill);
        assert(eq(res, ""));
        rstring_free(str);
        rstring_free(fill);
        rstring_free(res);
    }

    printf("OK\n");
}

static void test_like(void) {
    printf("test_like...");
    struct test {
        const char* pattern;
        const char* str;
        bool match;
    };
    const struct test tests[] = {
        {"%", "H", true},
        {"_", "H", true},
        {"H%", "Hi", true},
        {"H_", "Hi", true},
        {"%i", "Hi", true},
        {"_%", "Hi", true},
        {"%", "Hello", true},
        {"H_", "Ho", true},
        {"%llo", "Hello", true},
        {"H%o", "Hello", true},
        {"H_l_o", "Halo", false},
        {"%o, world", "Hello, world", true},
        {"% world", "Hello, world", true},
        {"Hel%rld", "Hello, world", true},
        {"H%lo, w%ld", "Hello, world", true},
        {"Hel_o, w__ld", "Hello, world", true},
        {"H%l_, w%ld", "Hello, world", true},
        {"H%l_, w%ld.", "Hello, world!", false},
        {"HeLLo, WoRlD", "Hello, world", true},
        {"%world", "Hello, world", true},
        {"H_llo, w_rld", "Hello, world", true},
        {"H__lo, w__ld", "Hello, world", true},
        {"H%world", "Hello, world", true},
        {"Hello, %d", "Hello, world", true},
        {"%o, w%ld", "Hello, world", true},
        {"H%lo, w%rld", "Hello, world", true},
        {"H_llo, w_rld.", "Hello, world!", false},
        {"He%o, wo%ld", "Hello, world", true},
        {"He%o, wo%ld.", "Hello, world!", false},
        {"Hello, world", "Hello, world", true},
        {"%ello, %orld", "Hello, world", true},
        {"H__lo, w___d", "Hello, world", true},
        {"H____, w____", "Hello, world", true},
        {"_ello, _orld", "Hello, world", true},
        {"H_llo, w__ld", "Hello, world!", false},
        {"Hello, world%", "Hello, world", true},
        {"Hello, world%11", "Hello, world", false},
        {"H%lo, w%ld%", "Hello, world", true},
        {"%", "", true},
        {"%", "a", true},
        {"_", "", false},
        {"_", "a", true},
        {"_%", "ab", true},
        {"a%", "ab", true},
        {"a_", "ab", true},
        {"a%z", "abcdefghijklmnopqrstuvwxyz", true},
        {"%bcdefghijklmnopqrstuvwxyz", "abcdefghijklmnopqrstuvwxyz", true},
        {"a%y", "abcdefghijklmnopqrstuvwyz", false},
        {"%mnopqrst%", "abcdefghijklmnopqrstuvwyz", true},
        {"a%z", "ab", false},
        {"_b%", "ab", true},
        {"%c%", "abc", true},
        {"a_c", "abc", true},
        {"%bc", "abc", true}
        // test cases
    };
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i) {
        const struct test* t = &tests[i];
        RuneString pattern = rstring_from_cstring(t->pattern);
        RuneString str = rstring_from_cstring(t->str);
        // printf("pattern: %s, s: %s, match: %d\n", t->pattern, t->s, t->match);
        assert(rstring_like(pattern, str) == t->match);
    }
    printf("OK\n");
}

int main(void) {
    test_cstring();
    test_at();
    test_slice();
    test_substring();
    test_index();
    test_last_index();
    test_translate();
    test_reverse();
    test_trim_left();
    test_trim_right();
    test_trim();
    test_pad_left();
    test_pad_right();
    test_like();
    return 0;
}
