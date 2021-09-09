.PHONY: download-sqlite download-native compile-linux compile-windows compile-macos test clean

clean:
	rm -rf dist

download-sqlite:
	curl -L http://sqlite.org/$(SQLITE_RELEASE_YEAR)/sqlite-amalgamation-$(SQLITE_VERSION).zip --output src.zip
	unzip src.zip
	mv sqlite-amalgamation-$(SQLITE_VERSION)/* src

download-native:
	curl -L https://github.com/sqlite/sqlite/raw/master/ext/misc/json1.c --output src/sqlite3-json1.c

dist:
	@mkdir -p dist
dist/crypto.so dist/crypto.dll dist/crypto.dylib: $(wildcard src/crypto/*.c)
dist/%.so: src/sqlite3-%.c
	gcc -fPIC -shared $^ -o $@ -lm
dist/%.dll: src/sqlite3-%.c
	gcc -shared -I. $^ -o $@
dist/%.dylib: src/sqlite3-%.c
	gcc -fPIC -dynamiclib -I src $^ -o $@ -lm

extensions = crypto json1 math re stats text unicode vsv
compile-linux: dist $(extensions:%=dist/%.so)
compile-windows: dist $(extensions:%=dist/%.dll)
compile-macos: dist $(extensions:%=dist/%.dylib)

# fails is grep does find a failed test case
# https://stackoverflow.com/questions/15367674/bash-one-liner-to-exit-with-the-opposite-status-of-a-grep-command/21788642
test:
	! sqlite3 < test/$(suite).sql | grep -Pvx "\d+.1"
