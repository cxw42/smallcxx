#!/bin/bash
# t/silent-t.sh: test silenceLog()

. common.sh

tmpfile="$(mktemp)"
trap 'rm -f "$tmpfile"' EXIT

unset V
unset LOG_LEVELS

"$here/silent-s" &> "$tmpfile"
does-not-contain 'error' "$tmpfile"
does-not-contain 'warning' "$tmpfile"
does-not-contain 'fixme' "$tmpfile"
does-not-contain 'info' "$tmpfile"
does-not-contain 'debug' "$tmpfile"
does-not-contain 'log' "$tmpfile"
does-not-contain 'trace' "$tmpfile"
does-not-contain 'peek' "$tmpfile"
does-not-contain 'snoop' "$tmpfile"

# TODO add tests in which I set $V or $LOG_LEVELS first

report-and-exit 0
