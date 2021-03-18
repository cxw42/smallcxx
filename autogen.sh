#!/bin/bash
# Copyright (c) 2021 Christopher White <cxwembedded@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

set -xEeuo pipefail

mkdir -p m4
# || in case of "too many loops" errors
aclocal -I m4 --install || aclocal -I m4 --install
autoreconf -f -i -I m4 || autoreconf -f -i -I m4

./configure "$@"
