#!/bin/bash
# Copyright (c) 2021 Christopher White <cxwembedded@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

set -x
set -eEuo pipefail

# Reconfigure if necessary so that coverage is enabled
if [[ ! -x ./config.status ]] || \
        ! ./config.status --config | grep -- '--enable-code-coverage'
then
    ./autogen.sh --enable-code-coverage CFLAGS='-g -O0' CXXFLAGS='-g -O0' "$@"
    if command -v git && git rev-parse --git-dir; then
        # shellcheck disable=SC2046
        touch $(git ls-files '*.c*' '*.h*')   # Force rebuilding
    else
        make -j clean
    fi
fi

# Clean up from old runs, e.g., test runs compiled with coverage turned on.
make -j4 remove-code-coverage-data

# Check it
make -j4 check-code-coverage
