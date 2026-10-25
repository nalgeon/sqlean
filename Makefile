# Copyright (c) 2021 Anton Zhiyanov, MIT License
# https://github.com/nalgeon/sqlean

.PHONY: test gcc

SQLITE_RELEASE_YEAR := 2024
SQLITE_VERSION := 3470200
SQLITE_BRANCH := 3.47

SQLEAN_VERSION := '"$(or $(shell git tag --points-at HEAD),main)"'

CC ?= gcc
CFLAGS ?=
COMMON_CFLAGS := -Isrc -DSQLEAN_VERSION=$(SQLEAN_VERSION)

LINIX_FLAGS := $(CFLAGS) -z now -z relro -Wall -Wsign-compare -Wno-unknown-pragmas -fPIC -shared $(COMMON_CFLAGS)
WINDO_FLAGS := $(CFLAGS) -shared $(COMMON_CFLAGS)
MACOS_FLAGS := $(CFLAGS) -Wall -Wsign-compare -fPIC -dynamiclib $(COMMON_CFLAGS)
CTEST_FLAGS := $(CFLAGS) -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unknown-pragmas -Isrc

# the sqlite3 shell used by the SQL tests
SQLITE ?= sqlite3

# address + undefined behavior sanitizers
SANITIZE_FLAGS := -g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=undefined
# Leak detection is Linux-only. Slow unwinding gives full stacks for
# allocations made inside SQLite, which is built without frame pointers.
ifeq ($(shell uname -s),Linux)
SANITIZE_ENV := ASAN_OPTIONS=detect_leaks=1:fast_unwind_on_malloc=0 UBSAN_OPTIONS=print_stacktrace=1
else
SANITIZE_ENV := ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=1
endif

# Runs make targets on Linux with GCC, in Docker: make gcc run="ctest-all-sanitize".
# The container gets its own empty dist/, so the host's build artifacts stay as they are.
GCC_IMAGE ?= gcc:16.2.0
GCC_PLATFORM ?= linux/arm64
SAN_FLAGS := $(CTEST_FLAGS) $(SANITIZE_FLAGS) -fPIC -shared

