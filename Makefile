# Copyright (c) 2021 Anton Zhiyanov, MIT License
# https://github.com/nalgeon/sqlean

# Run `make help` for the commands.

.DEFAULT_GOAL := help

SQLITE_RELEASE_YEAR := 2024
SQLITE_VERSION := 3470200

SQLEAN_VERSION := '"$(or $(shell git tag --points-at HEAD),main)"'

ifeq ($(origin CC),default)
CC := gcc
endif
CFLAGS ?=

# The GCC image for on=gcc.
GCC_IMAGE ?= gcc:16.2.0
GCC_PLATFORM ?= linux/arm64

ifeq ($(on),gcc)

# Runs the whole command in Docker, on Linux with GCC. The container gets
# its own empty dist/, so the host's build artifacts stay as they are.
GOALS := $(or $(MAKECMDGOALS),$(.DEFAULT_GOAL))
.PHONY: $(GOALS)
$(firstword $(GOALS)):
	docker run --rm --platform $(GCC_PLATFORM) -v "$(CURDIR)":/src -v /src/dist -w /src $(GCC_IMAGE) \
		make CC=gcc $(GOALS) only="$(only)" san=$(san)
$(filter-out $(firstword $(GOALS)),$(GOALS)):
	@:

else

#
# Variants
#

ifeq ($(OS),Windows_NT)
HOST := windows
else
HOST := $(shell uname -s)
endif

# A variant is the compiler, the flags and the output directory.
# Each variant has its own directory, so they don't overwrite each other.
ifeq ($(on),wine)
ifeq ($(san),1)
$(error san=1 is not supported with on=wine)
endif
DIR := dist/wine
BUILD_CC := zig cc -target x86_64-windows-gnu
TARGET := windows
else ifneq ($(on),)
$(error unknown on=$(on), use on=gcc or on=wine)
else ifeq ($(san),1)
DIR := dist/san
BUILD_CC := $(CC)
TARGET := $(HOST)
else
DIR := dist
BUILD_CC := $(CC)
TARGET := $(HOST)
endif

COMMON_CFLAGS := -Isrc -DSQLEAN_VERSION=$(SQLEAN_VERSION)
# Warnings for the C tests and the sanitized build.
CTEST_FLAGS := -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unknown-pragmas -Isrc
# Address + undefined behavior sanitizers.
SANITIZE_FLAGS := -g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=undefined
# Leak detection is Linux-only. Slow unwinding gives full stacks for
# allocations made inside SQLite, which is built without frame pointers.
ifeq ($(HOST),Linux)
SANITIZE_ENV := ASAN_OPTIONS=detect_leaks=1:fast_unwind_on_malloc=0 UBSAN_OPTIONS=print_stacktrace=1
else
SANITIZE_ENV := ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=1
endif

ifeq ($(TARGET),windows)
EXT := dll
EXE := .exe
TARGET_FLAGS := -shared
else ifeq ($(TARGET),Darwin)
EXT := dylib
TARGET_FLAGS := -Wall -Wsign-compare -fPIC -dynamiclib
else
EXT := so
TARGET_FLAGS := -z now -z relro -Wall -Wsign-compare -Wno-unknown-pragmas -fPIC -shared
endif

BUILD_FLAGS := $(CFLAGS) $(TARGET_FLAGS) $(COMMON_CFLAGS)
ifeq ($(san),1)
BUILD_FLAGS += $(CTEST_FLAGS) $(SANITIZE_FLAGS)
CTEST_FLAGS += $(SANITIZE_FLAGS)
RUN_ENV := $(SANITIZE_ENV)
# The shell is built without the sanitizers, since it's third-party code,
# but linked with them, so it can load the sanitized extensions.
SHELL_LDFLAGS := -fsanitize=address,undefined
else
BUILD_FLAGS += -O3
endif
CTEST_FLAGS := $(CFLAGS) $(CTEST_FLAGS)

# The sqlite3 shell that runs the SQL tests.
SQLITE ?= $(DIR)/sqlite3$(EXE)
ifeq ($(on),wine)
RUN_SQLITE := wine $(SQLITE)
else
RUN_SQLITE := $(SQLITE)
endif

#
# Modules
#

