// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/text/text.h"

static bool eq(RuneString str, const char* expected) {
    char* got = rstring.to_cstring(str);
    bool eq = strcmp(got, expected) == 0;
    free(got);
    return eq;
}

static void test_cstring(void) {
    printf("test_cstring...");
    RuneString str = rstring.from_cstring("привет мир");
    assert(eq(str, "привет мир"));
    rstring.free(str);
    printf("OK\n");
}

static void test_at(void) {
    printf("test_at...");
    RuneString str = rstring.from_cstring("привет мир");
    int32_t rune = rstring.at(str, 2);
    assert(rune == 1080);
    rstring.free(str);
    printf("OK\n");
}

static void test_slice(void) {
    printf("test_slice...");
    RuneString str = rstring.from_cstring("привет мир");

    {
        RuneString slice = rstring.slice(str, 7, 10);
        assert(eq(slice, "мир"));
        rstring.free(slice);
    }

    {
        RuneString slice = rstring.slice(str, 0, 6);
        assert(eq(slice, "привет"));
        rstring.free(slice);
    }

    {
        RuneString slice = rstring.slice(str, -3, str.length);
        assert(eq(slice, "мир"));
        rstring.free(slice);
    }

    {
        RuneString slice = rstring.slice(str, 3, 3);
        assert(eq(slice, ""));
        rstring.free(slice);
    }

    rstring.free(str);
    printf("OK\n");
}

static void test_substring(void) {
    printf("test_substring...");
    RuneString str = rstring.from_cstring("привет мир");

    {
        RuneString slice = rstring.substring(str, 7, 3);
        assert(eq(slice, "мир"));
        rstring.free(slice);
    }

    {
        RuneString slice = rstring.substring(str, 0, 6);
        assert(eq(slice, "привет"));
        rstring.free(slice);
    }

    {
        RuneString slice = rstring.substring(str, 0, str.length);
        assert(eq(slice, "привет мир"));
        rstring.free(slice);
    }

    {
        RuneString slice = rstring.substring(str, 7, str.length);
        assert(eq(slice, "мир"));
        rstring.free(slice);
    }

    {
        RuneString slice = rstring.substring(str, 1, 1);
        assert(eq(slice, "р"));
        rstring.free(slice);
    }

    {
        RuneString slice = rstring.substring(str, 1, 0);
        assert(eq(slice, ""));
        rstring.free(slice);
    }

    rstring.free(str);
    printf("OK\n");
}

static void test_index(void) {
    printf("test_index...");
    RuneString str = rstring.from_cstring("привет мир");

    {
        RuneString other = rstring.from_cstring("пр");
        int index = rstring.index(str, other);
        assert(index == 0);
        rstring.free(other);
    }

    {
        RuneString other = rstring.from_cstring("и");
        int index = rstring.index(str, other);
        assert(index == 2);
        rstring.free(other);
    }

    {
        RuneString other = rstring.from_cstring("ми");
        int index = rstring.index(str, other);
        assert(index == 7);
        rstring.free(other);
    }

    {
        RuneString other = rstring.from_cstring("ир");
        int index = rstring.index(str, other);
        assert(index == 8);
        rstring.free(other);
    }

    {
        RuneString other = rstring.from_cstring("ирк");
        int index = rstring.index(str, other);
        assert(index == -1);
        rstring.free(other);
    }

    {
        RuneString str = rstring.from_cstring("привет миф");
        RuneString other = rstring.from_cstring("ф");
        int index = rstring.index(str, other);
        assert(index == 9);
        rstring.free(other);
    }

    {
        RuneString other = rstring.from_cstring("р ");
        int index = rstring.index(str, other);
        assert(index == -1);
        rstring.free(other);
    }

    rstring.free(str);
    printf("OK\n");
}

static void test_last_index(void) {
    printf("test_last_index...");
    RuneString str = rstring.from_cstring("привет мир");

    {
        RuneString other = rstring.from_cstring("и");
        int index = rstring.last_index(str, other);
        assert(index == 8);
        rstring.free(other);
    }

    {
        RuneString other = rstring.from_cstring("при");
        int index = rstring.last_index(str, other);
        assert(index == 0);
        rstring.free(other);
    }

    {
        RuneString other = rstring.from_cstring("ирк");
        int index = rstring.last_index(str, other);
        assert(index == -1);
        rstring.free(other);
    }

    rstring.free(str);
    printf("OK\n");
}

