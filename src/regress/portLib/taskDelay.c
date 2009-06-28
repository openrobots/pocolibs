/*
 * Copyright (c) 1990, 2003-2004,2009 CNRS/LAAS
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

#include <sys/time.h>
#include <stdio.h>

#include <portLib.h>
#include <taskLib.h>

/**
 ** time_difference - compute a difference in milliseconds between
 **                   2 timevals 
 **/
static long
time_difference(struct timeval *t1, struct timeval *t2)
{
	return (t1->tv_sec - t2->tv_sec)*1000000 + t1->tv_usec - t2->tv_usec;
}


#define TPS	100

int
pocoregress_init()
{
   struct timeval tp1, tp2;
   int i;

   for (i = 0; i < 50; i++) {
      gettimeofday(&tp1, NULL);

      taskDelay(i);

      gettimeofday(&tp2, NULL);
      logMsg("taskDelay lasted %d us - should be %d\n",
	     time_difference(&tp2, &tp1), i * 1000000/sysClkRateGet());
   } /* for */

   return 0;
}
