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

#include "text/bstring.h"

static bool eq(ByteString str, const char* expected) {
    return strcmp(bstring_to_cstring(str), expected) == 0;
}

static void test_cstring(void) {
    printf("test_cstring...");
    ByteString str = bstring_from_cstring("hello world", 11);
    assert(eq(str, "hello world"));
    bstring_free(str);
    printf("OK\n");
}

static void test_at(void) {
    printf("test_at...");
    ByteString str = bstring_from_cstring("hello world", 11);
    char chr = bstring_at(str, 2);
    assert(chr == 'l');
    bstring_free(str);
    printf("OK\n");
}

static void test_slice(void) {
    printf("test_slice...");
    ByteString str = bstring_from_cstring("hello world", 11);

    {
        ByteString slice = bstring_slice(str, 6, 11);
        assert(eq(slice, "world"));
        bstring_free(slice);
    }

    {
        ByteString slice = bstring_slice(str, 0, 5);
        assert(eq(slice, "hello"));
        bstring_free(slice);
    }

    {
        ByteString slice = bstring_slice(str, -5, str.length);
        assert(eq(slice, "world"));
        bstring_free(slice);
    }

    bstring_free(str);
    printf("OK\n");
}

static void test_substring(void) {
    printf("test_substring_..");
    ByteString str = bstring_from_cstring("hello world", 11);

    {
        ByteString slice = bstring_substring(str, 6, 5);
        assert(eq(slice, "world"));
        bstring_free(slice);
    }

    {
        ByteString slice = bstring_substring(str, 0, 5);
        assert(eq(slice, "hello"));
        bstring_free(slice);
    }

    {
        ByteString slice = bstring_substring(str, 0, str.length);
        assert(eq(slice, "hello world"));
        bstring_free(slice);
    }

    {
        ByteString slice = bstring_substring(str, 6, str.length);
        assert(eq(slice, "world"));
        bstring_free(slice);
    }

    {
        ByteString slice = bstring_substring(str, 1, 1);
        assert(eq(slice, "e"));
        bstring_free(slice);
    }

    {
        ByteString slice = bstring_substring(str, 1, 0);
        assert(eq(slice, ""));
        bstring_free(slice);
    }

    bstring_free(str);
    printf("OK\n");
}

static void test_index(void) {
    printf("test_index...");
    ByteString str = bstring_from_cstring("hello world", 11);

    {
        ByteString other = bstring_from_cstring("l", 1);
        int index = bstring_index(str, other);
        assert(index == 2);
        bstring_free(other);
    }

    {
        ByteString other = bstring_from_cstring("rl", 2);
        int index = bstring_index(str, other);
        assert(index == 8);
        bstring_free(other);
    }

    {
        ByteString other = bstring_from_cstring("rlc", 3);
        int index = bstring_index(str, other);
        assert(index == -1);
        bstring_free(other);
    }

    {
        ByteString other = bstring_from_cstring("", 0);
        int index = bstring_index(str, other);
        assert(index == 0);
        bstring_free(other);
    }

    bstring_free(str);
    printf("OK\n");
}

static void test_last_index(void) {
    printf("test_last_index...");
    ByteString str = bstring_from_cstring("hello world", 11);

    {
        ByteString other = bstring_from_cstring("l", 1);
        int index = bstring_last_index(str, other);
        assert(index == 9);
        bstring_free(other);
    }

    {
        ByteString other = bstring_from_cstring("hel", 3);
        int index = bstring_last_index(str, other);
        assert(index == 0);
        bstring_free(other);
    }

    {
        ByteString other = bstring_from_cstring("rla", 3);
        int index = bstring_last_index(str, other);
        assert(index == -1);
        bstring_free(other);
    }

    {
        ByteString other = bstring_from_cstring("", 0);
        int index = bstring_last_index(str, other);
        assert(index == 10);
        bstring_free(other);
    }

    bstring_free(str);
    printf("OK\n");
}

