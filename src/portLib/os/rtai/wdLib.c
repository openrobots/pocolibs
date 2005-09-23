/*
 * Copyright (c) 1999-2005 CNRS/LAAS
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

#include "linux/kernel.h"
#include "linux/slab.h"

#include "portLib.h"

#include "errnoLib.h"
#include "semLib.h"
#include "wdLib.h"
static const H2_ERROR const wdLibH2errMsgs[] = WD_LIB_H2_ERR_MSGS;

WDOG_ID wdList = NULL;
SEM_ID wdMutex;

STATUS
wdLibInit(void)
{
    /* record error msgs */
    h2recordErrMsgs("wdLibInit", "wdLib", M_wdLib, 			
		    sizeof(wdLibH2errMsgs)/sizeof(H2_ERROR), 
		    wdLibH2errMsgs);

   wdMutex = semMCreate(0);
   if (!wdMutex) return ERROR;

   return OK;
}

WDOG_ID
wdCreate(void)
{
    WDOG_ID wd;

    wd = kmalloc(sizeof(struct wdog), GFP_KERNEL);
    if (wd == NULL) {
       errnoSet(S_wdLib_NOT_ENOUGH_MEMORY);
       return NULL;
    }
    wd->delay = -1;
    wd->pRoutine = NULL;
    wd->magic = WDLIB_MAGIC;

    /* Insert into wdList */
    semTake(wdMutex, WAIT_FOREVER);
    wd->next = wdList;
    wdList = wd;
    semGive(wdMutex);

    return wd;
}

STATUS
wdDelete(WDOG_ID wdId)
{
    WDOG_ID wd;

    if (wdId == NULL || wdId->magic != WDLIB_MAGIC) {
	errnoSet(S_wdLib_ID_ERROR);
	return ERROR;
    }

    /* Remove from wdList */
    semTake(wdMutex, WAIT_FOREVER);
    if (wdId == wdList) {
	wdList = wdId->next;
    } else {
	for (wd = wdList; wd->next != NULL; wd = wd->next) {
	    if (wd->next == wdId) {
		wd->next = wdId->next;
		break;
	    }
	} /* for */
    }
    kfree(wdId);
    semGive(wdMutex);

    return OK;
}

STATUS
wdStart(WDOG_ID wdId, int delay, FUNCPTR pRoutine, long parameter)
{
    if (wdId == NULL || wdId->magic != WDLIB_MAGIC) {
	errnoSet(S_wdLib_ID_ERROR);
	return ERROR;
    }

    semTake(wdMutex, WAIT_FOREVER);
    wdId->delay = delay;
    wdId->pRoutine = pRoutine;
    wdId->parameter = parameter;
    semGive(wdMutex);

    return OK;
}

STATUS
wdCancel(WDOG_ID wdId)
{
    if (wdId == NULL || wdId->magic != WDLIB_MAGIC) {
	errnoSet(S_wdLib_ID_ERROR);
	return ERROR;
    }

    semTake(wdMutex, WAIT_FOREVER);
    wdId->delay = 0;
    wdId->pRoutine = NULL;
    semGive(wdMutex);

    return OK;
}



	
