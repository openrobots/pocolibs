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

struct my_args {
	int arg1;
	double arg2;
	int arg3;
	char *arg4;
};

		
SEM_ID sem;

void *
myTask(void *ptr)
{
	struct my_args *args = (struct my_args *)ptr;

	printf("arg1 %d arg2 %lf arg3 %d arg4 %s\n", 
	    args->arg1, args->arg2, args->arg3, args->arg4);

	if (args->arg1 != ARG1)
		fprintf(stderr, "arg1 %d != %d\n", args->arg1, ARG1);
	if (args->arg2 != ARG2)
		fprintf(stderr, "arg2 %lf != %lf\n", args->arg2, ARG2);
	if (args->arg3 != ARG3)
		fprintf(stderr, "arg3 %d != %d\n", args->arg3, ARG3);
	if (strcmp(args->arg4, ARG4) != 0) 
		fprintf(stderr, "arg4 \"%s\" != \"%s\"\n", args->arg4, ARG4);
	semGive(sem);
	return NULL;
}

		
int
pocoregress_init()
{
	struct my_args args;

	sem = semBCreate(0, SEM_EMPTY);
	args.arg1 = ARG1;
	args.arg2 = ARG2;
	args.arg3 = ARG3;
	args.arg4 = ARG4;
	taskSpawn2("tArgTest", 100, VX_FP_TASK, 65536, myTask, &args);
	semTake(sem, WAIT_FOREVER);
	return 0;
}
