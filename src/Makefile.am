include $(top_srcdir)/rules.mk

lib_LIBRARIES = libsmallcxx.a

# --- Base modules ---

libsmallcxx_a_SOURCES = \
	logging.cpp \
	string.cpp \
	test.cpp \
	$(EOL)

libsmallcxx_a_CXXFLAGS = -I$(top_srcdir)/include $(AM_CXXFLAGS)

# --- Optional modules ---

if BUILD_GLOBSTARI
libsmallcxx_a_SOURCES += \
	globstari.cpp \
	globstari-matcher.cpp \
	globstari-traverse.cpp \
	$(EOL)

libsmallcxx_a_CXXFLAGS += $(PCRE2_CFLAGS)
extra_ldflags += $(PCRE2_LIBS)
endif
