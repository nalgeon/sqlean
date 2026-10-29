// Copyright (c) 2026 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Character classes adapted from the spellfix SQLite extension, Public Domain
// https://www.sqlite.org/src/file/ext/misc/spellfix.c

// UTF-8 decoding based on Go's unicode/utf8 package, BSD 3-Clause License
// https://github.com/golang/go

// Helpers shared by the domain files.

#ifndef FUZZY_COMMON_H
#define FUZZY_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#pragma region Characters

// is_ascii reports whether the n bytes of s are all ASCII characters.
static inline bool is_ascii(const char* s, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if ((unsigned char)s[i] >= 0x80) {
            return false;
        }
    }
    return true;
}

// is_alpha reports whether c is an ASCII letter. Unlike isalpha,
// it doesn't depend on the locale and accepts any char value.
static inline bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

// to_lower converts an ASCII letter to lowercase and leaves
// any other character as is.
static inline char to_lower(char c) {
    return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
}

// to_upper converts an ASCII letter to uppercase and leaves
// any other character as is.
static inline char to_upper(char c) {
    return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
}

#pragma endregion

#pragma region Character classes

// Character classes for ASCII characters, used by the spellfix functions
// (editdist, phonetic, script):
//
//   0   ''        Silent letters:   H W
//   1   'A'       Any vowel:   A E I O U (Y)
//   2   'B'       A bilabeal stop or fricative:  B F P V W
//   3   'C'       Other fricatives or back stops:  C G J K Q S X Z
//   4   'D'       Alveolar stops:  D T
//   5   'H'       Letter H at the beginning of a word
//   6   'L'       Glide:  L
//   7   'R'       Semivowel:  R
//   8   'M'       Nasals:  M N
//   9   'Y'       Letter Y at the beginning of a word.
//   10  '9'       Digits: 0 1 2 3 4 5 6 7 8 9
//   11  ' '       White space
//   12  '?'       Other.
#define CCLASS_SILENT 0
#define CCLASS_VOWEL 1
#define CCLASS_B 2
#define CCLASS_C 3
#define CCLASS_D 4
#define CCLASS_H 5
#define CCLASS_L 6
#define CCLASS_R 7
#define CCLASS_M 8
#define CCLASS_Y 9
#define CCLASS_DIGIT 10
#define CCLASS_SPACE 11
#define CCLASS_OTHER 12

