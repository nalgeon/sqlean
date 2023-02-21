// Copyright (c) 2023 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/text/text.h"

static bool eq(ByteString str, const char* expected) {
    return strcmp(bstring.to_cstring(str), expected) == 0;
}

static void test_cstring(void) {
    printf("test_cstring...");
    ByteString str = bstring.from_cstring("hello world", 11);
    assert(eq(str, "hello world"));
    bstring.free(str);
    printf("OK\n");
}

static void test_at(void) {
    printf("test_at...");
    ByteString str = bstring.from_cstring("hello world", 11);
    char chr = bstring.at(str, 2);
    assert(chr == 'l');
    bstring.free(str);
    printf("OK\n");
}

static void test_slice(void) {
    printf("test_slice...");
    ByteString str = bstring.from_cstring("hello world", 11);

    {
        ByteString slice = bstring.slice(str, 6, 11);
        assert(eq(slice, "world"));
        bstring.free(slice);
    }

    {
        ByteString slice = bstring.slice(str, 0, 5);
        assert(eq(slice, "hello"));
        bstring.free(slice);
    }

    {
        ByteString slice = bstring.slice(str, -5, str.length);
        assert(eq(slice, "world"));
        bstring.free(slice);
    }

    bstring.free(str);
    printf("OK\n");
}

static void test_substring(void) {
    printf("test_substring...");
    ByteString str = bstring.from_cstring("hello world", 11);

    {
        ByteString slice = bstring.substring(str, 6, 5);
        assert(eq(slice, "world"));
        bstring.free(slice);
    }

    {
        ByteString slice = bstring.substring(str, 0, 5);
        assert(eq(slice, "hello"));
        bstring.free(slice);
    }

    {
        ByteString slice = bstring.substring(str, 0, str.length);
        assert(eq(slice, "hello world"));
        bstring.free(slice);
    }

    {
        ByteString slice = bstring.substring(str, 1, 1);
        assert(eq(slice, "e"));
        bstring.free(slice);
    }

    {
        ByteString slice = bstring.substring(str, 1, 0);
        assert(eq(slice, ""));
        bstring.free(slice);
    }

    bstring.free(str);
    printf("OK\n");
}

static void test_index(void) {
    printf("test_index...");
    ByteString str = bstring.from_cstring("hello world", 11);

    {
        ByteString other = bstring.from_cstring("l", 1);
        int index = bstring.index(str, other);
        assert(index == 2);
        bstring.free(other);
    }

    {
        ByteString other = bstring.from_cstring("rl", 2);
        int index = bstring.index(str, other);
        assert(index == 8);
        bstring.free(other);
    }

    {
        ByteString other = bstring.from_cstring("rlc", 3);
        int index = bstring.index(str, other);
        assert(index == -1);
        bstring.free(other);
    }

    {
        ByteString other = bstring.from_cstring("", 0);
        int index = bstring.index(str, other);
        assert(index == 0);
        bstring.free(other);
    }

    bstring.free(str);
    printf("OK\n");
}

static void test_last_index(void) {
    printf("test_last_index...");
    ByteString str = bstring.from_cstring("hello world", 11);

    {
        ByteString other = bstring.from_cstring("l", 1);
        int index = bstring.last_index(str, other);
        assert(index == 9);
        bstring.free(other);
    }

    {
        ByteString other = bstring.from_cstring("hel", 3);
        int index = bstring.last_index(str, other);
        assert(index == 0);
        bstring.free(other);
    }

    {
        ByteString other = bstring.from_cstring("rla", 3);
        int index = bstring.last_index(str, other);
        assert(index == -1);
        bstring.free(other);
    }

    {
        ByteString other = bstring.from_cstring("", 0);
        int index = bstring.last_index(str, other);
        assert(index == 10);
        bstring.free(other);
    }

    bstring.free(str);
    printf("OK\n");
}

static void test_contains(void) {
    printf("test_contains...");
    ByteString str = bstring.from_cstring("hello world", 11);

    {
        ByteString other = bstring.from_cstring("l", 1);
        bool ok = bstring.contains(str, other);
        assert(ok);
        bstring.free(other);
    }

    {
        ByteString other = bstring.from_cstring("ld", 2);
        bool ok = bstring.contains(str, other);
        assert(ok);
        bstring.free(other);
    }

    {
        ByteString other = bstring.from_cstring("lda", 3);
        bool ok = bstring.contains(str, other);
        assert(!ok);
        bstring.free(other);
    }

    bstring.free(str);
    printf("OK\n");
}

