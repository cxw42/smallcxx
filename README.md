# smallcxx - Small C++ libraries

These use C++11.

All code Copyright (c) 2021 Christopher White, unless otherwise indicated.

## Overview

This is a C++11 project using autotools as the build system.

The modules are:

### Always built

- `logging`: multi-level logging library
- `string`: some string functions not in `string.h` and friends
- `test`: basic testcase-management and test-assertion library

### Optional

- `globstari`: file-globbing and ignore-files routines
  - Depends on PCRE2

## Using

### Compiling with gcc

    ./configure && make -j  # normal
    ./coverage.sh           # for code coverage
    ./asan.sh               # for Address Sanitizer

### Compiling with clang

    ./configure CC=clang CXX=clang++ && make -j
    ./asan.sh clang                             # for Address Sanitizer

Note that `CXX` does have to be `clang++`, not just `clang`.

## Third-party software used by smallcxx

- [editorconfig-core-c](https://github.com/editorconfig/editorconfig-core-c)
  by the EditorConfig team.  Licensed under various BSD licenses.
  Included in the `globstari` module

## Developing

### Development dependencies

- Pretty-printing: Artistic Style, Ubuntu package `astyle`.
- Docs: Doxygen and dot(1) (Ubuntu packages `doxygen` and `graphviz`)

### Starting fresh

    ./autogen.sh && make -j maintainer-clean && ./autogen.sh

That will leave you ready to run `make`

The first time you compile, you may get errors about missing `.deps/*` files.
If so, re-run `make`.  If that doesn't work, run `make -jk` once to generate
the deps files, and then you should be back in business.

### Running the tests

`make -j check`, or `./asan.sh` for Address Sanitizer testing.

### Structure of the codebase

- `src/`: implementation files
- `include/smallcxx/`: header files
- `t/`: tests.  `*-t.sh` and `*-t.cpp` are test sources.  `*-s.cpp` are
  supporting programs.
- `doc/`: documentation.  Doxygen output is in `doc/html/index.html`.

### Coding style

- 4-space tabs, cuddled `else`.  Run `make prettyprint` to format your code.
- All Doxygen tags start with `@` (not backslash).
