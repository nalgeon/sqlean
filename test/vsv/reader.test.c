// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Based on Go's encoding/csv package, BSD 3-Clause License
// https://github.com/golang/go

// Reader tests.

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vsv/vsv.h"

// ReadTest is the result of reading an input to the end. want has a line
// per record, with the fields as <length>:<bytes> separated by spaces.
// status is VSV_EOF if every record was read, or the parse error after
// the records in want.
typedef struct {
    const char* name;
    const char* input;
    size_t len;
    int32_t comma;
    const char* want;
    VsvStatus status;
    int64_t start_line;  // err_start_line
    int64_t line;        // err_pos.line
    int64_t col;         // err_pos.col
} ReadTest;

// The inputs of Go's encoding/csv reader_test.go (except the huge one), with
// the results of Go's Reader with FieldsPerRecord = -1 and the other options
// off, so every case is a golden value from Go.
// clang-format off
static const ReadTest go_tests[] = {
    {"Simple", "a,b,c\n", 6, ',', "1:a 1:b 1:c\n", VSV_EOF, 0, 0, 0},
    {"CRLF", "a,b\r\nc,d\r\n", 10, ',', "1:a 1:b\n1:c 1:d\n", VSV_EOF, 0, 0, 0},
    {"BareCR", "a,b\rc,d\r\n", 9, ',', "1:a 3:b\rc 1:d\n", VSV_EOF, 0, 0, 0},
    {"RFC4180test", "#field1,field2,field3\n\"aaa\",\"bb\nb\",\"ccc\"\n\"a,a\",\"b\"\"bb\",\"ccc\"\nzzz,yyy,xxx\n", 73, ',', "7:#field1 6:field2 6:field3\n3:aaa 4:bb\nb 3:ccc\n3:a,a 4:b\"bb 3:ccc\n3:zzz 3:yyy 3:xxx\n", VSV_EOF, 0, 0, 0},
    {"NoEOLTest", "a,b,c", 5, ',', "1:a 1:b 1:c\n", VSV_EOF, 0, 0, 0},
    {"Semicolon", "a;b;c\n", 6, ';', "1:a 1:b 1:c\n", VSV_EOF, 0, 0, 0},
    {"MultiLine", "\"two\nline\",\"one line\",\"three\nline\nfield\"", 40, ',', "8:two\nline 8:one line 16:three\nline\nfield\n", VSV_EOF, 0, 0, 0},
    {"BlankLine", "a,b,c\n\nd,e,f\n\n", 14, ',', "1:a 1:b 1:c\n1:d 1:e 1:f\n", VSV_EOF, 0, 0, 0},
    {"TrimSpace", " a,  b,   c\n", 12, ',', "2: a 3:  b 4:   c\n", VSV_EOF, 0, 0, 0},
    {"Comment", "#1,2,3\na,b,c\n#comment", 21, ',', "2:#1 1:2 1:3\n1:a 1:b 1:c\n8:#comment\n", VSV_EOF, 0, 0, 0},
    {"NoComment", "#1,2,3\na,b,c", 12, ',', "2:#1 1:2 1:3\n1:a 1:b 1:c\n", VSV_EOF, 0, 0, 0},
    {"LazyQuotes", "a \"word\",\"1\"2\",a\",\"b", 20, ',', "", VSV_BARE_QUOTE, 1, 1, 3},
    {"BareQuotes", "a \"word\",\"1\"2\",a\"", 17, ',', "", VSV_BARE_QUOTE, 1, 1, 3},
    {"BareDoubleQuotes", "a\"\"b,c", 6, ',', "", VSV_BARE_QUOTE, 1, 1, 2},
    {"TrimQuote", " \"a\",\" b\",c", 11, ',', "", VSV_BARE_QUOTE, 1, 1, 2},
    {"BadBareQuote", "a \"word\",\"b\"", 12, ',', "", VSV_BARE_QUOTE, 1, 1, 3},
    {"BadTrailingQuote", "\"a word\",b\"", 11, ',', "", VSV_BARE_QUOTE, 1, 1, 11},
    {"ExtraneousQuote", "\"a \"word\",\"b\"", 13, ',', "", VSV_QUOTE, 1, 1, 4},
    {"BadFieldCount", "a,b,c\nd,e", 9, ',', "1:a 1:b 1:c\n1:d 1:e\n", VSV_EOF, 0, 0, 0},
    {"BadFieldCountMultiple", "a,b,c\nd,e\nf", 11, ',', "1:a 1:b 1:c\n1:d 1:e\n1:f\n", VSV_EOF, 0, 0, 0},
    {"TrailingCommaEOF", "a,b,c,", 6, ',', "1:a 1:b 1:c 0:\n", VSV_EOF, 0, 0, 0},
    {"TrailingCommaEOL", "a,b,c,\n", 7, ',', "1:a 1:b 1:c 0:\n", VSV_EOF, 0, 0, 0},
    {"TrailingCommaSpaceEOF", "a,b,c, ", 7, ',', "1:a 1:b 1:c 1: \n", VSV_EOF, 0, 0, 0},
    {"TrailingCommaSpaceEOL", "a,b,c, \n", 8, ',', "1:a 1:b 1:c 1: \n", VSV_EOF, 0, 0, 0},
    {"TrailingCommaLine3", "a,b,c\nd,e,f\ng,hi,", 17, ',', "1:a 1:b 1:c\n1:d 1:e 1:f\n1:g 2:hi 0:\n", VSV_EOF, 0, 0, 0},
    {"CommaFieldTest", "x,y,z,w\nx,y,z,\nx,y,,\nx,,,\n,,,\n\"x\",\"y\",\"z\",\"w\"\n\"x\",\"y\",\"z\",\"\"\n\"x\",\"y\",\"\",\"\"\n\"x\",\"\",\"\",\"\"\n\"\",\"\",\"\",\"\"\n", 100, ',', "1:x 1:y 1:z 1:w\n1:x 1:y 1:z 0:\n1:x 1:y 0: 0:\n1:x 0: 0: 0:\n0: 0: 0: 0:\n1:x 1:y 1:z 1:w\n1:x 1:y 1:z 0:\n1:x 1:y 0: 0:\n1:x 0: 0: 0:\n0: 0: 0: 0:\n", VSV_EOF, 0, 0, 0},
    {"TrailingCommaIneffective1", "a,b,\nc,d,e", 10, ',', "1:a 1:b 0:\n1:c 1:d 1:e\n", VSV_EOF, 0, 0, 0},
    {"ReadAllReuseRecord", "a,b\nc,d", 7, ',', "1:a 1:b\n1:c 1:d\n", VSV_EOF, 0, 0, 0},
    {"StartLine1", "a,\"b\nc\"d,e", 10, ',', "", VSV_QUOTE, 1, 2, 2},
    {"StartLine2", "a,b\n\"d\n\n,e", 10, ',', "1:a 1:b\n", VSV_QUOTE, 2, 4, 3},
    {"CRLFInQuotedField", "A,\"Hello\r\nHi\",B\r\n", 17, ',', "1:A 8:Hello\nHi 1:B\n", VSV_EOF, 0, 0, 0},
    {"BinaryBlobField", "x09A\264\034,aktau", 12, ',', "6:x09A\264\034 5:aktau\n", VSV_EOF, 0, 0, 0},
    {"TrailingCR", "field1,field2\r", 14, ',', "6:field1 6:field2\n", VSV_EOF, 0, 0, 0},
    {"QuotedTrailingCR", "\"field\"\r", 8, ',', "5:field\n", VSV_EOF, 0, 0, 0},
    {"QuotedTrailingCRCR", "\"field\"\r\r", 9, ',', "", VSV_QUOTE, 1, 1, 7},
    {"FieldCR", "field\rfield\r", 12, ',', "11:field\rfield\n", VSV_EOF, 0, 0, 0},
    {"FieldCRCR", "field\r\rfield\r\r", 14, ',', "13:field\r\rfield\r\n", VSV_EOF, 0, 0, 0},
    {"FieldCRCRLF", "field\r\r\nfield\r\r\n", 16, ',', "6:field\r\n6:field\r\n", VSV_EOF, 0, 0, 0},
    {"FieldCRCRLFCR", "field\r\r\n\rfield\r\r\n\r", 18, ',', "6:field\r\n7:\rfield\r\n", VSV_EOF, 0, 0, 0},
    {"FieldCRCRLFCRCR", "field\r\r\n\r\rfield\r\r\n\r\r", 20, ',', "6:field\r\n8:\r\rfield\r\n1:\r\n", VSV_EOF, 0, 0, 0},
    {"MultiFieldCRCRLFCRCR", "field1,field2\r\r\n\r\rfield1,field2\r\r\n\r\r,", 37, ',', "6:field1 7:field2\r\n8:\r\rfield1 7:field2\r\n2:\r\r 0:\n", VSV_EOF, 0, 0, 0},
    {"NonASCIICommaAndComment", "a\302\243b,c\302\243 \011d,e\n\342\202\254 comment\n", 26, 0xa3, "1:a 3:b,c 5: \011d,e\n11:\342\202\254 comment\n", VSV_EOF, 0, 0, 0},
    {"NonASCIICommaAndCommentWithQuotes", "a\342\202\254\"  b,\"\342\202\254 c\n\316\273 comment\n", 27, 0x20ac, "1:a 4:  b, 2: c\n10:\316\273 comment\n", VSV_EOF, 0, 0, 0},
    {"NonASCIICommaConfusion", "\"ab\316\270cd\"\316\273ef\316\270gh", 16, 0x3bb, "6:ab\316\270cd 6:ef\316\270gh\n", VSV_EOF, 0, 0, 0},
    {"NonASCIICommentConfusion", "\316\273\n\316\273\n\316\270\n\316\273\n", 12, ',', "2:\316\273\n2:\316\273\n2:\316\270\n2:\316\273\n", VSV_EOF, 0, 0, 0},
    {"QuotedFieldMultipleLF", "\"\n\n\n\n\"", 6, ',', "4:\n\n\n\n\n", VSV_EOF, 0, 0, 0},
    {"MultipleCRLF", "\r\n\r\n\r\n\r\n", 8, ',', "", VSV_EOF, 0, 0, 0},
    {"QuoteWithTrailingCRLF", "\"foo\"bar\"\r\n", 11, ',', "", VSV_QUOTE, 1, 1, 5},
    {"DoubleQuoteWithTrailingCRLF", "\"foo\"\"bar\"\r\n", 12, ',', "7:foo\"bar\n", VSV_EOF, 0, 0, 0},
    {"EvenQuotes", "\"\"\"\"\"\"\"\"", 8, ',', "3:\"\"\"\n", VSV_EOF, 0, 0, 0},
    {"OddQuotes", "\"\"\"\"\"\"\"", 7, ',', "", VSV_QUOTE, 1, 1, 8},
    {"BadComment1", "", 0, ',', "", VSV_EOF, 0, 0, 0},
};