// mid_class gives the character class for non-initial ASCII characters.
static const unsigned char mid_class[128] = {
    /*   */ CCLASS_OTHER,  /*   */ CCLASS_OTHER, /*   */ CCLASS_OTHER,
    /*   */ CCLASS_OTHER,  /*   */ CCLASS_OTHER, /*   */ CCLASS_OTHER,
    /*   */ CCLASS_OTHER,  /*   */ CCLASS_OTHER, /*   */ CCLASS_OTHER,
    /*   */ CCLASS_SPACE,  /*   */ CCLASS_OTHER, /*   */ CCLASS_OTHER,
    /*   */ CCLASS_SPACE,  /*   */ CCLASS_SPACE, /*   */ CCLASS_OTHER,
    /*   */ CCLASS_OTHER,  /*   */ CCLASS_OTHER, /*   */ CCLASS_OTHER,
    /*   */ CCLASS_OTHER,  /*   */ CCLASS_OTHER, /*   */ CCLASS_OTHER,
    /*   */ CCLASS_OTHER,  /*   */ CCLASS_OTHER, /*   */ CCLASS_OTHER,
    /*   */ CCLASS_OTHER,  /*   */ CCLASS_OTHER, /*   */ CCLASS_OTHER,
    /*   */ CCLASS_OTHER,  /*   */ CCLASS_OTHER, /*   */ CCLASS_OTHER,
    /*   */ CCLASS_OTHER,  /*   */ CCLASS_OTHER, /*   */ CCLASS_SPACE,
    /* ! */ CCLASS_OTHER,  /* " */ CCLASS_OTHER, /* # */ CCLASS_OTHER,
    /* $ */ CCLASS_OTHER,  /* % */ CCLASS_OTHER, /* & */ CCLASS_OTHER,
    /* ' */ CCLASS_SILENT, /* ( */ CCLASS_OTHER, /* ) */ CCLASS_OTHER,
    /* * */ CCLASS_OTHER,  /* + */ CCLASS_OTHER, /* , */ CCLASS_OTHER,
    /* - */ CCLASS_OTHER,  /* . */ CCLASS_OTHER, /* / */ CCLASS_OTHER,
    /* 0 */ CCLASS_DIGIT,  /* 1 */ CCLASS_DIGIT, /* 2 */ CCLASS_DIGIT,
    /* 3 */ CCLASS_DIGIT,  /* 4 */ CCLASS_DIGIT, /* 5 */ CCLASS_DIGIT,
    /* 6 */ CCLASS_DIGIT,  /* 7 */ CCLASS_DIGIT, /* 8 */ CCLASS_DIGIT,
    /* 9 */ CCLASS_DIGIT,  /* : */ CCLASS_OTHER, /* ; */ CCLASS_OTHER,
    /* < */ CCLASS_OTHER,  /* = */ CCLASS_OTHER, /* > */ CCLASS_OTHER,
    /* ? */ CCLASS_OTHER,  /* @ */ CCLASS_OTHER, /* A */ CCLASS_VOWEL,
    /* B */ CCLASS_B,      /* C */ CCLASS_C,     /* D */ CCLASS_D,
    /* E */ CCLASS_VOWEL,  /* F */ CCLASS_B,     /* G */ CCLASS_C,
    /* H */ CCLASS_SILENT, /* I */ CCLASS_VOWEL, /* J */ CCLASS_C,
    /* K */ CCLASS_C,      /* L */ CCLASS_L,     /* M */ CCLASS_M,
    /* N */ CCLASS_M,      /* O */ CCLASS_VOWEL, /* P */ CCLASS_B,
    /* Q */ CCLASS_C,      /* R */ CCLASS_R,     /* S */ CCLASS_C,
    /* T */ CCLASS_D,      /* U */ CCLASS_VOWEL, /* V */ CCLASS_B,
    /* W */ CCLASS_B,      /* X */ CCLASS_C,     /* Y */ CCLASS_VOWEL,
    /* Z */ CCLASS_C,      /* [ */ CCLASS_OTHER, /* \ */ CCLASS_OTHER,
    /* ] */ CCLASS_OTHER,  /* ^ */ CCLASS_OTHER, /* _ */ CCLASS_OTHER,
    /* ` */ CCLASS_OTHER,  /* a */ CCLASS_VOWEL, /* b */ CCLASS_B,
    /* c */ CCLASS_C,      /* d */ CCLASS_D,     /* e */ CCLASS_VOWEL,
    /* f */ CCLASS_B,      /* g */ CCLASS_C,     /* h */ CCLASS_SILENT,
    /* i */ CCLASS_VOWEL,  /* j */ CCLASS_C,     /* k */ CCLASS_C,
    /* l */ CCLASS_L,      /* m */ CCLASS_M,     /* n */ CCLASS_M,
    /* o */ CCLASS_VOWEL,  /* p */ CCLASS_B,     /* q */ CCLASS_C,
    /* r */ CCLASS_R,      /* s */ CCLASS_C,     /* t */ CCLASS_D,
    /* u */ CCLASS_VOWEL,  /* v */ CCLASS_B,     /* w */ CCLASS_B,
    /* x */ CCLASS_C,      /* y */ CCLASS_VOWEL, /* z */ CCLASS_C,
    /* { */ CCLASS_OTHER,  /* | */ CCLASS_OTHER, /* } */ CCLASS_OTHER,
    /* ~ */ CCLASS_OTHER,  /*   */ CCLASS_OTHER,
};

