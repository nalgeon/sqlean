// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Based on Go's encoding/csv package, BSD 3-Clause License
// https://github.com/golang/go

// Reading CSV records.

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vsv/vsv.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#pragma region Input

// vsv_open_file opens the file at path (UTF-8) for reading.
// Returns 0 or an errno value. The caller closes the file with fclose.
int vsv_open_file(const char* path, FILE** file) {
#ifdef _WIN32
    // The narrow API takes the path in the ANSI code page, not UTF-8.
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, NULL, 0);
    if (n <= 0) {
        return EINVAL;
    }
    wchar_t* wpath = malloc((size_t)n * sizeof(wchar_t));
    if (wpath == NULL) {
        return ENOMEM;
    }
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wpath, n) <= 0) {
        free(wpath);
        return EINVAL;
    }
    FILE* f = _wfopen(wpath, L"rb");
    int err = errno;
    free(wpath);
#else
    FILE* f = fopen(path, "rb");
    int err = errno;
#endif
    if (f == NULL) {
        return err != 0 ? err : EIO;
    }
    *file = f;
    return 0;
}

// vsv_read_file is a VsvReadFunc that reads from src, an open FILE*.
VsvStatus vsv_read_file(void* src, char* buf, size_t size, size_t* n) {
    FILE* file = src;
    *n = fread(buf, 1, size, file);
    if (*n < size && ferror(file)) {
        return VSV_IOERR;
    }
    return VSV_OK;
}

// vsv_read_buffer is a VsvReadFunc that reads from src, a VsvBuffer*.
VsvStatus vsv_read_buffer(void* src, char* buf, size_t size, size_t* n) {
    VsvBuffer* b = src;
    *n = b->len < size ? b->len : size;
    // memcpy with a NULL pointer is UB even for 0 bytes, and data is NULL
    // for an empty buffer.
    if (*n > 0) {
        memcpy(buf, b->data, *n);
        b->data += *n;
        b->len -= *n;
    }
    return VSV_OK;
}

#pragma endregion

#pragma region Separator

// decode_rune decodes the UTF-8 character at the start of s (n > 0) and sets
// size to its length. Returns -1 for an invalid encoding: an overlong form,
// a surrogate, a code point above U+10FFFF, or a truncated sequence,
// as Go's utf8.DecodeRune does.
static int32_t decode_rune(const unsigned char* s, size_t n, size_t* size) {
    unsigned char c = s[0];
    if (c < 0x80) {
        *size = 1;
        return c;
    }
    size_t len;
    int32_t r;
    // The valid range of the second byte depends on the first one,
    // which rules out overlong forms, surrogates and values above U+10FFFF.
    unsigned char lo = 0x80, hi = 0xbf;
    if (c >= 0xc2 && c <= 0xdf) {
        len = 2;
        r = c & 0x1f;
    } else if (c >= 0xe0 && c <= 0xef) {
        len = 3;
        r = c & 0x0f;
        if (c == 0xe0) {
            lo = 0xa0;
        } else if (c == 0xed) {
            hi = 0x9f;
        }
    } else if (c >= 0xf0 && c <= 0xf4) {
        len = 4;
        r = c & 0x07;
        if (c == 0xf0) {
            lo = 0x90;
        } else if (c == 0xf4) {
            hi = 0x8f;
        }
    } else {
        return -1;
    }
    if (n < len || s[1] < lo || s[1] > hi) {
        return -1;
    }
    for (size_t i = 1; i < len; i++) {
        if ((s[i] & 0xc0) != 0x80) {
            return -1;
        }
        r = (r << 6) | (s[i] & 0x3f);
    }
    *size = len;
    return r;
}

