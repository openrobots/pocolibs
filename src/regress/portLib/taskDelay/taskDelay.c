/*
 * Copyright (c) 1990, 2003 CNRS/LAAS
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

#include "config.h"
__RCSID("$LAAS$");

#include <sys/time.h>
#include <portLib.h>
#include <taskLib.h>

#include <stdio.h>

/**
 ** time_difference - compute a difference in milliseconds between
 **                   2 timevals 
 **/
static long
time_difference(struct timeval *t1, struct timeval *t2)
{
	long usec_diff = t1->tv_usec - t2->tv_usec, retenue = 0;
	
	if (usec_diff < 0) {
		usec_diff = 1000000 + usec_diff;
		retenue = 1;
	}
	return (t1->tv_sec - t2->tv_sec - retenue)*1000 + usec_diff/1000;
}


int
main(int argc, char *argv[])
{
	struct timeval tp1, tp2;
	int i;

	h2initGlob(100);
	for (i = 0; i < 100; i++) {
		gettimeofday(&tp1, NULL);
		taskDelay(i);
		gettimeofday(&tp2, NULL);
		printf("%d: %ld\n", i, time_difference(&tp2, &tp1));
	} /* for */
	exit(0);
}

