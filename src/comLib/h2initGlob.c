/*
 * Copyright (c) 1990-2011 CNRS/LAAS
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

#ifdef __XENO__
#include <sys/mman.h>
#endif
#include <sys/types.h>
#include <stdio.h>
#include <stdarg.h>

#include "portLib.h"
#include "h2devLib.h"
#include "h2timerLib.h"
#include "smMemLib.h"
#include "h2initGlob.h"

/**
 ** ComLib initialisation procedure for Unix processes
 ** should be called once, before any other comLib/posterLib function
 **/

/* for error msg declarations */
#include "h2semLib.h"
#include "h2devLib.h"
#include "h2evnLib.h"
#include "smObjLib.h"


STATUS
h2initGlob(int ticksPerSec)
{

#ifdef __XENO__
    /* Lock process in RAM */
    mlockall(MCL_CURRENT | MCL_FUTURE);
#endif
    /* init error msgs for sub-libraries without specific init functions */
    h2evnRecordH2ErrMsgs();
    h2semRecordH2ErrMsgs();
    smObjRecordH2ErrMsgs();

    /* OS level initialization */
    if (osInit(ticksPerSec) == ERROR) {
	return ERROR;
    }
    setvbuf(stdout, (char *)NULL, _IOLBF, 0);
    setvbuf(stderr, (char *)NULL, _IOLBF, 0);

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
    h2timeInit();
    printf("%s execution environment version %s\n"
	"Copyright (c) 1999-2011 CNRS-LAAS\n",
	PACKAGE_NAME, PACKAGE_VERSION);

    return OK;
}
