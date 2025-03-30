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
    has-line-matching 'info1' "$tmpfile"    # LOG_INFO is enabled by default
    has-line-matching 'print1$' "$tmpfile"  # `$` to make sure there's a newline
    has-line-matching 'printerr1$' "$tmpfile"

    has-line-matching 'error2' "$tmpfile"
    has-line-matching 'warning2' "$tmpfile"
    has-line-matching 'fixme2' "$tmpfile"
    does-not-contain 'info2' "$tmpfile"     # Suppressed by change in default
    has-line-matching 'print2' "$tmpfile"   # Not affected by change in level
    has-line-matching 'printerr2' "$tmpfile"

    # How SILENT affects PRINT and PRINTERR
    LOG_LEVELS='*:0' "$tpgmdir/varying-log-s" &> "$tmpfile"
    has-line-matching 'print1$' "$tmpfile"
    has-line-matching 'printerr1$' "$tmpfile"
    does-not-contain 'print2' "$tmpfile"
    does-not-contain 'printerr2' "$tmpfile"

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

    # Default env var
    unset SMALLCXX_TEST_DEBUG
    "$tpgmdir/testfile-s" &> "$tmpfile"
    does-not-contain 'All tests passed' "$tmpfile"
    does-not-contain 'avocado' "$tmpfile"

    SMALLCXX_TEST_DEBUG='+test:4' "$tpgmdir/testfile-s" &> "$tmpfile"
    has-line-matching 'All tests passed' "$tmpfile"

    SMALLCXX_TEST_DEBUG='*:6' "$tpgmdir/testfile-s" &> "$tmpfile"
    does-not-contain 'All tests passed' "$tmpfile"
    has-line-matching 'avocado' "$tmpfile"

    return 0
}

main "$@"
report-and-exit
