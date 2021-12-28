/* Extra JSON functions */

/*
Copyright 2018 Shawn Wagner

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

static cJSON* json_find_path(cJSON* json, char* path, int* err) {
    *err = 0;

    if (!path) {
        *err = 1;
        return NULL;
    }
    if (path[0] != '$') {
        *err = 1;
        return NULL;
    }

    for (size_t i = 1; path[i] && json; i += 1) {
        if (path[i] == '.') {
            i += 1;
            size_t eok = strcspn(path + i, ".[");
            char saved = path[i + eok];
            path[i + eok] = '\0';
            json = cJSON_GetObjectItemCaseSensitive(json, path + i);
            path[i + eok] = saved;
            i = i + eok - 1;
        } else if (path[i] == '[') {
            char* eon;
            int idx = strtoul(path + i + 1, &eon, 10);
            if (*eon != ']') {
                *err = 1;
                json = NULL;
                break;
            }
            i = eon - path;
            json = cJSON_GetArrayItem(json, idx);
        } else {
            *err = 1;
            json = NULL;
        }
    }
    return json;
}

static void json_length(sqlite3_context* ctx, int nargs, sqlite3_value** args) {
    if (sqlite3_value_type(args[0]) == SQLITE_NULL) {
        return;
    }
    if (nargs == 2 && sqlite3_value_type(args[0]) == SQLITE_NULL) {
        return;
    }

    cJSON *orig, *json;
    orig = json = cJSON_Parse((const char*)sqlite3_value_text(args[0]));
    if (!json) {
        sqlite3_result_error(ctx, "malformed JSON", -1);
        return;
    }

    if (nargs == 2) {
        char* path = sqlite3_mprintf("%s", (const char*)sqlite3_value_text(args[1]));
        if (!path) {
            sqlite3_result_error_nomem(ctx);
            cJSON_Delete(orig);
            return;
        }
        int error = 0;
        json = json_find_path(json, path, &error);
        sqlite3_free(path);
        if (error) {
            sqlite3_result_error(ctx, "malformed path", -1);
        }
        if (!json) {
            cJSON_Delete(orig);
            return;
        }
    }

    if (cJSON_IsArray(json) || cJSON_IsObject(json)) {
        sqlite3_result_int(ctx, cJSON_GetArraySize(json));
    } else {
        sqlite3_result_int(ctx, 1);
    }
    cJSON_Delete(orig);
}

#if SQLITE_VERSION_NUMBER >= 3024000
/* MySQL style json_pretty() formatting */
static _Bool json_pp_fancy(sqlite3_str* out, cJSON* json, int level) {
    if (cJSON_IsArray(json)) {
        cJSON* elem;
        sqlite3_str_appendall(out, "[\n");
        cJSON_ArrayForEach(elem, json) {
            sqlite3_str_appendchar(out, level * 2 + 2, ' ');
            if (!json_pp_fancy(out, elem, level + 1)) {
                return 0;
            }
            if (elem->next) {
                sqlite3_str_appendchar(out, 1, ',');
            }
            sqlite3_str_appendchar(out, 1, '\n');
        }
        sqlite3_str_appendchar(out, level * 2, ' ');
        sqlite3_str_appendchar(out, 1, ']');
    } else if (cJSON_IsObject(json)) {
        cJSON* elem;
        sqlite3_str_appendall(out, "{\n");
        cJSON_ArrayForEach(elem, json) {
            cJSON* key = cJSON_CreateString(elem->string);
            if (!key) {
                return 0;
            }
            char* keytext = cJSON_Print(key);
            cJSON_Delete(key);
            if (!keytext) {
                return 0;
            }
            sqlite3_str_appendchar(out, level * 2 + 2, ' ');
            sqlite3_str_appendf(out, "%s: ", keytext);
            free(keytext);
            if (!json_pp_fancy(out, elem, level + 1)) {
                return 0;
            }
            if (elem->next) {
                sqlite3_str_appendchar(out, 1, ',');
            }
            sqlite3_str_appendchar(out, 1, '\n');
        }
        sqlite3_str_appendchar(out, level * 2, ' ');
        sqlite3_str_appendchar(out, 1, '}');
    } else {
        char* text = cJSON_Print(json);
        if (text) {
            sqlite3_str_appendall(out, text);
            free(text);
        } else {
            return 0;
        }
    }
    return 1;
}
#endif

static void json_pp(sqlite3_context* ctx, int nargs __attribute__((unused)), sqlite3_value** args) {
    cJSON* json;

    if (sqlite3_value_type(args[0]) == SQLITE_NULL) {
        return;
    }

    json = cJSON_Parse((const char*)sqlite3_value_text(args[0]));
    if (!json) {
        sqlite3_result_error(ctx, "malformed JSON", -1);
        return;
    }

#if SQLITE_VERSION_NUMBER >= 3024000

    sqlite3_str* out = sqlite3_str_new(sqlite3_context_db_handle(ctx));
    if (!out) {
        sqlite3_result_error_nomem(ctx);
        cJSON_Delete(json);
        return;
    }
    if (json_pp_fancy(out, json, 0)) {
        char* text = sqlite3_str_finish(out);
        if (text) {
            sqlite3_result_text(ctx, text, -1, sqlite3_free);
            sqlite3_result_subtype(ctx, 'J');
        }
    } else {
        char* text = sqlite3_str_finish(out);
        sqlite3_free(text);
    }

#else

    char* text = cJSON_Print(json);
    if (text) {
        sqlite3_result_text(ctx, text, -1, free);
        sqlite3_result_subtype(ctx, 'J');
    }

#endif

    cJSON_Delete(json);
}

