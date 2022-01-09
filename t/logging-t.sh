#!/bin/bash
# t/logging-t.sh: general logging tests

. common.sh

main() {
    tmpfile="$(mktemp)"
    trap 'rm -f "$tmpfile"' EXIT

    unset V
    unset LOG_LEVELS

    "$here/log-debug-message-s" &> "$tmpfile"
    does-not-contain 'avocado' "$tmpfile"

    V=10 "$here/log-debug-message-s" &> "$tmpfile"
    has-line-matching 'avocado' "$tmpfile"

    report-and-exit 0
}
