/* File: envfuncs.c
**
** Author: John Howie (john@howieconsultinginc.com)
**
** Description
**
** This file contains the source code for the library that implements the
** environment variables functions extension for SQLite3
**
**
** Modifications
**
** 2020-10-09 John Howie    Original.
**
** 2020-10-11 John Howie    Added three functions - one to return the length of
**                          the environment variable (if found), one to
**                          determine whether or not the variable exists, and
**                          one to return a specificied default value if it
**                          does not exist.
*/

/*

Copyright 2020 Howie Consulting Inc.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in
   the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.

*/

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

static const char* CopyrightNotice = "envfuncs.c (c) 2020 Howie Consulting Inc.";

// static void sqlite3_getenv (sqlite3_context *context, int argc, sqlite3_value **argv)
//
// This function is the implementation of the SQLite3 extension function
// GETENV(). The function takes as an argument a string, which is used in a
// search of environment variables. If a match is found the value of the
// environmental variable is returned. If the variable does not exist a NULL
// value is returned.
//
// NOTE: The extension function itself does not care about the case of the
//       name of the environment variable, but the underlying library or OS
//       most likely will.
//

static void sqlite3_getenv(sqlite3_context* context, int argc, sqlite3_value** argv) {
    const char *EnvVarName, *EnvVarValuePtr;
    char* EnvVarValueBuffer;
    int EnvVarValueLength = 0;

    // Make sure we got only a single argument, and that the argument is not
    // a NULL string

    assert(argc == 1);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL)
        return;

    // Get the argument supplied, which is the name of the environment variable
    // we need to look up

    EnvVarName = (const char*)sqlite3_value_text(argv[0]);

    // Lookup the environment variable. If the variable does not exist just
    // return

    EnvVarValuePtr = getenv(EnvVarName);
    if (EnvVarValuePtr == (const char*)0) {
        // The environmental variable does not exist, so just return NULL

        sqlite3_result_null(context);
        return;
    }

    // Get the length of the value associated with the environment value,
    // and allocate buffer to hold the value. We add one byte to the value
    // for the NULL terminating character

    EnvVarValueLength = strlen(EnvVarValuePtr);
    EnvVarValueBuffer = (char*)sqlite3_malloc(EnvVarValueLength + 1);
    if (EnvVarValueBuffer == (char*)0) {
        // An error occurred, and we could not allocate the memory required

        sqlite3_result_error_nomem(context);
        return;
    }

    // Set the buffer contents to NULL, and then copy over the value to
    // the buffer

    bzero((void*)EnvVarValueBuffer, (EnvVarValueLength + 1));
    bcopy((const void*)EnvVarValuePtr, (void*)EnvVarValueBuffer, EnvVarValueLength);

    // Set the result, free up memory, and then return

    sqlite3_result_text(context, (const char*)EnvVarValueBuffer, -1, sqlite3_free);
}

// static void sqlite3_getenvlen (sqlite3_context *context, int argc, sqlite3_value **argv)
//
// This function is the implementation of the SQLite3 extension function
// GETENVLEN(). The function takes as an argument a string, which is used in a
// search of environment variables. If a match is found the length of the
// environmental variable is returned. If the variable does not exist a NULL
// value is returned.
//
// NOTE: The extension function itself does not care about the case of the
//       name of the environment variable, but the underlying library or OS
//       most likely will.
//

static void sqlite3_getenvlen(sqlite3_context* context, int argc, sqlite3_value** argv) {
    const char *EnvVarName, *EnvVarValuePtr;
    char* EnvVarValueBuffer;
    int EnvVarValueLength = 0;

    // Make sure we got only a single argument, and that the argument is not
    // a NULL string

    assert(argc == 1);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL)
        return;

    // Get the argument supplied, which is the name of the environment variable
    // we need to look up

    EnvVarName = (const char*)sqlite3_value_text(argv[0]);

    // Lookup the environment variable. If the variable does not exist we
    // return a NULL value, otherwise we return its length

    EnvVarValuePtr = getenv(EnvVarName);
    if (EnvVarValuePtr == (const char*)0) {
        // The value does not exist, so return NULL

        sqlite3_result_null(context);
    } else {
        // Get the length of the value associated with the environmental value
        // and return it

        EnvVarValueLength = strlen(EnvVarValuePtr);
        sqlite3_result_int(context, EnvVarValueLength);
    }
}

// static void sqlite3_getenvexists (sqlite3_context *context, int argc, sqlite3_value
// **argv)
//
// This function is an implementation of the SQLite3 extension function
// GETENVEXISTS(). The function takes as an argument a string, which is used in
// a search of environment variables. If a match is found the function returns
// the value '1', otherwise it returns '0'
//
// NOTE: The extension function itself does not care about the case of the
//       name of the environment variable, but the underlying library or OS
//       most likely will.
//

