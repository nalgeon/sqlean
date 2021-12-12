// Originally by Henry Spencer, Spencer License 94
// https://github.com/garyhouston/regexp.old
// Modified by Anton Zhiyanov, https://github.com/nalgeon/sqlean/, MIT License

/*
 * Copyright (c) 1986, 1993, 1995 by University of Toronto.
 * Written by Henry Spencer.  Not derived from licensed software.
 *
 * Permission is granted to anyone to use this software for any
 * purpose on any computer system, and to redistribute it in any way,
 * subject to the following restrictions:
 *
 * 1. The author is not responsible for the consequences of use of
 * 	this software, no matter how awful, even if they arise
 * 	from defects in it.
 *
 * 2. The origin of this software must not be misrepresented, either
 * 	by explicit claim or by omission.
 *
 * 3. Altered versions must be plainly marked as such, and must not
 * 	be misrepresented (by explicit claim or omission) as being
 * 	the original software.
 *
 * 4. This notice must not be removed or altered.
 */

/*
 * Definitions etc. for regexp(3) routines.
 *
 * Caveat:  this is V8 regexp(3) [actually, a reimplementation thereof],
 * not the System V one.
 */
#define NSUBEXP 10
typedef struct regexp {
    char* startp[NSUBEXP];
    char* endp[NSUBEXP];
    char regstart;   /* Internal use only. */
    char reganch;    /* Internal use only. */
    char* regmust;   /* Internal use only. */
    int regmlen;     /* Internal use only. */
    char program[1]; /* Unwarranted chumminess with compiler. */
} regexp;

regexp* re_compile(const char* re);
int re_execute(regexp* rp, const char* s);
int re_substitute(const regexp* rp, const char* src, char* dst);
void re_error(char* message);
