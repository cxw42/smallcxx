# Copyright (c) 2021 Christopher White <cxwembedded@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause
include $(top_srcdir)/rules.mk

# === Basics ==============================================================

SUBDIRS = include src t doc

dist_noinst_SCRIPTS = \
	asan.sh \
	autogen.sh \
	coverage.sh \
	$(EOL)

EXTRA_DIST = README.md .astylerc

# === Docs ================================================================

# Docs.  `doc` generates them; `doccheck` does so, suppressing all output
# except for error and warning messages.

phony += doc doccheck
doc doccheck:
	+$(MAKE) -C doc $@

# === Tests ===============================================================

phony += test
test: check

# Build the tests but don't run them
phony += build-tests
build-tests: all
	+$(MAKE) -C t build-tests
	@echo "Config: CC=$(CC), CXX=$(CXX), coverage=$(CODE_COVERAGE_ENABLED), asan=$(ASAN_ENABLED)"

# === Working with the code ===============================================

phony += tags
tags:
	-ctags -R

phony += prettyprint
prettyprint:
	-astyle --project=".astylerc" `git ls-files '*.c*' '*.h*'`

phony += p
p:
	+$(MAKE) prettyprint
	+$(MAKE) tags

phony += shellcheck
shellcheck:
	if which shellcheck ; then echo "Shellcheck" ; shellcheck *.sh && cd t && shellcheck -x *.sh ; fi

# Run all the checks
phony += prep
prep: prettyprint shellcheck
	@: # Run `make all` a first time to try to regenerate the .deps files
	+-$(MAKE) all
	+$(MAKE) doccheck
	+$(MAKE) html
	+$(MAKE) all build-tests
	+$(MAKE) check
	+$(MAKE) distcheck
	+$(MAKE) tags

# === Misc. ===============================================================

# Used by coverage.sh --- remove the existing code-coverage data.
phony += remove-code-coverage-data
remove-code-coverage-data:
	-rm -rf "$(CODE_COVERAGE_OUTPUT_FILE)" "$(CODE_COVERAGE_OUTPUT_FILE).tmp" "$(CODE_COVERAGE_OUTPUT_DIRECTORY)"
	-find . -name "*.gcda" -delete