// Deviations from Go, and the cases that Go's tests don't cover.
static const ReadTest vsv_tests[] = {
    // a byte order mark at the start is skipped
    {"BOM", "\357\273\277a,b\n", 7, ',', "1:a 1:b\n", VSV_EOF, 0, 0, 0},
    {"BOMOnly", "\357\273\277", 3, ',', "", VSV_EOF, 0, 0, 0},
    {"BOMBlankLine", "\357\273\277\r\na\n", 6, ',', "1:a\n", VSV_EOF, 0, 0, 0},
    {"BOMQuoted", "\357\273\277\"a,b\",c", 10, ',', "3:a,b 1:c\n", VSV_EOF, 0, 0, 0},
    // columns are counted after it
    {"BOMBareQuote", "\357\273\277ab\"", 6, ',', "", VSV_BARE_QUOTE, 1, 1, 3},
    // but not anywhere else
    {"BOMSecondLine", "a\n\357\273\277b\n", 7, ',', "1:a\n4:\357\273\277b\n", VSV_EOF, 0, 0, 0},
    {"BOMSecondField", "a,\357\273\277b", 6, ',', "1:a 4:\357\273\277b\n", VSV_EOF, 0, 0, 0},
    {"BOMPartial", "\357\273a", 3, ',', "3:\357\273a\n", VSV_EOF, 0, 0, 0},
    // the last field is empty, not missing
    {"EmptyLastField", "a,", 2, ',', "1:a 0:\n", VSV_EOF, 0, 0, 0},
    {"EmptyFields", ",", 1, ',', "0: 0:\n", VSV_EOF, 0, 0, 0},
    {"EmptyQuoted", "\"\"", 2, ',', "0:\n", VSV_EOF, 0, 0, 0},
    // a line of spaces is not blank
    {"SpaceLine", "a\n \nb", 5, ',', "1:a\n1: \n1:b\n", VSV_EOF, 0, 0, 0},
    // every byte value is data (NUL is in test_read_nul)
    {"Bytes", "\001,\200\377,\"\377\001\"", 9, ',', "1:\001 2:\200\377 2:\377\001\n", VSV_EOF, 0, 0, 0},
    // other separators
    {"Tab", "a\tb c\t\"d\te\"\n", 12, '\t', "1:a 3:b c 3:d\te\n", VSV_EOF, 0, 0, 0},
    {"Pipe", "a|b,c\n", 6, '|', "1:a 3:b,c\n", VSV_EOF, 0, 0, 0},
    {"FourBytes", "a\360\237\230\200b\n", 7, 0x1f600, "1:a 1:b\n", VSV_EOF, 0, 0, 0},
    // a byte search, so the separator matches inside invalid UTF-8
    {"CommaInBadUTF8", "\342\302\247\n", 4, 0xa7, "1:\342 0:\n", VSV_EOF, 0, 0, 0},
    // the error positions count lines inside quoted fields
    {"ErrorAfterQuotedLines", "\"a\nb\"\nc\"\n", 9, ',', "3:a\nb\n", VSV_BARE_QUOTE, 3, 3, 2},
    {"UnterminatedQuote", "a\n\"b\nc", 6, ',', "1:a\n", VSV_QUOTE, 2, 3, 2},
    {"UnterminatedQuoteNL", "\"b\n", 3, ',', "", VSV_QUOTE, 1, 1, 4},
    {"Empty", "", 0, ',', "", VSV_EOF, 0, 0, 0},
    {"BlankLines", "\n\n\r\n\r", 5, ',', "", VSV_EOF, 0, 0, 0},
};
// clang-format on

