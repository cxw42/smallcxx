#!/bin/bash
# t/cover-colorlog-t.sh: check colorized output if possible

. common.sh

if [[ ! "$unbuffer_pgm" || "$unbuffer_pgm" = 'no' ]] || \
    ! "$unbuffer_pgm" echo </dev/null &>/dev/null
then
    # the `unbuffer echo` test is so I can make sure this works by
    # saying `./configure UNBUFFER=/bin/false`.  AC_PATH_PROG only accepts
    # absolute paths as overrides.  Thanks for that insight to
    # <https://github.com/apple/cups/issues/5412#issuecomment-430370975>.
    echo "Skipping - unbuffer(1) not available"
    exit 77
fi

tmpfile="$(mktemp)"
trap 'rm -f "$tmpfile"' EXIT

"$unbuffer_pgm" ./cover-misc-t &> "$tmpfile"
has-line-matching '/\e.*ERROR.*Oops/' "$tmpfile"
has-line-matching '/\e.*WARN.*Ummm/' "$tmpfile"

report-and-exit 0
