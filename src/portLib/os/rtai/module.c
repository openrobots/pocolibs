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

#include "portLib.h"

MODULE_AUTHOR ("LAAS/CNRS");
MODULE_DESCRIPTION ("portLib - portable real-time abstraction layer");
MODULE_LICENSE ("BSD");

int ticks_per_sec = 100;
MODULE_PARM(ticks_per_sec, "i");
MODULE_PARM_DESC(ticks_per_sec,
		 "Clock frequency in ticks per second (default 100)");

int
portLib_init(void)
{
   if (ticks_per_sec < 0 || ticks_per_sec > 1000000) {
      printk("portLib: %d is an invalid tick frequency\n", ticks_per_sec);
      return -EINVAL;
   }

   if (osInit(ticks_per_sec) != OK) {
      printk("portLib: initialization failed\n");
      return -EINVAL;
   }

   return 0;
}

void
portLib_exit(void)
{
   osExit();
}

module_init(portLib_init);
module_exit(portLib_exit);