// encode_rune writes the UTF-8 encoding of the valid code point r
// to buf and returns its length.
static size_t encode_rune(int32_t r, char buf[4]) {
    uint32_t u = (uint32_t)r;
    if (u < 0x80) {
        buf[0] = (char)u;
        return 1;
    }
    if (u < 0x800) {
        buf[0] = (char)(0xc0 | (u >> 6));
        buf[1] = (char)(0x80 | (u & 0x3f));
        return 2;
    }
    if (u < 0x10000) {
        buf[0] = (char)(0xe0 | (u >> 12));
        buf[1] = (char)(0x80 | ((u >> 6) & 0x3f));
        buf[2] = (char)(0x80 | (u & 0x3f));
        return 3;
    }
    buf[0] = (char)(0xf0 | (u >> 18));
    buf[1] = (char)(0x80 | ((u >> 12) & 0x3f));
    buf[2] = (char)(0x80 | ((u >> 6) & 0x3f));
    buf[3] = (char)(0x80 | (u & 0x3f));
    return 4;
}

// vsv_parse_comma sets comma to the code point of s, the n bytes of a field
// separator. Returns false unless s is a single valid UTF-8 character
// other than NUL, ", \r, \n and U+FFFD (as Go's Reader.Comma).
bool vsv_parse_comma(const char* s, size_t n, int32_t* comma) {
    if (n == 0) {
        return false;
    }
    size_t size;
    int32_t r = decode_rune((const unsigned char*)s, n, &size);
    if (r < 0 || size != n) {
        return false;
    }
    // As Go's validDelim.
    if (r == 0 || r == '"' || r == '\r' || r == '\n' || r == 0xfffd) {
        return false;
    }
    *comma = r;
    return true;
}

#pragma endregion

#pragma region Reader

// grow makes room for need elements of size elem in *buf, which has room for
// *cap of them, doubling the capacity. More than max elements is VSV_TOOBIG.
static VsvStatus grow(void** buf, size_t* cap, size_t need, size_t elem, size_t max) {
    if (need <= *cap) {
        return VSV_OK;
    }
    if (need > max) {
        return VSV_TOOBIG;
    }
    size_t n = *cap < 64 ? 64 : *cap;
    while (n < need) {
        n = n > SIZE_MAX / 2 ? need : n * 2;
    }
    if (n > max) {
        n = max;
    }
    if (n > SIZE_MAX / elem) {
        return VSV_NOMEM;
    }
    void* p = realloc(*buf, n * elem);
    if (p == NULL) {
        return VSV_NOMEM;
    }
    *buf = p;
    *cap = n;
    return VSV_OK;
}

// vsv_reader_init initializes the reader to read from src with read,
// using comma as the field separator (checked by vsv_parse_comma).
// A line or record larger than max_size bytes is VSV_TOOBIG.
// Allocates nothing; free the reader with vsv_reader_free.
void vsv_reader_init(VsvReader* r, VsvReadFunc read, void* src, int32_t comma, size_t max_size) {
    memset(r, 0, sizeof(*r));
    r->read = read;
    r->src = src;
    r->comma_len = encode_rune(comma, r->comma);
    r->max_size = max_size;
}

// read_line reads the next line with its trailing \n, if any, as Go's
// Reader.readLine does: a \r\n at the end becomes \n, and a \r at the end of
// the input is dropped. Unlike Go, it also skips a byte order mark at the start
// of the input. Returns VSV_EOF only if there are no bytes left. The line
// is valid until the next call.
static VsvStatus read_line(VsvReader* r, const char** line, size_t* len) {
    static const char bom[] = "\xef\xbb\xbf";
    r->line_len = 0;
    bool nl = false;
    while (!nl) {
        if (r->buf_pos == r->buf_len) {
            if (r->eof) {
                break;
            }
            size_t n = 0;
            VsvStatus rc = r->read(r->src, r->buf, sizeof(r->buf), &n);
            if (rc != VSV_OK) {
                return rc;
            }
            if (n == 0) {
                r->eof = true;
                break;
            }
            r->buf_pos = 0;
            r->buf_len = n;
        }
        const char* start = r->buf + r->buf_pos;
        size_t avail = r->buf_len - r->buf_pos;
        const char* end = memchr(start, '\n', avail);
        size_t n = end != NULL ? (size_t)(end - start) + 1 : avail;
        VsvStatus rc =
            grow((void**)&r->line, &r->line_cap, r->line_len + n, sizeof(char), r->max_size);
        if (rc != VSV_OK) {
            return rc;
        }
        memcpy(r->line + r->line_len, start, n);
        r->line_len += n;
        r->buf_pos += n;
        nl = end != NULL;
    }
    // Go counts a line even when there's nothing to read.
    bool first = r->num_line == 0;
    r->num_line++;
    if (r->line_len == 0) {
        return VSV_EOF;
    }

    char* s = r->line;
    size_t n = r->line_len;
    if (first && n >= 3 && memcmp(s, bom, 3) == 0) {
        s += 3;
        n -= 3;
    }
    if (!nl && n > 0 && s[n - 1] == '\r') {
        // For backwards compatibility, Go drops a trailing \r before EOF.
        n--;
    }
    if (n >= 2 && s[n - 2] == '\r' && s[n - 1] == '\n') {
        s[n - 2] = '\n';
        n--;
    }
    *line = s;
    *len = n;
    return VSV_OK;
}

