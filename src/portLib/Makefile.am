
lib_LTLIBRARIES = libportLib.la

libportLib_la_LDFLAGS = -version-info 5:0:0 -no-undefined

AM_CPPFLAGS = -I$(top_srcdir)/include

libportLib_la_SOURCES = \
        osInit.c        	    \
	h2errorLib.c		    \
        os/@OSAPI@/logLib.c        \
        os/@OSAPI@/semLib.c        \
        os/@OSAPI@/shellLib.c      \
        os/@OSAPI@/symLib.c        \
        os/@OSAPI@/sysLib.c        \
        os/@OSAPI@/taskLib.c       \
        os/@OSAPI@/tickLib.c       \
        os/@OSAPI@/wdLib.c