prepare-dist:
	mkdir -p dist
	rm -rf dist/*

download-sqlite:
	curl -L http://sqlite.org/$(SQLITE_RELEASE_YEAR)/sqlite-amalgamation-$(SQLITE_VERSION).zip --output src.zip
	unzip src.zip
	mv sqlite-amalgamation-$(SQLITE_VERSION)/* src
	rm -rf src.zip sqlite-amalgamation-$(SQLITE_VERSION)
# unzip keeps the archive timestamps, which can be older than the targets built from the previous version
	touch src/sqlite3.c src/sqlite3.h src/sqlite3ext.h src/shell.c

download-external:
	curl -L --silent https://github.com/mackyle/sqlite/raw/branch-$(SQLITE_BRANCH)/src/test_windirent.h --output src/test_windirent.h

# A sqlite3 shell to load the sanitized extensions into.
# Rebuilt only when the amalgamation changes (i.e. after download-sqlite).
dist/sqlite3-sanitize: src/sqlite3.c src/shell.c
	@mkdir -p dist
	$(CC) -g -O1 -w -c src/sqlite3.c -o dist/sqlite3.o
	$(CC) -g -O1 -w -c src/shell.c -o dist/shell.o
	$(CC) -fsanitize=address,undefined dist/sqlite3.o dist/shell.o -o $@
	rm -f dist/sqlite3.o dist/shell.o

# Builds the extensions with the sanitizers enabled, plus the shell above.
compile-sanitize: dist/sqlite3-sanitize
	$(CC) $(SAN_FLAGS) src/sqlite3-crypto.c src/crypto/*.c -o dist/crypto.so
	$(CC) $(SAN_FLAGS) src/sqlite3-define.c src/define/*.c -o dist/define.so
	$(CC) $(SAN_FLAGS) src/sqlite3-fileio.c src/fileio/*.c -o dist/fileio.so
	$(CC) $(SAN_FLAGS) src/sqlite3-fuzzy.c src/fuzzy/*.c -o dist/fuzzy.so
	$(CC) $(SAN_FLAGS) src/sqlite3-ipaddr.c src/ipaddr/*.c -o dist/ipaddr.so
	$(CC) $(SAN_FLAGS) src/sqlite3-math.c src/math/*.c -o dist/math.so -lm
	$(CC) $(SAN_FLAGS) -include src/regexp/constants.h src/sqlite3-regexp.c src/regexp/*.c src/regexp/pcre2/*.c -o dist/regexp.so
	$(CC) $(SAN_FLAGS) src/sqlite3-stats.c src/stats/*.c -o dist/stats.so -lm
	$(CC) $(SAN_FLAGS) src/sqlite3-text.c src/text/*.c src/text/*/*.c -o dist/text.so
	$(CC) $(SAN_FLAGS) src/sqlite3-time.c src/time/*.c -o dist/time.so
	$(CC) $(SAN_FLAGS) src/sqlite3-unicode.c src/unicode/*.c -o dist/unicode.so
	$(CC) $(SAN_FLAGS) src/sqlite3-uuid.c src/uuid/*.c -o dist/uuid.so
	$(CC) $(SAN_FLAGS) src/sqlite3-vsv.c src/vsv/*.c -o dist/vsv.so -lm
	$(CC) $(SAN_FLAGS) -include src/regexp/constants.h src/sqlite3-sqlean.c src/crypto/*.c src/define/*.c src/fileio/*.c src/fuzzy/*.c src/ipaddr/*.c src/math/*.c src/regexp/*.c src/regexp/pcre2/*.c src/stats/*.c src/text/*.c src/text/*/*.c src/time/*.c src/unicode/*.c src/uuid/*.c src/vsv/*.c -o dist/sqlean.so -lm

compile-linux:
	$(CC) -O3 $(LINIX_FLAGS) src/sqlite3-crypto.c src/crypto/*.c -o dist/crypto.so
	$(CC) -O3 $(LINIX_FLAGS) src/sqlite3-define.c src/define/*.c -o dist/define.so
	$(CC) -O3 $(LINIX_FLAGS) src/sqlite3-fileio.c src/fileio/*.c -o dist/fileio.so
	$(CC) -O3 $(LINIX_FLAGS) src/sqlite3-fuzzy.c src/fuzzy/*.c -o dist/fuzzy.so
	$(CC) -O3 $(LINIX_FLAGS) src/sqlite3-ipaddr.c src/ipaddr/*.c -o dist/ipaddr.so
	$(CC) -O3 $(LINIX_FLAGS) src/sqlite3-math.c src/math/*.c -o dist/math.so -lm
	$(CC) -O3 $(LINIX_FLAGS) -include src/regexp/constants.h src/sqlite3-regexp.c src/regexp/*.c src/regexp/pcre2/*.c -o dist/regexp.so
	$(CC) -O3 $(LINIX_FLAGS) src/sqlite3-stats.c src/stats/*.c -o dist/stats.so -lm
	$(CC) -O3 $(LINIX_FLAGS) src/sqlite3-text.c src/text/*.c src/text/*/*.c -o dist/text.so
	$(CC) -O3 $(LINIX_FLAGS) src/sqlite3-time.c src/time/*.c -o dist/time.so
	$(CC) -O3 $(LINIX_FLAGS) src/sqlite3-unicode.c src/unicode/*.c -o dist/unicode.so
	$(CC) -O3 $(LINIX_FLAGS) src/sqlite3-uuid.c src/uuid/*.c -o dist/uuid.so
	$(CC) -O3 $(LINIX_FLAGS) src/sqlite3-vsv.c src/vsv/*.c -o dist/vsv.so -lm
	$(CC) -O3 $(LINIX_FLAGS) -include src/regexp/constants.h src/sqlite3-sqlean.c src/crypto/*.c src/define/*.c src/fileio/*.c src/fuzzy/*.c src/ipaddr/*.c src/math/*.c src/regexp/*.c src/regexp/pcre2/*.c src/stats/*.c src/text/*.c src/text/*/*.c src/time/*.c src/unicode/*.c src/uuid/*.c src/vsv/*.c -o dist/sqlean.so -lm

compile-linux-x64:
	mkdir -p dist/x64
	$(CC) -O3 $(LINIX_FLAGS) src/sqlite3-crypto.c src/crypto/*.c -o dist/x64/crypto.so
	$(CC) -O3 $(LINIX_FLAGS) src/sqlite3-define.c src/define/*.c -o dist/x64/define.so
	$(CC) -O3 $(LINIX_FLAGS) src/sqlite3-fileio.c src/fileio/*.c -o dist/x64/fileio.so
	$(CC) -O3 $(LINIX_FLAGS) src/sqlite3-fuzzy.c src/fuzzy/*.c -o dist/x64/fuzzy.so
	$(CC) -O3 $(LINIX_FLAGS) src/sqlite3-ipaddr.c src/ipaddr/*.c -o dist/x64/ipaddr.so
	$(CC) -O3 $(LINIX_FLAGS) src/sqlite3-math.c src/math/*.c -o dist/x64/math.so -lm
	$(CC) -O3 $(LINIX_FLAGS) -include src/regexp/constants.h src/sqlite3-regexp.c src/regexp/*.c src/regexp/pcre2/*.c -o dist/x64/regexp.so
	$(CC) -O3 $(LINIX_FLAGS) src/sqlite3-stats.c src/stats/*.c -o dist/x64/stats.so -lm
	$(CC) -O3 $(LINIX_FLAGS) src/sqlite3-text.c src/text/*.c src/text/*/*.c -o dist/x64/text.so
	$(CC) -O3 $(LINIX_FLAGS) src/sqlite3-time.c src/time/*.c -o dist/x64/time.so
	$(CC) -O3 $(LINIX_FLAGS) src/sqlite3-unicode.c src/unicode/*.c -o dist/x64/unicode.so
	$(CC) -O3 $(LINIX_FLAGS) src/sqlite3-uuid.c src/uuid/*.c -o dist/x64/uuid.so
	$(CC) -O3 $(LINIX_FLAGS) src/sqlite3-vsv.c src/vsv/*.c -o dist/x64/vsv.so -lm
	$(CC) -O3 $(LINIX_FLAGS) -include src/regexp/constants.h src/sqlite3-sqlean.c src/crypto/*.c src/define/*.c src/fileio/*.c src/fuzzy/*.c src/ipaddr/*.c src/math/*.c src/regexp/*.c src/regexp/pcre2/*.c src/stats/*.c src/text/*.c src/text/*/*.c src/time/*.c src/unicode/*.c src/uuid/*.c src/vsv/*.c -o dist/x64/sqlean.so -lm

compile-linux-musl:
	mkdir -p dist/musl
	musl-gcc -O3 $(LINIX_FLAGS) src/sqlite3-crypto.c src/crypto/*.c -o dist/musl/crypto.so
	musl-gcc -O3 $(LINIX_FLAGS) src/sqlite3-define.c src/define/*.c -o dist/musl/define.so
	musl-gcc -O3 $(LINIX_FLAGS) src/sqlite3-fileio.c src/fileio/*.c -o dist/musl/fileio.so
	musl-gcc -O3 $(LINIX_FLAGS) src/sqlite3-fuzzy.c src/fuzzy/*.c -o dist/musl/fuzzy.so
	musl-gcc -O3 $(LINIX_FLAGS) src/sqlite3-ipaddr.c src/ipaddr/*.c -o dist/musl/ipaddr.so
	musl-gcc -O3 $(LINIX_FLAGS) src/sqlite3-math.c src/math/*.c -o dist/musl/math.so -lm
	musl-gcc -O3 $(LINIX_FLAGS) -include src/regexp/constants.h src/sqlite3-regexp.c src/regexp/*.c src/regexp/pcre2/*.c -o dist/musl/regexp.so
	musl-gcc -O3 $(LINIX_FLAGS) src/sqlite3-stats.c src/stats/*.c -o dist/musl/stats.so -lm
	musl-gcc -O3 $(LINIX_FLAGS) src/sqlite3-text.c src/text/*.c src/text/*/*.c -o dist/musl/text.so
	musl-gcc -O3 $(LINIX_FLAGS) src/sqlite3-time.c src/time/*.c -o dist/musl/time.so
	musl-gcc -O3 $(LINIX_FLAGS) src/sqlite3-unicode.c src/unicode/*.c -o dist/musl/unicode.so
	musl-gcc -O3 $(LINIX_FLAGS) src/sqlite3-uuid.c src/uuid/*.c -o dist/musl/uuid.so
	musl-gcc -O3 $(LINIX_FLAGS) src/sqlite3-vsv.c src/vsv/*.c -o dist/musl/vsv.so -lm
	musl-gcc -O3 $(LINIX_FLAGS) -include src/regexp/constants.h src/sqlite3-sqlean.c src/crypto/*.c src/define/*.c src/fileio/*.c src/fuzzy/*.c src/ipaddr/*.c src/math/*.c src/regexp/*.c src/regexp/pcre2/*.c src/stats/*.c src/text/*.c src/text/*/*.c src/time/*.c src/unicode/*.c src/uuid/*.c src/vsv/*.c -o dist/musl/sqlean.so -lm

compile-linux-arm64:
	mkdir -p dist/arm64
	aarch64-linux-gnu-gcc -O3 $(LINIX_FLAGS) src/sqlite3-crypto.c src/crypto/*.c -o dist/arm64/crypto.so
	aarch64-linux-gnu-gcc -O3 $(LINIX_FLAGS) src/sqlite3-define.c src/define/*.c -o dist/arm64/define.so
	aarch64-linux-gnu-gcc -O3 $(LINIX_FLAGS) src/sqlite3-fileio.c src/fileio/*.c -o dist/arm64/fileio.so
	aarch64-linux-gnu-gcc -O3 $(LINIX_FLAGS) src/sqlite3-fuzzy.c src/fuzzy/*.c -o dist/arm64/fuzzy.so
	aarch64-linux-gnu-gcc -O3 $(LINIX_FLAGS) src/sqlite3-ipaddr.c src/ipaddr/*.c -o dist/arm64/ipaddr.so
	aarch64-linux-gnu-gcc -O3 $(LINIX_FLAGS) src/sqlite3-math.c src/math/*.c -o dist/arm64/math.so -lm
	aarch64-linux-gnu-gcc -O3 $(LINIX_FLAGS) -include src/regexp/constants.h src/sqlite3-regexp.c src/regexp/*.c src/regexp/pcre2/*.c -o dist/arm64/regexp.so
	aarch64-linux-gnu-gcc -O3 $(LINIX_FLAGS) src/sqlite3-stats.c src/stats/*.c -o dist/arm64/stats.so -lm
	aarch64-linux-gnu-gcc -O3 $(LINIX_FLAGS) src/sqlite3-text.c src/text/*.c src/text/*/*.c -o dist/arm64/text.so
	aarch64-linux-gnu-gcc -O3 $(LINIX_FLAGS) src/sqlite3-time.c src/time/*.c -o dist/arm64/time.so
	aarch64-linux-gnu-gcc -O3 $(LINIX_FLAGS) src/sqlite3-unicode.c src/unicode/*.c -o dist/arm64/unicode.so
	aarch64-linux-gnu-gcc -O3 $(LINIX_FLAGS) src/sqlite3-uuid.c src/uuid/*.c -o dist/arm64/uuid.so
	aarch64-linux-gnu-gcc -O3 $(LINIX_FLAGS) src/sqlite3-vsv.c src/vsv/*.c -o dist/arm64/vsv.so -lm
	aarch64-linux-gnu-gcc -O3 $(LINIX_FLAGS) -include src/regexp/constants.h src/sqlite3-sqlean.c src/crypto/*.c src/define/*.c src/fileio/*.c src/fuzzy/*.c src/ipaddr/*.c src/math/*.c src/regexp/*.c src/regexp/pcre2/*.c src/stats/*.c src/text/*.c src/text/*/*.c src/time/*.c src/unicode/*.c src/uuid/*.c src/vsv/*.c -o dist/arm64/sqlean.so -lm

pack-linux:
	zip -j dist/sqlean-linux-x64.zip dist/x64/*.so
	zip -j dist/sqlean-linux-arm64.zip dist/arm64/*.so

compile-windows:
	gcc -O3 $(WINDO_FLAGS) src/sqlite3-crypto.c src/crypto/*.c -o dist/crypto.dll
	gcc -O3 $(WINDO_FLAGS) src/sqlite3-define.c src/define/*.c -o dist/define.dll
	gcc -O3 $(WINDO_FLAGS) src/sqlite3-fileio.c src/fileio/*.c -o dist/fileio.dll
	gcc -O3 $(WINDO_FLAGS) src/sqlite3-fuzzy.c src/fuzzy/*.c -o dist/fuzzy.dll
	gcc -O3 $(WINDO_FLAGS) src/sqlite3-ipaddr.c src/ipaddr/*.c -o dist/ipaddr.dll -lws2_32
	gcc -O3 $(WINDO_FLAGS) src/sqlite3-math.c src/math/*.c -o dist/math.dll -lm
	gcc -O3 $(WINDO_FLAGS) src/sqlite3-regexp.c -include src/regexp/constants.h src/regexp/*.c src/regexp/pcre2/*.c -o dist/regexp.dll
	gcc -O3 $(WINDO_FLAGS) src/sqlite3-stats.c src/stats/*.c -o dist/stats.dll -lm
	gcc -O3 $(WINDO_FLAGS) src/sqlite3-text.c src/text/*.c src/text/*/*.c -o dist/text.dll
	gcc -O3 $(WINDO_FLAGS) src/sqlite3-time.c src/time/*.c -o dist/time.dll
	gcc -O3 $(WINDO_FLAGS) src/sqlite3-unicode.c src/unicode/*.c -o dist/unicode.dll
	gcc -O3 $(WINDO_FLAGS) src/sqlite3-uuid.c src/uuid/*.c -o dist/uuid.dll
	gcc -O3 $(WINDO_FLAGS) src/sqlite3-vsv.c src/vsv/*.c -o dist/vsv.dll -lm
	gcc -O3 $(WINDO_FLAGS) -include src/regexp/constants.h src/sqlite3-sqlean.c src/crypto/*.c src/define/*.c src/fileio/*.c src/fuzzy/*.c src/math/*.c src/regexp/*.c src/regexp/pcre2/*.c src/stats/*.c src/text/*.c src/text/*/*.c src/time/*.c src/unicode/*.c src/uuid/*.c src/vsv/*.c -o dist/sqlean.dll -lm

pack-windows:
	7z a -tzip dist/sqlean-win-x64.zip ./dist/*.dll

compile-macos:
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-crypto.c src/crypto/*.c -o dist/crypto.dylib
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-define.c src/define/*.c -o dist/define.dylib
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-fileio.c src/fileio/*.c -o dist/fileio.dylib
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-fuzzy.c src/fuzzy/*.c -o dist/fuzzy.dylib
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-ipaddr.c src/ipaddr/*.c -o dist/ipaddr.dylib
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-math.c src/math/*.c -o dist/math.dylib -lm
	$(CC) -O3 $(MACOS_FLAGS) -include src/regexp/constants.h src/sqlite3-regexp.c src/regexp/*.c src/regexp/pcre2/*.c -o dist/regexp.dylib
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-stats.c src/stats/*.c -o dist/stats.dylib -lm
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-text.c src/text/*.c src/text/*/*.c -o dist/text.dylib
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-time.c src/time/*.c -o dist/time.dylib
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-unicode.c src/unicode/*.c -o dist/unicode.dylib
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-uuid.c src/uuid/*.c -o dist/uuid.dylib
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-vsv.c src/vsv/*.c -o dist/vsv.dylib -lm
	$(CC) -O3 $(MACOS_FLAGS) -include src/regexp/constants.h src/sqlite3-sqlean.c src/crypto/*.c src/define/*.c src/fileio/*.c src/fuzzy/*.c src/ipaddr/*.c src/math/*.c src/regexp/*.c src/regexp/pcre2/*.c src/stats/*.c src/text/*.c src/text/*/*.c src/time/*.c src/unicode/*.c src/uuid/*.c src/vsv/*.c -o dist/sqlean.dylib -lm

compile-macos-x64:
	mkdir -p dist/x64
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-crypto.c src/crypto/*.c -o dist/x64/crypto.dylib -target x86_64-apple-macos10.12
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-define.c src/define/*.c -o dist/x64/define.dylib -target x86_64-apple-macos10.12
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-fileio.c src/fileio/*.c -o dist/x64/fileio.dylib -target x86_64-apple-macos10.12
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-fuzzy.c src/fuzzy/*.c -o dist/x64/fuzzy.dylib -target x86_64-apple-macos10.12
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-ipaddr.c src/ipaddr/*.c -o dist/x64/ipaddr.dylib -target x86_64-apple-macos10.12
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-math.c src/math/*.c -o dist/x64/math.dylib -target x86_64-apple-macos10.12 -lm
	$(CC) -O3 $(MACOS_FLAGS) -include src/regexp/constants.h src/sqlite3-regexp.c src/regexp/*.c src/regexp/pcre2/*.c -o dist/x64/regexp.dylib -target x86_64-apple-macos10.12
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-stats.c src/stats/*.c -o dist/x64/stats.dylib -target x86_64-apple-macos10.12 -lm
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-text.c src/text/*.c src/text/*/*.c -o dist/x64/text.dylib -target x86_64-apple-macos10.12
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-time.c src/time/*.c -o dist/x64/time.dylib -target x86_64-apple-macos10.12
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-unicode.c src/unicode/*.c -o dist/x64/unicode.dylib -target x86_64-apple-macos10.12
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-uuid.c src/uuid/*.c -o dist/x64/uuid.dylib -target x86_64-apple-macos10.12
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-vsv.c src/vsv/*.c -o dist/x64/vsv.dylib -target x86_64-apple-macos10.12 -lm
	$(CC) -O3 $(MACOS_FLAGS) -include src/regexp/constants.h src/sqlite3-sqlean.c src/crypto/*.c src/define/*.c src/fileio/*.c src/fuzzy/*.c src/ipaddr/*.c src/math/*.c src/regexp/*.c src/regexp/pcre2/*.c src/stats/*.c src/text/*.c src/text/*/*.c src/time/*.c src/unicode/*.c src/uuid/*.c src/vsv/*.c -o dist/x64/sqlean.dylib -target x86_64-apple-macos10.12 -lm

compile-macos-arm64:
	mkdir -p dist/arm64
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-crypto.c src/crypto/*.c -o dist/arm64/crypto.dylib -target arm64-apple-macos11
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-define.c src/define/*.c -o dist/arm64/define.dylib -target arm64-apple-macos11
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-fileio.c src/fileio/*.c -o dist/arm64/fileio.dylib -target arm64-apple-macos11
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-fuzzy.c src/fuzzy/*.c -o dist/arm64/fuzzy.dylib -target arm64-apple-macos11
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-ipaddr.c src/ipaddr/*.c -o dist/arm64/ipaddr.dylib -target arm64-apple-macos11
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-math.c src/math/*.c -o dist/arm64/math.dylib -target arm64-apple-macos11 -lm
	$(CC) -O3 $(MACOS_FLAGS) -include src/regexp/constants.h src/sqlite3-regexp.c src/regexp/*.c src/regexp/pcre2/*.c -o dist/arm64/regexp.dylib -target arm64-apple-macos11
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-stats.c src/stats/*.c -o dist/arm64/stats.dylib -target arm64-apple-macos11 -lm
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-text.c src/text/*.c src/text/*/*.c -o dist/arm64/text.dylib -target arm64-apple-macos11
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-time.c src/time/*.c -o dist/arm64/time.dylib -target arm64-apple-macos11
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-unicode.c src/unicode/*.c -o dist/arm64/unicode.dylib -target arm64-apple-macos11
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-uuid.c src/uuid/*.c -o dist/arm64/uuid.dylib -target arm64-apple-macos11
	$(CC) -O3 $(MACOS_FLAGS) src/sqlite3-vsv.c src/vsv/*.c -o dist/arm64/vsv.dylib -target arm64-apple-macos11 -lm
	$(CC) -O3 $(MACOS_FLAGS) -include src/regexp/constants.h src/sqlite3-sqlean.c src/crypto/*.c src/define/*.c src/fileio/*.c src/fuzzy/*.c src/ipaddr/*.c src/math/*.c src/regexp/*.c src/regexp/pcre2/*.c src/stats/*.c src/text/*.c src/text/*/*.c src/time/*.c src/unicode/*.c src/uuid/*.c src/vsv/*.c -o dist/arm64/sqlean.dylib -target arm64-apple-macos11 -lm

pack-macos:
	zip -j dist/sqlean-macos-x64.zip dist/x64/*.dylib
	zip -j dist/sqlean-macos-arm64.zip dist/arm64/*.dylib

test-all:
	make test suite=crypto
	make test suite=define
	make test suite=fileio
	make test suite=fuzzy
	make test suite=ipaddr
	make test suite=math
	make test suite=regexp
	make test suite=stats
	make test suite=time
	make test suite=text
	make test suite=unicode
	make test suite=uuid
	make test suite=vsv
	make test suite=sqlean

# Runs the SQL tests against a sanitized build. A sanitizer report kills
# the shell, so the failing suite fails the target.
test-all-sanitize:
	$(SANITIZE_ENV) make test-all SQLITE=./dist/sqlite3-sanitize

gcc:
	docker run --rm --platform $(GCC_PLATFORM) -v "$(CURDIR)":/src -v /src/dist -w /src $(GCC_IMAGE) make CC=gcc $(run)

# Runs both test layers with the sanitizers, on Linux with GCC.
sanitize-gcc:
	$(MAKE) gcc run="ctest-all-sanitize compile-sanitize test-all-sanitize"

# fails if grep does find a failed test case
# https://stackoverflow.com/questions/15367674/bash-one-liner-to-exit-with-the-opposite-status-of-a-grep-command/21788642
test:
	@$(SQLITE) < test/$(suite).sql > test.log
	@cat test.log | (! grep -Ex "[0-9_]+.[^1]")

# C tests as <package>/<module>, each compiling test/<package>/<module>.test.c
# together with the sources listed in CTEST_SRC_<package>_<module>.
CTESTS := crypto/base32 crypto/base64 crypto/base85 crypto/hash crypto/hex crypto/url define/define text/bstring text/rstring text/utf8 time/time time/duration
CTEST_SRC_crypto_base32 := src/crypto/base32.c
CTEST_SRC_crypto_base64 := src/crypto/base64.c
CTEST_SRC_crypto_base85 := src/crypto/base85.c
CTEST_SRC_crypto_hash := src/crypto/md5.c src/crypto/sha1.c src/crypto/sha2.c src/crypto/blake3.c src/crypto/xxhash.c src/crypto/xxh3.c
CTEST_SRC_crypto_hex := src/crypto/hex.c
CTEST_SRC_crypto_url := src/crypto/url.c
CTEST_SRC_define_define := src/define/*.c dist/ctest-sqlite3.o
CTEST_FLAGS_define_define := -DSQLITE_CORE
CTEST_SRC_text_bstring := src/text/*.c src/text/*/*.c
CTEST_SRC_text_rstring := src/text/*.c src/text/*/*.c
CTEST_SRC_text_utf8 := src/text/utf8/*.c
CTEST_SRC_time_time := src/time/*.c
CTEST_SRC_time_duration := src/time/*.c

ctest-all:
	@for t in $(CTESTS); do $(MAKE) ctest package=$${t%/*} module=$${t#*/} || exit 1; done