static void test_equals(void) {
    printf("test_equals...");

    {
        ByteString s1 = bstring.from_cstring("hello world", 11);
        ByteString s2 = bstring.from_cstring("hello world", 11);
        assert(bstring.equals(s1, s2));
        bstring.free(s1);
        bstring.free(s2);
    }

    {
        ByteString s1 = bstring.from_cstring("hello world", 11);
        ByteString s2 = bstring.from_cstring("world", 5);
        assert(!bstring.equals(s1, s2));
        bstring.free(s1);
        bstring.free(s2);
    }

    {
        ByteString s1 = bstring.from_cstring("hello world", 11);
        ByteString s2 = bstring.from_cstring("", 0);
        assert(!bstring.equals(s1, s2));
        bstring.free(s1);
        bstring.free(s2);
    }

    {
        ByteString s1 = bstring.from_cstring("hello world", 11);
        ByteString s2 = bstring.new();
        assert(!bstring.equals(s1, s2));
        bstring.free(s1);
        bstring.free(s2);
    }

    {
        ByteString s1 = bstring.from_cstring("", 0);
        ByteString s2 = bstring.from_cstring("", 0);
        assert(bstring.equals(s1, s2));
        bstring.free(s1);
        bstring.free(s2);
    }

    {
        ByteString s1 = bstring.new();
        ByteString s2 = bstring.new();
        assert(bstring.equals(s1, s2));
        bstring.free(s1);
        bstring.free(s2);
    }

    printf("OK\n");
}

static void test_has_prefix(void) {
    printf("test_has_prefix...");

    {
        ByteString str = bstring.from_cstring("hello world", 11);
        ByteString other = bstring.from_cstring("hell", 4);
        bool ok = bstring.has_prefix(str, other);
        assert(ok);
        bstring.free(str);
        bstring.free(other);
    }

    {
        ByteString str = bstring.from_cstring("hello world", 11);
        ByteString other = bstring.from_cstring("ello", 4);
        bool ok = bstring.has_prefix(str, other);
        assert(!ok);
        bstring.free(str);
        bstring.free(other);
    }

    {
        ByteString str = bstring.from_cstring("hello world", 11);
        ByteString other = bstring.from_cstring("", 0);
        bool ok = bstring.has_prefix(str, other);
        assert(ok);
        bstring.free(str);
        bstring.free(other);
    }

    {
        ByteString str = bstring.from_cstring("", 0);
        ByteString other = bstring.from_cstring("hell", 4);
        bool ok = bstring.has_prefix(str, other);
        assert(!ok);
        bstring.free(str);
        bstring.free(other);
    }

    printf("OK\n");
}

static void test_has_suffix(void) {
    printf("test_has_suffix...");

    {
        ByteString str = bstring.from_cstring("hello world", 11);
        ByteString other = bstring.from_cstring("orld", 4);
        bool ok = bstring.has_suffix(str, other);
        assert(ok);
        bstring.free(str);
        bstring.free(other);
    }

    {
        ByteString str = bstring.from_cstring("hello world", 11);
        ByteString other = bstring.from_cstring("d", 1);
        bool ok = bstring.has_suffix(str, other);
        assert(ok);
        bstring.free(str);
        bstring.free(other);
    }

    {
        ByteString str = bstring.from_cstring("hello world", 11);
        ByteString other = bstring.from_cstring("worl", 4);
        bool ok = bstring.has_suffix(str, other);
        assert(!ok);
        bstring.free(str);
        bstring.free(other);
    }

    {
        ByteString str = bstring.from_cstring("hello world", 11);
        ByteString other = bstring.from_cstring("", 0);
        bool ok = bstring.has_suffix(str, other);
        assert(ok);
        bstring.free(str);
        bstring.free(other);
    }

    {
        ByteString str = bstring.from_cstring("", 0);
        ByteString other = bstring.from_cstring("hell", 4);
        bool ok = bstring.has_suffix(str, other);
        assert(!ok);
        bstring.free(str);
        bstring.free(other);
    }

    printf("OK\n");
}

static void test_count(void) {
    printf("test_count...");
    ByteString str = bstring.from_cstring("hello world", 11);

    ByteString other = bstring.from_cstring("l", 1);
    int count = bstring.count(str, other);
    assert(count == 3);
    bstring.free(other);

    other = bstring.from_cstring("ld", 2);
    count = bstring.count(str, other);
    assert(count == 1);
    bstring.free(other);

    other = bstring.from_cstring("lda", 3);
    count = bstring.count(str, other);
    assert(count == 0);
    bstring.free(other);

    bstring.free(str);
    printf("OK\n");
}

