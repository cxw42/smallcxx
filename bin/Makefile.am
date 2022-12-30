# bin/Makefile.am - part of smallcxx
# Copyright (c) 2021 Christopher White <cxwembedded@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause
include $(top_srcdir)/rules.mk

noinst_PROGRAMS = smallcxxlog

smallcxxlog_SOURCES = smallcxxlog.cpp

LOCAL_CFLAGS = -I$(top_srcdir)/include -DSRCDIR="\"$(abs_srcdir)\""
LDADD = $(top_builddir)/src/libsmallcxx.a
