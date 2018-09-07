
lib_LTLIBRARIES = libposterLib.la

libposterLib_la_LDFLAGS = -version-info 7:0:1 -no-undefined
libposterLib_la_LIBADD = -L../comLib -L../portLib -lcomLib -lportLib

AM_CPPFLAGS = -I$(top_srcdir)/include \
	-DPOSTER_SERV_PATH=\"$(bindir)/posterServ\"

libposterLib_la_SOURCES = \
	posterLib.c \
	localPosterLib.c \
	posterLibPriv.h

# Multi-thread rpcgen for remotePosterLib and posterServ
if RPCGEN_M

libposterLib_la_LIBADD += libposters_xdr.la
libposterLib_la_SOURCES += \
	os/@OSAPI@/hostclient.c \
	os/@OSAPI@/remotePosterLib.c \
	posters_clnt.c \
	os/@OSAPI@/remotePosterLibPriv.h \
	posters.x

bin_PROGRAMS = posterServ

posterServ_LDADD = libposters_xdr.la localPosterLib.lo \
	-L../comLib -L../portLib -lcomLib -lportLib

posterServ_SOURCES = \
	posterServ.c \
	remPosterId.c \
	posters_svc.c \
	remPosterId.h \
	tree.h

noinst_LTLIBRARIES = libposters_xdr.la
libposters_xdr_la_SOURCES = posters_xdr.c posters.h
# rpcgen generated code has some unused variables
libposters_xdr_la_CFLAGS = -Wno-unused-variable

BUILT_SOURCES = \
	posters.h \
	posters_xdr.c \
	posters_clnt.c \
	posters_svc.c

posters_clnt.c: posters.x Makefile
	rm -f $@
	(cd $(dir $<) ; $(RPCGEN) $(RPCGEN_C) -M -l posters.x) > $@

posters_svc.c: posters.x Makefile
	rm -f $@
	(cd $(dir $<) ; $(RPCGEN)  $(RPCGEN_C) -M -m posters.x) > $@

posters_xdr.c: posters.x Makefile
	rm -f $@
	(cd $(dir $<) ; $(RPCGEN) $(RPCGEN_C) -M -c posters.x) > $@

posters.h: posters.x Makefile
	rm -f $@
	(cd $(dir $<) ; $(RPCGEN) $(RPCGEN_C) -M -h posters.x) > $@

CLEANFILES = $(BUILT_SOURCES)

else

libposterLib_la_CFLAGS = -DPOSTERLIB_ONLY_LOCAL
endif
