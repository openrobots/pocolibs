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

#include "portLib.h"
#include "h2timeLib.h"

static char *dayStr [] = {
	"sunday", "monday", "tuesday", "wednesday", "thursday",
	"friday", "saturday"};

int
pocoregress_init(void)
{
	struct timeval tv;
	H2TIME h2time;

	/* Special case for bugzilla #102 */
	tv.tv_sec = 1262172119;
	tv.tv_usec = 796000;

	h2timeFromTimeval(&h2time, &tv);

	printf("Date: %02u-%02u-%02u, %s, %02uh:%02umin:%02us\n\n", 
	    h2time.month, h2time.date, h2time.year + 1900, 
	    dayStr[h2time.day], h2time.hour, h2time.minute, 
	    h2time.sec);

	if (h2time.month != 12 || h2time.date != 30 || h2time.year != 109
		|| h2time.day != 3) {
		fprintf(stderr, "Error decoding date\n");
		return 1;
	}
	return 0;
}
