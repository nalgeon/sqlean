#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0502
#endif

#include <wincrypt.h>
#include <windows.h>

#include "sqlite3ext.h"

SQLITE_EXTENSION_INIT1

// Hash sub ids

#ifndef ALG_SID_MD2
#define ALG_SID_MD2 1
#endif
#ifndef ALG_SID_MD4
#define ALG_SID_MD4 2
#endif
#ifndef ALG_SID_MD5
#define ALG_SID_MD5 3
#endif
#ifndef ALG_SID_SHA1
#define ALG_SID_SHA1 4
#endif
#ifndef ALG_SID_SHA_256
#define ALG_SID_SHA_256 12
#endif
#ifndef ALG_SID_SHA_384
#define ALG_SID_SHA_384 13
#endif
#ifndef ALG_SID_SHA_512
#define ALG_SID_SHA_512 14
#endif

// algorithm identifier definitions

#ifndef CALG_MD2
#define CALG_MD2 (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_MD2)
#endif
#ifndef CALG_MD4
#define CALG_MD4 (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_MD4)
#endif
#ifndef CALG_MD5
#define CALG_MD5 (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_MD5)
#endif
#ifndef CALG_SHA1
#define CALG_SHA1 (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_SHA1)
#endif
#ifndef CALG_SHA_256
#define CALG_SHA_256 (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_SHA_256)
#endif
#ifndef CALG_SHA_384
#define CALG_SHA_384 (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_SHA_384)
#endif
#ifndef CALG_SHA_512
#define CALG_SHA_512 (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_SHA_512)
#endif

// define the RSA_AES provider

#ifndef PROV_RSA_AES
#define PROV_RSA_AES 24
#endif

//define the Enhanced RSA and AES Provider Strings

#ifndef MS_ENH_RSA_AES_PROV_A
#define MS_ENH_RSA_AES_PROV_A "Microsoft Enhanced RSA and AES Cryptographic Provider"
#endif
#ifndef MS_ENH_RSA_AES_PROV_W
#define MS_ENH_RSA_AES_PROV_W L"Microsoft Enhanced RSA and AES Cryptographic Provider"
#endif
#ifdef UNICODE
#ifndef MS_ENH_RSA_AES_PROV_XP
#define MS_ENH_RSA_AES_PROV_XP MS_ENH_RSA_AES_PROV_XP_W
#endif
#ifndef MS_ENH_RSA_AES_PROV
#define MS_ENH_RSA_AES_PROV MS_ENH_RSA_AES_PROV_W
#endif
#else
#ifndef MS_ENH_RSA_AES_PROV_XP
#define MS_ENH_RSA_AES_PROV_XP MS_ENH_RSA_AES_PROV_XP_A
#endif
#ifndef MS_ENH_RSA_AES_PROV
#define MS_ENH_RSA_AES_PROV MS_ENH_RSA_AES_PROV_A
#endif
#endif

typedef struct aggregateHash aggregateHash;
struct aggregateHash {
    HCRYPTPROV cryptProv;
    HCRYPTHASH cryptHash;
    int iserror;
    int format;
};

static void sqlite3_hash(sqlite3_context *context, int argc, sqlite3_value **argv) {
    HCRYPTPROV cryptProv;
    HCRYPTHASH cryptHash;
    char hex[16] = "0123456789ABCDEF";
    int i = 0;
    int datalen;
    void *data;
    BYTE hash[128];
    DWORD cbHash = 128;
    char strHash[1024] = {0};
    int format = 1;
    intptr_t algo = (intptr_t)sqlite3_user_data(context);

    if (algo < 0) {
        algo = -algo;
        format = -1;
    }

    if (!CryptAcquireContext(&cryptProv, NULL, MS_ENH_RSA_AES_PROV, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        return;
    }
    if (!CryptCreateHash(cryptProv, algo, 0, 0, &cryptHash)) {
        CryptReleaseContext(cryptProv, 0);
        return;
    }
    for (i = 0; i < argc; i++) {
        data = (void *)sqlite3_value_text(argv[i]);
        datalen = sqlite3_value_bytes(argv[i]);
        if (datalen > 0) {
            if (!CryptHashData(cryptHash, data, datalen, 0)) {
                CryptDestroyHash(cryptHash);
                CryptReleaseContext(cryptProv, 0);
                return;
            }
        }
    }
    if (!CryptGetHashParam(cryptHash, HP_HASHVAL, hash, &cbHash, 0)) {
        CryptDestroyHash(cryptHash);
        CryptReleaseContext(cryptProv, 0);
        return;
    }
    CryptDestroyHash(cryptHash);
    CryptReleaseContext(cryptProv, 0);
    if (format < 0) {
        sqlite3_result_blob(context, hash, cbHash, SQLITE_TRANSIENT);
        return;
    }
    for (i = 0; i < (long)cbHash; i++) {
        strHash[i * 2] = hex[hash[i] >> 4];
        strHash[(i * 2) + 1] = hex[hash[i] & 0xF];
    }
    sqlite3_result_text(context, strHash, cbHash * 2, SQLITE_TRANSIENT);
    return;
}

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_crypto_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi) {
    SQLITE_EXTENSION_INIT2(pApi);

    int err_count = 0;
    err_count +=
        sqlite3_create_function(db, "md5", -1, SQLITE_UTF8 | SQLITE_DETERMINISTIC, (void *)CALG_MD5, sqlite3_hash, 0, 0);
    err_count +=
        sqlite3_create_function(db, "sha1", -1, SQLITE_UTF8 | SQLITE_DETERMINISTIC, (void *)CALG_SHA1, sqlite3_hash, 0, 0);
    err_count +=
        sqlite3_create_function(db, "sha256", -1, SQLITE_UTF8 | SQLITE_DETERMINISTIC, (void *)CALG_SHA_256, sqlite3_hash, 0, 0);
    err_count +=
        sqlite3_create_function(db, "sha384", -1, SQLITE_UTF8 | SQLITE_DETERMINISTIC, (void *)CALG_SHA_384, sqlite3_hash, 0, 0);
    err_count +=
        sqlite3_create_function(db, "sha512", -1, SQLITE_UTF8 | SQLITE_DETERMINISTIC, (void *)CALG_SHA_512, sqlite3_hash, 0, 0);
    return err_count ? SQLITE_ERROR : SQLITE_OK;
}