# Each module is built from src/sqlite3-<module>.c and src/<module>/,
# the bundle from src/sqlite3-sqlean.c and every module.
MODULES := $(sort $(filter-out sqlean,$(patsubst src/sqlite3-%.c,%,$(wildcard src/sqlite3-*.c))))
ifeq ($(TARGET),windows)
BUNDLED := $(filter-out ipaddr,$(MODULES))
else
BUNDLED := $(MODULES)
endif

FLAGS_regexp := -include src/regexp/constants.h
FLAGS_sqlean := -include src/regexp/constants.h
LIBS_math := -lm
LIBS_stats := -lm
LIBS_vsv := -lm
LIBS_sqlean := -lm
ifeq ($(TARGET),windows)
LIBS_ipaddr := -lws2_32
endif

# The C files of a module directory, and all of its files.
module_c = $(wildcard src/$(1)/*.c src/$(1)/*/*.c)
module_files = $(wildcard src/$(1)/* src/$(1)/*/*)
# The sources of a module or the bundle, and the files it depends on.
sources = src/sqlite3-$(1).c $(if $(filter sqlean,$(1)),$(foreach m,$(BUNDLED),$(call module_c,$(m))),$(call module_c,$(1)))
deps = src/sqlite3-$(1).c src/*.h Makefile $(if $(filter sqlean,$(1)),$(foreach m,$(BUNDLED),$(call module_files,$(m))),$(call module_files,$(1)))

define module_rule
$(DIR)/$(1).$(EXT): $(call deps,$(1)) | $(DIR)
	$$(BUILD_CC) $$(BUILD_FLAGS) $$(FLAGS_$(1)) $(call sources,$(1)) -o $$@ $$(LIBS_$(1))
endef
$(foreach m,$(MODULES) sqlean,$(eval $(call module_rule,$(m))))

$(DIR):
	mkdir -p $@

$(DIR)/sqlite3$(EXE): src/sqlite3.c src/shell.c | $(DIR)
	$(BUILD_CC) -g -O1 -w -c src/sqlite3.c -o $(DIR)/sqlite3.o
	$(BUILD_CC) -g -O1 -w -c src/shell.c -o $(DIR)/shell.o
	$(BUILD_CC) $(SHELL_LDFLAGS) $(DIR)/sqlite3.o $(DIR)/shell.o -o $@ $(if $(filter windows,$(TARGET)),,-lm)
	rm -f $(DIR)/sqlite3.o $(DIR)/shell.o

#
# Tests
#

# SQL tests: test/<suite>.sql, one suite per module plus the bundle.
SUITES := $(patsubst test/%.sql,%,$(wildcard test/*.sql))

# C tests as <package>/<module>: test/<package>/<module>.test.c.
# regexp/regexp is out of date with the regexp API.
CTESTS := $(filter-out regexp/regexp,$(patsubst test/%.test.c,%,$(wildcard test/*/*.test.c)))

