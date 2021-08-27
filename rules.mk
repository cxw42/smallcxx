# rules.mk: common rules for all Makefile.am's
# Part of smallcxx
# Copyright (c) 2021 Christopher White <cxwembedded@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

# common rules

# for convenience at the ends of lists
EOL =

# An easy place to hang phony targets
phony =
.PHONY: $(phony)

# === Flags ===============================================================

# Added to by subdir Makefiles
extra_ldflags =
extra_libs =

# C settings, which are the same throughout.  LOCAL_CFLAGS is filled in
# by each Makefile.am.
AM_CFLAGS = -Wall -Werror $(LOCAL_CFLAGS) $(CODE_COVERAGE_CFLAGS) $(ASAN_CFLAGS)
AM_CXXFLAGS = -Wall -Werror $(LOCAL_CFLAGS) $(CODE_COVERAGE_CXXFLAGS) $(ASAN_CXXFLAGS)
AM_CPPFLAGS = $(CODE_COVERAGE_CPPFLAGS)
AM_LDFLAGS = $(extra_ldflags) $(ASAN_LDFLAGS)
LIBS = $(extra_libs) $(LOCAL_LIBS) $(CODE_COVERAGE_LIBS)

# === Coverage ============================================================

# Per https://www.gnu.org/software/autoconf-archive/ax_code_coverage.html
clean-local: code-coverage-clean
distclean-local: code-coverage-dist-clean

MOSTLYCLEANFILES = *.gcda *.gcno *.trs

CODE_COVERAGE_OUTPUT_FILE = $(PACKAGE_TARNAME)-coverage.info
CODE_COVERAGE_OUTPUT_DIRECTORY = $(PACKAGE_TARNAME)-coverage

include $(top_srcdir)/aminclude_static.am