static void sqlite3_getenvexists(sqlite3_context* context, int argc, sqlite3_value** argv) {
    const char *EnvVarName, *EnvVarValuePtr;
    char* EnvVarValueBuffer;

    // Make sure we got only a single argument, and that the argument is not
    // a NULL string

    assert(argc == 1);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL)
        return;

    // Get the argument supplied, which is the name of the environment variable
    // we need to look up

    EnvVarName = (const char*)sqlite3_value_text(argv[0]);

    // Lookup the environment variable. If the variable does not exist we
    // return a NULL value, otherwise we return its length

    EnvVarValuePtr = getenv(EnvVarName);
    if (EnvVarValuePtr == (const char*)0) {
        // The value does not exist, so return 0

        sqlite3_result_int(context, 0);
    } else {
        // The value exists so return 1

        sqlite3_result_int(context, 1);
    }
}

// static void sqlite3_getenvdefault (sqlite3_context *context, int argc, sqlite3_value
// **argv)
//
// This function is an implementation of the SQLite3 extension function
// GETENVDEFAULT(). The function takes as an argument a string, which is used
// in a search of environment variables. If a match is found the value of the
// environmental variable is returned. If the variable does not exist the
// default string is returned instead.
//
// NOTE: The extension function itself does not care about the case of the
//       name of the environment variable, but the underlying library or OS
//       most likely will.
//

static void sqlite3_getenvdefault(sqlite3_context* context, int argc, sqlite3_value** argv) {
    const char *EnvVarName, *DefaultValuePtr, *EnvVarValuePtr, *ValueToReturnPtr;
    char* ValueToReturnBuffer;
    int ValueToReturnLength = 0;

    // Make sure we got only two arguments, and that the first argument is not
    // a NULL string. We need the first string to lookup the environment
    // variable, but it is acceptable to specify a NULL as the default

    assert(argc == 2);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL)
        return;

    // Get the first argument supplied, which is the name of the environment
    // variable we need to look up

    EnvVarName = (const char*)sqlite3_value_text(argv[0]);

    // Lookup the environment variable. If the variable does not exist we will
    // use the default value provided

    EnvVarValuePtr = getenv(EnvVarName);
    if (EnvVarValuePtr == (const char*)0) {
        // The environment variable does not exist, so fetch the default value

        if (sqlite3_value_type(argv[1]) == SQLITE_NULL) {
            // The default value is a NULL, so we can make this really easy
            // and just return a NULL here

            sqlite3_result_null(context);
            return;
        } else {
            // Get the default value we are to use, and make the value we will
            // return the default value

            DefaultValuePtr = (const char*)sqlite3_value_text(argv[1]);
            ValueToReturnPtr = DefaultValuePtr;
        }
    } else {
        // The environment variable does exist so we use that

        ValueToReturnPtr = EnvVarValuePtr;
    }

    // Get the length of the value associated with the value to return,
    // and allocate buffer to hold the value. We add one byte to the value
    // for the NULL terminating character

    ValueToReturnLength = strlen(ValueToReturnPtr);
    ValueToReturnBuffer = (char*)sqlite3_malloc(ValueToReturnLength + 1);
    if (ValueToReturnBuffer == (char*)0) {
        // An error occurred, and we could not allocate the memory required

        sqlite3_result_error_nomem(context);
        return;
    }

    // Set the buffer contents to NULL, and then copy over the value to
    // the buffer

    bzero((void*)ValueToReturnBuffer, (ValueToReturnLength + 1));
    bcopy((const void*)ValueToReturnPtr, (void*)ValueToReturnBuffer, ValueToReturnLength);

    // Set the result, free up memory, and then return

    sqlite3_result_text(context, (const char*)ValueToReturnBuffer, -1, sqlite3_free);
}

// int sqlite3_envfuncs_init (sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi)
//
// This is the entry function for the library, and registers the extension
// functions
//

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_envfuncs_init(sqlite3* db, char** pzErrMsg, const sqlite3_api_routines* pApi) {
    SQLITE_EXTENSION_INIT2(pApi);
    int flags = SQLITE_UTF8 | SQLITE_DETERMINISTIC;
    sqlite3_create_function(db, "getenv", 1, flags, 0, sqlite3_getenv, 0, 0);
    sqlite3_create_function(db, "getenvlen", 1, flags, 0, sqlite3_getenvlen, 0, 0);
    sqlite3_create_function(db, "getenvexists", 1, flags, 0, sqlite3_getenvexists, 0, 0);
    sqlite3_create_function(db, "getenvdefault", 2, flags, 0, sqlite3_getenvdefault, 0, 0);
    return SQLITE_OK;
}