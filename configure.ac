dnl autoconf script for portLib/comLib (aka pocolibs)
dnl
dnl Copyright (c) 2004 
dnl      Autonomous Systems Lab, Swiss Federal Institute of Technology.
dnl Copyright (c) 2003-2004,2010,2011 CNRS/LAAS
dnl
dnl GPL, since some parts were copied from other configure.in
dnl 
dnl
AC_INIT([pocolibs],[2.9],[openrobots@laas.fr])
AC_PREREQ(2.59)

AC_CANONICAL_HOST

AC_PROG_CC
if test "x${GCC}" = "xyes"; then
  CFLAGS="${CFLAGS} -Wall"
fi

dnl --- OS flavour checks -----------------------------------------------

OSAPI=posix
AC_SUBST(OSAPI)

AC_ARG_WITH(xenomai,
	AC_HELP_STRING([--with-xenomai=yes|no], [configure to use Xenomai]),
	[opt_xenomai=$withval],[opt_xenomai=no])
AC_ARG_VAR([XENO_CONFIG], [Path to the xeno-config command for xenomai])

if test "x$opt_xenomai" = "xyes"; then
	AC_PATH_PROG([XENO_CONFIG], [xeno-config], [no],
		[/usr/xenomai/bin:${PATH}])
	if test "x$XENO_CONFIG" = "xno" ; then
		AC_MSG_FAILURE([--with-xenomai=yes specified, but xeno-config not found])
		opt_xenomai=no
	else
		AC_MSG_RESULT([Configuring to use Xenomai])
		XENO_CFLAGS=`${XENO_CONFIG} --skin posix --cflags`
		XENO_LDFLAGS=`${XENO_CONFIG} --skin posix --ldflags`
	fi
fi

CFLAGS="${CFLAGS} ${XENO_CFLAGS}"
LDFLAGS="${LDFLAGS} ${XENO_LDFLAGS}"

# there's no simple way to check this
case $host_os in
	darwin*) CFLAGS="${CFLAGS} -DUSE_SEM_OPEN" ;;
esac

dnl --- end of OS flavour checks ----------------------------------------

AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL

dnl add -ldl if needed 
if test -n "$lt_cv_dlopen_libs" ; then
	LIBS=$lt_cv_dlopen_libs
fi

AC_PROG_INSTALL

dnl check for mkdep
AC_ARG_VAR(MKDEP, [full path of the LAAS' dependency generator])
AC_PATH_PROG(MKDEP, mkdep, no, $prefix/bin:$exec_prefix/bin:$PATH)
if test "x$MKDEP" = "xno"; then
    AC_MSG_ERROR( [Cannot find the mkdep executable, needed to build pocolibs] )
fi

dnl check for rpcgen
AC_ARG_VAR(RPCGEN, [Remote Procedure Call (RPC) protocol compiler])
AC_PATH_PROG(RPCGEN, rpcgen, no, [$PATH])
AC_RPCGEN_C

dnl check for purify
opt_purify=no
AC_ARG_WITH(purify, [  --with-purify           configure to postprocess with purify], opt_purify=$withval)
if test "x$opt_purify" = xyes; then
  AC_MSG_RESULT([enabling purify])
  PURIFY="purify"
else
  PURIFY=
fi
AC_SUBST(PURIFY)

dnl processor tests 
AC_C_BIGENDIAN

dnl
AC_TYPE_SIGNAL

AC_CHECK_HEADERS([getopt.h])

dnl --- check for pthread -----------------------------------------------
if test "x$opt_xenomai" != "xyes" ; then
# AC_CHECK_PTHREADS(action if ok, action if not ok, action if cross-compiling)
AC_DEFUN([AC_CHECK_PTHREADS],
	AC_LANG_SAVE
	AC_LANG_C
	AC_TRY_RUN([
#include <pthread.h>
void *pipo(void *a) { exit(0); }
 
int main() {
    pthread_t t;
    pthread_create(&t, NULL, pipo, NULL);
    pthread_join(t, NULL);
    exit(2);
}
	],[$1],[$2],:)
	AC_LANG_RESTORE
)

AC_CACHE_CHECK(
	[if compiler recognizes -pthread],
	poco_cv_gcc_pthread,
	ac_save_CFLAGS=$CFLAGS
	CFLAGS="$CFLAGS -pthread"
	AC_CHECK_PTHREADS(
		poco_cv_gcc_pthread=yes, 
		poco_cv_gcc_pthread=no
	)
	CFLAGS=$ac_save_CFLAGS
)
if test "$poco_cv_gcc_pthread" = "yes"; then
	poco_threads=""
	CFLAGS="$CFLAGS -pthread"
	LDFLAGS="$LDFLAGS -pthread";
else
	AC_CACHE_CHECK(
		[if compiler recognizes -mt],
		poco_cv_cc_mt,
		ac_save_CFLAGS=$CFLAGS
		CFLAGS="$CFLAGS -mt"
		AC_CHECK_PTHREADS(
			poco_cv_cc_mt=yes,
			poco_cv_cc_mt=no
		)
		CFLAGS=$ac_save_CFLAGS
	)
	if test "$poco_cv_cc_mt" = "yes"; then
		CFLAGS="$CFLAGS -mt";
		LDFLAGS="$LDFLAGS -mt";
	else
		AC_CHECK_LIB(pthread,pthread_create,poco_threads=-lpthread,
        	     [AC_CHECK_LIB(c_r,pthread_create,poco_threads=-lc_r)])
		ac_save_LIBS="$LIBS"
		if test -n "$poco_threads" ; then
			LIBS="$poco_threads $LIBS"
		fi
		AC_CHECK_PTHREADS([
			:
		],[
			AC_MSG_ERROR([No working thread support found (fatal)])
			exit 2
		])
	fi
fi

# Linux requires -D_XOPEN_SOURCE=500 to build with libpthread and -Wall
AC_MSG_CHECKING([whether you need -D_XOPEN_SOURCE to use libpthread])
xopen=no
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
		#include <sys/types.h>
		#include <pthread.h>
	],[
		pthread_rwlock_t lock;
	])], [xopen=no], [CPPFLAGS="$CPPFLAGS -D_XOPEN_SOURCE=500"; xopen=yes])