// ChunkSource is the input for read_chunks: len bytes at data,
// read at most chunk bytes at a time.
typedef struct {
    const char* data;
    size_t len;
    size_t chunk;
} ChunkSource;

// read_chunks is a VsvReadFunc that reads from src, a ChunkSource*.
static VsvStatus read_chunks(void* src, char* buf, size_t size, size_t* n) {
    ChunkSource* s = src;
    *n = s->len;
    if (*n > s->chunk) {
        *n = s->chunk;
    }
    if (*n > size) {
        *n = size;
    }
    if (*n > 0) {
        memcpy(buf, s->data, *n);
        s->data += *n;
        s->len -= *n;
    }
    return VSV_OK;
}

// read_failing is a VsvReadFunc that reads like read_chunks
// until the input ends, then fails.
static VsvStatus read_failing(void* src, char* buf, size_t size, size_t* n) {
    ChunkSource* s = src;
    if (s->len == 0) {
        return VSV_IOERR;
    }
    return read_chunks(src, buf, size, n);
}

// Output collects the records in the ReadTest format.
typedef struct {
    char buf[4096];
    size_t len;
} Output;

static void out_append(Output* out, const char* s, size_t n) {
    assert(out->len + n <= sizeof(out->buf));
    if (n > 0) {
        memcpy(out->buf + out->len, s, n);
        out->len += n;
    }
}