static void json_equal(sqlite3_context* ctx,
                       int nargs __attribute__((unused)),
                       sqlite3_value** args) {
    if (sqlite3_value_type(args[0]) == SQLITE_NULL || sqlite3_value_type(args[1]) == SQLITE_NULL) {
        return;
    }

    cJSON* j1 = cJSON_Parse((const char*)sqlite3_value_text(args[0]));
    if (!j1) {
        sqlite3_result_error(ctx, "malformed JSON", -1);
        return;
    }
    cJSON* j2 = cJSON_Parse((const char*)sqlite3_value_text(args[1]));
    if (!j2) {
        sqlite3_result_error(ctx, "malformed JSON", -1);
        cJSON_Delete(j1);
        return;
    }

    sqlite3_result_int(ctx, cJSON_Compare(j1, j2, 1));
    cJSON_Delete(j1);
    cJSON_Delete(j2);
}

static void json_keys(sqlite3_context* ctx, int nargs, sqlite3_value** args) {
    if (sqlite3_value_type(args[0]) == SQLITE_NULL) {
        return;
    }
    if (nargs == 2 && sqlite3_value_type(args[1]) == SQLITE_NULL) {
        return;
    }

    cJSON *orig, *json;
    orig = json = cJSON_Parse((const char*)sqlite3_value_text(args[0]));
    if (!json) {
        sqlite3_result_error(ctx, "malformed JSON", -1);
        return;
    }

    if (nargs == 2) {
        char* path = sqlite3_mprintf("%s", (const char*)sqlite3_value_text(args[1]));
        if (!path) {
            sqlite3_result_error_nomem(ctx);
            cJSON_Delete(orig);
            return;
        }
        int error = 0;
        json = json_find_path(json, path, &error);
        sqlite3_free(path);
        if (error) {
            sqlite3_result_error(ctx, "malformed path", -1);
        }
        if (!json) {
            cJSON_Delete(orig);
            return;
        }
    }

    if (!cJSON_IsObject(json)) {
        cJSON_Delete(orig);
        return;
    }

    cJSON* res = cJSON_CreateArray();
    if (!res) {
        cJSON_Delete(orig);
        sqlite3_result_error_nomem(ctx);
        return;
    }

    cJSON* elem;
    cJSON_ArrayForEach(elem, json) {
        cJSON* key = cJSON_CreateString(elem->string);
        if (!key) {
            cJSON_Delete(res);
            cJSON_Delete(orig);
            sqlite3_result_error_nomem(ctx);
            return;
        }

        cJSON_AddItemToArray(res, key);
    }

    sqlite3_result_text(ctx, cJSON_PrintUnformatted(res), -1, cJSON_free);
    sqlite3_result_subtype(ctx, 'J');
    cJSON_Delete(res);
    cJSON_Delete(orig);
}

static void json_contains_path(sqlite3_context* ctx, int nargs, sqlite3_value** args) {
    if (sqlite3_value_type(args[0]) == SQLITE_NULL || sqlite3_value_type(args[1]) == SQLITE_NULL) {
        return;
    }

    cJSON *orig, *json;
    orig = json = cJSON_Parse((const char*)sqlite3_value_text(args[0]));
    if (!json) {
        sqlite3_result_error(ctx, "malformed JSON", -1);
        return;
    }

    char* path = sqlite3_mprintf("%s", (const char*)sqlite3_value_text(args[1]));
    if (!path) {
        sqlite3_result_error_nomem(ctx);
        cJSON_Delete(orig);
        return;
    }
    int error = 0;
    json = json_find_path(json, path, &error);
    if (error) {
        sqlite3_result_error(ctx, "malformed path", -1);
        cJSON_Delete(orig);
        return;
    }
    sqlite3_free(path);
    sqlite3_result_int(ctx, !!json);
    cJSON_Delete(orig);
}

static void* my_cjson_malloc(size_t sz) {
    return sqlite3_malloc64(sz);
}

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_json_init(sqlite3* db, char** pzErrMsg, const sqlite3_api_routines* pApi) {
    SQLITE_EXTENSION_INIT2(pApi);
    cJSON_Hooks memhooks = {my_cjson_malloc, sqlite3_free};
    cJSON_InitHooks(&memhooks);
    int flags = SQLITE_UTF8 | SQLITE_DETERMINISTIC;
    sqlite3_create_function(db, "json_pretty", 1, flags, 0, json_pp, 0, 0);
    sqlite3_create_function(db, "json_length", 1, flags, 0, json_length, 0, 0);
    sqlite3_create_function(db, "json_length", 2, flags, 0, json_length, 0, 0);
    sqlite3_create_function(db, "json_equal", 2, flags, 0, json_equal, 0, 0);
    sqlite3_create_function(db, "json_keys", 1, flags, 0, json_keys, 0, 0);
    sqlite3_create_function(db, "json_keys", 2, flags, 0, json_keys, 0, 0);
    sqlite3_create_function(db, "json_contains_path", 2, flags, 0, json_contains_path, 0, 0);
    return SQLITE_OK;
}
