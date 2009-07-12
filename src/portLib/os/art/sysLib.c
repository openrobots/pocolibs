/*
 * Copyright (c) 1999, 2003,2009 CNRS/LAAS
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
#include "pocolibs-config.h"

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h>

#include <linux/art_task.h>

#include "portLib.h"
#include "errnoLib.h"


/*----------------------------------------------------------------------*/

/**
 ** Emulation of VxWorks system Clock
 **
 ** Based on public VxWorks BSP Sources
 **/
static FUNCPTR sysClkRoutine = NULL;
static int sysClkArg = 0;
static int sysClkTicksPerSecond = 100;
static int sysClkRunning = FALSE;

static void *sysClkThread(void *arg);
static pthread_t sysClkThreadId;

/*
 * Connect a routine to the clock interrupt
 */
STATUS
sysClkConnect(FUNCPTR routine, int arg)
{
    sysClkRoutine = routine;
    sysClkArg = arg;
    return OK;
}

/*
 * Enable clock interrupts
 */
void
sysClkEnable(void)
{
    if (!sysClkRunning) {
        sysClkRunning = TRUE;
        pthread_create(&sysClkThreadId, NULL, sysClkThread, NULL);
    }
}

/*
 * Disable clock interrupts
 */
void
sysClkDisable(void)
{
    if (sysClkRunning) {
        sysClkRunning = FALSE;
        pthread_join(sysClkThreadId, NULL);
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

/*
 * A thread to handle clock
 */
static void *
sysClkThread(void *arg)
{
    int s;

    s = art_enter(ART_PRIO_MAX,
		  ART_TASK_PERIODIC, 1000000/sysClkTicksPerSecond);
    if (s) {
      perror("art_enter");
      exit(255);
    }

    while (sysClkRunning == TRUE) {
      s = art_wait();
      if (s) {
	perror("art_wait");
	exit(255);
      }
      if (sysClkRoutine != NULL) {
	  (*sysClkRoutine)(sysClkArg);
      }
    }

    art_exit();
    return NULL;
}
