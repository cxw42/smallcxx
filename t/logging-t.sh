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

    report-and-exit 0
}
