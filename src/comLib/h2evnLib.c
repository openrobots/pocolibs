/*
 * Copyright (c) 1990, 2003-2004 CNRS/LAAS
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

#if defined(__RTAI__) && defined(__KERNEL__)
# include <linux/kernel.h>
#else
# include <unistd.h>
#endif

#include "taskLib.h"
#include "h2semLib.h"
#include "h2devLib.h"
#include "mboxLib.h"
#include "h2evnLib.h"
static const H2_ERROR const h2evnLibH2errMsgs[] = H2_EVN_LIB_H2_ERR_MSGS;

#include "errnoLib.h"

#ifdef DEBUG
#include <stdio.h>
#include <stdarg.h>
#include "xes.h"
#endif

#ifdef COMLIB_DEBUG_H2EVNLIB
# define LOGDBG(x)	logMsg x
#else
# define LOGDBG(x)
#endif

/******************************************************************************
 *
 * h2evnRecordH2ErrMsgs - Record errors messages
 *
 */
int
h2evnRecordH2ErrMsgs(void)
{
    return h2recordErrMsgs("h2evnRecordH2ErrMsg", "h2evnLib", M_h2evnLib,
			   sizeof(h2evnLibH2errMsgs)/sizeof(H2_ERROR), 
			   h2evnLibH2errMsgs);
}

/******************************************************************************
*
*  h2evnSusp  -  Suspendre en attente d'un evenement
*
*  Description :
*  La tache se suspend, en attendand un evenement externe. L'arrivee
*  de cet evenement externe se traduit par la liberation du semaphore
*  de synchronisation de la tache suspendue. Ainsi, les drivers de 
*  communication inter-taches devront etre construits de maniere qu'ils
*  liberent ce semaphore quand il y a la "generation" d'un evenement.
*
*  Retourne : TRUE, FALSE ou ERROR
*/


BOOL
h2evnSusp(int timeout)
{
    unsigned long dev = taskGetUserData(0);

    /* Si le semaphore de synchro n'existe pas, 
       appelle mboxInit() pour le creer */
    if (dev == 0) {
	if (mboxInit(NULL) == ERROR) {
	    return ERROR;
	}
	/* La c'est bon normalement on a un semaphore de synchro */
	dev = taskGetUserData(0);
	if (dev == 0) {
	   /* Encore rate' ?? */
	   LOGDBG(("comLib:h2evnSusp: dev == 0\n"));
	   return ERROR;
	}
    }

    LOGDBG(("comLib:h2evnSusp: task %lx device %d\n", taskIdSelf(), dev));
    return h2semTake(H2DEV_TASK_SEM_ID(dev), timeout);
}


/******************************************************************************
*
*  h2evnSignal  -  Signaler un evenement
*
*  Description :
*  Cette fonction permet de signaler l'existence d'evenement a traiter
*
*  Retourne: OK ou ERROR
*/

STATUS
h2evnSignal(int taskId)
{
    unsigned dev = taskGetUserData(taskId);

    /* if the task didn't call h2evnSusp before, report an error */
    if (dev == 0) {
       LOGDBG(("h2evnSignal: bad dev %d, task %lx\n", dev, taskId));
       errnoSet(S_h2evnLib_BAD_TASK_ID);
       return ERROR;
    }

    LOGDBG(("comLib:h2evnSignal: task %lx device %d\n", taskId, dev));
    return(h2semGive(H2DEV_TASK_SEM_ID(dev)));
}

/***********************************************************************
 * 
 * h2evnClear - reset des evenements en attente
 */

void
h2evnClear(void)
{
    unsigned long dev = taskGetUserData(0);

    h2semFlush(H2DEV_TASK_SEM_ID(dev));
}
