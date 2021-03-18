#!/bin/bash
# asan.sh: run with Address Sanitizer turned on
# Copyright (c) 2021 Christopher White <cxwembedded@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

set -x
set -eEuo pipefail

# Accept $1 == clang as a shortcut for user convenience
dirty=
args=()
if [[ "${1:-}" = 'clang' ]]; then
    args+=( 'CC=clang' 'CXX=clang++' )
    # Be conservative
    dirty=1
    shift
fi

# Reconfigure if necessary so that coverage is enabled
if (( dirty )) || [[ ! -x ./config.status ]] || \
        ! ./config.status --config | grep -- '--enable-asan'
then
    ./autogen.sh --enable-asan "${args[@]}" "$@"
    if command -v git && git rev-parse --git-dir; then
        # shellcheck disable=SC2046
        touch $(git ls-files '*.c*' '*.h*')   # Force rebuilding
    else
        make -j clean
    fi
fi

# Check it
make -j4 check
