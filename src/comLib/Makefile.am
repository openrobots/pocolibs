
lib_LTLIBRARIES = libcomLib.la

libcomLib_la_LDFLAGS = -version-info 10:0:1 -no-undefined
libcomLib_la_LIBADD = -L../portLib -lportLib

AM_CPPFLAGS = -I$(top_srcdir)/include \
	-DPOSTER_SERV_PATH=\"$(bindir)/posterServ\"

libcomLib_la_SOURCES = \
        commonStructLib.c       	\
        csLib.c                 	\
        gcomLib.c               	\
        h2devLib.c              	\
        os/$(OSAPI)/h2devOsLib.c        \
        h2endianness.c          	\
        h2evnLib.c              	\
        h2initGlob.c            	\
        os/$(OSAPI)/h2logLib.c          \
        h2mathLib.c             	\
        h2rngLib.c              	\
        os/$(OSAPI)/h2semLib.c          \
        h2timeLib.c             	\
        h2timerLib.c            	\
        os/$(OSAPI)/h2timerTest.c       \
        mboxLib.c               	\
        os/$(OSAPI)/mboxEssay.c         \
        smMemLib.c              	\
        os/$(OSAPI)/smMemOsLib.c        \
        smObjLib.c

EXTRA_DIST = \
	h2mathTab.c