AC_MSG_RESULT([$xopen])

fi 
dnl --- end of checks for pthread ---------------------------------------


AC_SEARCH_LIBS([fabs], [m])
AC_SEARCH_LIBS([atan], [m])

AC_SEARCH_LIBS([timer_create], [rt])

if test "x$opt_xenomai" != "xyes" ; then
case `uname -r` in
	*-adeos)
		enable_posix_timers=yes
		AC_MSG_WARN([Running adeos, enabling posix timers])
		;;
esac
fi

AC_ARG_ENABLE([posix-timers], 
	AC_HELP_STRING([--disable-posix-timers Force use of setitimer(2)],
			[instead of timer_create(3p)]),
	[ if test "$enable_posix_timers" = "yes"; then
		AC_MSG_NOTICE([forcing use of POSIX timers])
		AC_DEFINE([HAVE_POSIX_TIMERS], 1, [timer_*() functions])
	else
		AC_MSG_NOTICE([disabling use of POSIX timers])
	fi ],
	AC_POSIX_TIMERS_WORKS
)

AC_SEARCH_LIBS(clock_gettime, [rt posix4])
AC_CHECK_FUNCS(clock_gettime)

AC_SEARCH_LIBS(sched_get_priority_min, [rt])

AC_MSG_CHECKING([for pthread_attr_setschedpolicy])
AC_TRY_LINK([#include <pthread.h>], 
	[pthread_attr_setschedpolicy(NULL, SCHED_RR);],
	[AC_MSG_RESULT([yes])]
	[AC_DEFINE(HAVE_SETSCHEDPOLICY, 1, 
		[posix threads scheduling policies])],
	[AC_MSG_RESULT([no])]
)

AC_CHECK_FUNCS(gethostbyname, [], [
	  AC_CHECK_LIB(nsl, gethostbyname)
	])
AC_CHECK_LIB(socket, socket)

AC_MSG_CHECKING([for vfscanf])
AC_TRY_LINK([#include <stdio.h>],
	[vfscanf(NULL, NULL, NULL);],
	[AC_MSG_RESULT([yes])]
	[AC_DEFINE([HAVE_VFSCANF], 1, [vfscanf()])],
	[AC_MSG_RESULT([no])]
)

AC_CHECK_FUNCS([fnmatch],[], [
	COMLIB_COMPATSRC="${COMLIB_COMPATSRC} fnmatch.c"
])
AC_SUBST(COMLIB_COMPATSRC)

AC_CHECK_HEADERS([sys/tree.h])

dnl --- Debugging options -----------------------------------------------

AC_ARG_ENABLE(valgrind,
            AC_HELP_STRING([--enable-valgrind], [compile with valgrind support]),
            [valgrind=$enableval], [valgrind=no])
if test "x$valgrind" = "xyes"; then
    PKG_CHECK_MODULES(VALGRIND, valgrind >= 1.1, 
        [ AC_DEFINE([VALGRIND_SUPPORT], 1, 
		[Support for running under valgrind])], 
	[ AC_MSG_ERROR(Valgrind not present and Valgrind support requested) ])
fi

AC_ARG_ENABLE(debug,
           [AC_HELP_STRING([--enable-debug], [compile with DEBUG defined])],
                [debug=$enableval],[debug=no])
if test "x$debug" = "xyes"; then
    	CFLAGS="${CFLAGS} -DDEBUG"
    	AC_MSG_RESULT([enabling -DDEBUG])
elif test "x$opt_debug" = "xno"; then
    	CFLAGS="${CFLAGS} -DNDEBUG"
fi

AC_ARG_WITH(debug-output,
           AC_HELP_STRING([--with-debug-output=all task sys h2dev smmem h2evn mbox gcom h2timer],
           [print debugging information at run-time (comma separated list)]),
           [opt_debug_output=$withval],[opt_debug_output=])

if test "x$opt_debug" = "xall"; then
   opt_debug="task,sys,h2dev,smmem,h2evn,mbox,gcom,h2timer"
fi

ac_save_ifs=$IFS; IFS=","
for dopt in $opt_debug_output; do 
	test -z "$dopt" && break

	case "$dopt" in
	   task)	
		AC_DEFINE([PORTLIB_DEBUG_TASKLIB], 1,
			 [Enable debug in taskLib]) 
		;;
	   sys)		
		AC_DEFINE([PORTLIB_DEBUG_SYSLIB], 1,
			[Enable debug in sysLib])
		;;

	   h2dev)	
		AC_DEFINE([COMLIB_DEBUG_H2DEVLIB], 1,
			[Enable debug in h2devLib])
		;;
	   smmem)	
		AC_DEFINE([COMLIB_DEBUG_SMMEMLIB], 1,
			[Enable debug in smMemLib])
		;;
	   h2evn)	
		AC_DEFINE([COMLIB_DEBUG_H2EVNLIB], 1,
			[Enable debug in h2evnLib])
		;;
	   mbox)	
		AC_DEFINE([COMLIB_DEBUG_MBOXLIB], 1, 
			[Enable debug in mboxLib])
		;;
	   gcom)
		AC_DEFINE([COMLIB_DEBUG_GCOMLIB], 1,
			[Enable debug in gcomLib])
		;;
	   h2timer)
		AC_DEFINE([COMLIB_DEBUG_H2TIMERLIB], 1,
			[Enable debug in h2timerLib])
		;;
	   *)		
	    IFS=$ac_save_ifs
	    AC_MSG_ERROR([Invalid debug output mode "$dopt"]) ;;
	esac

	AC_MSG_RESULT([debugging output in $dopt])
done
IFS=$ac_save_ifs

dnl --- Check perl path -------------------------------------------------
dnl Check for perl
AC_ARG_VAR(PERL, full path of Perl interpreter)
AC_PATH_PROG(PERL, perl, no)
if test "x$PERL" = "xno"; then
    AC_MSG_ERROR(Cannot find the perl executable)
fi

dnl --- GNU tar (for make dist only) ------------------------------------
AC_ARG_VAR(TAR, [GNU tar command])
AC_CHECK_PROGS(TAR, [gtar gnutar gnu-tar tar], [no])
if test "x$TAR" = "xno"; then
	AC_MSG_ERROR([Cannot find a tar program])
fi

dnl --- Make makefiles --------------------------------------------------

AC_CONFIG_HEADER(src/pocolibs-config.h)
AC_OUTPUT(Init.make
	Makefile
	pocolibs.pc
	include/Makefile
	src/portLib/Makefile
	src/errorLib/Makefile
	src/comLib/Makefile
	src/posterLib/Makefile
	src/posterLib/posterServ/Makefile
	src/posterLib/test/basic/Makefile
	src/posterLib/test/endian/Makefile
	src/h2/Makefile
	src/shell/Makefile
	src/regress/Makefile
)

