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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>

#include <portLib.h>
#include <taskLib.h>
#include <h2devLib.h>
#include <h2timerLib.h>
#include <smMemLib.h>

MODULE_AUTHOR ("LAAS/CNRS");
MODULE_DESCRIPTION ("comLib - real-time communication layer");
MODULE_LICENSE ("BSD");

int shm_size = SM_MEM_SIZE;
MODULE_PARM(shm_size, "i");
MODULE_PARM_DESC(shm_size,
		 "Shared memory segment size (default 4Mo)");

int
comLib_init(void)
{
   if (h2devInit(shm_size) == ERROR) {
      logMsg("comLib: could not initialize h2 devices\n");
      return -EINVAL;
   }
   logMsg("comLib: using %d bytes of shared memory\n", shm_size);

   if (sysClkRateGet() != 0) {
      if (h2timerInit() == ERROR) {
	 logMsg("comLib: could not initialize h2 timers\n");
	 h2devEnd();
	 return -EINVAL;
      }
   }

   logMsg("comLib: h2 devices initialized\n");
   return 0;
}

void
comLib_exit(void)
{
   if (h2timerEnd() == ERROR) {
      logMsg("comLib: could not delete h2 timers\n");
   }

   if (h2devEnd() == ERROR) {
      logMsg("Error: could not delete h2 devices\n");
   }
}

module_init(comLib_init);
module_exit(comLib_exit);
