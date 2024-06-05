# regression tests

TESTS=\
	portLib/semaphore	\
	portLib/wd		\
	portLib/wd2		\
	portLib/taskDelete	\
	portLib/taskDeleteHook	\
	portLib/taskDelay	\
	portLib/taskSpawn2	\
	portLib/taskSuspend	\
	portLib/tick		\
				\
	comLib/commonStruct	\
	comLib/commonStructScheduler \
	comLib/gcomAlloc	\
	comLib/h2dev		\
	comLib/h2devMax		\
	comLib/h2sem		\
	comLib/h2semAlloc	\
	comLib/mbox		\
	comLib/mboxRecycle	\
	comLib/h2timer		\
	comLib/h2timersem	\
	comLib/h2timefromts	\
	comLib/h2rngRealloc	\
	comLib/smMem		\
	comLib/csLibMax		\
				\
	posterLib/fresh		\
	posterLib/poster	\
	posterLib/resize

# build test programs
check_PROGRAMS=${TESTS}
AM_CPPFLAGS=	-I$(top_srcdir)/include
LDADD=		libosapi.la
LDADD+=		../comLib/libcomLib.la
LDADD+=		../posterLib/libposterLib.la
LDADD+=		../portLib/libportLib.la

# common source for all test programs.
check_LTLIBRARIES=	libosapi.la
libosapi_la_SOURCES=	posix.c

# export useful variables to the test scripts
export H2DEV_DIR=.
