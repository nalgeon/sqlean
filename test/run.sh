#!/bin/sh
# Copyright (c) 2026 Anton Zhiyanov, MIT License
# https://github.com/nalgeon/sqlean

# Runs a SQL test suite:
# test/run.sh <suite> <dir with the extensions> <sqlite3 shell...>
#
# Feeds test/<suite>.sql to the shell, with `.load dist/<module>` and
# `load_extension('dist/<module>')` pointed at the given directory.
# Writes the output to <dir>/<suite>.log and prints the failed test cases.
#
# Test cases are `select '<label>', <boolean expression>;`. The suite fails
# if any of them is not true, or if the shell exits with an error (it does
# after an SQL error).
#
# With `wine <sqlite3.exe>` as the shell, the `.shell` lines run on the host
# (sh), not in Windows, since the fixtures use Unix tools. Before each one,
# the runner waits for sqlite3.exe to finish the lines before it.

set -e

suite=$1
dir=$2
shift 2

log=$dir/$suite.log
script=$(sed "s#^\.load dist/#.load $dir/#; s#load_extension('dist/#load_extension('$dir/#g" "test/$suite.sql")

run_wine() {
    sync=$dir/$suite.sync
    fifo=$(mktemp -u)
    mkfifo "$fifo"
    rm -f "$sync"

    # Silence wine's debug output and MoltenVK's logging on macOS.
    WINEDEBUG=-all MVK_CONFIG_LOG_LEVEL=0 "$@" <"$fifo" >"$log" &
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
                        echo "run.sh: sqlite3.exe exited early" >&2
                        return 1
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
$script
EOF

    exec 3>&-
    rc=0
    wait "$pid" || rc=$?
    rm -f "$sync"
    # wine prints \r\n line endings, which would hide the failures.
    tr -d '\r' <"$log" >"$log.tmp"
    mv "$log.tmp" "$log"
    return $rc
}

rc=0
if [ "$1" = wine ]; then
    run_wine "$@" || rc=$?
else
    printf '%s\n' "$script" | "$@" >"$log" || rc=$?
fi

if grep -Ex "[0-9_]+.[^1]" "$log"; then
    rc=1
fi
exit $rc
