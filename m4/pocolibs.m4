dnl
dnl Copyright (c) 2004 
dnl      Autonomous Systems Lab, Swiss Federal Institute of Technology.
dnl Copyright (c) 2004,2010,2017,2024 CNRS/LAAS
dnl
dnl Permission to use, copy, modify, and distribute this software for any
dnl purpose with or without fee is hereby granted, provided that the above
dnl copyright notice and this permission notice appear in all copies.
dnl
dnl THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
dnl WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
dnl MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
dnl ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
dnl WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
dnl ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
dnl OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
dnl
dnl ----------------------------------------------------------------------
dnl
dnl tests if rpcgen supports -C 
dnl
AC_DEFUN([AC_RPCGEN_C],
  [AC_MSG_CHECKING(for rpcgen -C)
  AC_CACHE_VAL(poco_cv_prog_RPCGEN_C,
  [if $RPCGEN -C -c </dev/null >/dev/null 2>/dev/null
  then
    poco_cv_prog_RPCGEN_C=yes
  else
    poco_cv_prog_RPCGEN_C=no
  fi])dnl
  AC_MSG_RESULT($poco_cv_prog_RPCGEN_C)
  if test $poco_cv_prog_RPCGEN_C = yes; then
    RPCGEN_C=-C
  fi
  AC_SUBST(RPCGEN_C)
  test -n "$RPCGEN_C" && AC_DEFINE([HAVE_RPCGEN_C], 1, [define if rpcgen supports the -C option])
])dnl
dnl ----------------------------------------------------------------------
dnl
dnl tests if rpcgen supports -M
dnl
AC_DEFUN([AC_RPCGEN_M],
  [AC_MSG_CHECKING(for rpcgen -M)
  AC_CACHE_VAL(poco_cv_prog_RPCGEN_M,
  [if $RPCGEN -M -c </dev/null >/dev/null 2>/dev/null
  then
    poco_cv_prog_RPCGEN_M=yes
  else
    poco_cv_prog_RPCGEN_M=no
  fi])dnl
  AC_MSG_RESULT($poco_cv_prog_RPCGEN_M)
  AM_CONDITIONAL(RPCGEN_M, test "$poco_cv_prog_RPCGEN_M" = "yes")
])dnl
dnl ----------------------------------------------------------------------
dnl
dnl test for POSIX timers usability
dnl
AC_DEFUN([AC_POSIX_TIMERS_WORKS],
[AC_MSG_CHECKING([for usable posix timers])
AC_RUN_IFELSE([AC_LANG_SOURCE([[m4_include(m4/test-posix-timer.c)]])],
	[AC_MSG_RESULT([yes])]
	[AC_DEFINE(HAVE_POSIX_TIMERS)]
	[enable_posix_timers=yes]
,
	[AC_MSG_RESULT([no])]
	[enable_posix_timers=no]
,
	AC_MSG_RESULT([assuming yes for cross-compilation])
	AC_DEFINE(HAVE_POSIX_TIMERS)
	[enable_posix_timers=yes]
,
)])dnl

