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

#include <portLib.h>
#include <taskLib.h>

MODULE_AUTHOR ("LAAS/CNRS");
MODULE_DESCRIPTION ("pocolibs regression tests");
MODULE_LICENSE ("BSD");

extern int pocoregress_init();

int
pocoregress_i(void)
{
   int tid;

   printk("--- pocolibs regression test -------------------\n");

   tid = taskSpawn("pocoregress", 255, VX_FP_TASK, 20000, pocoregress_init);
   if (tid == ERROR) return 1;

   return 0;
}

void
pocoregress_e(void)
{
}

module_init(pocoregress_i);
module_exit(pocoregress_e);
