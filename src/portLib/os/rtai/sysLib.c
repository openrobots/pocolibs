/*
 * Copyright (c) 1999, 2003-2004 CNRS/LAAS
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
__RCSID("$LAAS$");

#include <rtai_sched.h>

#include "portLib.h"
#include "taskLib.h"

/* #define PORTLIB_DEBUG_SYSLIB */

#ifdef PORTLIB_DEBUG_SYSLIB
# define LOGDBG(x)	logMsg x
#else
# define LOGDBG(x)
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
static int sysClkTickCount = 100;
static int sysClkConnected = FALSE;
static int sysClkRunning = FALSE;

static int sysClkThread();
static long sysClkThreadId;


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
   rt_set_periodic_mode();

   sysClkTickCount = start_rt_timer(sysClkTickCount);

   LOGDBG(("portLib:sysLib:sysClkEnable: starting rt timer at "
	   "%d ticks/s (period %d)\n", sysClkTicksPerSecond, sysClkTickCount));

   sysClkThreadId = 
      taskSpawn("sysClkThread", 10, VX_FP_TASK, 1024, sysClkThread);
   if (sysClkThreadId == ERROR) {
      logMsg("portLib:sysClkEnable: cannot start clock thread\n");
      return;
   }

   if (rt_task_make_periodic((RT_TASK*)sysClkThreadId,
			     rt_get_time()+sysClkTickCount,
			     sysClkTickCount))
      logMsg("portLib:sysClkThread: failed to make periodic\n");

   sysClkRunning = TRUE;
}

/*
 * Disable clock interrupts
 */
void
sysClkDisable(void)
{
   if (sysClkRunning) {
      stop_rt_timer();

      LOGDBG(("portLib:sysLib:sysClkDisable: stoping rt timer\n"));

      taskDelete(sysClkThreadId);
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

    if (sysClkTicksPerSecond != 0)
       sysClkTickCount = nano2count(1e9 / sysClkTicksPerSecond);
    else
       sysClkTickCount = 0;

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
 * Get ticks length in internal count units
 */
int
sysClkTickDuration()
{
   return sysClkTickCount;
}


/*
 * A thread to handle CLK_SIGNAL
 */
static int
sysClkThread()
{
   while(sysClkRunning != TRUE)
      rt_sleep(sysClkTickCount);

   while (1) {
      LOGDBG(("portLib:sysClkInt: tick\n"));

      if (sysClkRoutine != NULL) {
	 (*sysClkRoutine)(sysClkArg);
      }

      rt_task_wait_period();
   }
}