# Runs the C tests with the sanitizers enabled.
ctest-all-sanitize:
	$(SANITIZE_ENV) $(MAKE) ctest-all CFLAGS="$(SANITIZE_FLAGS)"

# Compiles and runs a single C test, then deletes the binary:
# make ctest package=time module=duration
# Object files in CTEST_SRC_<package>_<module> are built as prerequisites.
ctest: $(filter %.o,$(CTEST_SRC_$(package)_$(module)))
	$(if $(CTEST_SRC_$(package)_$(module)),,$(error unknown C test: $(package)/$(module), see CTESTS))
	$(CC) $(CTEST_FLAGS) $(CTEST_FLAGS_$(package)_$(module)) test/$(package)/$(module).test.c $(CTEST_SRC_$(package)_$(module)) -o $(package).$(module) -lm
	@./$(package).$(module); status=$$?; rm -rf $(package).$(module) $(package).$(module).dSYM; exit $$status

# SQLite for the C tests that need a real connection (define). Built without
# warnings and sanitizers, since it's third-party code, and without threads
# and extension loading, so it links without -lpthread and -ldl.
dist/ctest-sqlite3.o: src/sqlite3.c
	@mkdir -p dist
	$(CC) -g -O1 -w -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION -c src/sqlite3.c -o $@

# Runs a single C test with the sanitizers enabled.
ctest-sanitize:
	$(SANITIZE_ENV) $(MAKE) ctest package=$(package) module=$(module) CFLAGS="$(SANITIZE_FLAGS)"
