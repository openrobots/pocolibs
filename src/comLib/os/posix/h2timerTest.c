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

#include "pocolibs-config.h"
__RCSID("$LAAS$");

#include <stdio.h>

#include "portLib.h"
#include "errnoLib.h"
#include "h2errorLib.h"
#include "h2timerLib.h"
#include "h2timeLib.h"

int
h2timerTest(int period)
{
    H2TIMER_ID timer;
    H2TIME  oldTime;
    u_long msec, tmax = 0;
    int i = 0;

    if (period == 0) 
	period = 10;

    h2timeGet(&oldTime);
    timer = h2timerAlloc();
    if (timer == NULL) {
	printf("Error allocating a h2timer: ");
	h2printErrno(errnoGet());
	return(ERROR);
    }
    
    if (h2timerStart(timer, period, 1) == ERROR) {
	return(ERROR);
    }

    while (1) {
	
	h2timerPause(timer);
	printf("ok\n");
	h2timeInterval(&oldTime, &msec);
	h2timeGet(&oldTime);
	if (msec > tmax) {
	    tmax = msec;
	}
	if (i++ == 100) {
	    printf("%ld %ld\n", msec, tmax);
	    i = 0;
	    tmax = 0;
	}
    }
}