static void test_contains(void) {
    printf("test_contains...");
    ByteString str = bstring_from_cstring("hello world", 11);

    {
        ByteString other = bstring_from_cstring("l", 1);
        bool ok = bstring_contains(str, other);
        assert(ok);
        bstring_free(other);
    }

    {
        ByteString other = bstring_from_cstring("ld", 2);
        bool ok = bstring_contains(str, other);
        assert(ok);
        bstring_free(other);
    }

    {
        ByteString other = bstring_from_cstring("lda", 3);
        bool ok = bstring_contains(str, other);
        assert(!ok);
        bstring_free(other);
    }

    bstring_free(str);
    printf("OK\n");
}

static void test_equals(void) {
    printf("test_equals...");

    {
        ByteString s1 = bstring_from_cstring("hello world", 11);
        ByteString s2 = bstring_from_cstring("hello world", 11);
        assert(bstring_equals(s1, s2));
        bstring_free(s1);
        bstring_free(s2);
    }

    {
        ByteString s1 = bstring_from_cstring("hello world", 11);
        ByteString s2 = bstring_from_cstring("world", 5);
        assert(!bstring_equals(s1, s2));
        bstring_free(s1);
        bstring_free(s2);
    }

    {
        ByteString s1 = bstring_from_cstring("hello world", 11);
        ByteString s2 = bstring_from_cstring("", 0);
        assert(!bstring_equals(s1, s2));
        bstring_free(s1);
        bstring_free(s2);
    }

    {
        ByteString s1 = bstring_from_cstring("hello world", 11);
        ByteString s2 = bstring_new();
        assert(!bstring_equals(s1, s2));
        bstring_free(s1);
        bstring_free(s2);
    }

    {
        ByteString s1 = bstring_from_cstring("", 0);
        ByteString s2 = bstring_from_cstring("", 0);
        assert(bstring_equals(s1, s2));
        bstring_free(s1);
        bstring_free(s2);
    }

    {
        ByteString s1 = bstring_new();
        ByteString s2 = bstring_new();
        assert(bstring_equals(s1, s2));
        bstring_free(s1);
        bstring_free(s2);
    }

    printf("OK\n");
}

static void test_has_prefix(void) {
    printf("test_has_prefix...");

    {
        ByteString str = bstring_from_cstring("hello world", 11);
        ByteString other = bstring_from_cstring("hell", 4);
        bool ok = bstring_has_prefix(str, other);
        assert(ok);
        bstring_free(str);
        bstring_free(other);
    }

    {
        ByteString str = bstring_from_cstring("hello world", 11);
        ByteString other = bstring_from_cstring("ello", 4);
        bool ok = bstring_has_prefix(str, other);
        assert(!ok);
        bstring_free(str);
        bstring_free(other);
    }

    {
        ByteString str = bstring_from_cstring("hello world", 11);
        ByteString other = bstring_from_cstring("", 0);
        bool ok = bstring_has_prefix(str, other);
        assert(ok);
        bstring_free(str);
        bstring_free(other);
    }

    {
        ByteString str = bstring_from_cstring("", 0);
        ByteString other = bstring_from_cstring("hell", 4);
        bool ok = bstring_has_prefix(str, other);
        assert(!ok);
        bstring_free(str);
        bstring_free(other);
    }

    printf("OK\n");
}

static void test_has_suffix(void) {
    printf("test_has_suffix...");

    {
        ByteString str = bstring_from_cstring("hello world", 11);
        ByteString other = bstring_from_cstring("orld", 4);
        bool ok = bstring_has_suffix(str, other);
        assert(ok);
        bstring_free(str);
        bstring_free(other);
    }

    {
        ByteString str = bstring_from_cstring("hello world", 11);
        ByteString other = bstring_from_cstring("d", 1);
        bool ok = bstring_has_suffix(str, other);
        assert(ok);
        bstring_free(str);
        bstring_free(other);
    }

    {
        ByteString str = bstring_from_cstring("hello world", 11);
        ByteString other = bstring_from_cstring("worl", 4);
        bool ok = bstring_has_suffix(str, other);
        assert(!ok);
        bstring_free(str);
        bstring_free(other);
    }

    {
        ByteString str = bstring_from_cstring("hello world", 11);
        ByteString other = bstring_from_cstring("", 0);
        bool ok = bstring_has_suffix(str, other);
        assert(ok);
        bstring_free(str);
        bstring_free(other);
    }

    {
        ByteString str = bstring_from_cstring("", 0);
        ByteString other = bstring_from_cstring("hell", 4);
        bool ok = bstring_has_suffix(str, other);
        assert(!ok);
        bstring_free(str);
        bstring_free(other);
    }

    printf("OK\n");
}

