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

#include <portLib.h>
#ifdef __RTAI__
# include <rtai_sched.h>
#else
# include <stdio.h>
#endif
#include <sys/time.h>
#include <tickLib.h>

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
	return (t1->tv_sec - t2->tv_sec - retenue)*1000000 + usec_diff;
}

int pocoregress_init()
{
	long t1, t2;
	struct timeval tp1, tp2;
	
#if defined(__RTAI__) && defined(__KERNEL__)
	do_gettimeofday(&tp1);
#else
	gettimeofday(&tp1, NULL);
#endif
	t1 = tickGet();
	do {
		t2 = tickGet();
	} while (t2 - t1 < 100);
#if defined(__RTAI__) && defined(__KERNEL__)
	do_gettimeofday(&tp2);
#else
	gettimeofday(&tp2, NULL);
#endif
	logMsg("100 ticks lasted %d us - should be %d\n",
	    time_difference(&tp2, &tp1), 1000000000/sysClkRateGet());
}
