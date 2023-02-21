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
        RuneString other = rstring.from_cstring("и");
        int index = rstring.index(str, other);
        assert(index == 2);
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
        RuneString str = rstring.new();
        RuneString res = rstring.reverse(str);
        assert(eq(res, ""));
        rstring.free(str);
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
    return 0;
}
