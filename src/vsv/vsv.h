// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Based on Go's encoding/csv package, BSD 3-Clause License
// https://github.com/golang/go

// Reading CSV records, and converting their fields.

// The reader follows RFC 4180, as Go's encoding/csv Reader does with its
// default settings:
//
// - Records are separated by \n. A \r\n is read as \n everywhere, including
//   inside quoted fields, and a \r at the end of the input is dropped.
//   The last record may or may not end with a newline.
// - Fields are separated by a comma, or another single character.
// - Blank lines are skipped. A line of spaces is not blank.
// - A field that starts with " is quoted: it ends at the next " that is not
//   doubled, and can contain separators and newlines. A "" inside it is a ".
// - Spaces are part of a field.
//
// Deviations from Go:
//
// - A UTF-8 byte order mark at the start of the input is skipped, and
//   columns on the first line are counted after it.
// - An error doesn't return the fields read before it.
// - A read error is reported as is, without the record read before it.
// - A line or record larger than the size limit is an error.
// - The reader tells whether each field was quoted.
//
// Records can have different numbers of fields (FieldsPerRecord = -1 in Go).
// Fields are bytes: the reader doesn't validate UTF-8.

#ifndef VSV_VSV_H
#define VSV_VSV_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// VsvStatus is the outcome of a reader function.
typedef enum {
    VSV_OK,
    VSV_EOF,         // no more records
    VSV_BARE_QUOTE,  // a " in an unquoted field (Go's ErrBareQuote)
    VSV_QUOTE,       // an extraneous or missing " in a quoted field (Go's ErrQuote)
    VSV_NOMEM,       // out of memory
    VSV_TOOBIG,      // a line or record is larger than the size limit
    VSV_IOERR,       // reading the input failed
} VsvStatus;

// Input.

// VsvReadFunc reads up to size bytes from src into buf, and sets n to
// the number of bytes read. Setting n to 0 means the end of the input.
// Returns VSV_OK, or VSV_IOERR if reading failed.
typedef VsvStatus (*VsvReadFunc)(void* src, char* buf, size_t size, size_t* n);

// vsv_open_file opens the file at path, given in UTF-8, for reading.
// Returns 0 or an errno value. The caller closes the file with fclose.
int vsv_open_file(const char* path, FILE** file);

// vsv_read_file is a VsvReadFunc that reads from src, an open FILE*.
VsvStatus vsv_read_file(void* src, char* buf, size_t size, size_t* n);

// VsvBuffer is the input for vsv_read_buffer: len bytes at data.
// Reading advances data and decreases len.
typedef struct {
    const char* data;
    size_t len;
} VsvBuffer;

// vsv_read_buffer is a VsvReadFunc that reads from src, a VsvBuffer*.
VsvStatus vsv_read_buffer(void* src, char* buf, size_t size, size_t* n);

// Separator.

// vsv_parse_comma sets comma to the code point of s, the n bytes of a field
// separator. Returns false unless s is a single valid UTF-8 character
// other than NUL, ", \r, \n and U+FFFD (as Go's Reader.Comma).
bool vsv_parse_comma(const char* s, size_t n, int32_t* comma);

// Reader.

// VSV_BUF_SIZE is the size of the reader's input buffer.
#define VSV_BUF_SIZE 4096

// VsvPos is a position in the input. Lines and columns start at 1,
// and columns are counted in bytes.
typedef struct {
    int64_t line;
    int64_t col;
} VsvPos;

// VsvFieldEnd is where a field of the current record ends,
// and whether it was quoted.
typedef struct {
    size_t end;
    bool quoted;
} VsvFieldEnd;

// VsvReader reads records from an input. Use the vsv_reader functions,
// and read the fields only through vsv_reader_field.
typedef struct {
    // input
    VsvReadFunc read;
    void* src;
    char buf[VSV_BUF_SIZE];
    size_t buf_pos;  // next unread byte in buf
    size_t buf_len;  // bytes in buf
    bool eof;        // read reported the end of the input

    // options
    char comma[4];  // field separator, encoded as UTF-8
    size_t comma_len;
    size_t max_size;  // the size limit for a line and for a record

    // the current line
    char* line;
    size_t line_len;
    size_t line_cap;
    int64_t num_line;  // the number of lines read

    // the current record: the fields one after another in rec,
    // field i ends at ends[i].end
    char* rec;
    size_t rec_len;
    size_t rec_cap;
    VsvFieldEnd* ends;
    size_t nfields;
    size_t ends_cap;

    // the position of the last parse error
    int64_t err_start_line;  // the line where the record starts
    VsvPos err_pos;          // where the error occurred
} VsvReader;

// vsv_reader_init initializes the reader to read from src with read,
// using comma as the field separator (checked by vsv_parse_comma).
// A line or record larger than max_size bytes is VSV_TOOBIG.
// Allocates nothing; free the reader with vsv_reader_free.
void vsv_reader_init(VsvReader* r, VsvReadFunc read, void* src, int32_t comma, size_t max_size);

// vsv_reader_next reads the next record. Returns VSV_OK and sets the fields,
// or VSV_EOF if there are no records left. VSV_BARE_QUOTE and VSV_QUOTE
// set the error position (err_start_line and err_pos). After an error,
// the reader can only be freed.
VsvStatus vsv_reader_next(VsvReader* r);

// vsv_reader_count returns the number of fields in the current record.
size_t vsv_reader_count(const VsvReader* r);

// vsv_reader_field returns field i of the current record (i < vsv_reader_count),
// and sets len to its length. The field is not NUL-terminated and is valid
// until the next call to vsv_reader_next or vsv_reader_free. It's never NULL,
// even for an empty field.
const char* vsv_reader_field(const VsvReader* r, size_t i, size_t* len);

// vsv_reader_quoted reports whether field i of the current record
// (i < vsv_reader_count) was quoted. Not in Go, which can't tell
// an empty field from "".
bool vsv_reader_quoted(const VsvReader* r, size_t i);

// vsv_reader_free frees the memory used by the reader. Doesn't touch src.
void vsv_reader_free(VsvReader* r);

// Values.

// VsvAffinity is SQLite's type affinity of a column.
typedef enum {
    VSV_AFF_BLOB,  // no conversion (a column without a declared type)
    VSV_AFF_TEXT,
    VSV_AFF_NUMERIC,
    VSV_AFF_INTEGER,
    VSV_AFF_REAL,
} VsvAffinity;

// vsv_affinity returns the affinity of a column with the declared type,
// following SQLite's rules (https://sqlite.org/datatype3.html#affname).
// An empty type is VSV_AFF_BLOB.
VsvAffinity vsv_affinity(const char* type);

// vsv_real_to_int sets i to the value of r and returns true if r is an integer
// that SQLite's INTEGER and NUMERIC affinities store as one: an exact integer
// that fits in int64_t, except for its smallest and largest values.
bool vsv_real_to_int(double r, int64_t* i);

#endif /* VSV_VSV_H */
