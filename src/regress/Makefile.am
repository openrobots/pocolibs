# regression tests

TESTS=\
	portLib/semaphore	\
	portLib/wd		\
	portLib/wd2		\
	portLib/taskDelete	\
	portLib/taskDeleteHook	\
	portLib/taskDelay	\
	portLib/taskSpawn	\
	portLib/taskSpawn2	\
	portLib/taskSuspend	\
	portLib/tick		\
				\
	comLib/commonStruct	\
	comLib/h2dev		\
	comLib/h2sem		\
	comLib/mbox		\
	comLib/h2timer		\
	comLib/h2timersem	\
	comLib/h2timefromts	\
	comLib/h2timefromtv	\
				\
	posterLib/poster	\
	posterLib/resize

XFAIL_TESTS=	portLib/taskSuspend	# since this dumps core

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
