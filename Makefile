# Copyright (c) 2021 Anton Zhiyanov, MIT License
# https://github.com/nalgeon/sqlean

.PHONY: test

prepare-dist:
	mkdir -p dist
	rm -rf dist/*

download-sqlite:
	curl -L http://sqlite.org/$(SQLITE_RELEASE_YEAR)/sqlite-amalgamation-$(SQLITE_VERSION).zip --output src.zip
	unzip src.zip
	mv sqlite-amalgamation-$(SQLITE_VERSION)/* src

download-external:
	curl -L https://github.com/sqlite/sqlite/raw/branch-$(SQLITE_BRANCH)/ext/misc/json1.c --output src/sqlite3-json1.c
	curl -L https://github.com/mackyle/sqlite/raw/branch-$(SQLITE_BRANCH)/src/test_windirent.h --output src/test_windirent.h

compile-linux:
	gcc -fPIC -shared src/sqlite3-crypto.c src/crypto/*.c -o dist/crypto.so
	gcc -fPIC -shared src/sqlite3-define.c -o dist/define.so
	gcc -fPIC -shared src/sqlite3-fileio.c -o dist/fileio.so
	gcc -fPIC -shared src/sqlite3-fuzzy.c src/fuzzy/*.c -o dist/fuzzy.so
	gcc -fPIC -shared src/sqlite3-ipaddr.c -o dist/ipaddr.so
	gcc -fPIC -shared src/sqlite3-json1.c -o dist/json1.so
	gcc -fPIC -shared src/sqlite3-math.c -o dist/math.so -lm
	gcc -fPIC -shared src/sqlite3-re.c src/re.c -o dist/re.so
	gcc -fPIC -shared src/sqlite3-stats.c -o dist/stats.so -lm
	gcc -fPIC -shared src/sqlite3-text.c -o dist/text.so
	gcc -fPIC -shared src/sqlite3-unicode.c -o dist/unicode.so
	gcc -fPIC -shared src/sqlite3-uuid.c -o dist/uuid.so
	gcc -fPIC -shared src/sqlite3-vsv.c -o dist/vsv.so -lm

pack-linux:
	zip -j dist/sqlean-linux-x86.zip dist/*.so

compile-windows:
	gcc -shared -I. src/sqlite3-crypto.c src/crypto/*.c -o dist/crypto.dll
	gcc -shared -I. src/sqlite3-define.c -o dist/define.dll
	gcc -shared -I. src/sqlite3-fileio.c -o dist/fileio.dll
	gcc -shared -I. src/sqlite3-fuzzy.c src/fuzzy/*.c -o dist/fuzzy.dll
	gcc -shared -I. src/sqlite3-json1.c -o dist/json1.dll
	gcc -shared -I. src/sqlite3-math.c -o dist/math.dll -lm
	gcc -shared -I. src/sqlite3-re.c src/re.c -o dist/re.dll
	gcc -shared -I. src/sqlite3-stats.c -o dist/stats.dll -lm
	gcc -shared -I. src/sqlite3-text.c -o dist/text.dll
	gcc -shared -I. src/sqlite3-unicode.c -o dist/unicode.dll
	gcc -shared -I. src/sqlite3-uuid.c -o dist/uuid.dll
	gcc -shared -I. src/sqlite3-vsv.c -o dist/vsv.dll -lm

pack-windows:
	7z a -tzip dist/sqlean-win-x64.zip ./dist/*.dll

compile-macos:
	gcc -fPIC -dynamiclib -I src src/sqlite3-crypto.c src/crypto/*.c -o dist/crypto.dylib
	gcc -fPIC -dynamiclib -I src src/sqlite3-define.c -o dist/define.dylib
	gcc -fPIC -dynamiclib -I src src/sqlite3-fileio.c -o dist/fileio.dylib
	gcc -fPIC -dynamiclib -I src src/sqlite3-fuzzy.c src/fuzzy/*.c -o dist/fuzzy.dylib
	gcc -fPIC -dynamiclib -I src src/sqlite3-ipaddr.c -o dist/ipaddr.dylib
	gcc -fPIC -dynamiclib -I src src/sqlite3-json1.c -o dist/json1.dylib
	gcc -fPIC -dynamiclib -I src src/sqlite3-math.c -o dist/math.dylib -lm
	gcc -fPIC -dynamiclib -I src src/sqlite3-re.c src/re.c -o dist/re.dylib
	gcc -fPIC -dynamiclib -I src src/sqlite3-stats.c -o dist/stats.dylib -lm
	gcc -fPIC -dynamiclib -I src src/sqlite3-text.c -o dist/text.dylib
	gcc -fPIC -dynamiclib -I src src/sqlite3-unicode.c -o dist/unicode.dylib
	gcc -fPIC -dynamiclib -I src src/sqlite3-uuid.c -o dist/uuid.dylib
	gcc -fPIC -dynamiclib -I src src/sqlite3-vsv.c -o dist/vsv.dylib -lm

compile-macos-x86:
	mkdir -p dist/x86
	gcc -fPIC -dynamiclib -I src src/sqlite3-crypto.c src/crypto/*.c -o dist/x86/crypto.dylib -target x86_64-apple-macos10.12
	gcc -fPIC -dynamiclib -I src src/sqlite3-define.c -o dist/x86/define.dylib -target x86_64-apple-macos10.12
	gcc -fPIC -dynamiclib -I src src/sqlite3-fileio.c -o dist/x86/fileio.dylib -target x86_64-apple-macos10.12
	gcc -fPIC -dynamiclib -I src src/sqlite3-fuzzy.c src/fuzzy/*.c -o dist/x86/fuzzy.dylib -target x86_64-apple-macos10.12
	gcc -fPIC -dynamiclib -I src src/sqlite3-ipaddr.c -o dist/x86/ipaddr.dylib -target x86_64-apple-macos10.12
	gcc -fPIC -dynamiclib -I src src/sqlite3-json1.c -o dist/x86/json1.dylib -target x86_64-apple-macos10.12
	gcc -fPIC -dynamiclib -I src src/sqlite3-math.c -o dist/x86/math.dylib -target x86_64-apple-macos10.12 -lm
	gcc -fPIC -dynamiclib -I src src/sqlite3-re.c src/re.c -o dist/x86/re.dylib -target x86_64-apple-macos10.12
	gcc -fPIC -dynamiclib -I src src/sqlite3-stats.c -o dist/x86/stats.dylib -target x86_64-apple-macos10.12 -lm
	gcc -fPIC -dynamiclib -I src src/sqlite3-text.c -o dist/x86/text.dylib -target x86_64-apple-macos10.12
	gcc -fPIC -dynamiclib -I src src/sqlite3-unicode.c -o dist/x86/unicode.dylib -target x86_64-apple-macos10.12
	gcc -fPIC -dynamiclib -I src src/sqlite3-uuid.c -o dist/x86/uuid.dylib -target x86_64-apple-macos10.12
	gcc -fPIC -dynamiclib -I src src/sqlite3-vsv.c -o dist/x86/vsv.dylib -target x86_64-apple-macos10.12 -lm

compile-macos-arm64:
	mkdir -p dist/arm64
	gcc -fPIC -dynamiclib -I src src/sqlite3-crypto.c src/crypto/*.c -o dist/arm64/crypto.dylib -target arm64-apple-macos11
	gcc -fPIC -dynamiclib -I src src/sqlite3-define.c -o dist/arm64/define.dylib -target arm64-apple-macos11
	gcc -fPIC -dynamiclib -I src src/sqlite3-fileio.c -o dist/arm64/fileio.dylib -target arm64-apple-macos11
	gcc -fPIC -dynamiclib -I src src/sqlite3-fuzzy.c src/fuzzy/*.c -o dist/arm64/fuzzy.dylib -target arm64-apple-macos11
	gcc -fPIC -dynamiclib -I src src/sqlite3-ipaddr.c -o dist/arm64/ipaddr.dylib -target arm64-apple-macos11
	gcc -fPIC -dynamiclib -I src src/sqlite3-json1.c -o dist/arm64/json1.dylib -target arm64-apple-macos11
	gcc -fPIC -dynamiclib -I src src/sqlite3-math.c -o dist/arm64/math.dylib -target arm64-apple-macos11 -lm
	gcc -fPIC -dynamiclib -I src src/sqlite3-re.c src/re.c -o dist/arm64/re.dylib -target arm64-apple-macos11
	gcc -fPIC -dynamiclib -I src src/sqlite3-stats.c -o dist/arm64/stats.dylib -target arm64-apple-macos11 -lm
	gcc -fPIC -dynamiclib -I src src/sqlite3-text.c -o dist/arm64/text.dylib -target arm64-apple-macos11
	gcc -fPIC -dynamiclib -I src src/sqlite3-unicode.c -o dist/arm64/unicode.dylib -target arm64-apple-macos11
	gcc -fPIC -dynamiclib -I src src/sqlite3-uuid.c -o dist/arm64/uuid.dylib -target arm64-apple-macos11
	gcc -fPIC -dynamiclib -I src src/sqlite3-vsv.c -o dist/arm64/vsv.dylib -target arm64-apple-macos11 -lm

pack-macos:
	zip -j dist/sqlean-macos-x86.zip dist/x86/*.dylib
	zip -j dist/sqlean-macos-arm64.zip dist/arm64/*.dylib

test-all:
	make test suite=crypto
	make test suite=define
	make test suite=fileio
	make test suite=fuzzy
	make test suite=ipaddr
	make test suite=json1
	make test suite=math
	make test suite=re
	make test suite=stats
	make test suite=text
	make test suite=unicode
	make test suite=uuid
	make test suite=vsv

# fails if grep does find a failed test case
# https://stackoverflow.com/questions/15367674/bash-one-liner-to-exit-with-the-opposite-status-of-a-grep-command/21788642
test:
	@sqlite3 < test/$(suite).sql > test.log
	@cat test.log | (! grep -Ex "[0-9]+.[^1]")