static void test_split_part(void) {
    printf("test_split_part...");
    {
        ByteString str = bstring.from_cstring("one,two,thr", 11);
        ByteString sep = bstring.from_cstring(",", 1);
        ByteString part = bstring.split_part(str, sep, 1);
        assert(eq(part, "two"));
        bstring.free(part);
        bstring.free(str);
        bstring.free(sep);
    }
    {
        ByteString str = bstring.from_cstring("one,two,thr", 11);
        ByteString sep = bstring.from_cstring(",", 1);
        ByteString part = bstring.split_part(str, sep, 2);
        assert(eq(part, "thr"));
        bstring.free(part);
        bstring.free(str);
        bstring.free(sep);
    }
    {
        ByteString str = bstring.from_cstring("one,two,thr", 11);
        ByteString sep = bstring.from_cstring(",", 1);
        ByteString part = bstring.split_part(str, sep, 3);
        assert(eq(part, ""));
        bstring.free(part);
        bstring.free(str);
        bstring.free(sep);
    }
    {
        ByteString str = bstring.from_cstring("one,two,thr", 11);
        ByteString sep = bstring.from_cstring(";", 1);
        ByteString part = bstring.split_part(str, sep, 1);
        assert(eq(part, ""));
        bstring.free(part);
        bstring.free(str);
        bstring.free(sep);
    }
    {
        ByteString str = bstring.from_cstring("", 0);
        ByteString sep = bstring.from_cstring(",", 1);
        ByteString part = bstring.split_part(str, sep, 1);
        assert(eq(part, ""));
        bstring.free(part);
        bstring.free(str);
        bstring.free(sep);
    }
    {
        const char* s = "один,два,три";
        ByteString str = bstring.from_cstring(s, strlen(s));
        ByteString sep = bstring.from_cstring(",", 1);
        ByteString part = bstring.split_part(str, sep, 1);
        assert(eq(part, "два"));
        bstring.free(part);
        bstring.free(str);
        bstring.free(sep);
    }
    {
        const char* s = "один-*-два-*-три";
        ByteString str = bstring.from_cstring(s, strlen(s));
        ByteString sep = bstring.from_cstring("-*-", 3);
        ByteString part = bstring.split_part(str, sep, 1);
        assert(eq(part, "два"));
        bstring.free(part);
        bstring.free(str);
        bstring.free(sep);
    }
    printf("OK\n");
}

static void test_join(void) {
    printf("test_join...");

    ByteString s1 = bstring.from_cstring("one", 3);
    ByteString s2 = bstring.from_cstring("two", 3);
    ByteString s3 = bstring.from_cstring("eight", 5);
    ByteString strings[] = {s1, s2, s3};
    ByteString sep = bstring.from_cstring(",", 1);

    ByteString res = bstring.join(strings, 3, sep);
    assert(eq(res, "one,two,eight"));

    bstring.free(s1);
    bstring.free(s2);
    bstring.free(s3);
    bstring.free(sep);
    bstring.free(res);

    printf("OK\n");
}

static void test_concat(void) {
    printf("test_concat...");

    ByteString s1 = bstring.from_cstring("one", 3);
    ByteString s2 = bstring.from_cstring("two", 3);
    ByteString s3 = bstring.from_cstring("eight", 5);
    ByteString strings[] = {s1, s2, s3};

    ByteString res = bstring.concat(strings, 3);
    assert(eq(res, "onetwoeight"));

    bstring.free(s1);
    bstring.free(s2);
    bstring.free(s3);
    bstring.free(res);

    printf("OK\n");
}

static void test_repeat(void) {
    printf("test_repeat...");
    ByteString str = bstring.from_cstring("one", 3);
    ByteString res = bstring.repeat(str, 3);
    assert(eq(res, "oneoneone"));
    bstring.free(res);
    bstring.free(str);

    str = bstring.from_cstring("*", 1);
    res = bstring.repeat(str, 5);
    assert(eq(res, "*****"));
    bstring.free(res);
    bstring.free(str);

    printf("OK\n");
}

static void test_replace(void) {
    printf("test_replace...");
    ByteString str = bstring.from_cstring("hello world", 11);

    {
        ByteString old = bstring.from_cstring("o", 1);
        ByteString new = bstring.from_cstring("***", 3);
        ByteString res = bstring.replace(str, old, new, -1);
        assert(eq(res, "hell*** w***rld"));
        bstring.free(old);
        bstring.free(new);
        bstring.free(res);
    }

    {
        ByteString old = bstring.from_cstring("e", 1);
        ByteString new = bstring.from_cstring("***", 3);
        ByteString res = bstring.replace(str, old, new, 1);
        assert(eq(res, "h***llo world"));
        bstring.free(old);
        bstring.free(new);
        bstring.free(res);
    }

    {
        ByteString old = bstring.from_cstring("world", 5);
        ByteString new = bstring.from_cstring("o", 1);
        ByteString res = bstring.replace(str, old, new, -1);
        assert(eq(res, "hello o"));
        bstring.free(old);
        bstring.free(new);
        bstring.free(res);
    }

    {
        ByteString old = bstring.from_cstring("wok", 3);
        ByteString new = bstring.from_cstring("***", 3);
        ByteString res = bstring.replace(str, old, new, -1);
        assert(eq(res, "hello world"));
        bstring.free(old);
        bstring.free(new);
        bstring.free(res);
    }

    bstring.free(str);
    printf("OK\n");
}

