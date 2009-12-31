/*
 * Copyright (c) 2009 CNRS/LAAS
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

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

#include "portLib.h"
#include "h2timeLib.h"

static char *dayStr [] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

int
pocoregress_init(void)
{
	struct timeval tv;
	struct tm tm;
	H2TIME h2time;
	int total = 0, error = 0;

#if 0
	/* Special case for bugzilla #102 */
	tv.tv_sec = 1262172119;
	tv.tv_usec = 796000;
#endif
	tv.tv_usec = 0;
	for (tv.tv_sec = 1000000000; tv.tv_sec < 2147400000;
	     tv.tv_sec += 3600) {
		
		h2timeFromTimeval(&h2time, &tv);
		gmtime_r(&tv.tv_sec, &tm);
		
		if (h2time.month != tm.tm_mon + 1 || h2time.date != tm.tm_mday 
		    || h2time.year != tm.tm_year || h2time.day != tm.tm_wday) {
			fprintf(stderr, "Error decoding date: %ld %s", 
			    tv.tv_sec, ctime(&tv.tv_sec));
			fprintf(stderr, 
			    "->  %s %02u-%02u-%04u, %02u:%02u:%02u\n", 
			    dayStr[h2time.day], 
			    h2time.month, h2time.date, h2time.year + 1900, 
			    h2time.hour, h2time.minute, h2time.sec);
			error++;
		}
		total++;
	}
	if (error) {
		printf("%d/%d errors\n", error, total);
		return 1;
	}
	return 0;
}
