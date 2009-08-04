/*
 * Copyright (c) 2004,2009 CNRS/LAAS
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

#include <stdio.h>

#include "portLib.h"
#include "semLib.h"
#include "taskLib.h"
#include "tickLib.h"
#include "h2devLib.h"
#include "smMemLib.h"
#include "h2timerLib.h"
#include "semLib.h"


int
execTask(SEM_ID done)
{
   unsigned long t1, t2;
   H2TIMER_ID t;
   SEM_ID sem;
   int i, result;

   t = h2timerAlloc();
   if (!t) {
      logMsg("Error: could not create timer\n");
      return 1;
   }

   sem = semBCreate(0, SEM_FULL);
   logMsg("starting timer\n");

   h2timerStart(t, 10, 10);

   for(i=0; i<3; i++) {
      h2timerPause(t);
      t1 = tickGet();
      result = semTake(sem, 5);
      t2 = tickGet();
      printf("result: %s %lu ticks\n", result == OK ? "OK" : "ERROR", t2 - t1);
      semGive(sem);
   }

   h2timerStop(t);

   h2timerFree(t);
   
   semGive(done);
   return 0;
}

int
pocoregress_init()
{
	long tid;
	SEM_ID sem;
	
	sem = semBCreate(0, SEM_EMPTY);
	if ((tid = taskSpawn("tTest", 100, 0, 4096, execTask, sem)) == ERROR) {
		logMsg("error creating test task\n");
		return 1;
	}
	semTake(sem, WAIT_FOREVER);
	return 0;
}
