# Copyright (c) 2021 Christopher White <cxwembedded@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause
include $(top_srcdir)/rules.mk

# --- Base modules ---

nobase_include_HEADERS = \
	smallcxx/common.hpp \
	smallcxx/logging.hpp \
	smallcxx/string.hpp \
	smallcxx/test.hpp \
	$(EOL)

# --- Optional modules ---

# globstari
if HAVE_PCRE2
nobase_include_HEADERS += smallcxx/globstari.hpp
endif
