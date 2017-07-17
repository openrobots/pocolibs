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
#include "tickLib.h"

int
pocoregress_init(void)
{
	H2TIMESPEC ts;
	H2TIME h2time;
	int i;
	unsigned long t1;

	h2timeInit();

	for (i = 0; i < 300; i++) {
		h2GetTimeSpec(&ts);
		h2timeFromTimespec(&h2time, &ts);
		printf("%02d ticks -> %lu %lu\n", i, h2time.ntick, tickGet());
		t1 = tickGet();
		while (tickGet() == t1)
			;
	}
	return 0;
}