// read_all reads every record from r into out and returns the final status.
static VsvStatus read_all(VsvReader* r, Output* out) {
    VsvStatus rc;
    while ((rc = vsv_reader_next(r)) == VSV_OK) {
        size_t count = vsv_reader_count(r);
        assert(count > 0);
        for (size_t i = 0; i < count; i++) {
            size_t len = 0;
            const char* field = vsv_reader_field(r, i, &len);
            assert(field != NULL);
            char prefix[32];
            int n = snprintf(prefix, sizeof(prefix), "%s%zu:", i > 0 ? " " : "", len);
            out_append(out, prefix, (size_t)n);
            out_append(out, field, len);
        }
        out_append(out, "\n", 1);
    }
    return rc;
}

// check_read reads the test input with read from src and checks the result.
static void check_read(const ReadTest* test, VsvReadFunc read, void* src) {
    VsvReader r;
    vsv_reader_init(&r, read, src, test->comma, SIZE_MAX);
    Output out = {.len = 0};
    VsvStatus rc = read_all(&r, &out);
    bool ok = rc == test->status && out.len == strlen(test->want) &&
              memcmp(out.buf, test->want, out.len) == 0;
    if (ok && rc != VSV_EOF) {
        ok = r.err_start_line == test->start_line && r.err_pos.line == test->line &&
             r.err_pos.col == test->col;
    }
    if (!ok) {
        printf("\n%s: got status %d, error at %lld/%lld:%lld, records:\n%.*s\n", test->name, rc,
               (long long)r.err_start_line, (long long)r.err_pos.line, (long long)r.err_pos.col,
               (int)out.len, out.buf);
    }
    assert(ok);
    vsv_reader_free(&r);
}

