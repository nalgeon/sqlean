# Copyright (c) 2021 Anton Zhiyanov, MIT License
# https://github.com/nalgeon/sqlean

.PHONY: prepare-dist download-sqlite download-external compile-linux compile-windows compile-macos test test-all

SQLITE_RELEASE_YEAR := 2024
SQLITE_VERSION := 3450000

prepare-dist:
	mkdir -p dist
	rm -rf dist/*

download-sqlite:
	curl -L http://sqlite.org/$(SQLITE_RELEASE_YEAR)/sqlite-amalgamation-$(SQLITE_VERSION).zip --output src.zip
	unzip src.zip
	mv sqlite-amalgamation-$(SQLITE_VERSION)/* src
	rm -rf sqlite-amalgamation-$(SQLITE_VERSION)

download-external:
	curl -L https://github.com/daschr/sqlite3_extensions/raw/master/cron.c --output src/cron.c
	curl -L https://github.com/sqlite/sqlite/raw/branch-3.36/ext/misc/json1.c --output src/json1.c
	curl -L https://raw.githubusercontent.com/jakethaw/pivot_vtab/08ab07973a02b74a91ab3a61f2afa26b658191d0/pivot_vtab.c --output src/pivotvtab.c
	curl -L https://github.com/jakethaw/xml_to_json/raw/master/xml_to_json.c --output src/xmltojson.c

compile-linux:
	make compile-linux-extension name=array src="src/array/*.c"
	make compile-linux-extension name=besttype
	make compile-linux-extension name=bloom
	make compile-linux-extension name=cbrt args="-lm"
	make compile-linux-extension name=classifier
	make compile-linux-extension name=cron
	make compile-linux-extension name=envfuncs
	make compile-linux-extension name=fcmp
	make compile-linux-extension name=interpolate
	make compile-linux-extension name=isodate
	make compile-linux-extension name=json1
	make compile-linux-extension name=math2
	make compile-linux-extension name=pearson
	make compile-linux-extension name=pivotvtab
	make compile-linux-extension name=recsize
	make compile-linux-extension name=stats2
	make compile-linux-extension name=stats3
	make compile-linux-extension name=text2
	make compile-linux-extension name=unhex
	make compile-linux-extension name=xmltojson args="-DSQLITE"

compile-linux-extension:
	gcc -O2 -fPIC -shared -Isrc src/$(name).c $(src) -o dist/$(name).so $(args)

compile-windows:
	make compile-windows-extension name=array src="src/array/*.c"
	make compile-windows-extension name=besttype
	make compile-windows-extension name=bloom
	make compile-windows-extension name=cbrt args="-lm"
	make compile-windows-extension name=classifier
	make compile-windows-extension name=closure
	make compile-windows-extension name=cron
	make compile-windows-extension name=envfuncs
	make compile-windows-extension name=fcmp
	make compile-windows-extension name=interpolate
	make compile-windows-extension name=isodate
	make compile-windows-extension name=json1
	make compile-windows-extension name=math2
	make compile-windows-extension name=pearson
	make compile-windows-extension name=pivotvtab
	make compile-windows-extension name=recsize
	make compile-windows-extension name=stats2
	make compile-windows-extension name=stats3
	make compile-windows-extension name=text2
	make compile-windows-extension name=unhex
	make compile-windows-extension name=xmltojson args="-DSQLITE"

compile-windows-extension:
	gcc -O2 -fPIC -shared -Isrc src/$(name).c $(src) -o dist/$(name).dll $(args)

compile-macos:
	make compile-macos-extension name=array src="src/array/*.c"
	make compile-macos-extension name=besttype
	make compile-macos-extension name=bloom
	make compile-macos-extension name=cbrt
	make compile-macos-extension name=classifier
	make compile-macos-extension name=cron
	make compile-macos-extension name=envfuncs
	make compile-macos-extension name=fcmp
	make compile-macos-extension name=interpolate
	make compile-macos-extension name=isodate
	make compile-macos-extension name=json1
	make compile-macos-extension name=math2
	make compile-macos-extension name=pearson
	make compile-macos-extension name=pivotvtab
	make compile-macos-extension name=recsize
	make compile-macos-extension name=stats2
	make compile-macos-extension name=stats3
	make compile-macos-extension name=text2
	make compile-macos-extension name=unhex
	make compile-macos-extension name=xmltojson args="-DSQLITE"

compile-macos-extension:
	gcc -O2 -fPIC -shared -Isrc src/$(name).c $(src) -o dist/$(name).x86_64.dylib -target x86_64-apple-macos10.13 $(args)
	gcc -O2 -fPIC -shared -Isrc src/$(name).c $(src) -o dist/$(name).arm64.dylib -target arm64-apple-macos11 $(args)
	lipo -create -output dist/$(name).dylib dist/$(name).x86_64.dylib dist/$(name).arm64.dylib
	rm -f dist/$(name).x86_64.dylib dist/$(name).arm64.dylib

test-all:
	sqlite3 --version
	echo 'PRAGMA compile_options;' | sqlite3
	make test suite=array
	make test suite=besttype
	make test suite=bloom
	make test suite=cbrt
	make test suite=classifier
	make test suite=cron
	make test suite=envfuncs
	make test suite=fcmp
	make test suite=interpolate
	make test suite=isodate
	make test suite=json1
	make test suite=math2
	make test suite=pearson
	make test suite=pivotvtab
	make test suite=recsize
	make test suite=stats2
	make test suite=stats3
	make test suite=text2
	make test suite=unhex
	make test suite=xmltojson

# fails if grep does find a failed test case
# https://stackoverflow.com/questions/15367674/bash-one-liner-to-exit-with-the-opposite-status-of-a-grep-command/21788642
test:
	@sqlite3 < test/$(suite).sql > test.log
	@cat test.log | (! grep -Ex "[0-9]+.[^1]")