static void test_count(void) {
    printf("test_count...");
    ByteString str = bstring_from_cstring("hello yellow", 12);

    {
        ByteString other = bstring_from_cstring("h", 1);
        int count = bstring_count(str, other);
        assert(count == 1);
        bstring_free(other);
    }

    {
        ByteString other = bstring_from_cstring("e", 1);
        int count = bstring_count(str, other);
        assert(count == 2);
        bstring_free(other);
    }

    {
        ByteString other = bstring_from_cstring("o", 1);
        int count = bstring_count(str, other);
        assert(count == 2);
        bstring_free(other);
    }

    {
        ByteString other = bstring_from_cstring("l", 1);
        int count = bstring_count(str, other);
        assert(count == 4);
        bstring_free(other);
    }

    {
        ByteString other = bstring_from_cstring("he", 2);
        int count = bstring_count(str, other);
        assert(count == 1);
        bstring_free(other);
    }

    {
        ByteString other = bstring_from_cstring(" he", 3);
        int count = bstring_count(str, other);
        assert(count == 0);
        bstring_free(other);
    }

    {
        ByteString other = bstring_from_cstring("ow", 2);
        int count = bstring_count(str, other);
        assert(count == 1);
        bstring_free(other);
    }

    {
        ByteString other = bstring_from_cstring("ow ", 3);
        int count = bstring_count(str, other);
        assert(count == 0);
        bstring_free(other);
    }

    bstring_free(str);
    printf("OK\n");
}

static void test_split_part(void) {
    printf("test_split_part...");
    {
        ByteString str = bstring_from_cstring("one,two,thr", 11);
        ByteString sep = bstring_from_cstring(",", 1);
        ByteString part = bstring_split_part(str, sep, 1);
        assert(eq(part, "two"));
        bstring_free(part);
        bstring_free(str);
        bstring_free(sep);
    }
    {
        ByteString str = bstring_from_cstring("one,two,thr", 11);
        ByteString sep = bstring_from_cstring(",", 1);
        ByteString part = bstring_split_part(str, sep, 2);
        assert(eq(part, "thr"));
        bstring_free(part);
        bstring_free(str);
        bstring_free(sep);
    }
    {
        ByteString str = bstring_from_cstring("one,two,thr", 11);
        ByteString sep = bstring_from_cstring(",", 1);
        ByteString part = bstring_split_part(str, sep, 3);
        assert(eq(part, ""));
        bstring_free(part);
        bstring_free(str);
        bstring_free(sep);
    }
    {
        ByteString str = bstring_from_cstring("one,two,thr", 11);
        ByteString sep = bstring_from_cstring(";", 1);
        ByteString part = bstring_split_part(str, sep, 1);
        assert(eq(part, ""));
        bstring_free(part);
        bstring_free(str);
        bstring_free(sep);
    }
    {
        ByteString str = bstring_from_cstring("", 0);
        ByteString sep = bstring_from_cstring(",", 1);
        ByteString part = bstring_split_part(str, sep, 1);
        assert(eq(part, ""));
        bstring_free(part);
        bstring_free(str);
        bstring_free(sep);
    }
    {
        const char* s = "один,два,три";
        ByteString str = bstring_from_cstring(s, strlen(s));
        ByteString sep = bstring_from_cstring(",", 1);
        ByteString part = bstring_split_part(str, sep, 1);
        assert(eq(part, "два"));
        bstring_free(part);
        bstring_free(str);
        bstring_free(sep);
    }
    {
        const char* s = "один-*-два-*-три";
        ByteString str = bstring_from_cstring(s, strlen(s));
        ByteString sep = bstring_from_cstring("-*-", 3);
        ByteString part = bstring_split_part(str, sep, 1);
        assert(eq(part, "два"));
        bstring_free(part);
        bstring_free(str);
        bstring_free(sep);
    }
    printf("OK\n");
}

