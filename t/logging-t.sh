#!/bin/bash
# t/logging-t.sh: general logging tests

. common.sh

main() {
    tmpfile="$(mktemp)"
    trap 'rm -f "$tmpfile"' EXIT

    # Basics

    unset V
    unset LOG_LEVELS

    "$tpgmdir/log-debug-message-s" &> "$tmpfile"
    does-not-contain 'avocado' "$tmpfile"

    V=10 "$tpgmdir/log-debug-message-s" &> "$tmpfile"
    has-line-matching 'avocado' "$tmpfile"

    # Varying defaults

    LOG_LEVELS='*:3' "$tpgmdir/varying-log-s" &> "$tmpfile"
    has-line-matching 'error1' "$tmpfile"
    has-line-matching 'warning1' "$tmpfile"
    has-line-matching 'fixme1' "$tmpfile"
    has-line-matching 'info1' "$tmpfile"

    has-line-matching 'error2' "$tmpfile"
    has-line-matching 'warning2' "$tmpfile"
    has-line-matching 'fixme2' "$tmpfile"
    does-not-contain 'info2' "$tmpfile"     # Suppressed by change in default

    # Explicit log levels.  log-explicit-domain logs "avocado" @ DEBUG to +fruit

    LOG_LEVELS='*:10' "$tpgmdir"/log-explicit-domain-s &> "$tmpfile"
    does-not-contain 'avocado' "$tmpfile"

    LOG_LEVELS='*:0,+fruit:10' "$tpgmdir"/log-explicit-domain-s &> "$tmpfile"
    has-line-matching 'avocado' "$tmpfile"

    LOG_LEVELS='*:0,+fruit:5' "$tpgmdir"/log-explicit-domain-s &> "$tmpfile"
    has-line-matching 'avocado' "$tmpfile"

    LOG_LEVELS='+fruit:5,*:0' "$tpgmdir"/log-explicit-domain-s &> "$tmpfile"
    has-line-matching 'avocado' "$tmpfile"

    LOG_LEVELS='*:0,+fruit:4' "$tpgmdir"/log-explicit-domain-s &> "$tmpfile"
    does-not-contain 'avocado' "$tmpfile"

    return 0
}

main "$@"
report-and-exit "$?"
