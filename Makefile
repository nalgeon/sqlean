# Copyright (c) 2021 Anton Zhiyanov, MIT License
# https://github.com/nalgeon/sqlean

.PHONY: prepare-dist download-sqlite download-external compile-linux compile-windows compile-macos test test-all

SQLITE_RELEASE_YEAR := 2023
SQLITE_VERSION := 3420000

prepare-dist:
	mkdir -p dist
	rm -f dist/*

download-sqlite:
	curl -L http://sqlite.org/$(SQLITE_RELEASE_YEAR)/sqlite-amalgamation-$(SQLITE_VERSION).zip --output src.zip
	unzip src.zip
	mv sqlite-amalgamation-$(SQLITE_VERSION)/* src

download-external:
	curl -L https://github.com/sqlite/sqlite/raw/master/ext/misc/btreeinfo.c --output src/btreeinfo.c
	# curl -L https://github.com/davegamble/cjson/raw/master/cJSON.c --output src/cJSON.c
	# curl -L https://github.com/davegamble/cjson/raw/master/cJSON.h --output src/cJSON.h
	curl -L https://github.com/sqlite/sqlite/raw/master/ext/misc/closure.c --output src/closure.c
	curl -L https://github.com/sqlite/sqlite/raw/master/ext/misc/compress.c --output src/compress.c
	curl -L https://github.com/daschr/sqlite3_extensions/raw/master/cron.c --output src/cron.c
	curl -L https://github.com/sqlite/sqlite/raw/master/ext/misc/decimal.c --output src/decimal.c
	curl -L https://github.com/sqlite/sqlite/raw/master/ext/misc/ieee754.c --output src/ieee754.c
	cat src/lines.h > src/lines.c
	curl -L https://github.com/sqlite/sqlite/raw/branch-3.36/ext/misc/json1.c --output src/json1.c
	curl -L https://github.com/asg017/sqlite-lines/raw/main/sqlite-lines.c >> src/lines.c
	curl -L https://github.com/sqlite/sqlite/raw/master/ext/misc/memstat.c --output src/memstat.c
	cat src/path.h > src/path.c
	curl -L https://github.com/asg017/sqlite-path/raw/main/sqlite-path.c >> src/path.c
	curl -L https://github.com/likle/cwalk/raw/master/include/cwalk.h --output src/cwalk.h
	curl -L https://github.com/likle/cwalk/raw/master/src/cwalk.c --output src/cwalk.c
	curl -L https://raw.githubusercontent.com/jakethaw/pivot_vtab/08ab07973a02b74a91ab3a61f2afa26b658191d0/pivot_vtab.c --output src/pivotvtab.c
	curl -L https://github.com/sqlite/sqlite/raw/master/ext/misc/prefixes.c --output src/prefixes.c
	curl -L https://github.com/sqlite/sqlite/raw/master/ext/misc/spellfix.c --output src/spellfix.c
	curl -L https://github.com/sqlite/sqlite/raw/master/ext/misc/sqlar.c --output src/sqlar.c
	curl -L https://github.com/sqlite/sqlite/raw/master/ext/misc/stmt.c --output src/stmt.c
	curl -L https://github.com/sqlite/sqlite/raw/master/ext/misc/uint.c --output src/uint.c
	curl -L https://github.com/sqlite/sqlite/raw/master/ext/misc/unionvtab.c --output src/unionvtab.c
	curl -L https://github.com/jakethaw/xml_to_json/raw/master/xml_to_json.c --output src/xmltojson.c
	curl -L https://github.com/sqlite/sqlite/raw/master/ext/misc/zipfile.c --output src/zipfile.c
	curl -L https://github.com/sqlite/sqlite/raw/master/ext/misc/zorder.c --output src/zorder.c

compile-linux:
	make compile-linux-extension name=array src="src/array/*.c"
	make compile-linux-extension name=besttype
	make compile-linux-extension name=bloom
	make compile-linux-extension name=btreeinfo
	make compile-linux-extension name=cbrt args="-lm"
	make compile-linux-extension name=classifier
	make compile-linux-extension name=closure
	make compile-linux-extension name=compress args="-lz"
	make compile-linux-extension name=cron
	make compile-linux-extension name=dbdump
	make compile-linux-extension name=decimal
	make compile-linux-extension name=envfuncs
	make compile-linux-extension name=fcmp
	make compile-linux-extension name=ieee754
	make compile-linux-extension name=interpolate
	make compile-linux-extension name=isodate
	make compile-linux-extension name=json1
	# make compile-linux-extension name=json2 src="src/cJSON.c"
	make compile-linux-extension name=lines
	make compile-linux-extension name=math2
	make compile-linux-extension name=memstat
	make compile-linux-extension name=path src="src/cwalk.c"
	make compile-linux-extension name=pearson
	make compile-linux-extension name=pivotvtab
	make compile-linux-extension name=prefixes
	make compile-linux-extension name=recsize
	make compile-linux-extension name=rotate
	make compile-linux-extension name=spellfix
	make compile-linux-extension name=sqlar args="-lz"
	make compile-linux-extension name=stats2
	make compile-linux-extension name=stats3
	make compile-linux-extension name=stmt
	make compile-linux-extension name=text2
	make compile-linux-extension name=uint
	make compile-linux-extension name=unhex
	make compile-linux-extension name=unionvtab
	make compile-linux-extension name=xmltojson args="-DSQLITE"
	make compile-linux-extension name=zipfile args="-lz"
	make compile-linux-extension name=zorder

compile-linux-extension:
	gcc -fPIC -shared -Isrc src/$(name).c $(src) -o dist/$(name).so $(args)

compile-windows:
	make compile-windows-extension name=array src="src/array/*.c"
	make compile-windows-extension name=besttype
	make compile-windows-extension name=bloom
	make compile-windows-extension name=btreeinfo
	make compile-windows-extension name=cbrt args="-lm"
	make compile-windows-extension name=classifier
	make compile-windows-extension name=closure
	# make compile-windows-extension name=compress args="-lz"
	make compile-windows-extension name=cron
	make compile-windows-extension name=dbdump
	make compile-windows-extension name=decimal
	make compile-windows-extension name=envfuncs
	make compile-windows-extension name=fcmp
	make compile-windows-extension name=ieee754
	make compile-windows-extension name=interpolate
	make compile-windows-extension name=isodate
	make compile-windows-extension name=json1
	# make compile-windows-extension name=json2 src="src/cJSON.c"
	make compile-windows-extension name=math2
	make compile-windows-extension name=memstat
	make compile-windows-extension name=path src="src/cwalk.c"
	make compile-windows-extension name=pearson
	make compile-windows-extension name=pivotvtab
	make compile-windows-extension name=prefixes
	make compile-windows-extension name=recsize
	make compile-windows-extension name=rotate
	make compile-windows-extension name=spellfix
	# make compile-windows-extension name=sqlar args="-lz"
	make compile-windows-extension name=stats2
	make compile-windows-extension name=stats3
	make compile-windows-extension name=stmt
	make compile-windows-extension name=text2
	make compile-windows-extension name=uint
	make compile-windows-extension name=unhex
	make compile-windows-extension name=unionvtab
	make compile-windows-extension name=xmltojson args="-DSQLITE"
	# make compile-windows-extension name=zipfile args="-lz"
	make compile-windows-extension name=zorder

compile-windows-extension:
	gcc -fPIC -shared -Isrc src/$(name).c $(src) -o dist/$(name).dll $(args)

compile-macos:
	make compile-macos-extension name=array src="src/array/*.c"
	make compile-macos-extension name=besttype
	make compile-macos-extension name=bloom
	make compile-macos-extension name=btreeinfo
	make compile-macos-extension name=cbrt
	make compile-macos-extension name=classifier
	make compile-macos-extension name=closure
	make compile-macos-extension name=compress args="-lz"
	make compile-macos-extension name=cron
	make compile-macos-extension name=dbdump
	make compile-macos-extension name=decimal
	make compile-macos-extension name=envfuncs
	make compile-macos-extension name=fcmp
	make compile-macos-extension name=ieee754
	make compile-macos-extension name=interpolate
	make compile-macos-extension name=isodate
	make compile-macos-extension name=json1
	# make compile-macos-extension name=json2 src="src/cJSON.c"
	make compile-macos-extension name=lines
	make compile-macos-extension name=math2
	make compile-macos-extension name=memstat
	make compile-macos-extension name=path src="src/cwalk.c"
	make compile-macos-extension name=pearson
	make compile-macos-extension name=pivotvtab
	make compile-macos-extension name=prefixes
	make compile-macos-extension name=recsize
	make compile-macos-extension name=rotate
	make compile-macos-extension name=spellfix
	make compile-macos-extension name=sqlar args="-lz"
	make compile-macos-extension name=stats2
	make compile-macos-extension name=stats3
	make compile-macos-extension name=stmt
	make compile-macos-extension name=text2
	make compile-macos-extension name=uint
	make compile-macos-extension name=unhex
	make compile-macos-extension name=unionvtab
	make compile-macos-extension name=xmltojson args="-DSQLITE"
	make compile-macos-extension name=zipfile args="-lz"
	make compile-macos-extension name=zorder

compile-macos-extension:
	gcc -fPIC -shared -Isrc src/$(name).c $(src) -o dist/$(name).x86_64.dylib -target x86_64-apple-macos10.13 $(args)
	gcc -fPIC -shared -Isrc src/$(name).c $(src) -o dist/$(name).arm64.dylib -target arm64-apple-macos11 $(args)
	lipo -create -output dist/$(name).dylib dist/$(name).x86_64.dylib dist/$(name).arm64.dylib
	rm -f dist/$(name).x86_64.dylib dist/$(name).arm64.dylib

test-all:
	sqlite3 --version
	echo 'PRAGMA compile_options;' | sqlite3
	make test suite=array
	make test suite=besttype
	# requires sqlite_dbpage, which is missing on github actions servers
	# make test suite=btreeinfo
	make test suite=bloom
	make test suite=cbrt
	make test suite=classifier
	make test suite=closure
	make test suite=compress
	make test suite=cron
	make test suite=decimal
	make test suite=dbdump
	make test suite=envfuncs
	make test suite=fcmp
	make test suite=ieee754
	make test suite=interpolate
	make test suite=isodate
	make test suite=json1
	# tests fail on ubuntu with segmentation fault
	# make test suite=json2
	make test suite=lines
	make test suite=math2
	make test suite=memstat
	make test suite=pearson
	make test suite=pivotvtab
	make test suite=prefixes
	make test suite=recsize
	make test suite=rotate
	make test suite=spellfix
	make test suite=sqlar
	make test suite=stats2
	make test suite=stats3
	make test suite=stmt
	make test suite=text2
	make test suite=uint
	make test suite=unhex
	make test suite=unionvtab
	make test suite=xmltojson
	make test suite=zipfile
	make test suite=zorder

# fails if grep does find a failed test case
# https://stackoverflow.com/questions/15367674/bash-one-liner-to-exit-with-the-opposite-status-of-a-grep-command/21788642
test:
	@sqlite3 < test/$(suite).sql > test.log
	@cat test.log | (! grep -Ex "[0-9]+.[^1]")