// init_class gives the character class for ASCII characters that form
// the initial character of a word. The only difference from mid_class
// is with the letters H, W, and Y.
static const unsigned char init_class[128] = {
    /*   */ CCLASS_OTHER,  /*   */ CCLASS_OTHER, /*   */ CCLASS_OTHER,
    /*   */ CCLASS_OTHER,  /*   */ CCLASS_OTHER, /*   */ CCLASS_OTHER,
    /*   */ CCLASS_OTHER,  /*   */ CCLASS_OTHER, /*   */ CCLASS_OTHER,
    /*   */ CCLASS_SPACE,  /*   */ CCLASS_OTHER, /*   */ CCLASS_OTHER,
    /*   */ CCLASS_SPACE,  /*   */ CCLASS_SPACE, /*   */ CCLASS_OTHER,
    /*   */ CCLASS_OTHER,  /*   */ CCLASS_OTHER, /*   */ CCLASS_OTHER,
    /*   */ CCLASS_OTHER,  /*   */ CCLASS_OTHER, /*   */ CCLASS_OTHER,
    /*   */ CCLASS_OTHER,  /*   */ CCLASS_OTHER, /*   */ CCLASS_OTHER,
    /*   */ CCLASS_OTHER,  /*   */ CCLASS_OTHER, /*   */ CCLASS_OTHER,
    /*   */ CCLASS_OTHER,  /*   */ CCLASS_OTHER, /*   */ CCLASS_OTHER,
    /*   */ CCLASS_OTHER,  /*   */ CCLASS_OTHER, /*   */ CCLASS_SPACE,
    /* ! */ CCLASS_OTHER,  /* " */ CCLASS_OTHER, /* # */ CCLASS_OTHER,
    /* $ */ CCLASS_OTHER,  /* % */ CCLASS_OTHER, /* & */ CCLASS_OTHER,
    /* ' */ CCLASS_OTHER,  /* ( */ CCLASS_OTHER, /* ) */ CCLASS_OTHER,
    /* * */ CCLASS_OTHER,  /* + */ CCLASS_OTHER, /* , */ CCLASS_OTHER,
    /* - */ CCLASS_OTHER,  /* . */ CCLASS_OTHER, /* / */ CCLASS_OTHER,
    /* 0 */ CCLASS_DIGIT,  /* 1 */ CCLASS_DIGIT, /* 2 */ CCLASS_DIGIT,
    /* 3 */ CCLASS_DIGIT,  /* 4 */ CCLASS_DIGIT, /* 5 */ CCLASS_DIGIT,
    /* 6 */ CCLASS_DIGIT,  /* 7 */ CCLASS_DIGIT, /* 8 */ CCLASS_DIGIT,
    /* 9 */ CCLASS_DIGIT,  /* : */ CCLASS_OTHER, /* ; */ CCLASS_OTHER,
    /* < */ CCLASS_OTHER,  /* = */ CCLASS_OTHER, /* > */ CCLASS_OTHER,
    /* ? */ CCLASS_OTHER,  /* @ */ CCLASS_OTHER, /* A */ CCLASS_VOWEL,
    /* B */ CCLASS_B,      /* C */ CCLASS_C,     /* D */ CCLASS_D,
    /* E */ CCLASS_VOWEL,  /* F */ CCLASS_B,     /* G */ CCLASS_C,
    /* H */ CCLASS_SILENT, /* I */ CCLASS_VOWEL, /* J */ CCLASS_C,
    /* K */ CCLASS_C,      /* L */ CCLASS_L,     /* M */ CCLASS_M,
    /* N */ CCLASS_M,      /* O */ CCLASS_VOWEL, /* P */ CCLASS_B,
    /* Q */ CCLASS_C,      /* R */ CCLASS_R,     /* S */ CCLASS_C,
    /* T */ CCLASS_D,      /* U */ CCLASS_VOWEL, /* V */ CCLASS_B,
    /* W */ CCLASS_B,      /* X */ CCLASS_C,     /* Y */ CCLASS_Y,
    /* Z */ CCLASS_C,      /* [ */ CCLASS_OTHER, /* \ */ CCLASS_OTHER,
    /* ] */ CCLASS_OTHER,  /* ^ */ CCLASS_OTHER, /* _ */ CCLASS_OTHER,
    /* ` */ CCLASS_OTHER,  /* a */ CCLASS_VOWEL, /* b */ CCLASS_B,
    /* c */ CCLASS_C,      /* d */ CCLASS_D,     /* e */ CCLASS_VOWEL,
    /* f */ CCLASS_B,      /* g */ CCLASS_C,     /* h */ CCLASS_SILENT,
    /* i */ CCLASS_VOWEL,  /* j */ CCLASS_C,     /* k */ CCLASS_C,
    /* l */ CCLASS_L,      /* m */ CCLASS_M,     /* n */ CCLASS_M,
    /* o */ CCLASS_VOWEL,  /* p */ CCLASS_B,     /* q */ CCLASS_C,
    /* r */ CCLASS_R,      /* s */ CCLASS_C,     /* t */ CCLASS_D,
    /* u */ CCLASS_VOWEL,  /* v */ CCLASS_B,     /* w */ CCLASS_B,
    /* x */ CCLASS_C,      /* y */ CCLASS_Y,     /* z */ CCLASS_C,
    /* { */ CCLASS_OTHER,  /* | */ CCLASS_OTHER, /* } */ CCLASS_OTHER,
    /* ~ */ CCLASS_OTHER,  /*   */ CCLASS_OTHER,
};

