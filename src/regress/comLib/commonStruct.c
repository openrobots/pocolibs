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

# include <stdio.h>

#include <portLib.h>
#include <taskLib.h>
#include <commonStructLib.h>

struct csTest {
   int v1, v2;
};

struct args {
        struct csTest *cs;
        int v;
};

static void *
pocoregress_task1(void *ptr)
{
   struct args *args = ptr;
   struct csTest *cs = args->cs;
   int v = args->v;

   taskDelay(v * 10);

   if (commonStructTake(cs) != OK) {
      printf("Task %d: cannot take common struct\n", v);
      return 0;
   }

   cs->v1 = v;
   cs->v2 = v;
   printf("Task %d: wrote value in common struct\n", v);

   if (commonStructGive(cs) != OK) {
      printf("Task %d: cannot give common struct\n", v);
      return 0;
   }

   return 0;
}


int
pocoregress_init()
{
   int ok;
   struct csTest *cs;
   struct args args1, args2;

   if (commonStructCreate(sizeof(struct csTest), (void **)&cs) != OK) {
      return 1;
   }

   cs->v1 = cs->v2 = 0;

   args1.cs = cs;
   args1.v = 1;
   args2.cs = cs;
   args2.v = 2;

   taskSpawn2(NULL, 200, VX_FP_TASK, 20000, pocoregress_task1, &args1);
   taskSpawn2(NULL, 200, VX_FP_TASK, 20000, pocoregress_task1, &args2);

   do {
      ok = 0;
      commonStructTake(cs);
      if (cs->v1 == 2 && cs->v2 == 2) ok = 1;
      printf("common struct contains %d %d\n", cs->v1, cs->v2);
      commonStructGive(cs);

      taskDelay(10);
   } while (!ok);

   commonStructDelete(cs);
   return 0;
}