# A C test compiles with the sources of its package, except extension.c,
# unless CTEST_SRC_<package> says otherwise. define and fileio run
# the extension in a real connection, so they link SQLite.
CTEST_SRC_define := $(wildcard src/define/*.c) dist/ctest-sqlite3.o
CTEST_FLAGS_define := -DSQLITE_CORE
CTEST_SRC_fileio := $(wildcard src/fileio/*.c) dist/ctest-sqlite3.o
CTEST_FLAGS_fileio := -DSQLITE_CORE
ctest_src = $(or $(CTEST_SRC_$(1)),$(filter-out src/$(1)/extension.c,$(call module_c,$(1))))

# only= picks the modules: only="fuzzy text", or only=time/duration
# for a single C test.
ONLY := $(sort $(foreach o,$(only),$(firstword $(subst /, ,$(o)))))
ifneq ($(filter-out $(MODULES) sqlean,$(ONLY)),)
$(error unknown module in only=$(only))
endif
ONLY_MODULES := $(if $(only),$(ONLY),$(MODULES) sqlean)
ONLY_SUITES := $(filter $(ONLY_MODULES),$(SUITES))
ONLY_CTESTS := $(if $(only),$(filter $(only) $(addsuffix /%,$(only)),$(CTESTS)),$(CTESTS))

.PHONY: help build test ctest check

help:
	@echo "Usage: make <action> [option=value ...]"
	@echo
	@echo "Actions:"
	@echo "  build    build the extensions"
	@echo "  test     build the extensions and run the SQL tests (test/<module>.sql)"
	@echo "  ctest    run the C tests (test/<module>/*.test.c)"
	@echo "  check    build, ctest and test"
	@echo
	@echo "Options:"
	@echo "  only=fuzzy      only these modules (only=\"fuzzy text\"),"
	@echo "                  or a single C test (only=time/duration)"
	@echo "  san=1           with ASan and UBSan, into dist/san"
	@echo "  on=gcc          on Linux with GCC, in Docker"
	@echo "  on=wine         the Windows build under wine, into dist/wine"
	@echo "  SQLITE=<path>   the sqlite3 shell for the SQL tests (built by default)"
	@echo
	@echo "Setup: make prepare-dist download-sqlite"
	@echo "Releases: compile-{linux,macos}-{x64,arm64}, compile-linux-musl, pack-{linux,macos,windows}"

build: $(foreach m,$(ONLY_MODULES),$(DIR)/$(m).$(EXT))

# Runs every suite, even after a failure, and lists the failed ones.
# A sanitizer report kills the shell, so it fails the suite.
test: $(foreach s,$(ONLY_SUITES),$(DIR)/$(s).$(EXT)) $(filter $(DIR)/%,$(SQLITE))
	@failed=""; for s in $(ONLY_SUITES); do \
		echo "test $$s"; \
		$(RUN_ENV) sh test/run.sh $$s $(DIR) $(RUN_SQLITE) || failed="$$failed $$s"; \
	done; \
	if [ -n "$$failed" ]; then echo "failed:$$failed"; exit 1; fi

ifeq ($(on),wine)
ctest:
	$(error the C tests don't run under wine)
check: build test
else
ctest: $(addprefix ctest/,$(ONLY_CTESTS))
check: build ctest test
endif

# Compiles and runs a C test, then deletes the binary.
ctest/%: | $(DIR)
	@echo "ctest $*"
	@$(CC) $(CTEST_FLAGS) $(CTEST_FLAGS_$(*D)) test/$*.test.c $(call ctest_src,$(*D)) -o $(DIR)/ctest-$(*D)-$(*F) -lm
	@$(RUN_ENV) ./$(DIR)/ctest-$(*D)-$(*F); rc=$$?; rm -rf $(DIR)/ctest-$(*D)-$(*F) $(DIR)/ctest-$(*D)-$(*F).dSYM; exit $$rc

$(addprefix ctest/,$(filter define/% fileio/%,$(CTESTS))): dist/ctest-sqlite3.o

# SQLite for the C tests that link it. Built without warnings and sanitizers,
# since it's third-party code, and without threads and extension loading,
# so it links without -lpthread and -ldl.
dist/ctest-sqlite3.o: src/sqlite3.c
	@mkdir -p dist
	$(CC) -g -O1 -w -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION -c src/sqlite3.c -o $@

#
# Setup and releases
#

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

# Release builds, each into its own directory (Windows uses `make build`).
compile-linux-x64:
	$(MAKE) build DIR=dist/x64

compile-linux-arm64:
	$(MAKE) build DIR=dist/arm64 CC=aarch64-linux-gnu-gcc

compile-linux-musl:
	$(MAKE) build DIR=dist/musl CC=musl-gcc

compile-macos-x64:
	$(MAKE) build DIR=dist/x64 CFLAGS="-target x86_64-apple-macos10.12"

compile-macos-arm64:
	$(MAKE) build DIR=dist/arm64 CFLAGS="-target arm64-apple-macos11"

pack-linux:
	zip -j dist/sqlean-linux-x64.zip dist/x64/*.so
	zip -j dist/sqlean-linux-arm64.zip dist/arm64/*.so

pack-windows:
	7z a -tzip dist/sqlean-win-x64.zip ./dist/*.dll

pack-macos:
	zip -j dist/sqlean-macos-x64.zip dist/x64/*.dylib
	zip -j dist/sqlean-macos-arm64.zip dist/arm64/*.dylib

endif