// class_name maps a character class to its symbol.
static const char class_name[] = ".ABCDHLRMY9 ?";

// char_class returns the class of the ASCII character c following prev,
// where prev is 0 for the first character of a word.
static inline unsigned char char_class(char prev, char c) {
    return prev == 0 ? init_class[c & 0x7f] : mid_class[c & 0x7f];
}

#pragma endregion

#pragma region Memory

// alloc_array allocates an array of count elements of the given size.
// Returns NULL if out of memory or if the total size overflows.
// Never returns NULL for a zero count.
static inline void* alloc_array(size_t count, size_t size) {
    if (count == 0) {
        count = 1;
    }
    if (count > SIZE_MAX / size) {
        return NULL;
    }
    return malloc(count * size);
}

#pragma endregion

#pragma region UTF-8

// RUNE_ERROR is the code point that replaces an invalid UTF-8 sequence.
#define RUNE_ERROR 0xFFFD

// utf8_decode decodes the first code point of the UTF-8 string s of n > 0
// bytes and sets size to its length in bytes. An invalid sequence decodes
// to RUNE_ERROR with size 1, as in Go's utf8.DecodeRuneInString: overlong
// encodings, surrogates and values above U+10FFFF are invalid.
static inline uint32_t utf8_decode(const char* s, size_t n, size_t* size) {
    const unsigned char* p = (const unsigned char*)s;
    uint32_t c = p[0];
    *size = 1;
    if (c < 0x80) {
        return c;
    }

    // the length of the sequence, and the range of the second byte
    // that rules out overlong encodings, surrogates and values > U+10FFFF
    size_t sz;
    unsigned char lo = 0x80, hi = 0xBF;
    if (c >= 0xC2 && c <= 0xDF) {
        sz = 2;
        c &= 0x1F;
    } else if (c >= 0xE0 && c <= 0xEF) {
        sz = 3;
        lo = (c == 0xE0) ? 0xA0 : 0x80;
        hi = (c == 0xED) ? 0x9F : 0xBF;
        c &= 0x0F;
    } else if (c >= 0xF0 && c <= 0xF4) {
        sz = 4;
        lo = (c == 0xF0) ? 0x90 : 0x80;
        hi = (c == 0xF4) ? 0x8F : 0xBF;
        c &= 0x07;
    } else {
        return RUNE_ERROR;
    }

    if (n < sz || p[1] < lo || p[1] > hi) {
        return RUNE_ERROR;
    }
    c = (c << 6) | (p[1] & 0x3F);
    for (size_t i = 2; i < sz; i++) {
        if (p[i] < 0x80 || p[i] > 0xBF) {
            return RUNE_ERROR;
        }
        c = (c << 6) | (p[i] & 0x3F);
    }
    *size = sz;
    return c;
}

// utf8_runes decodes the UTF-8 string s of n bytes into a new array
// of code points and sets count to their number. Returns NULL if out
// of memory. The caller frees the array with free.
static inline uint32_t* utf8_runes(const char* s, size_t n, size_t* count) {
    // a string of n bytes has at most n characters
    uint32_t* runes = alloc_array(n, sizeof(uint32_t));
    if (runes == NULL) {
        return NULL;
    }
    size_t i = 0, k = 0;
    while (i < n) {
        size_t size;
        runes[k++] = utf8_decode(s + i, n - i, &size);
        i += size;
    }
    *count = k;
    return runes;
}

#pragma endregion

#endif /* FUZZY_COMMON_H */
