/*
 * Copyright (c) 2004 CNRS/LAAS
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

#ifdef __RTAI__
# include <rtai_sched.h>
#else
# include <stdio.h>
#endif

#include "portLib.h"
#include "semLib.h"
#include "taskLib.h"
#include "h2devLib.h"
#include "smMemLib.h"
#include "mboxLib.h"

SEM_ID sem1, sem2;

struct test {
   int a;
   double b;
};

int
pocoregress_mbox1()
{
   struct test data = { 0, 0. };
   int s;
   MBOX_ID id, id2;

   if (mboxInit("mbox1") == ERROR) {
      logMsg("Error: could not initialize mbox\n");
      goto done;
   }

   if (mboxCreate("mbox1", sizeof(struct test)*2+1, &id) != OK) {
      logMsg("Error: could not create mbox\n");
      goto done;
   }
   logMsg("mbox1 created\n");

   while(mboxPause(id, 0) != TRUE);

   s = mboxRcv(id, &id2, (char *)&data, sizeof(data), WAIT_FOREVER);
   if (s < 0)
      logMsg("mbox1 couldn't get message\n");
   else
      logMsg("mbox1 got message of %d bytes\n", s);

   logMsg("data received is %d %d.%d\n",
	  data.a, (int)data.b, (int)(data.b*1000)-(int)data.b*1000);

   semTake(sem2, WAIT_FOREVER);

   h2devShow();
   smMemShow(TRUE);

   if (mboxDelete(id) != OK) {
      logMsg("Error: could not delete mbox\n");
   }

  done:
   semGive(sem1);
   return 0;
}

int
pocoregress_mbox2()
{
   struct test data = { 3, 3.1415926 };
   MBOX_ID id1, id2;

   if (mboxInit("mbox2") == ERROR) {
      logMsg("Error: could not initialize h2 devices\n");
      goto done;
   }

   if (mboxCreate("mbox2", sizeof(struct test)*2+1, &id2) != OK) {
      logMsg("Error: could not create mbox2\n");
      goto done;
   }
   logMsg("mbox2 created\n");

   while(mboxFind("mbox1", &id1) == ERROR)
      taskDelay(1);

   logMsg("mbox1 found\n");

   if (mboxSend(id1, id2, (char *)&data, sizeof(data)) != OK) {
      logMsg("couldn't send message\n");
   } else {
      logMsg("data sent is %d %d.%d\n",
	     data.a, (int)data.b, (int)(data.b*1000)-(int)data.b*1000);
   }

   if (mboxDelete(id2) != OK) {
      logMsg("Error: could not delete mbox\n");
   }

  done:
   semGive(sem2);
   return 0;
}


int
pocoregress_init()
{
   sem1 = semBCreate(0, SEM_EMPTY);
   sem2 = semBCreate(0, SEM_EMPTY);

   taskSpawn("mbox1", 200, VX_FP_TASK, 20000, pocoregress_mbox1);
   taskSpawn("mbox2", 200, VX_FP_TASK, 20000, pocoregress_mbox2);

   semTake(sem1, WAIT_FOREVER);

   semDelete(sem1);
   semDelete(sem2);

   return 0;
}