// check_reads checks the test with the input split into chunks of every size
// up to its length, so that every line and field crosses a chunk boundary,
// and with vsv_read_buffer.
static void check_reads(const ReadTest* test) {
    for (size_t chunk = 1; chunk <= test->len + 1; chunk++) {
        ChunkSource src = {test->input, test->len, chunk};
        check_read(test, read_chunks, &src);
    }
    VsvBuffer buf = {test->input, test->len};
    check_read(test, vsv_read_buffer, &buf);
}

static void test_read_go(void) {
    printf("test_read_go...");
    for (size_t i = 0; i < sizeof(go_tests) / sizeof(go_tests[0]); i++) {
        check_reads(&go_tests[i]);
    }
    printf("OK\n");
}

static void test_read_vsv(void) {
    printf("test_read_vsv...");
    for (size_t i = 0; i < sizeof(vsv_tests) / sizeof(vsv_tests[0]); i++) {
        check_reads(&vsv_tests[i]);
    }
    printf("OK\n");
}

// QuotedTest is an input and whether its fields are quoted: a line per record,
// with q for a quoted field and - for an unquoted one.
typedef struct {
    const char* input;
    int32_t comma;
    const char* want;
} QuotedTest;

static const QuotedTest quoted_tests[] = {
    {"a,\"b\",,\"\"\n", ',', "-q-q\n"},    // empty fields, quoted or not
    {"\"\"", ',', "q\n"},                  // "" at the end of the input
    {"\"\",", ',', "q-\n"},                // an empty field after it
    {",\n\"\"\r\n", ',', "--\nq\n"},       // CRLF after a quote
    {"\"a\nb\",c\n\"\"", ',', "q-\nq\n"},  // a multi-line field
    {"\"a\"\"\"→→\"\"", 0x2192, "q-q\n"},  // a multi-byte separator
};

// test_read_quoted checks vsv_reader_quoted with the input split into chunks
// of every size.
static void test_read_quoted(void) {
    printf("test_read_quoted...");
    for (size_t i = 0; i < sizeof(quoted_tests) / sizeof(quoted_tests[0]); i++) {
        const QuotedTest* test = &quoted_tests[i];
        size_t len = strlen(test->input);
        for (size_t chunk = 1; chunk <= len + 1; chunk++) {
            ChunkSource src = {test->input, len, chunk};
            VsvReader r;
            vsv_reader_init(&r, read_chunks, &src, test->comma, SIZE_MAX);
            Output out = {.len = 0};
            VsvStatus rc;
            while ((rc = vsv_reader_next(&r)) == VSV_OK) {
                for (size_t j = 0; j < vsv_reader_count(&r); j++) {
                    out_append(&out, vsv_reader_quoted(&r, j) ? "q" : "-", 1);
                }
                out_append(&out, "\n", 1);
            }
            assert(rc == VSV_EOF);
            assert(out.len == strlen(test->want) && memcmp(out.buf, test->want, out.len) == 0);
            vsv_reader_free(&r);
        }
    }
    printf("OK\n");
}

// make_field returns a malloc'ed string of n copies of c.
static char* make_field(size_t n, char c) {
    char* s = malloc(n > 0 ? n : 1);
    assert(s != NULL);
    memset(s, c, n);
    return s;
}

