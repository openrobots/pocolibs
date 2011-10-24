
lib_LTLIBRARIES = liberrorLib.la

liberrorLib_la_LDFLAGS = -version-number 4:0:1 -no-undefined

AM_CPPFLAGS = -I$(top_srcdir)/include \
	-DPOSTER_SERV_PATH=\"$(BINDIR)/posterServ\"

liberrorLib_la_SOURCES = \
	h2errorLib.c
