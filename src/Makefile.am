include $(top_srcdir)/rules.mk

lib_LIBRARIES = libsmallcxx.a

libsmallcxx_a_SOURCES = \
	logging.cpp \
	string.cpp \
	test.cpp \
	$(EOL)

libsmallcxx_a_CXXFLAGS = -I$(top_srcdir)/include $(AM_CXXFLAGS)
