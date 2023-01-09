# t/Makefile.am: Makefile.am for tests
# Part of smallcxx
# Copyright (c) 2021 Christopher White <cxwembedded@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

include $(top_srcdir)/rules.mk

# Don't put any log files of your own in this directory!
CLEANFILES = *.log

EXTRA_DIST =

# === What the tests are ==================================================

# Ones that always fail
XFAIL_TESTS = \
	cover-no-tests-run-t \
	cover-no-tests-run-by-testcase-t \
	cover-test-failures-t \
	no-assertions-t.sh \
	$(EOL)

# Test scripts that don't need to be compiled (TESTS but not check_PROGRAMS)
testscripts = \
	cover-colorlog-t.sh \
	logging-t.sh \
	no-assertions-t.sh \
	silent-t.sh \
	$(EOL)

# Test programs to compile (both TESTS and check_PROGRAMS)
testprograms = \
	cover-misc-t \
	cover-no-tests-run-t \
	cover-no-tests-run-by-testcase-t \
	cover-test-failures-t \
	meta-t \
	string-t \
	$(EOL)

# Programs to compile in support of tests (check_PROGRAMS but not TESTS)
alsocompile = \
	log-debug-message-s \
	log-explicit-domain-s \
	silent-s \
	testfile-s \
	varying-log-s \
	$(EOL)

# Data files needed by the tests.
# If you list directories here, *do not* end them with a trailing slash.
# E.g., disting `foo/` will give you `foo/foo/` in the tarball.
testdatafiles = \
	globstari-basic-disk \
	globstari-basic-disk-ignores \
	$(EOL)

# --- Tests for optional modules ---

if BUILD_GLOBSTARI
testprograms += \
	globstari-basic-t \
	globstari-globset-t \
	globstari-matcher-t \
	globstari-userdata-t \
	$(EOL)
endif
###########################################################################
###########################################################################

# You shouldn't need to change anything below this line.

# === Valgrind support ====================================================

@VALGRIND_CHECK_RULES@
#VALGRIND_SUPPRESSIONS_FILES = smallcxx.supp
#EXTRA_DIST += smallcxx.supp

# === Running the tests ===================================================

TESTS_ENVIRONMENT = VALGRIND='$(VALGRIND)'

phony += build-tests
build-tests: $(testscripts) $(testprograms) $(alsocompile) $(testdatafiles)

TESTS = \
	$(testscripts) \
	$(testprograms) \
	$(EOL)

# === Data files ==========================================================

dist_noinst_DATA = $(testdatafiles)

# === Programs ============================================================

# So we don't have to list the sources of every test program individually
AM_DEFAULT_SOURCE_EXT = .cpp

LOCAL_CFLAGS = \
	$(NO_VAR_TRACK_CFLAGS) \
	-I$(top_srcdir)/include \
	-DSRCDIR="\"$(abs_srcdir)\"" \
	$(PCRE2_CFLAGS) \
	$(EOL)

LDADD = $(top_builddir)/src/libsmallcxx.a $(PCRE2_LIBS) $(LIBRT_LIBS)

check_PROGRAMS = \
	$(testprograms) \
	$(alsocompile) \
	$(EOL)

# Make sure common.sh is regenerated before the tests run
noinst_SCRIPTS = common.sh

dist_noinst_SCRIPTS = \
	$(testscripts) \
	$(EOL)