// test_read_long checks lines and fields longer than the input buffer, with
// a multi-byte separator and the quotes at every position around its end.
static void test_read_long(void) {
    printf("test_read_long...");
    const char comma[] = "\xc2\xa7";  // U+00A7
    for (size_t n = VSV_BUF_SIZE - 8; n <= VSV_BUF_SIZE + 8; n++) {
        for (int quoted = 0; quoted <= 1; quoted++) {
            // x...x§y\n, or "x...x"§y\n
            size_t len = n + (quoted ? 2 : 0) + 4;
            char* input = make_field(len, 'x');
            size_t pos = 0;
            if (quoted) {
                input[pos++] = '"';
                pos += n;
                input[pos++] = '"';
            } else {
                pos += n;
            }
            memcpy(input + pos, comma, 2);
            input[pos + 2] = 'y';
            input[pos + 3] = '\n';

            VsvBuffer buf = {input, len};
            VsvReader r;
            vsv_reader_init(&r, vsv_read_buffer, &buf, 0xa7, SIZE_MAX);
            assert(vsv_reader_next(&r) == VSV_OK);
            assert(vsv_reader_count(&r) == 2);
            size_t flen = 0;
            const char* field = vsv_reader_field(&r, 0, &flen);
            assert(flen == n);
            for (size_t i = 0; i < n; i++) {
                assert(field[i] == 'x');
            }
            field = vsv_reader_field(&r, 1, &flen);
            assert(flen == 1 && field[0] == 'y');
            assert(vsv_reader_next(&r) == VSV_EOF);
            vsv_reader_free(&r);
            free(input);
        }
    }
    printf("OK\n");
}

// test_read_nul checks that NUL is data, in unquoted and quoted fields.
static void test_read_nul(void) {
    printf("test_read_nul...");
    const char input[] = "\0,a\0b,\"\0\"\n";
    VsvBuffer buf = {input, sizeof(input) - 1};
    VsvReader r;
    vsv_reader_init(&r, vsv_read_buffer, &buf, ',', SIZE_MAX);
    assert(vsv_reader_next(&r) == VSV_OK);
    assert(vsv_reader_count(&r) == 3);
    size_t len = 0;
    const char* field = vsv_reader_field(&r, 0, &len);
    assert(len == 1 && field[0] == '\0');
    field = vsv_reader_field(&r, 1, &len);
    assert(len == 3 && memcmp(field, "a\0b", 3) == 0);
    field = vsv_reader_field(&r, 2, &len);
    assert(len == 1 && field[0] == '\0');
    assert(vsv_reader_next(&r) == VSV_EOF);
    vsv_reader_free(&r);
    printf("OK\n");
}

// MaxSizeTest reads the input with a size limit.
typedef struct {
    const char* input;
    size_t max_size;
    VsvStatus status;  // of the first record
} MaxSizeTest;

// clang-format off
static const MaxSizeTest max_size_tests[] = {
    // the limit is on the line as read, with the newline
    {"abc\n", 4, VSV_OK},
    {"abc\n", 3, VSV_TOOBIG},
    {"abc", 3, VSV_OK},
    {"abc\r\n", 4, VSV_TOOBIG},
    // and on the record, so a quoted field can't grow past it over many lines
    // (the lines are 4 bytes, the record "ab\ncd" is 5)
    {"\"ab\ncd\"\n", 4, VSV_TOOBIG},
    {"\"ab\ncd\"\n", 5, VSV_OK},
    {"\"ab\ncd\ne\"", 5, VSV_TOOBIG}, // 7 bytes
    {"", 0, VSV_EOF},
    {"a", 0, VSV_TOOBIG},
};
// clang-format on

static void test_max_size(void) {
    printf("test_max_size...");
    for (size_t i = 0; i < sizeof(max_size_tests) / sizeof(max_size_tests[0]); i++) {
        const MaxSizeTest* test = &max_size_tests[i];
        VsvBuffer buf = {test->input, strlen(test->input)};
        VsvReader r;
        vsv_reader_init(&r, vsv_read_buffer, &buf, ',', test->max_size);
        VsvStatus rc = vsv_reader_next(&r);
        // printf("%s/%zu: want %d, got %d\n", test->input, test->max_size, test->status, rc);
        assert(rc == test->status);
        vsv_reader_free(&r);
    }
    printf("OK\n");
}