static void test_reverse(void) {
    printf("test_reverse...");
    {
        RuneString str = rstring.from_cstring("привет");
        RuneString res = rstring.reverse(str);
        assert(eq(res, "тевирп"));
        rstring.free(str);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("привет мир");
        RuneString res = rstring.reverse(str);
        assert(eq(res, "рим тевирп"));
        rstring.free(str);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("𐌀𐌁𐌂");
        RuneString res = rstring.reverse(str);
        assert(eq(res, "𐌂𐌁𐌀"));
        rstring.free(str);
        rstring.free(res);
    }
    {
        RuneString str = rstring.new();
        RuneString res = rstring.reverse(str);
        assert(eq(res, ""));
        rstring.free(str);
        rstring.free(res);
    }
    printf("OK\n");
}

static void test_trim_left(void) {
    printf("test_trim_left...");
    {
        RuneString str = rstring.from_cstring("   привет");
        RuneString chars = rstring.from_cstring(" ");
        RuneString res = rstring.trim_left(str, chars);
        assert(eq(res, "привет"));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("273привет");
        RuneString chars = rstring.from_cstring("987654321");
        RuneString res = rstring.trim_left(str, chars);
        assert(eq(res, "привет"));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("273привет");
        RuneString chars = rstring.from_cstring("98765421");
        RuneString res = rstring.trim_left(str, chars);
        assert(eq(res, "3привет"));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("хохохпривет");
        RuneString chars = rstring.from_cstring("ох");
        RuneString res = rstring.trim_left(str, chars);
        assert(eq(res, "привет"));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("привет");
        RuneString chars = rstring.from_cstring(" ");
        RuneString res = rstring.trim_left(str, chars);
        assert(eq(res, "привет"));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("   ");
        RuneString chars = rstring.from_cstring(" ");
        RuneString res = rstring.trim_left(str, chars);
        assert(eq(res, ""));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("");
        RuneString chars = rstring.from_cstring(" ");
        RuneString res = rstring.trim_left(str, chars);
        assert(eq(res, ""));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    printf("OK\n");
}

static void test_trim_right(void) {
    printf("test_trim_right...");
    {
        RuneString str = rstring.from_cstring("привет   ");
        RuneString chars = rstring.from_cstring(" ");
        RuneString res = rstring.trim_right(str, chars);
        assert(eq(res, "привет"));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("привет372");
        RuneString chars = rstring.from_cstring("987654321");
        RuneString res = rstring.trim_right(str, chars);
        assert(eq(res, "привет"));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("привет372");
        RuneString chars = rstring.from_cstring("98765421");
        RuneString res = rstring.trim_right(str, chars);
        assert(eq(res, "привет3"));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("приветхохох");
        RuneString chars = rstring.from_cstring("ох");
        RuneString res = rstring.trim_right(str, chars);
        assert(eq(res, "привет"));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("привет");
        RuneString chars = rstring.from_cstring(" ");
        RuneString res = rstring.trim_right(str, chars);
        assert(eq(res, "привет"));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("   ");
        RuneString chars = rstring.from_cstring(" ");
        RuneString res = rstring.trim_right(str, chars);
        assert(eq(res, ""));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("");
        RuneString chars = rstring.from_cstring(" ");
        RuneString res = rstring.trim_right(str, chars);
        assert(eq(res, ""));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    printf("OK\n");
}

static void test_trim(void) {
    printf("test_trim...");
    {
        RuneString str = rstring.from_cstring("   привет");
        RuneString chars = rstring.from_cstring(" ");
        RuneString res = rstring.trim(str, chars);
        assert(eq(res, "привет"));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("273привет");
        RuneString chars = rstring.from_cstring("987654321");
        RuneString res = rstring.trim(str, chars);
        assert(eq(res, "привет"));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("273привет");
        RuneString chars = rstring.from_cstring("98765421");
        RuneString res = rstring.trim(str, chars);
        assert(eq(res, "3привет"));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("хохохпривет");
        RuneString chars = rstring.from_cstring("ох");
        RuneString res = rstring.trim(str, chars);
        assert(eq(res, "привет"));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("привет");
        RuneString chars = rstring.from_cstring(" ");
        RuneString res = rstring.trim(str, chars);
        assert(eq(res, "привет"));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("   ");
        RuneString chars = rstring.from_cstring(" ");
        RuneString res = rstring.trim(str, chars);
        assert(eq(res, ""));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("");
        RuneString chars = rstring.from_cstring(" ");
        RuneString res = rstring.trim(str, chars);
        assert(eq(res, ""));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("привет   ");
        RuneString chars = rstring.from_cstring(" ");
        RuneString res = rstring.trim(str, chars);
        assert(eq(res, "привет"));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("привет372");
        RuneString chars = rstring.from_cstring("987654321");
        RuneString res = rstring.trim(str, chars);
        assert(eq(res, "привет"));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("привет372");
        RuneString chars = rstring.from_cstring("98765421");
        RuneString res = rstring.trim(str, chars);
        assert(eq(res, "привет3"));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("приветхохох");
        RuneString chars = rstring.from_cstring("ох");
        RuneString res = rstring.trim(str, chars);
        assert(eq(res, "привет"));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("привет");
        RuneString chars = rstring.from_cstring(" ");
        RuneString res = rstring.trim(str, chars);
        assert(eq(res, "привет"));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("");
        RuneString chars = rstring.from_cstring(" ");
        RuneString res = rstring.trim(str, chars);
        assert(eq(res, ""));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("   привет  ");
        RuneString chars = rstring.from_cstring(" ");
        RuneString res = rstring.trim(str, chars);
        assert(eq(res, "привет"));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("19привет372");
        RuneString chars = rstring.from_cstring("987654321");
        RuneString res = rstring.trim(str, chars);
        assert(eq(res, "привет"));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("139привет372");
        RuneString chars = rstring.from_cstring("98765421");
        RuneString res = rstring.trim(str, chars);
        assert(eq(res, "39привет3"));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("хохохприветххх");
        RuneString chars = rstring.from_cstring("ох");
        RuneString res = rstring.trim(str, chars);
        assert(eq(res, "привет"));
        rstring.free(str);
        rstring.free(chars);
        rstring.free(res);
    }
    printf("OK\n");
}

static void test_pad_left(void) {
    printf("test_pad_left...");
    {
        RuneString str = rstring.from_cstring("hello");
        RuneString fill = rstring.from_cstring("0");
        RuneString res = rstring.pad_left(str, 8, fill);
        assert(eq(res, "000hello"));
        rstring.free(str);
        rstring.free(fill);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("hello");
        RuneString fill = rstring.from_cstring("xo");
        RuneString res = rstring.pad_left(str, 8, fill);
        assert(eq(res, "xoxhello"));
        rstring.free(str);
        rstring.free(fill);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("hello");
        RuneString fill = rstring.from_cstring("★");
        RuneString res = rstring.pad_left(str, 8, fill);
        assert(eq(res, "★★★hello"));
        rstring.free(str);
        rstring.free(fill);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("привет");
        RuneString fill = rstring.from_cstring(" ");
        RuneString res = rstring.pad_left(str, 8, fill);
        assert(eq(res, "  привет"));
        rstring.free(str);
        rstring.free(fill);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("привет");
        RuneString fill = rstring.from_cstring("★");
        RuneString res = rstring.pad_left(str, 8, fill);
        assert(eq(res, "★★привет"));
        rstring.free(str);
        rstring.free(fill);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("привет");
        RuneString fill = rstring.from_cstring("хо");
        RuneString res = rstring.pad_left(str, 9, fill);
        assert(eq(res, "хохпривет"));
        rstring.free(str);
        rstring.free(fill);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("привет");
        RuneString fill = rstring.from_cstring("★");
        RuneString res = rstring.pad_left(str, 6, fill);
        assert(eq(res, "привет"));
        rstring.free(str);
        rstring.free(fill);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("привет");
        RuneString fill = rstring.from_cstring("★");
        RuneString res = rstring.pad_left(str, 4, fill);
        assert(eq(res, "прив"));
        rstring.free(str);
        rstring.free(fill);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("привет");
        RuneString fill = rstring.from_cstring("★");
        RuneString res = rstring.pad_left(str, 0, fill);
        assert(eq(res, ""));
        rstring.free(str);
        rstring.free(fill);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("привет");
        RuneString fill = rstring.from_cstring("");
        RuneString res = rstring.pad_left(str, 8, fill);
        assert(eq(res, "привет"));
        rstring.free(str);
        rstring.free(fill);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("");
        RuneString fill = rstring.from_cstring("★");
        RuneString res = rstring.pad_left(str, 5, fill);
        assert(eq(res, "★★★★★"));
        rstring.free(str);
        rstring.free(fill);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("");
        RuneString fill = rstring.from_cstring("");
        RuneString res = rstring.pad_left(str, 5, fill);
        assert(eq(res, ""));
        rstring.free(str);
        rstring.free(fill);
        rstring.free(res);
    }

    printf("OK\n");
}

static void test_pad_right(void) {
    printf("test_pad_right...");
    {
        RuneString str = rstring.from_cstring("hello");
        RuneString fill = rstring.from_cstring("0");
        RuneString res = rstring.pad_right(str, 8, fill);
        assert(eq(res, "hello000"));
        rstring.free(str);
        rstring.free(fill);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("hello");
        RuneString fill = rstring.from_cstring("xo");
        RuneString res = rstring.pad_right(str, 8, fill);
        assert(eq(res, "helloxox"));
        rstring.free(str);
        rstring.free(fill);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("hello");
        RuneString fill = rstring.from_cstring("★");
        RuneString res = rstring.pad_right(str, 8, fill);
        assert(eq(res, "hello★★★"));
        rstring.free(str);
        rstring.free(fill);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("привет");
        RuneString fill = rstring.from_cstring(" ");
        RuneString res = rstring.pad_right(str, 8, fill);
        assert(eq(res, "привет  "));
        rstring.free(str);
        rstring.free(fill);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("привет");
        RuneString fill = rstring.from_cstring("★");
        RuneString res = rstring.pad_right(str, 8, fill);
        assert(eq(res, "привет★★"));
        rstring.free(str);
        rstring.free(fill);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("привет");
        RuneString fill = rstring.from_cstring("хо");
        RuneString res = rstring.pad_right(str, 9, fill);
        assert(eq(res, "приветхох"));
        rstring.free(str);
        rstring.free(fill);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("привет");
        RuneString fill = rstring.from_cstring("★");
        RuneString res = rstring.pad_right(str, 6, fill);
        assert(eq(res, "привет"));
        rstring.free(str);
        rstring.free(fill);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("привет");
        RuneString fill = rstring.from_cstring("★");
        RuneString res = rstring.pad_right(str, 4, fill);
        assert(eq(res, "прив"));
        rstring.free(str);
        rstring.free(fill);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("привет");
        RuneString fill = rstring.from_cstring("★");
        RuneString res = rstring.pad_right(str, 0, fill);
        assert(eq(res, ""));
        rstring.free(str);
        rstring.free(fill);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("привет");
        RuneString fill = rstring.from_cstring("");
        RuneString res = rstring.pad_right(str, 8, fill);
        assert(eq(res, "привет"));
        rstring.free(str);
        rstring.free(fill);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("");
        RuneString fill = rstring.from_cstring("★");
        RuneString res = rstring.pad_right(str, 5, fill);
        assert(eq(res, "★★★★★"));
        rstring.free(str);
        rstring.free(fill);
        rstring.free(res);
    }
    {
        RuneString str = rstring.from_cstring("");
        RuneString fill = rstring.from_cstring("");
        RuneString res = rstring.pad_right(str, 5, fill);
        assert(eq(res, ""));
        rstring.free(str);
        rstring.free(fill);
        rstring.free(res);
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
    test_reverse();
    test_trim_left();
    test_trim_right();
    test_trim();
    test_pad_left();
    test_pad_right();
    return 0;
}
