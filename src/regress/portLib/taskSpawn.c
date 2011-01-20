/*
 * Copyright (c) 2011 CNRS/LAAS
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

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <portLib.h>
#include <taskLib.h>
#include <semLib.h>

#define ARG1 1234567
#define ARG2 M_PI
#define ARG3 -1234567
#define ARG4 "rumpelstilskin"

SEM_ID sem;

int
myTask(int arg1, double arg2, int arg3, char *arg4)
{
	printf("arg1 %d arg2 %lf arg3 %d arg4 %s\n", arg1, arg2, arg3, arg4);
	if (arg1 != ARG1)
		fprintf(stderr, "arg1 %d != %d\n", arg1, ARG1);
	if (arg2 != ARG2)
		fprintf(stderr, "arg2 %lf != %lf\n", arg2, ARG2);
	if (arg3 != ARG3)
		fprintf(stderr, "arg1 %d != %d\n", arg3, ARG3);
	if (strcmp(arg4, ARG4) != 0) 
		fprintf(stderr, "arg3 \"%s\" != \"%s\"\n", arg4, ARG4);
	semGive(sem);
	return 0;
}

		
int
pocoregress_init()
{

	sem = semBCreate(0, SEM_EMPTY);
	taskSpawn("tArgTest", 100, VX_FP_TASK, 65536, myTask, 
	    ARG1, ARG2, ARG3, ARG4);
	semTake(sem, WAIT_FOREVER);
	return 0;
}
