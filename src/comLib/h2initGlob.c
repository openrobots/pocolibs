/*
 * Copyright (c) 1990, 2004 CNRS/LAAS
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

/***
 *** Global initialisation procedure for comLib
 ***
 *** ticksPerSec -> main clock frequency
 ***                0 -> no clock
 ***
 ***/

#include "pocolibs-config.h"
__RCSID("$LAAS$");

#include <sys/types.h>
#include <stdio.h>
#include <stdarg.h>

#include "portLib.h"
#include "h2devLib.h"
#include "h2timerLib.h"
#include "smMemLib.h"
#include "h2initGlob.h"
#include "xes.h"

/**
 ** ComLib initialisation procedure for Unix processes 
 ** should be called once, before any other comLib/posterLib function
 **/

STATUS
h2initGlob(int ticksPerSec)
{
    /* OS level initialization */
    if (osInit(ticksPerSec) == ERROR) {
	return ERROR;
    }
    /* I/O redirections  */
    if (xesStdioInit() == ERROR) {
        return ERROR;
    }
    /* attach to h2 devices */
    if (h2devAttach() == ERROR) {
	printf("Error: could not find h2 devices\n"
		"Did you execute `h2 init' ?\n");
	return ERROR;
    }
    /* attach to shared memory */
    if (smMemAttach() == ERROR) {
	printf("Error: could not attach shared memory\n");
	return ERROR;
    }
		
    /* Start h2 timers if a clock is available */
    if (ticksPerSec != 0) {
	if (h2timerInit() == ERROR) {
	    return ERROR;
	}
    }
    printf("Hilare2 execution environment version 2.0\n"
	   "Copyright (c) 1999-2004 CNRS-LAAS\n");

    return OK;
}
