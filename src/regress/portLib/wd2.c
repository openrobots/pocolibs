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
#include <stdint.h>

#include <portLib.h>
#include <semLib.h>
#include <wdLib.h>
#include <tickLib.h>

SEM_ID sem1, sem2;

int
wddone(SEM_ID *sem)
{
   semGive(*sem);
   return 0;
}


int
pocoregress_init()
{
   long t1, t2;
   WDOG_ID wd1, wd2;

   wd1 = wdCreate();
   if (!wd1) {
      logMsg("cannot create watchdog1\n");
      return 0;
   }
   wd2 = wdCreate();
   if (!wd2) {
	   logMsg("cannot create watchdog2\n");
	   return 0;
   }

   sem1 = semBCreate(0, SEM_EMPTY);
   if (!sem1) {
      logMsg("cannot create semaphore1\n");
      return 0;
   }

   sem2 = semBCreate(0, SEM_EMPTY);
   if (!sem2) {
      logMsg("cannot create semaphore2\n");
      return 0;
   }

   logMsg("waiting %d ticks...\n", 10);

   t1 = tickGet();
   wdStart(wd1, 10, wddone, (intptr_t)&sem1);
   wdStart(wd2, 20, wddone, (intptr_t)&sem2);
   
   semTake(sem1, WAIT_FOREVER);
   t2 = tickGet();

   logMsg("watchodog1 triggered after %d ticks (should be %d)\n", t2-t1, 10);
   
   semTake(sem2, WAIT_FOREVER);
   t2 = tickGet();

   logMsg("watchodog2 triggered after %d ticks (should be %d)\n", t2-t1, 20);
   wdDelete(wd1);
   wdDelete(wd2);

   return 0;
}
