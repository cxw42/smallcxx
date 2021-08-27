dnl configure.ac for smallcxx
dnl Copyright (c) 2021 Christopher White <cxwembedded@gmail.com>
dnl SPDX-License-Identifier: BSD-3-Clause

dnl === Basic setup =======================================================
AC_INIT([smallcxx], [0.0.1], [], [smallcxx], [https://github.com/cxw42/smallcxx])
AC_PREREQ([2.65])
AC_COPYRIGHT([Copyright (C) 2021 Christopher White])
AC_CONFIG_SRCDIR([rules.mk])    dnl make sure the srcdir is correctly specified
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 foreign subdir-objects])

AC_PROG_CC
AC_PROG_CXX

dnl noext = no GNU extensions; mandatory = die if C++11 not available.
AX_CXX_COMPILE_STDCXX_11([noext], [mandatory])
AC_LANG([C++])

AC_PROG_RANLIB

AX_PTHREAD
LIBS="$PTHREAD_LIBS $LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
CC="$PTHREAD_CC"

dnl Omit this for now and see what happens => CXX="$PTHREAD_CC"
CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"

dnl === Analysis tools ====================================================

AX_AM_MACROS_STATIC
AX_CODE_COVERAGE

CXW_ASAN
AX_VALGRIND_CHECK

dnl === Optional dependencies =============================================
dnl If you don't have these, you simply won't be able to build the relevant
dnl modules.

dnl PCRE2: required for globstari
PKG_CHECK_MODULES([PCRE2], [libpcre2-8],
    [have_pcre2=1],
    [have_pcre2=0]
)
AC_DEFINE_UNQUOTED([HAVE_PCRE2],[$have_pcre2],[Whether we have PCRE2])
AC_SUBST([HAVE_PCRE2])
AM_CONDITIONAL([HAVE_PCRE2], [test "$have_pcre2" = '1'])

dnl === Docs ==============================================================

AC_PATH_PROG([DOXYGEN], [doxygen], [no])
AM_CONDITIONAL([HAVE_DOXYGEN], [test "x$DOXYGEN" '!=' "xno"])
AM_COND_IF([HAVE_DOXYGEN],
    [ AC_SUBST([DOXYGEN_DIST], [html]) ],
    [ AC_SUBST([DOXYGEN_DIST], [ ]) ]
)

dnl === Dependencies for extra tests, and for dist ========================

AC_PATH_PROG([UNBUFFER], [unbuffer], [no])  dnl For t/cover-colorlog-t.sh

AC_PATH_PROG([GIT], [git], [no])
AM_CONDITIONAL([HAVE_GIT], [test "x$GIT" '!=' "xno"])

AC_MSG_CHECKING([if we are in a git repo])
git_working_dir=no
AM_COND_IF([HAVE_GIT],
    [
        AS_IF([git -C "$top_srcdir" rev-parse --show-toplevel >/dev/null 2>&1],
              [git_working_dir=`git -C "$top_srcdir" rev-parse --show-toplevel`])
    ]
)
AC_MSG_RESULT([$git_working_dir])

dnl === Output ============================================================

AC_CONFIG_FILES([
    Makefile
    doc/Doxyfile
    doc/Makefile
    include/Makefile
    src/Makefile
    t/Makefile
    t/common.sh
])

dnl TODO
dnl dnl Extra-dist all of `git ls-files`.  Only used from the top-level Makefile,
dnl dnl so we don't need a path.
dnl
dnl dnl Tell Automake where it can find the file after configure runs.  We need
dnl dnl this variable so Automake doesn't try to include it before it's created.
dnl AC_SUBST([dist_all_git_files], [dist-all-git-files.mk])
dnl
dnl AC_CONFIG_FILES([dist-all-git-files.mk],
dnl     [
dnl         AS_IF([test "x$git_working_dir" '!=' "xno"],
dnl         [
dnl             (cd "$git_working_dir" &&
dnl              git ls-files | sed 's/^/\t/; s/$/ \\/' > "$tmp/smallcxx-git-files" ) &&
dnl             cat "$tmp/smallcxx-git-files" >> dist-all-git-files.mk
dnl         ])
dnl         printf '\t\x24(EOL)\n' >> dist-all-git-files.mk
dnl     ],
dnl     [
dnl         git_working_dir="$git_working_dir"
dnl     ]
dnl )
dnl

AM_SILENT_RULES([yes])
AC_OUTPUT
