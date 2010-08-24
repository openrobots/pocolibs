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

#include <portLib.h>
#include <semLib.h>
#include <wdLib.h>
#include <tickLib.h>

SEM_ID sem;

int
wddone()
{
   semGive(sem);
   return 0;
}

int
pocoregress_init()
{
   long t1, t2;
   WDOG_ID wd;

   wd = wdCreate();
   if (!wd) {
      logMsg("cannot create watchdog\n");
      return 0;
   }

   sem = semBCreate(0, SEM_EMPTY);
   if (!sem) {
      logMsg("cannot create semaphore\n");
      return 0;
   }

   logMsg("waiting %d ticks...\n", 10);

   t1 = tickGet();
   wdStart(wd, 10, wddone, 0);
   semTake(sem, WAIT_FOREVER);
   t2 = tickGet();

   logMsg("watchodog triggered after %d ticks (should be %d)\n", t2-t1, 10);
   
   wdDelete(wd);

   return 0;
}
