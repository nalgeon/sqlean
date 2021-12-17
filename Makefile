# Copyright (c) 2021 Anton Zhiyanov, MIT License
# https://github.com/nalgeon/sqlean

.PHONY: prepare-dist download-sqlite download-external compile-linux compile-windows compile-macos test test-all

prepare-dist:
	mkdir -p dist
	rm -f dist/*

download-sqlite:
	curl -L http://sqlite.org/$(SQLITE_RELEASE_YEAR)/sqlite-amalgamation-$(SQLITE_VERSION).zip --output src.zip
	unzip src.zip
	mv sqlite-amalgamation-$(SQLITE_VERSION)/* src

download-external: ;

compile-linux:
	gcc -fPIC -shared src/cbrt.c -o dist/cbrt.so -lm

compile-windows:
	gcc -shared -I. src/cbrt.c -o dist/cbrt.dll -lm

compile-macos:
	gcc -fPIC -dynamiclib -I src src/cbrt.c -o dist/cbrt.dylib -lm

# fails if grep does find a failed test case
# https://stackoverflow.com/questions/15367674/bash-one-liner-to-exit-with-the-opposite-status-of-a-grep-command/21788642
test:
	sqlite3 < test/$(suite).sql | (! grep -Ex "[0-9]+.0")

test-all:
	@for suite in test/*.sql; do\
		echo $${suite};\
		sqlite3 < $${suite} | (! grep -Ex "[0-9]+.0");\
    done
