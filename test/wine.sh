#!/bin/sh
# Copyright (c) 2026 Anton Zhiyanov, MIT License
# https://github.com/nalgeon/sqlean

# Runs a SQL test suite with the Windows build under wine:
# test/wine.sh <suite> <sqlite3.exe> <dir with the extensions>
#
# The suite is fed to sqlite3.exe line by line, with `.load dist/<module>`
# and `load_extension('dist/<module>')` pointed at the Windows build.
# The `.shell` lines run on the host (sh), not in Windows, since the fixtures
# use Unix tools. Before each one, the runner waits for sqlite3.exe to finish
# the lines before it.
#
# Prints the output to stdout. Exits with the exit code of sqlite3.exe.

set -e

suite=$1
exe=$2
dir=$3

sync=wine-sync.txt
fifo=$(mktemp -u)
mkfifo "$fifo"
rm -f "$sync"

# Silence wine's debug output and MoltenVK's logging on macOS.
WINEDEBUG=-all MVK_CONFIG_LOG_LEVEL=0 wine "$exe" <"$fifo" &
pid=$!
exec 3>"$fifo"
rm -f "$fifo"

n=0
while IFS= read -r line || [ -n "$line" ]; do
    case "$line" in
        .shell\ *)
            # .output flushes the file when it's switched back to stdout,
            # so the marker appears once the lines before it are done.
            n=$((n + 1))
            printf '.output %s\n.print %d\n.output stdout\n' "$sync" "$n" >&3
            until [ "$(tr -d '\r' 2>/dev/null <"$sync")" = "$n" ]; do
                if ! kill -0 "$pid" 2>/dev/null; then
                    echo "wine.sh: sqlite3.exe exited early" >&2
                    exit 1
                fi
                sleep 0.05
            done
            # The shell unescapes \\ in the command, so does the runner.
            cmd=$(printf '%s\n' "${line#.shell }" | sed 's/\\\\/\\/g')
            sh -c "$cmd" || true
            ;;
        *)
            printf '%s\n' "$line" >&3
            ;;
    esac
done <<EOF
$(sed "s#^\.load dist/#.load $dir/#; s#load_extension('dist/#load_extension('$dir/#g" "test/$suite.sql")
EOF

exec 3>&-
rc=0
wait "$pid" || rc=$?
rm -f "$sync"
exit $rc