// length_nl returns the length of the trailing \n of line: 1 or 0.
static size_t length_nl(const char* line, size_t len) {
    return len > 0 && line[len - 1] == '\n' ? 1 : 0;
}

// find_comma returns the first separator in s, or NULL if there is none.
// For a multi-byte separator this is a byte search, as in Go's bytes.IndexRune.
static const char* find_comma(const VsvReader* r, const char* s, size_t n) {
    while (n >= r->comma_len) {
        const char* p = memchr(s, r->comma[0], n - r->comma_len + 1);
        if (p == NULL) {
            return NULL;
        }
        if (memcmp(p, r->comma, r->comma_len) == 0) {
            return p;
        }
        n -= (size_t)(p - s) + 1;
        s = p + 1;
    }
    return NULL;
}

// has_comma reports whether s starts with the separator.
static bool has_comma(const VsvReader* r, const char* s, size_t n) {
    return n >= r->comma_len && memcmp(s, r->comma, r->comma_len) == 0;
}

// append_rec appends n bytes of data to the current record.
static VsvStatus append_rec(VsvReader* r, const char* data, size_t n) {
    // memcpy with a NULL pointer is UB even for 0 bytes.
    if (n == 0) {
        return VSV_OK;
    }
    if (n > r->max_size - r->rec_len) {
        return VSV_TOOBIG;
    }
    VsvStatus rc = grow((void**)&r->rec, &r->rec_cap, r->rec_len + n, sizeof(char), r->max_size);
    if (rc != VSV_OK) {
        return rc;
    }
    memcpy(r->rec + r->rec_len, data, n);
    r->rec_len += n;
    return VSV_OK;
}

// end_field ends the current field of the record.
static VsvStatus end_field(VsvReader* r, bool quoted) {
    VsvStatus rc =
        grow((void**)&r->ends, &r->ends_cap, r->nfields + 1, sizeof(VsvFieldEnd), SIZE_MAX);
    if (rc != VSV_OK) {
        return rc;
    }
    r->ends[r->nfields].end = r->rec_len;
    r->ends[r->nfields].quoted = quoted;
    r->nfields++;
    return VSV_OK;
}

// parse_error records the position of a parse error and returns its status.
static VsvStatus parse_error(VsvReader* r,
                             VsvStatus status,
                             int64_t start,
                             int64_t line,
                             int64_t col) {
    r->err_start_line = start;
    r->err_pos.line = line;
    r->err_pos.col = col;
    return status;
}

