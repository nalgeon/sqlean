# Copyright (c) 2021 Anton Zhiyanov, MIT License
# https://github.com/nalgeon/sqlean

.PHONY: test

SQLITE_RELEASE_YEAR := 2021
SQLITE_VERSION := 3360000
SQLITE_BRANCH := 3.36

SQLEAN_VERSION := '"$(or $(shell git tag --points-at HEAD),main)"'

LINIX_FLAGS := -z now -z relro -Wall -Wsign-compare -Wno-unknown-pragmas -fPIC -shared -Isrc -DSQLEAN_VERSION=$(SQLEAN_VERSION)
WINDO_FLAGS := -shared -Isrc -DSQLEAN_VERSION=$(SQLEAN_VERSION)
WIN32_FLAGS := $(WINDO_FLAGS) -DSQLEAN_OMIT_UUID7
MACOS_FLAGS := -Wall -Wsign-compare -fPIC -dynamiclib -Isrc -DSQLEAN_VERSION=$(SQLEAN_VERSION)
CTEST_FLAGS := -Wall -Wsign-compare -Wno-unknown-pragmas -Isrc

prepare-dist:
	mkdir -p dist
	rm -rf dist/*

download-sqlite:
	curl -L http://sqlite.org/$(SQLITE_RELEASE_YEAR)/sqlite-amalgamation-$(SQLITE_VERSION).zip --output src.zip
	unzip src.zip
	mv sqlite-amalgamation-$(SQLITE_VERSION)/* src

download-external:
	curl -L https://github.com/mackyle/sqlite/raw/branch-$(SQLITE_BRANCH)/src/test_windirent.h --output src/test_windirent.h

compile-linux:
	gcc -O1 $(LINIX_FLAGS) src/sqlite3-crypto.c src/crypto/*.c -o dist/crypto.so
	gcc -O3 $(LINIX_FLAGS) src/sqlite3-define.c src/define/*.c -o dist/define.so
	gcc -O3 $(LINIX_FLAGS) src/sqlite3-fileio.c src/fileio/*.c -o dist/fileio.so
	gcc -O1 $(LINIX_FLAGS) src/sqlite3-fuzzy.c src/fuzzy/*.c -o dist/fuzzy.so
	gcc -O3 $(LINIX_FLAGS) src/sqlite3-ipaddr.c src/ipaddr/*.c -o dist/ipaddr.so
	gcc -O3 $(LINIX_FLAGS) src/sqlite3-math.c src/math/*.c -o dist/math.so -lm
	gcc -O3 $(LINIX_FLAGS) -include src/regexp/constants.h src/sqlite3-regexp.c src/regexp/*.c src/regexp/pcre2/*.c -o dist/regexp.so
	gcc -O3 $(LINIX_FLAGS) src/sqlite3-stats.c src/stats/*.c -o dist/stats.so -lm
	gcc -O3 $(LINIX_FLAGS) src/sqlite3-text.c src/text/*.c src/text/*/*.c -o dist/text.so
	gcc -O3 $(LINIX_FLAGS) src/sqlite3-time.c src/time/*.c -o dist/time.so
	gcc -O3 $(LINIX_FLAGS) src/sqlite3-unicode.c src/unicode/*.c -o dist/unicode.so
	gcc -O3 $(LINIX_FLAGS) src/sqlite3-uuid.c src/uuid/*.c -o dist/uuid.so
	gcc -O3 $(LINIX_FLAGS) src/sqlite3-vsv.c src/vsv/*.c -o dist/vsv.so -lm
	gcc -O1 $(LINIX_FLAGS) -include src/regexp/constants.h src/sqlite3-sqlean.c src/crypto/*.c src/define/*.c src/fileio/*.c src/fuzzy/*.c src/ipaddr/*.c src/math/*.c src/regexp/*.c src/regexp/pcre2/*.c src/stats/*.c src/text/*.c src/text/*/*.c src/time/*.c src/unicode/*.c src/uuid/*.c src/vsv/*.c -o dist/sqlean.so -lm

compile-linux-x86:
	mkdir -p dist/x86
	gcc -O1 $(LINIX_FLAGS) src/sqlite3-crypto.c src/crypto/*.c -o dist/x86/crypto.so
	gcc -O3 $(LINIX_FLAGS) src/sqlite3-define.c src/define/*.c -o dist/x86/define.so
	gcc -O3 $(LINIX_FLAGS) src/sqlite3-fileio.c src/fileio/*.c -o dist/x86/fileio.so
	gcc -O1 $(LINIX_FLAGS) src/sqlite3-fuzzy.c src/fuzzy/*.c -o dist/x86/fuzzy.so
	gcc -O3 $(LINIX_FLAGS) src/sqlite3-ipaddr.c src/ipaddr/*.c -o dist/x86/ipaddr.so
	gcc -O3 $(LINIX_FLAGS) src/sqlite3-math.c src/math/*.c -o dist/x86/math.so -lm
	gcc -O3 $(LINIX_FLAGS) -include src/regexp/constants.h src/sqlite3-regexp.c src/regexp/*.c src/regexp/pcre2/*.c -o dist/x86/regexp.so
	gcc -O3 $(LINIX_FLAGS) src/sqlite3-stats.c src/stats/*.c -o dist/x86/stats.so -lm
	gcc -O3 $(LINIX_FLAGS) src/sqlite3-text.c src/text/*.c src/text/*/*.c -o dist/x86/text.so
	gcc -O3 $(LINIX_FLAGS) src/sqlite3-time.c src/time/*.c -o dist/x86/time.so
	gcc -O3 $(LINIX_FLAGS) src/sqlite3-unicode.c src/unicode/*.c -o dist/x86/unicode.so
	gcc -O3 $(LINIX_FLAGS) src/sqlite3-uuid.c src/uuid/*.c -o dist/x86/uuid.so
	gcc -O3 $(LINIX_FLAGS) src/sqlite3-vsv.c src/vsv/*.c -o dist/x86/vsv.so -lm
	gcc -O1 $(LINIX_FLAGS) -include src/regexp/constants.h src/sqlite3-sqlean.c src/crypto/*.c src/define/*.c src/fileio/*.c src/fuzzy/*.c src/ipaddr/*.c src/math/*.c src/regexp/*.c src/regexp/pcre2/*.c src/stats/*.c src/text/*.c src/text/*/*.c src/time/*.c src/unicode/*.c src/uuid/*.c src/vsv/*.c -o dist/x86/sqlean.so -lm

compile-linux-musl:
	mkdir -p dist/musl
	musl-gcc -O1 $(LINIX_FLAGS) src/sqlite3-crypto.c src/crypto/*.c -o dist/musl/crypto.so
	musl-gcc -O3 $(LINIX_FLAGS) src/sqlite3-define.c src/define/*.c -o dist/musl/define.so
	musl-gcc -O3 $(LINIX_FLAGS) src/sqlite3-fileio.c src/fileio/*.c -o dist/musl/fileio.so
	musl-gcc -O1 $(LINIX_FLAGS) src/sqlite3-fuzzy.c src/fuzzy/*.c -o dist/musl/fuzzy.so
	musl-gcc -O3 $(LINIX_FLAGS) src/sqlite3-ipaddr.c src/ipaddr/*.c -o dist/musl/ipaddr.so
	musl-gcc -O3 $(LINIX_FLAGS) src/sqlite3-math.c src/math/*.c -o dist/musl/math.so -lm
	musl-gcc -O3 $(LINIX_FLAGS) -include src/regexp/constants.h src/sqlite3-regexp.c src/regexp/*.c src/regexp/pcre2/*.c -o dist/musl/regexp.so
	musl-gcc -O3 $(LINIX_FLAGS) src/sqlite3-stats.c src/stats/*.c -o dist/musl/stats.so -lm
	musl-gcc -O3 $(LINIX_FLAGS) src/sqlite3-text.c src/text/*.c src/text/*/*.c -o dist/musl/text.so
	musl-gcc -O3 $(LINIX_FLAGS) src/sqlite3-time.c src/time/*.c -o dist/musl/time.so
	musl-gcc -O3 $(LINIX_FLAGS) src/sqlite3-unicode.c src/unicode/*.c -o dist/musl/unicode.so
	musl-gcc -O3 $(LINIX_FLAGS) src/sqlite3-uuid.c src/uuid/*.c -o dist/musl/uuid.so
	musl-gcc -O3 $(LINIX_FLAGS) src/sqlite3-vsv.c src/vsv/*.c -o dist/musl/vsv.so -lm
	musl-gcc -O1 $(LINIX_FLAGS) -include src/regexp/constants.h src/sqlite3-sqlean.c src/crypto/*.c src/define/*.c src/fileio/*.c src/fuzzy/*.c src/ipaddr/*.c src/math/*.c src/regexp/*.c src/regexp/pcre2/*.c src/stats/*.c src/text/*.c src/text/*/*.c src/time/*.c src/unicode/*.c src/uuid/*.c src/vsv/*.c -o dist/musl/sqlean.so -lm

compile-linux-arm64:
	mkdir -p dist/arm64
	aarch64-linux-gnu-gcc -O1 $(LINIX_FLAGS) src/sqlite3-crypto.c src/crypto/*.c -o dist/arm64/crypto.so
	aarch64-linux-gnu-gcc -O3 $(LINIX_FLAGS) src/sqlite3-define.c src/define/*.c -o dist/arm64/define.so
	aarch64-linux-gnu-gcc -O3 $(LINIX_FLAGS) src/sqlite3-fileio.c src/fileio/*.c -o dist/arm64/fileio.so
	aarch64-linux-gnu-gcc -O1 $(LINIX_FLAGS) src/sqlite3-fuzzy.c src/fuzzy/*.c -o dist/arm64/fuzzy.so
	aarch64-linux-gnu-gcc -O3 $(LINIX_FLAGS) src/sqlite3-ipaddr.c src/ipaddr/*.c -o dist/arm64/ipaddr.so
	aarch64-linux-gnu-gcc -O3 $(LINIX_FLAGS) src/sqlite3-math.c src/math/*.c -o dist/arm64/math.so -lm
	aarch64-linux-gnu-gcc -O3 $(LINIX_FLAGS) -include src/regexp/constants.h src/sqlite3-regexp.c src/regexp/*.c src/regexp/pcre2/*.c -o dist/arm64/regexp.so
	aarch64-linux-gnu-gcc -O3 $(LINIX_FLAGS) src/sqlite3-stats.c src/stats/*.c -o dist/arm64/stats.so -lm
	aarch64-linux-gnu-gcc -O3 $(LINIX_FLAGS) src/sqlite3-text.c src/text/*.c src/text/*/*.c -o dist/arm64/text.so
	aarch64-linux-gnu-gcc -O3 $(LINIX_FLAGS) src/sqlite3-time.c src/time/*.c -o dist/arm64/time.so
	aarch64-linux-gnu-gcc -O3 $(LINIX_FLAGS) src/sqlite3-unicode.c src/unicode/*.c -o dist/arm64/unicode.so
	aarch64-linux-gnu-gcc -O3 $(LINIX_FLAGS) src/sqlite3-uuid.c src/uuid/*.c -o dist/arm64/uuid.so
	aarch64-linux-gnu-gcc -O3 $(LINIX_FLAGS) src/sqlite3-vsv.c src/vsv/*.c -o dist/arm64/vsv.so -lm
	aarch64-linux-gnu-gcc -O1 $(LINIX_FLAGS) -include src/regexp/constants.h src/sqlite3-sqlean.c src/crypto/*.c src/define/*.c src/fileio/*.c src/fuzzy/*.c src/ipaddr/*.c src/math/*.c src/regexp/*.c src/regexp/pcre2/*.c src/stats/*.c src/text/*.c src/text/*/*.c src/time/*.c src/unicode/*.c src/uuid/*.c src/vsv/*.c -o dist/arm64/sqlean.so -lm

pack-linux:
	zip -j dist/sqlean-linux-x86.zip dist/x86/*.so
	zip -j dist/sqlean-linux-arm64.zip dist/arm64/*.so

compile-windows:
	gcc -O1 $(WINDO_FLAGS) src/sqlite3-crypto.c src/crypto/*.c -o dist/crypto.dll
	gcc -O3 $(WINDO_FLAGS) src/sqlite3-define.c src/define/*.c -o dist/define.dll
	gcc -O3 $(WINDO_FLAGS) src/sqlite3-fileio.c src/fileio/*.c -o dist/fileio.dll
	gcc -O1 $(WINDO_FLAGS) src/sqlite3-fuzzy.c src/fuzzy/*.c -o dist/fuzzy.dll
	gcc -O3 $(WINDO_FLAGS) src/sqlite3-math.c src/math/*.c -o dist/math.dll -lm
	gcc -O3 $(WINDO_FLAGS) src/sqlite3-regexp.c -include src/regexp/constants.h src/regexp/*.c src/regexp/pcre2/*.c -o dist/regexp.dll
	gcc -O3 $(WINDO_FLAGS) src/sqlite3-stats.c src/stats/*.c -o dist/stats.dll -lm
	gcc -O3 $(WINDO_FLAGS) src/sqlite3-text.c src/text/*.c src/text/*/*.c -o dist/text.dll
	gcc -O3 $(WINDO_FLAGS) src/sqlite3-time.c src/time/*.c -o dist/time.dll
	gcc -O3 $(WINDO_FLAGS) src/sqlite3-unicode.c src/unicode/*.c -o dist/unicode.dll
	gcc -O3 $(WINDO_FLAGS) src/sqlite3-uuid.c src/uuid/*.c -o dist/uuid.dll
	gcc -O3 $(WINDO_FLAGS) src/sqlite3-vsv.c src/vsv/*.c -o dist/vsv.dll -lm
	gcc -O1 $(WINDO_FLAGS) -include src/regexp/constants.h src/sqlite3-sqlean.c src/crypto/*.c src/define/*.c src/fileio/*.c src/fuzzy/*.c src/math/*.c src/regexp/*.c src/regexp/pcre2/*.c src/stats/*.c src/text/*.c src/text/*/*.c src/time/*.c src/unicode/*.c src/uuid/*.c src/vsv/*.c -o dist/sqlean.dll -lm

compile-windows-x86:
	mkdir -p dist/x86
	gcc -O1 $(WIN32_FLAGS) src/sqlite3-crypto.c src/crypto/*.c -o dist/x86/crypto.dll
	gcc -O3 $(WIN32_FLAGS) src/sqlite3-define.c src/define/*.c -o dist/x86/define.dll
	gcc -O3 $(WIN32_FLAGS) src/sqlite3-fileio.c src/fileio/*.c -o dist/x86/fileio.dll
	gcc -O1 $(WIN32_FLAGS) src/sqlite3-fuzzy.c src/fuzzy/*.c -o dist/x86/fuzzy.dll
	gcc -O3 $(WIN32_FLAGS) src/sqlite3-math.c src/math/*.c -o dist/x86/math.dll -lm
	gcc -O3 $(WIN32_FLAGS) src/sqlite3-regexp.c -include src/regexp/constants.h src/regexp/*.c src/regexp/pcre2/*.c -o dist/x86/regexp.dll
	gcc -O3 $(WIN32_FLAGS) src/sqlite3-stats.c src/stats/*.c -o dist/x86/stats.dll -lm
	gcc -O3 $(WIN32_FLAGS) src/sqlite3-text.c src/text/*.c src/text/*/*.c -o dist/x86/text.dll
	gcc -O3 $(WIN32_FLAGS) src/sqlite3-unicode.c src/unicode/*.c -o dist/x86/unicode.dll
	gcc -O3 $(WIN32_FLAGS) src/sqlite3-uuid.c src/uuid/*.c -o dist/x86/uuid.dll
	gcc -O3 $(WIN32_FLAGS) src/sqlite3-vsv.c src/vsv/*.c -o dist/x86/vsv.dll -lm
	gcc -O1 $(WIN32_FLAGS) -include src/regexp/constants.h src/sqlite3-sqlean.c src/crypto/*.c src/define/*.c src/fileio/*.c src/fuzzy/*.c src/math/*.c src/regexp/*.c src/regexp/pcre2/*.c src/stats/*.c src/text/*.c src/text/*/*.c src/unicode/*.c src/uuid/*.c src/vsv/*.c -o dist/x86/sqlean.dll -lm

pack-windows:
	7z a -tzip dist/sqlean-win-x86.zip ./dist/x86/*.dll
	7z a -tzip dist/sqlean-win-x64.zip ./dist/*.dll

compile-macos:
	gcc -O1 $(MACOS_FLAGS) src/sqlite3-crypto.c src/crypto/*.c -o dist/crypto.dylib
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-define.c src/define/*.c -o dist/define.dylib
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-fileio.c src/fileio/*.c -o dist/fileio.dylib
	gcc -O1 $(MACOS_FLAGS) src/sqlite3-fuzzy.c src/fuzzy/*.c -o dist/fuzzy.dylib
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-ipaddr.c src/ipaddr/*.c -o dist/ipaddr.dylib
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-math.c src/math/*.c -o dist/math.dylib -lm
	gcc -O3 $(MACOS_FLAGS) -include src/regexp/constants.h src/sqlite3-regexp.c src/regexp/*.c src/regexp/pcre2/*.c -o dist/regexp.dylib
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-stats.c src/stats/*.c -o dist/stats.dylib -lm
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-text.c src/text/*.c src/text/*/*.c -o dist/text.dylib
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-time.c src/time/*.c -o dist/time.dylib
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-unicode.c src/unicode/*.c -o dist/unicode.dylib
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-uuid.c src/uuid/*.c -o dist/uuid.dylib
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-vsv.c src/vsv/*.c -o dist/vsv.dylib -lm
	gcc -O1 $(MACOS_FLAGS) -include src/regexp/constants.h src/sqlite3-sqlean.c src/crypto/*.c src/define/*.c src/fileio/*.c src/fuzzy/*.c src/ipaddr/*.c src/math/*.c src/regexp/*.c src/regexp/pcre2/*.c src/stats/*.c src/text/*.c src/text/*/*.c src/time/*.c src/unicode/*.c src/uuid/*.c src/vsv/*.c -o dist/sqlean.dylib -lm

compile-macos-x86:
	mkdir -p dist/x86
	gcc -O1 $(MACOS_FLAGS) src/sqlite3-crypto.c src/crypto/*.c -o dist/x86/crypto.dylib -target x86_64-apple-macos10.12
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-define.c src/define/*.c -o dist/x86/define.dylib -target x86_64-apple-macos10.12
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-fileio.c src/fileio/*.c -o dist/x86/fileio.dylib -target x86_64-apple-macos10.12
	gcc -O1 $(MACOS_FLAGS) src/sqlite3-fuzzy.c src/fuzzy/*.c -o dist/x86/fuzzy.dylib -target x86_64-apple-macos10.12
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-ipaddr.c src/ipaddr/*.c -o dist/x86/ipaddr.dylib -target x86_64-apple-macos10.12
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-math.c src/math/*.c -o dist/x86/math.dylib -target x86_64-apple-macos10.12 -lm
	gcc -O3 $(MACOS_FLAGS) -include src/regexp/constants.h src/sqlite3-regexp.c src/regexp/*.c src/regexp/pcre2/*.c -o dist/x86/regexp.dylib -target x86_64-apple-macos10.12
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-stats.c src/stats/*.c -o dist/x86/stats.dylib -target x86_64-apple-macos10.12 -lm
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-text.c src/text/*.c src/text/*/*.c -o dist/x86/text.dylib -target x86_64-apple-macos10.12
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-time.c src/time/*.c -o dist/x86/time.dylib -target x86_64-apple-macos10.12
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-unicode.c src/unicode/*.c -o dist/x86/unicode.dylib -target x86_64-apple-macos10.12
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-uuid.c src/uuid/*.c -o dist/x86/uuid.dylib -target x86_64-apple-macos10.12
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-vsv.c src/vsv/*.c -o dist/x86/vsv.dylib -target x86_64-apple-macos10.12 -lm
	gcc -O1 $(MACOS_FLAGS) -include src/regexp/constants.h src/sqlite3-sqlean.c src/crypto/*.c src/define/*.c src/fileio/*.c src/fuzzy/*.c src/ipaddr/*.c src/math/*.c src/regexp/*.c src/regexp/pcre2/*.c src/stats/*.c src/text/*.c src/text/*/*.c src/time/*.c src/unicode/*.c src/uuid/*.c src/vsv/*.c -o dist/x86/sqlean.dylib -target x86_64-apple-macos10.12 -lm

compile-macos-arm64:
	mkdir -p dist/arm64
	gcc -O1 $(MACOS_FLAGS) src/sqlite3-crypto.c src/crypto/*.c -o dist/arm64/crypto.dylib -target arm64-apple-macos11
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-define.c src/define/*.c -o dist/arm64/define.dylib -target arm64-apple-macos11
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-fileio.c src/fileio/*.c -o dist/arm64/fileio.dylib -target arm64-apple-macos11
	gcc -O1 $(MACOS_FLAGS) src/sqlite3-fuzzy.c src/fuzzy/*.c -o dist/arm64/fuzzy.dylib -target arm64-apple-macos11
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-ipaddr.c src/ipaddr/*.c -o dist/arm64/ipaddr.dylib -target arm64-apple-macos11
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-math.c src/math/*.c -o dist/arm64/math.dylib -target arm64-apple-macos11 -lm
	gcc -O3 $(MACOS_FLAGS) -include src/regexp/constants.h src/sqlite3-regexp.c src/regexp/*.c src/regexp/pcre2/*.c -o dist/arm64/regexp.dylib -target arm64-apple-macos11
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-stats.c src/stats/*.c -o dist/arm64/stats.dylib -target arm64-apple-macos11 -lm
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-text.c src/text/*.c src/text/*/*.c -o dist/arm64/text.dylib -target arm64-apple-macos11
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-time.c src/time/*.c -o dist/arm64/time.dylib -target arm64-apple-macos11
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-unicode.c src/unicode/*.c -o dist/arm64/unicode.dylib -target arm64-apple-macos11
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-uuid.c src/uuid/*.c -o dist/arm64/uuid.dylib -target arm64-apple-macos11
	gcc -O3 $(MACOS_FLAGS) src/sqlite3-vsv.c src/vsv/*.c -o dist/arm64/vsv.dylib -target arm64-apple-macos11 -lm
	gcc -O1 $(MACOS_FLAGS) -include src/regexp/constants.h src/sqlite3-sqlean.c src/crypto/*.c src/define/*.c src/fileio/*.c src/fuzzy/*.c src/ipaddr/*.c src/math/*.c src/regexp/*.c src/regexp/pcre2/*.c src/stats/*.c src/text/*.c src/text/*/*.c src/time/*.c src/unicode/*.c src/uuid/*.c src/vsv/*.c -o dist/arm64/sqlean.dylib -target arm64-apple-macos11 -lm

pack-macos:
	zip -j dist/sqlean-macos-x86.zip dist/x86/*.dylib
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

# fails if grep does find a failed test case
# https://stackoverflow.com/questions/15367674/bash-one-liner-to-exit-with-the-opposite-status-of-a-grep-command/21788642
test:
	@sqlite3 < test/$(suite).sql > test.log
	@cat test.log | (! grep -Ex "[0-9_]+.[^1]")

ctest-all:
	gcc $(CTEST_FLAGS) test/text/bstring.test.c src/text/*.c src/text/*/*.c -o text.bstring
	make ctest package=text module=bstring
	gcc $(CTEST_FLAGS) test/text/rstring.test.c src/text/*.c src/text/*/*.c -o text.rstring
	make ctest package=text module=rstring
	gcc $(CTEST_FLAGS) test/text/utf8.test.c src/text/utf8/*.c -o text.utf8
	make ctest package=text module=utf8
	gcc $(CTEST_FLAGS) test/time/time.test.c src/time/*.c -o time.time
	make ctest package=time module=time
	gcc $(CTEST_FLAGS) test/time/duration.test.c src/time/*.c -o time.duration -lm
	make ctest package=time module=duration

ctest:
	@chmod +x $(package).$(module)
	@./$(package).$(module)
	@rm -f $(package).$(module)
