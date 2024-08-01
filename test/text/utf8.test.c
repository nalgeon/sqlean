// Copyright (c) 2024 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "text/utf8/utf8.h"

static void test_len(void) {
    printf("test_len...");
    const char* s = "Hello, 世界!";
    assert(utf8_len(s, strlen(s)) == 10);
    printf("OK\n");
}

static void test_peek(void) {
    printf("test_peek...");
    const char* s = "Hello, 世界!";
    assert(utf8_peek(s) == 'H');
    assert(utf8_peek(s + 2) == 'l');
    assert(utf8_peek(s + 4) == 'o');
    assert(utf8_peek(s + 6) == ' ');
    assert(utf8_peek(s + 7) == 0x4E16);
    assert(utf8_peek(s + 10) == 0x754c);
    assert(utf8_peek(s + 13) == '!');
    printf("OK\n");
}

static void test_peek_at(void) {
    printf("test_peek_at...");
    const char* s = "Hello, 世界!";
    size_t n = strlen(s);
    assert(utf8_peek_at(s, n, 0) == 'H');
    assert(utf8_peek_at(s, n, 2) == 'l');
    assert(utf8_peek_at(s, n, 4) == 'o');
    assert(utf8_peek_at(s, n, 6) == ' ');
    assert(utf8_peek_at(s, n, 7) == 0x4E16);
    assert(utf8_peek_at(s, n, 8) == 0x754C);
    assert(utf8_peek_at(s, n, 9) == '!');
    printf("OK\n");
}

static void test_icmp(void) {
    printf("test_icmp...");
    {
        const char* s1 = "Hello, 世界!";
        const char* s2 = "hello, 世界!";
        assert(utf8_icmp(s1, strlen(s1), s2, strlen(s2)) == 0);
    }
    {
        const char* s1 = "Hello, 世界!";
        const char* s2 = "HELLO, 世界!";
        assert(utf8_icmp(s1, strlen(s1), s2, strlen(s2)) == 0);
    }
    {
        const char* s1 = "Hello, 世界!";
        const char* s2 = "HELLO, 世界";
        assert(utf8_icmp(s1, strlen(s1), s2, strlen(s2)) > 0);
    }
    {
        const char* s1 = "Hello, 世界";
        const char* s2 = "HELLO, 世界!";
        assert(utf8_icmp(s1, strlen(s1), s2, strlen(s2)) < 0);
    }
    printf("OK\n");
}

static void test_valid(void) {
    printf("test_valid...");
    const char* s = "Hello, 世界!";
    assert(utf8_valid(s, strlen(s)));
    printf("OK\n");
}

static void test_tolower(void) {
    printf("test_tolower...");
    {
        char s[] = "Hello, WORLD!";
        utf8_tolower(s, strlen(s));
        assert(strcmp(s, "hello, world!") == 0);
    }
    {
        char s[] = "Hello, 世界!";
        utf8_tolower(s, strlen(s));
        assert(strcmp(s, "hello, 世界!") == 0);
    }
    {
        char s[] = "CÓMO ESTÁS";
        utf8_tolower(s, strlen(s));
        assert(strcmp(s, "cómo estás") == 0);
    }
    {
        char s[] = "Привет, МИР!";
        utf8_tolower(s, strlen(s));
        assert(strcmp(s, "привет, мир!") == 0);
    }
    printf("OK\n");
}

static void test_toupper(void) {
    printf("test_toupper...");
    {
        char s[] = "Hello, world!";
        utf8_toupper(s, strlen(s));
        assert(strcmp(s, "HELLO, WORLD!") == 0);
    }
    {
        char s[] = "Hello, 世界!";
        utf8_toupper(s, strlen(s));
        assert(strcmp(s, "HELLO, 世界!") == 0);
    }
    {
        char s[] = "cómo estás";
        utf8_toupper(s, strlen(s));
        assert(strcmp(s, "CÓMO ESTÁS") == 0);
    }
    {
        char s[] = "Привет, мир!";
        utf8_toupper(s, strlen(s));
        assert(strcmp(s, "ПРИВЕТ, МИР!") == 0);
    }
    printf("OK\n");
}

static void test_totitle(void) {
    printf("test_totitle...");
    {
        char s[] = "hello, world!";
        utf8_totitle(s, strlen(s));
        assert(strcmp(s, "Hello, World!") == 0);
    }
    {
        char s[] = "hello, 世界!";
        utf8_totitle(s, strlen(s));
        assert(strcmp(s, "Hello, 世界!") == 0);
    }
    {
        char s[] = "cómo estás";
        utf8_totitle(s, strlen(s));
        assert(strcmp(s, "Cómo Estás") == 0);
    }
    {
        char s[] = "привет, мир!";
        utf8_totitle(s, strlen(s));
        assert(strcmp(s, "Привет, Мир!") == 0);
    }
    printf("OK\n");
}

static void test_casefold(void) {
    printf("test_casefold...");
    {
        char s[] = "Hello, WORLD!";
        utf8_casefold(s, strlen(s));
        assert(strcmp(s, "hello, world!") == 0);
    }
    {
        char s[] = "Hello, 世界!";
        utf8_casefold(s, strlen(s));
        assert(strcmp(s, "hello, 世界!") == 0);
    }
    {
        char s[] = "CÓMO ESTÁS";
        utf8_casefold(s, strlen(s));
        assert(strcmp(s, "cómo estás") == 0);
    }
    {
        char s[] = "Привет, МИР!";
        utf8_casefold(s, strlen(s));
        assert(strcmp(s, "привет, мир!") == 0);
    }
    printf("OK\n");
}

int main(void) {
    test_len();
    test_peek();
    test_peek_at();
    test_icmp();
    test_valid();
    test_tolower();
    test_toupper();
    test_totitle();
    test_casefold();
}
