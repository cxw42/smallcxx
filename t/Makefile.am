# t/Makefile.am: Makefile.am for tests
# NOTE: all tests assume they are run from t/ .
include $(top_srcdir)/rules.mk

# Don't put any log files of your own in this directory!
CLEANFILES = *.log

EXTRA_DIST =

# === What the tests are ==================================================

# Ones that always fail
XFAIL_TESTS = \
	cover-no-tests-run-t \
	cover-test-failures-t \
	$(EOL)

# Test scripts that don't need to be compiled (TESTS but not check_PROGRAMS)
testscripts = \
	cover-colorlog-t.sh \
	$(EOL)

# Test programs to compile (both TESTS and check_PROGRAMS)
testprograms = \
	cover-misc-t \
	cover-no-tests-run-t \
	cover-test-failures-t \
	meta-t \
	string-t \
	$(EOL)

# Programs to compile in support of tests (check_PROGRAMS but not TESTS)
alsocompile = \
	log-debug-message-s \
	$(EOL)

# Data files needed by the tests
testdatafiles =

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

LOCAL_CFLAGS = -I$(top_srcdir)/include -DSRCDIR="\"$(abs_srcdir)\"" $(PCRE2_CFLAGS)
LDADD = $(top_builddir)/src/libsmallcxx.a $(PCRE2_LIBS) -lrt

check_PROGRAMS = \
	$(testprograms) \
	$(alsocompile) \
	$(EOL)

# Make sure common.sh is regenerated before the tests run
noinst_SCRIPTS = common.sh

dist_noinst_SCRIPTS = \
	$(testscripts) \
	$(EOL)
