/*
 * Copyright (c) 1999, 2003 CNRS/LAAS
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include "config.h"
__RCSID("$LAAS$");

#include <stdio.h>
#include <string.h>
#ifdef HAVE_POSIX_TIMERS
#include <time.h>
#else
#include <sys/time.h>
#endif
#include <errno.h>
#include <pthread.h>
#include <signal.h>

#include "portLib.h"
#include "errnoLib.h"

#define CLK_SIGNAL SIGALRM

#ifndef TEST 
# define ERRNO_SET(x) errnoSet(x)
#else
# define ERRNO_SET(x) /**/
#endif

/*----------------------------------------------------------------------*/

/**
 ** Emulation of VxWorks system Clock
 **
 ** Based on public VxWorks BSP Sources
 **/
static FUNCPTR sysClkRoutine = NULL;
static int sysClkArg = 0;
static int sysClkTicksPerSecond = 100;
static int sysClkConnected = FALSE;
static int sysClkRunning = FALSE;
#ifdef HAVE_POSIX_TIMERS
static timer_t sysClkTimer;
#endif

static void sysClkInt(int sig);
#ifdef USE_CLK_THREAD
static void *sysClkThread(void *arg);
static pthread_t sysClkThreadId;
#endif
static sigset_t sysClkSignalSet;

/*
 * Connect a routine to the clock interrupt
 */
STATUS
sysClkConnect(FUNCPTR routine, int arg)
{
    sysClkRoutine = routine;
    sysClkArg = arg;
    sysClkConnected = TRUE;

    return OK;
}

/*
 * Enable clock interrupts 
 */
void
sysClkEnable(void)
{
    struct sigaction act;
#ifdef HAVE_POSIX_TIMERS
    struct itimerspec tv;
#else
    struct itimerval tv;
#endif

    if (!sysClkRunning) {

	sigemptyset(&sysClkSignalSet);
	sigaddset(&sysClkSignalSet, CLK_SIGNAL);
#ifdef USE_CLK_THREAD
	/* Block CLK_SIGNAL */
	sigprocmask(SIG_BLOCK, &sysClkSignalSet, NULL);
#else
	/* Unblock CLK_SIGNAL */
	sigprocmask(SIG_UNBLOCK, &sysClkSignalSet, NULL);
#endif
	
	act.sa_handler = sysClkInt;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (sigaction(CLK_SIGNAL, &act, NULL) == -1) {
	    printf("Erreur sigaction %d\n", errno);
	}

#ifdef HAVE_POSIX_TIMERS
	/* Cree le timer global */
	if (timer_create(CLOCK_REALTIME, NULL, &sysClkTimer) == -1) {
	    printf("Erreur creation timer %d\n", errno);
	    ERRNO_SET(errno);
	}
	tv.it_interval.tv_nsec = 1000000000/sysClkTicksPerSecond;
	tv.it_interval.tv_sec = 0;
	tv.it_value.tv_nsec = 1000000000/sysClkTicksPerSecond;
	tv.it_value.tv_sec = 0;
#else 
	tv.it_interval.tv_usec = 1000000/sysClkTicksPerSecond;
	tv.it_interval.tv_sec = 0;
	tv.it_value.tv_usec = 1000000/sysClkTicksPerSecond;
	tv.it_value.tv_sec = 0;
#endif
    
#ifdef HAVE_POSIX_TIMERS
	if (timer_settime(sysClkTimer, 0, &tv, NULL) == -1) {
	    printf("Erreur set timer %d\n", errno);
	    ERRNO_SET(errno);
	}
#else
	if (setitimer(ITIMER_REAL, &tv, NULL) == -1) {
	    printf("Erreur set timer %d\n", errno);
	    ERRNO_SET(errno);
	}
#endif


#ifdef USE_CLK_THREAD
	/* Create a thread to handle signals */
	pthread_create(&sysClkThreadId, NULL, sysClkThread, NULL);
#endif
	sysClkRunning = TRUE;
    }
}

/*
 * Disable clock interrupts
 */
void
sysClkDisable(void)
{
    static struct sigaction act;
#ifndef HAVE_POSIX_TIMERS
    struct itimerval tv;
#endif

    if (sysClkRunning) {


#ifdef HAVE_POSIX_TIMERS
	timer_delete(sysClkTimer);
#else
	tv.it_value.tv_usec = 0;
	tv.it_value.tv_sec = 0;
	setitimer(ITIMER_REAL, &tv, NULL);
#endif

	act.sa_handler = SIG_DFL;
	act.sa_mask = sysClkSignalSet;
	act.sa_flags = 0;
	sigaction(CLK_SIGNAL, &act, NULL);

#ifdef USE_CLK_THREAD
	pthread_cancel(sysClkThreadId);
	sigprocmask(SIG_UNBLOCK, &sysClkSignalSet, NULL);
#endif
	sysClkRunning = FALSE;
    }
}

/*
 * Set clock Rate
 */
STATUS
sysClkRateSet(int ticksPerSecond)
{
    sysClkTicksPerSecond = ticksPerSecond;
    if (sysClkRunning) {
	sysClkDisable();
	sysClkEnable();
    }
    return OK;
}

/* 
 * Get clock rate
 */
int
sysClkRateGet(void)
{
    return sysClkTicksPerSecond;
}


static void
sysClkInt(int sig)
{
#ifdef DEBUG_H2TIMER
    if (sysClkTicksPerSecond <= 10) {
	printf("-- tick\n");
    }
#endif
    if (sysClkRoutine != NULL) {
	(*sysClkRoutine)(sysClkArg);
    }
}


#ifdef USE_CLK_THREAD
/*
 * A thread to handle CLK_SIGNAL
 */
static void *
sysClkThread(void *arg)
{
    int err, sig;    

    while (1) {
	err = sigwait(&sysClkSignalSet, &sig);
	if (sig == CLK_SIGNAL) {
	    sysClkInt(sig);
	}
    }
}
#endif

#ifdef TEST
int
main(int argc, char *argv)
{
    sigset_t set;

    sysClkRateSet(5);
    sysClkEnable();
    sigemptyset(&set);
    while (1) {
	sigsuspend(&set);
    }
    exit(0);
}
#endif
