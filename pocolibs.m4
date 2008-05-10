dnl $LAAS$
dnl
dnl Copyright (c) 2004 
dnl      Autonomous Systems Lab, Swiss Federal Institute of Technology.
dnl Copyright (c) 2004 CNRS/LAAS
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
dnl test for POSIX timers usability
dnl
AC_DEFUN([AC_POSIX_TIMERS_WORKS],
[AC_MSG_CHECKING([for usable posix timers])
AC_TRY_RUN([
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static volatile int done = 0;
static void timerInt(int sig) { done++; }

static long
time_difference(struct timeval *t1, struct timeval *t2)
{
	long usec_diff = t1->tv_usec - t2->tv_usec, retenue = 0;
	
	if (usec_diff < 0) {
		usec_diff = 1000000 + usec_diff;
		retenue = 1;
	}
	return (t1->tv_sec - t2->tv_sec - retenue)*1000000 + usec_diff;
}

int
main(int argc, char *argv[])
{
	timer_t t;
	struct itimerspec tv;
	sigset_t sigset;
	struct sigaction act;
	struct timeval tp1, tp2;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) == -1) {
		perror("sigprocmask");
		exit(2);
	}
	act.sa_handler = timerInt;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (sigaction(SIGALRM, &act, NULL) == -1) {
		perror("sigaction");
		exit(2);
	}
	if (timer_create(CLOCK_REALTIME, NULL, &t) == -1) {
		perror("timer_create");
		exit(2);
	}
	tv.it_interval.tv_nsec = 10000000;
	tv.it_interval.tv_sec = 0;
	tv.it_value.tv_nsec = 10000000;
	tv.it_value.tv_sec = 0;
	if (timer_settime(t, 0, &tv, NULL) == -1) {
		perror("timer_settime");
		exit(2);
	}
	gettimeofday(&tp1, NULL);
	while (done < 100)
		;
	gettimeofday(&tp2, NULL);
	if (time_difference(&tp2, &tp1) < 1200000) 
		exit(0);
	else {
		fprintf(stderr, "no able to generate 100 ticks/s\n");
		exit(2);
	}
}
],
	[AC_MSG_RESULT([yes])]
 	[AC_DEFINE(HAVE_POSIX_TIMERS)],
	[AC_MSG_RESULT([no])]
)])dnl