// vsv_reader_next reads the next record. Returns VSV_OK and sets the fields,
// or VSV_EOF if there are no records left. VSV_BARE_QUOTE and VSV_QUOTE
// set the error position (err_start_line and err_pos). After an error,
// the reader can only be freed.
//
// A port of Go's Reader.readRecord with LazyQuotes, TrimLeadingSpace and
// Comment off and FieldsPerRecord = -1.
VsvStatus vsv_reader_next(VsvReader* r) {
    const char* line = NULL;
    size_t len = 0;
    VsvStatus rc;

    // Read line (automatically skipping past empty lines).
    do {
        rc = read_line(r, &line, &len);
        if (rc != VSV_OK) {
            return rc;
        }
    } while (len == length_nl(line, len));

    // Parse each field in the record.
    int64_t rec_line = r->num_line;  // starting line for record
    r->rec_len = 0;
    r->nfields = 0;
    VsvPos pos = {r->num_line, 1};
    for (;;) {
        if (len == 0 || line[0] != '"') {
            // Non-quoted string field.
            const char* comma = find_comma(r, line, len);
            size_t n = comma != NULL ? (size_t)(comma - line) : len - length_nl(line, len);
            // Check to make sure a quote does not appear in field.
            const char* quote = n > 0 ? memchr(line, '"', n) : NULL;
            if (quote != NULL) {
                int64_t col = pos.col + (int64_t)(quote - line);
                return parse_error(r, VSV_BARE_QUOTE, rec_line, r->num_line, col);
            }
            if ((rc = append_rec(r, line, n)) != VSV_OK || (rc = end_field(r, false)) != VSV_OK) {
                return rc;
            }
            if (comma == NULL) {
                return VSV_OK;
            }
            size_t skip = n + r->comma_len;
            line += skip;
            len -= skip;
            pos.col += (int64_t)skip;
            continue;
        }

        // Quoted string field.
        line++;
        len--;
        pos.col++;
        for (;;) {
            const char* quote = memchr(line, '"', len);
            if (quote != NULL) {
                // Hit next quote.
                size_t n = (size_t)(quote - line);
                if ((rc = append_rec(r, line, n)) != VSV_OK) {
                    return rc;
                }
                line += n + 1;
                len -= n + 1;
                pos.col += (int64_t)n + 1;
                if (len > 0 && line[0] == '"') {
                    // `""` sequence (append quote).
                    if ((rc = append_rec(r, "\"", 1)) != VSV_OK) {
                        return rc;
                    }
                    line++;
                    len--;
                    pos.col++;
                } else if (has_comma(r, line, len)) {
                    // `",` sequence (end of field).
                    line += r->comma_len;
                    len -= r->comma_len;
                    pos.col += (int64_t)r->comma_len;
                    if ((rc = end_field(r, true)) != VSV_OK) {
                        return rc;
                    }
                    break;
                } else if (length_nl(line, len) == len) {
                    // `"\n` sequence (end of line).
                    return end_field(r, true);
                } else {
                    // `"*` sequence (invalid non-escaped quote).
                    return parse_error(r, VSV_QUOTE, rec_line, r->num_line, pos.col - 1);
                }
            } else if (len > 0) {
                // Hit end of line (copy all data so far).
                if ((rc = append_rec(r, line, len)) != VSV_OK) {
                    return rc;
                }
                pos.col += (int64_t)len;
                rc = read_line(r, &line, &len);
                if (rc == VSV_EOF) {
                    len = 0;
                } else if (rc != VSV_OK) {
                    return rc;
                } else if (len > 0) {
                    pos.line++;
                    pos.col = 1;
                }
            } else {
                // Abrupt end of file.
                return parse_error(r, VSV_QUOTE, rec_line, pos.line, pos.col);
            }
        }
    }
}

// vsv_reader_count returns the number of fields in the current record.
size_t vsv_reader_count(const VsvReader* r) {
    return r->nfields;
}

// vsv_reader_field returns field i of the current record (i < vsv_reader_count),
// and sets len to its length. The field is not NUL-terminated and is valid
// until the next call to vsv_reader_next or vsv_reader_free. It's never NULL,
// even for an empty field.
const char* vsv_reader_field(const VsvReader* r, size_t i, size_t* len) {
    size_t start = i == 0 ? 0 : r->ends[i - 1].end;
    *len = r->ends[i].end - start;
    // rec is NULL if every field so far was empty,
    // and NULL + 0 is UB.
    return r->rec != NULL ? r->rec + start : "";
}

// vsv_reader_quoted reports whether field i of the current record
// (i < vsv_reader_count) was quoted.
bool vsv_reader_quoted(const VsvReader* r, size_t i) {
    return r->ends[i].quoted;
}

// vsv_reader_free frees the memory used by the reader. Doesn't touch src.
void vsv_reader_free(VsvReader* r) {
    free(r->line);
    free(r->rec);
    free(r->ends);
    r->line = NULL;
    r->rec = NULL;
    r->ends = NULL;
    r->line_cap = r->rec_cap = r->ends_cap = 0;
}

#pragma endregion