static void test_join(void) {
    printf("test_join...");

    ByteString s1 = bstring_from_cstring("one", 3);
    ByteString s2 = bstring_from_cstring("two", 3);
    ByteString s3 = bstring_from_cstring("eight", 5);
    ByteString strings[] = {s1, s2, s3};
    ByteString sep = bstring_from_cstring(",", 1);

    ByteString res = bstring_join(strings, 3, sep);
    assert(eq(res, "one,two,eight"));

    bstring_free(s1);
    bstring_free(s2);
    bstring_free(s3);
    bstring_free(sep);
    bstring_free(res);

    printf("OK\n");
}

static void test_concat(void) {
    printf("test_concat...");

    ByteString s1 = bstring_from_cstring("one", 3);
    ByteString s2 = bstring_from_cstring("two", 3);
    ByteString s3 = bstring_from_cstring("eight", 5);
    ByteString strings[] = {s1, s2, s3};

    ByteString res = bstring_concat(strings, 3);
    assert(eq(res, "onetwoeight"));

    bstring_free(s1);
    bstring_free(s2);
    bstring_free(s3);
    bstring_free(res);

    printf("OK\n");
}

static void test_repeat(void) {
    printf("test_repeat...");
    {
        ByteString str = bstring_from_cstring("one", 3);
        ByteString res = bstring_repeat(str, 3);
        assert(eq(res, "oneoneone"));
        bstring_free(res);
        bstring_free(str);
    }

    {
        ByteString str = bstring_from_cstring("*", 1);
        ByteString res = bstring_repeat(str, 5);
        assert(eq(res, "*****"));
        bstring_free(res);
        bstring_free(str);
    }

    {
        ByteString str = bstring_from_cstring("one", 3);
        ByteString res = bstring_repeat(str, 0);
        assert(eq(res, ""));
        bstring_free(res);
        bstring_free(str);
    }

    printf("OK\n");
}

static void test_replace(void) {
    printf("test_replace...");
    ByteString str = bstring_from_cstring("hello world", 11);

    {
        ByteString old = bstring_from_cstring("o", 1);
        ByteString new = bstring_from_cstring("***", 3);
        ByteString res = bstring_replace(str, old, new, -1);
        assert(eq(res, "hell*** w***rld"));
        bstring_free(old);
        bstring_free(new);
        bstring_free(res);
    }

    {
        ByteString old = bstring_from_cstring("o", 1);
        ByteString new = bstring_from_cstring("***", 3);
        ByteString res = bstring_replace(str, old, new, 1);
        assert(eq(res, "hell*** world"));
        bstring_free(old);
        bstring_free(new);
        bstring_free(res);
    }

    {
        ByteString old = bstring_from_cstring("o", 1);
        ByteString new = bstring_from_cstring("***", 3);
        ByteString res = bstring_replace(str, old, new, 2);
        assert(eq(res, "hell*** w***rld"));
        bstring_free(old);
        bstring_free(new);
        bstring_free(res);
    }

    {
        ByteString old = bstring_from_cstring("e", 1);
        ByteString new = bstring_from_cstring("***", 3);
        ByteString res = bstring_replace(str, old, new, 1);
        assert(eq(res, "h***llo world"));
        bstring_free(old);
        bstring_free(new);
        bstring_free(res);
    }

    {
        ByteString old = bstring_from_cstring("world", 5);
        ByteString new = bstring_from_cstring("o", 1);
        ByteString res = bstring_replace(str, old, new, -1);
        assert(eq(res, "hello o"));
        bstring_free(old);
        bstring_free(new);
        bstring_free(res);
    }

    {
        ByteString old = bstring_from_cstring("wok", 3);
        ByteString new = bstring_from_cstring("***", 3);
        ByteString res = bstring_replace(str, old, new, -1);
        assert(eq(res, "hello world"));
        bstring_free(old);
        bstring_free(new);
        bstring_free(res);
    }

    bstring_free(str);
    printf("OK\n");
}

