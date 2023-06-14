#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "regexp/pcre2/pcre2.h"
#include "regexp/regexp.h"

bool eq(const char* str, const char* expected) {
    return strcmp(str, expected) == 0;
}

void test_like(void) {
    printf("test_like...");
    char* pattern = "[0-9]+";
    pcre2_code* re = regexp.compile(pattern);
    assert(re != NULL);
    {
        char* source = "the year is 2021";
        int rc = regexp.like(re, source);
        assert(rc == 1);
    }
    {
        char* source = "the year is unknown";
        int rc = regexp.like(re, source);
        assert(rc == 0);
    }
    printf("OK\n");
}

void test_substr(void) {
    printf("test_substr...");
    char* pattern = "[0-9]+";
    pcre2_code* re = regexp.compile(pattern);
    assert(re != NULL);
    {
        char* source = "the year is 2021";
        char* substr = NULL;

        int rc = regexp.substr(re, source, &substr);
        assert(rc == 1);
        free(substr);
    }
    {
        char* source = "the year is unknown";
        char* substr = NULL;

        int rc = regexp.substr(re, source, &substr);
        assert(rc == 0);
        assert(substr == NULL);
    }
    printf("OK\n");
}

void test_replace(void) {
    printf("test_replace...");
    char* pattern = "[0-9]+";
    pcre2_code* re = regexp.compile(pattern);
    assert(re != NULL);
    {
        char* source = "the year is 2021";
        char* repl = "2050";
        char* dest = NULL;
        int rc = regexp.replace(re, source, repl, &dest);
        assert(rc == 1);
        assert(eq(dest, "the year is 2050"));
        free(dest);
    }
    {
        char* source = "the year is 2021";
        char* repl = "unknown";
        char* dest = NULL;
        int rc = regexp.replace(re, source, repl, &dest);
        assert(rc == 1);
        assert(eq(dest, "the year is unknown"));
        free(dest);
    }
    {
        char* source = "the year is 2021";
        char* repl = "*";
        char* dest = NULL;
        int rc = regexp.replace(re, source, repl, &dest);
        assert(rc == 1);
        assert(eq(dest, "the year is *"));
        free(dest);
    }
    {
        char* source = "the year is unknown";
        char* repl = "2050";
        char* dest = NULL;
        int rc = regexp.replace(re, source, repl, &dest);
        assert(rc == 0);
        assert(dest == NULL);
    }
    printf("OK\n");
}

int main(void) {
    test_like();
    test_substr();
    test_replace();
}
