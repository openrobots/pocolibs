/*
 * Copyright (c) 2016 CNRS/LAAS
 *
 * Permission to use, copy, modify, and/or distribute this software for any
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

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <portLib.h>
#include <logLib.h>
#include <taskLib.h>
#include <smMemLib.h>

pthread_barrier_t barrier;

#define MAX_TESTS 200

void *
smMemTask(void *param)
{
	int i;
	char *ptr;

	for (i = 0; i < MAX_TESTS; i++) {
		size_t len = 0x100 + 0x100*(rand() % 4);
		ptr = smMemMalloc(len);
		if (ptr == NULL) {
			logMsg("%s: smMemMalloc 0x%lx failed\n", taskName(0), 
			    (long)len);
			continue;
		}
		logMsg("%s: 0x%lx %p\n", taskName(0), (long)len,  ptr);
		sched_yield();
		memset(ptr, i, len);
		smMemFree(ptr);
		sched_yield();
	}
	pthread_barrier_wait(&barrier);
	return NULL;
}


int
pocoregress_init(void)
{
	int i;
	char name[4][10];
	unsigned long base;

	base = smMemBase();
	logMsg("base: %lx\n", base);
	pthread_barrier_init(&barrier, NULL, 5);

	for (i = 0; i < 4; i++) {
		snprintf(name[i], sizeof(name), "tSmMem%d", i);
		taskSpawn2(name[i], 100, 0, 65536, smMemTask, NULL);
	}
	pthread_barrier_wait(&barrier);
	return(0);
}

