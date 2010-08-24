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

#include <stdio.h>
#include <sys/time.h>

#include "portLib.h"
#include "semLib.h"
#include "taskLib.h"
#include "tickLib.h"
#include "h2devLib.h"
#include "smMemLib.h"
#include "h2timerLib.h"

int
pocoregress_init()
{
   struct timeval start, end;
   unsigned long t1, t2;
   H2TIMER_ID t;
   int i;

   t = h2timerAlloc();
   if (!t) {
      logMsg("Error: could not create timer\n");
      return 1;
   }

   logMsg("starting timer\n");

   h2timerStart(t, 10, 3);

   for(i=0; i<3; i++) {
      (void)gettimeofday(&start, NULL);
      t1 = tickGet();
      h2timerPause(t);
      t2 = tickGet();
      (void)gettimeofday(&end, NULL);
      logMsg("timer pause took %d ticks (%f ms)\n", t2-t1,
	     (end.tv_sec*1000. + end.tv_usec/1000.) -
	     (start.tv_sec*1000. + start.tv_usec/1000.));
   }

   h2timerStop(t);

   h2timerStart(t, 5, 12);

   for(i=0; i<3; i++) {
      (void)gettimeofday(&start, NULL);
      t1 = tickGet();
      h2timerPause(t);
      t2 = tickGet();
      (void)gettimeofday(&end, NULL);
      logMsg("timer pause took %d ticks (%f ms)\n", t2-t1,
	     (end.tv_sec*1000. + end.tv_usec/1000.) -
	     (start.tv_sec*1000. + start.tv_usec/1000.));
   }

   h2timerStop(t);
   h2timerFree(t);

   return 0;
}
