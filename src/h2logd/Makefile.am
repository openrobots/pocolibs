bin_PROGRAMS = h2logd

h2logd_SOURCES = h2logd.c

AM_CPPFLAGS = -I$(top_srcdir)/include

h2logd_LDADD = -L../comLib -L../posterLib -L../portLib \
	-lcomLib -lposterLib -lportLib
