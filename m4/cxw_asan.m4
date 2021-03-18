# cxw_asan.m4
#
# SYNOPSIS
#
#   CXW_ASAN
#
# DESCRIPTION
#
#   Defines ASAN_CXXFLAGS, ASAN_CFLAGS, and ASAN_LDFLAGS.  Their values
#   are contingent on new option --enable-asan.  Also defines automake
#   conditional and subst-ed var ASAN_ENABLED.
#
# LICENSE
#
#   Copyright (c) 2021 Christopher White.  All rights reserved.
#

#serial 1

AC_DEFUN([CXW_ASAN],[

    AC_MSG_CHECKING([whether to enable asan])
    AC_ARG_ENABLE([asan],
        [AS_HELP_STRING([--enable-asan], [Whether to enable Address Sanitizer])],
        [], [enable_asan=no])
    AC_MSG_RESULT([$enable_asan])

    AM_CONDITIONAL([ASAN_ENABLED], [test "x$enable_asan" = 'xyes'])
    AC_SUBST([ASAN_ENABLED], [$enable_asan])

    dnl Thanks for the following flags to Hanno Boeck at The Fuzzing Project,
    dnl https://fuzzing-project.org/tutorial-cflags.html
    AM_COND_IF([ASAN_ENABLED],
        [
            ASAN_CFLAGS="-fsanitize=address,undefined -Wformat -Werror=format-security -Werror=array-bounds -g"
            ASAN_CXXFLAGS="-fsanitize=address,undefined -Wformat -Werror=format-security -Werror=array-bounds -g"
            ASAN_LDFLAGS="-fsanitize=address,undefined"
        ],
        [
            ASAN_CFLAGS=''
            ASAN_CXXFLAGS=''
            ASAN_LDFLAGS=''
        ]
    )

    AC_SUBST([ASAN_CFLAGS])
    AC_SUBST([ASAN_CXXFLAGS])
    AC_SUBST([ASAN_LDFLAGS])

])
# vi: set ft=config: #
