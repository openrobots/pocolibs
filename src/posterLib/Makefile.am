
lib_LTLIBRARIES = libposterLib.la

libposterLib_la_LDFLAGS = -version-info 7:0:1 -no-undefined
libposterLib_la_LIBADD = -L../comLib -L../portLib -lcomLib -lportLib

AM_CPPFLAGS = -I$(top_srcdir)/include \
	-DPOSTER_SERV_PATH=\"$(BINDIR)/posterServ\"

libposterLib_la_SOURCES = \
	os/$(OSAPI)/hostclient.c \
	posterLib.c \
	localPosterLib.c \
	os/$(OSAPI)/remotePosterLib.c \
	posters_clnt.c \
	posters_xdr.c \
	posterLibPriv.h \
	os/$(OSAPI)/remotePosterLibPriv.h \
	posters.x

bin_PROGRAMS = posterServ

posterServ_LDFLAGS =
posterServ_LDADD =  posters_xdr.lo localPosterLib.lo \
	-L../comLib -L../portLib -lcomLib -lportLib

posterServ_SOURCES = \
	posterServ.c \
	remPosterId.c \
	posters_svc.c \
	remPosterId.h \
	tree.h

BUILT_SOURCES = \
	posters.h \
	posters_xdr.c \
	posters_clnt.c \
	posters_svc.c

posters_clnt.c: posters.x
	rm -f $@
	(cd $(dir $<) ; $(RPCGEN) $(RPCGEN_C) -l posters.x) > $@

posters_svc.c: posters.x
	rm -f $@
	(cd $(dir $<) ; $(RPCGEN)  $(RPCGEN_C) -m posters.x) > $@

posters_xdr.c: posters.x
	rm -f $@
	(cd $(dir $<) ; $(RPCGEN) $(RPCGEN_C) -c posters.x) > $@

posters.h: posters.x
	rm -f $@
	(cd $(dir $<) ; $(RPCGEN) $(RPCGEN_C) -h posters.x) > $@

CLEANFILES = $(BUILT_SOURCES)