static void test_replace_all(void) {
    printf("test_replace_all...");
    ByteString str = bstring.from_cstring("hello world", 11);

    {
        ByteString old = bstring.from_cstring("o", 1);
        ByteString new = bstring.from_cstring("***", 3);
        ByteString res = bstring.replace_all(str, old, new);
        bstring.free(old);
        bstring.free(new);
        bstring.free(res);
    }

    {
        ByteString old = bstring.from_cstring("world", 5);
        ByteString new = bstring.from_cstring("o", 1);
        ByteString res = bstring.replace_all(str, old, new);
        assert(eq(res, "hello o"));
        bstring.free(old);
        bstring.free(new);
        bstring.free(res);
    }

    {
        ByteString old = bstring.from_cstring("wok", 3);
        ByteString new = bstring.from_cstring("***", 3);
        ByteString res = bstring.replace_all(str, old, new);
        assert(eq(res, "hello world"));
        bstring.free(old);
        bstring.free(new);
        bstring.free(res);
    }

    bstring.free(str);
    printf("OK\n");
}

static void test_reverse(void) {
    printf("test_reverse...");
    {
        ByteString str = bstring.from_cstring("hello", 5);
        ByteString res = bstring.reverse(str);
        assert(eq(res, "olleh"));
        bstring.free(str);
        bstring.free(res);
    }
    {
        ByteString str = bstring.from_cstring("hello world", 11);
        ByteString res = bstring.reverse(str);
        assert(eq(res, "dlrow olleh"));
        bstring.free(str);
        bstring.free(res);
    }
    {
        ByteString str = bstring.new();
        ByteString res = bstring.reverse(str);
        assert(eq(res, ""));
        bstring.free(str);
        bstring.free(res);
    }
    printf("OK\n");
}

static void test_trim_left(void) {
    printf("test_trim_left...");
    {
        ByteString str = bstring.from_cstring("   hello", 8);
        ByteString res = bstring.trim_left(str);
        assert(eq(res, "hello"));
        bstring.free(str);
        bstring.free(res);
    }
    {
        ByteString str = bstring.from_cstring("hello", 5);
        ByteString res = bstring.trim_left(str);
        assert(eq(res, "hello"));
        bstring.free(str);
        bstring.free(res);
    }
    {
        ByteString str = bstring.from_cstring("", 0);
        ByteString res = bstring.trim_left(str);
        assert(eq(res, ""));
        bstring.free(str);
        bstring.free(res);
    }
    printf("OK\n");
}

static void test_trim_right(void) {
    printf("test_trim_right...");
    {
        ByteString str = bstring.from_cstring("hello   ", 8);
        ByteString res = bstring.trim_right(str);
        assert(eq(res, "hello"));
        bstring.free(str);
        bstring.free(res);
    }
    {
        ByteString str = bstring.from_cstring("hello", 5);
        ByteString res = bstring.trim_right(str);
        assert(eq(res, "hello"));
        bstring.free(str);
        bstring.free(res);
    }
    {
        ByteString str = bstring.from_cstring("", 0);
        ByteString res = bstring.trim_right(str);
        assert(eq(res, ""));
        bstring.free(str);
        bstring.free(res);
    }
    printf("OK\n");
}

static void test_trim(void) {
    printf("test_trim...");
    {
        ByteString str = bstring.from_cstring("   hello   ", 11);
        ByteString res = bstring.trim(str);
        assert(eq(res, "hello"));
        bstring.free(str);
        bstring.free(res);
    }
    {
        ByteString str = bstring.from_cstring("   hello", 8);
        ByteString res = bstring.trim(str);
        assert(eq(res, "hello"));
        bstring.free(str);
        bstring.free(res);
    }
    {
        ByteString str = bstring.from_cstring("hello   ", 8);
        ByteString res = bstring.trim(str);
        assert(eq(res, "hello"));
        bstring.free(str);
        bstring.free(res);
    }
    {
        ByteString str = bstring.from_cstring("hello", 5);
        ByteString res = bstring.trim(str);
        assert(eq(res, "hello"));
        bstring.free(str);
        bstring.free(res);
    }
    {
        ByteString str = bstring.from_cstring("", 0);
        ByteString res = bstring.trim(str);
        assert(eq(res, ""));
        bstring.free(str);
        bstring.free(res);
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
    test_contains();
    test_equals();
    test_has_prefix();
    test_has_suffix();
    test_count();

    test_split_part();
    test_join();
    test_concat();
    test_repeat();

    test_replace();
    test_replace_all();
    test_reverse();

    test_trim_left();
    test_trim_right();
    test_trim();

    return 0;
}