static void test_read_error(void) {
    printf("test_read_error...");
    // a read error is reported, at the start of a record or inside one
    const char* inputs[] = {"", "a,b\n", "a,b", "\"a\nb"};
    for (size_t i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++) {
        ChunkSource src = {inputs[i], strlen(inputs[i]), 1};
        VsvReader r;
        vsv_reader_init(&r, read_failing, &src, ',', SIZE_MAX);
        VsvStatus rc;
        while ((rc = vsv_reader_next(&r)) == VSV_OK) {
        }
        assert(rc == VSV_IOERR);
        vsv_reader_free(&r);
    }
    printf("OK\n");
}

static void test_read_file(void) {
    printf("test_read_file...");
    FILE* file = tmpfile();
    assert(file != NULL);
    assert(fputs("a,b\n\"c\nd\",e", file) >= 0);
    rewind(file);
    VsvReader r;
    vsv_reader_init(&r, vsv_read_file, file, ',', SIZE_MAX);
    Output out = {.len = 0};
    assert(read_all(&r, &out) == VSV_EOF);
    const char* want = "1:a 1:b\n3:c\nd 1:e\n";
    assert(out.len == strlen(want) && memcmp(out.buf, want, out.len) == 0);
    vsv_reader_free(&r);
    fclose(file);
    printf("OK\n");
}

// CommaTest is a field separator for vsv_parse_comma.
typedef struct {
    const char* s;
    size_t len;
    int32_t want;  // -1 if invalid
} CommaTest;

// clang-format off
static const CommaTest comma_tests[] = {
    {",", 1, ','},
    {";", 1, ';'},
    {"\t", 1, '\t'},
    {" ", 1, ' '},
    {"\302\247", 2, 0xa7},          // 2 bytes
    {"\342\202\254", 3, 0x20ac},    // 3 bytes
    {"\360\237\230\200", 4, 0x1f600},  // 4 bytes
    {"\177", 1, 0x7f},
    {"\337\277", 2, 0x7ff},
    {"\357\277\277", 3, 0xffff},
    {"\364\217\277\277", 4, 0x10ffff},
    // Go's invalid delimiters
    {"", 0, -1},
    {"\000", 1, -1},
    {"\"", 1, -1},
    {"\r", 1, -1},
    {"\n", 1, -1},
    {"\357\277\275", 3, -1},        // U+FFFD
    // more than one character
    {",,", 2, -1},
    {"ab", 2, -1},
    {",\000", 2, -1},
    {"\302\247,", 3, -1},
    // invalid UTF-8
    {"\200", 1, -1},                // continuation byte
    {"\377", 1, -1},
    {"\302", 1, -1},                // truncated
    {"\342\202", 2, -1},
    {"\360\237\230", 3, -1},
    {"\300\257", 2, -1},            // overlong
    {"\340\201\201", 3, -1},
    {"\360\200\200\257", 4, -1},
    {"\355\240\200", 3, -1},        // surrogate U+D800
    {"\355\277\277", 3, -1},        // U+DFFF
    {"\364\220\200\200", 4, -1},    // U+110000
    {"\370\210\200\200\200", 5, -1},
    {"\302\302", 2, -1},            // bad continuation
};
// clang-format on

static void test_parse_comma(void) {
    printf("test_parse_comma...");
    for (size_t i = 0; i < sizeof(comma_tests) / sizeof(comma_tests[0]); i++) {
        const CommaTest* test = &comma_tests[i];
        int32_t got = -1;
        bool ok = vsv_parse_comma(test->s, test->len, &got);
        // printf("%zu: want %d, got %d (%d)\n", i, test->want, got, ok);
        assert(ok == (test->want >= 0));
        assert(!ok || got == test->want);
    }
    printf("OK\n");
}

int main(void) {
    test_read_go();
    test_read_vsv();
    test_read_nul();
    test_read_quoted();
    test_read_long();
    test_max_size();
    test_read_error();
    test_read_file();
    test_parse_comma();
    return 0;
}