static void test_replace_all(void) {
    printf("test_replace_all...");
    ByteString str = bstring_from_cstring("hello world", 11);

    {
        ByteString old = bstring_from_cstring("o", 1);
        ByteString new = bstring_from_cstring("***", 3);
        ByteString res = bstring_replace_all(str, old, new);
        bstring_free(old);
        bstring_free(new);
        bstring_free(res);
    }

    {
        ByteString old = bstring_from_cstring("world", 5);
        ByteString new = bstring_from_cstring("o", 1);
        ByteString res = bstring_replace_all(str, old, new);
        assert(eq(res, "hello o"));
        bstring_free(old);
        bstring_free(new);
        bstring_free(res);
    }

    {
        ByteString old = bstring_from_cstring("wok", 3);
        ByteString new = bstring_from_cstring("***", 3);
        ByteString res = bstring_replace_all(str, old, new);
        assert(eq(res, "hello world"));
        bstring_free(old);
        bstring_free(new);
        bstring_free(res);
    }

    bstring_free(str);
    printf("OK\n");
}

static void test_reverse(void) {
    printf("test_reverse...");
    {
        ByteString str = bstring_from_cstring("hello", 5);
        ByteString res = bstring_reverse(str);
        assert(eq(res, "olleh"));
        bstring_free(str);
        bstring_free(res);
    }
    {
        ByteString str = bstring_from_cstring("hello world", 11);
        ByteString res = bstring_reverse(str);
        assert(eq(res, "dlrow olleh"));
        bstring_free(str);
        bstring_free(res);
    }
    {
        ByteString str = bstring_new();
        ByteString res = bstring_reverse(str);
        assert(eq(res, ""));
        bstring_free(str);
        bstring_free(res);
    }
    printf("OK\n");
}

static void test_trim_left(void) {
    printf("test_trim_left...");
    {
        ByteString str = bstring_from_cstring("   hello", 8);
        ByteString res = bstring_trim_left(str);
        assert(eq(res, "hello"));
        bstring_free(str);
        bstring_free(res);
    }
    {
        ByteString str = bstring_from_cstring("hello", 5);
        ByteString res = bstring_trim_left(str);
        assert(eq(res, "hello"));
        bstring_free(str);
        bstring_free(res);
    }
    {
        ByteString str = bstring_from_cstring("", 0);
        ByteString res = bstring_trim_left(str);
        assert(eq(res, ""));
        bstring_free(str);
        bstring_free(res);
    }
    printf("OK\n");
}

static void test_trim_right(void) {
    printf("test_trim_right...");
    {
        ByteString str = bstring_from_cstring("hello   ", 8);
        ByteString res = bstring_trim_right(str);
        assert(eq(res, "hello"));
        bstring_free(str);
        bstring_free(res);
    }
    {
        ByteString str = bstring_from_cstring("hello", 5);
        ByteString res = bstring_trim_right(str);
        assert(eq(res, "hello"));
        bstring_free(str);
        bstring_free(res);
    }
    {
        ByteString str = bstring_from_cstring("", 0);
        ByteString res = bstring_trim_right(str);
        assert(eq(res, ""));
        bstring_free(str);
        bstring_free(res);
    }
    printf("OK\n");
}

static void test_trim(void) {
    printf("test_trim...");
    {
        ByteString str = bstring_from_cstring("   hello   ", 11);
        ByteString res = bstring_trim(str);
        assert(eq(res, "hello"));
        bstring_free(str);
        bstring_free(res);
    }
    {
        ByteString str = bstring_from_cstring("   hello", 8);
        ByteString res = bstring_trim(str);
        assert(eq(res, "hello"));
        bstring_free(str);
        bstring_free(res);
    }
    {
        ByteString str = bstring_from_cstring("hello   ", 8);
        ByteString res = bstring_trim(str);
        assert(eq(res, "hello"));
        bstring_free(str);
        bstring_free(res);
    }
    {
        ByteString str = bstring_from_cstring("hello", 5);
        ByteString res = bstring_trim(str);
        assert(eq(res, "hello"));
        bstring_free(str);
        bstring_free(res);
    }
    {
        ByteString str = bstring_from_cstring("", 0);
        ByteString res = bstring_trim(str);
        assert(eq(res, ""));
        bstring_free(str);
        bstring_free(res);
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
