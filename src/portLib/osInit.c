/*
 * Copyright (c) 1999, 2003-2005 CNRS/LAAS
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

#include "portLib.h"
#ifndef __KERNEL__
#include <signal.h>
#endif

#include "taskLib.h"
#include "tickLib.h"
#include "sysLib.h"
#include "symLib.h"
#include "wdLib.h"

#include "semLib.h"
#include "portLib.h"

/**
 ** Global initialisation of all OS-level services (VxWorks Emulation)
 **/

STATUS
osInit(int clkRate)
{

    /* init all error msgs */
    semRecordH2ErrMsgs();
    symRecordH2ErrMsgs();
    portRecordH2ErrMsgs();

    /* Initialize log library */
    if (logInit(0/* ignored */, 256) == ERROR) {
        return ERROR;
    }
    
    /* Initialize task library */
    if (taskLibInit() == ERROR) {
	return ERROR;
    }

    /* Initialize watchdog library */
    if (wdLibInit() == ERROR) {
       return ERROR;
    }

    if (clkRate > 0) {
	/* Start system clock */
	sysClkConnect((FUNCPTR)tickAnnounce, 0);
	sysClkRateSet(clkRate);
	sysClkEnable();
    }

    /* initialize symbol table access */
    if (symLibInit() == ERROR) {
	return ERROR;
    }

    return OK;
}
    

/**
 ** Cleanup routine
 **/

void
osExit(void)
{
   sysClkDisable();
   logEnd();
}
