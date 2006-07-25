/*
 * Copyright (c) 1990, 2003-2006 CNRS/LAAS
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

/* DESCRIPTION :
   This is the library providing timers, used for task scheduling,
   with programmable time slices.
*/

#include "pocolibs-config.h"
__RCSID("$LAAS$");

#include "portLib.h"

#if defined(__RTAI__) && defined(__KERNEL__)
# include <linux/kernel.h>
# include <linux/sched.h>
#else
# include <stdio.h>
# include <string.h>
#endif

#include "errnoLib.h"
#include "semLib.h"
#include "wdLib.h"
#include "h2timerLib.h"

static const H2_ERROR const h2timerLibH2errMsgs[] = H2_TIMER_LIB_H2_ERR_MSGS;

/* #define COMLIB_DEBUG_H2TIMERLIB */

#ifdef COMLIB_DEBUG_H2TIMERLIB
# define LOGDBG(x)     logMsg x
#else
# define LOGDBG(x)
#endif

static int delayCount;			/* global synchronisation counter */
static SEM_ID timerMutex;
static WDOG_ID timerWd;			/* timer watchdog */
static H2TIMER timerTab[NMAX_TIMERS];	/* array of timers */
static BOOL h2timerInited = FALSE;

static void timerInt(int arg); /* timer handler procedure */

/******************************************************************************
*
*   h2timerInit	 -  Initializes the timer library
*
*   Description:
*   Initializes the timer library, starts the watchdog that genrate time
*   slices interrupts.
*
*   Returns: OK or ERROR
*/
STATUS
h2timerInit(void)
{
	int nTimer;

	LOGDBG(("comLib:h2timerLib:h2timerInit: ---beginning\n"));

	/* Create watchdog and mutex semaphore */
	timerMutex = semMCreate(0);
	semTake(timerMutex, WAIT_FOREVER);

	if ((timerWd = wdCreate ()) == NULL) {
		semGive(timerMutex);
		return ERROR;
	}

	/* Reset timer array */
	memset (timerTab, 0, sizeof(timerTab));

	/* allocate a synchronisation variable for each timer */
	for (nTimer = 0; nTimer < NMAX_TIMERS; nTimer++) {
		timerTab[nTimer].semSync = semBCreate(SEM_Q_PRIORITY,
		    SEM_EMPTY);
		if (timerTab[nTimer].semSync == NULL) {
			logMsg("h2timerInit: error semBCreate %d\n", nTimer);
			return ERROR;
		}
	}
	/* reset global delay counter */
	delayCount = 0;

	/* done with shared data */
	semGive(timerMutex);

	/* Start the watchdog */
	wdStart (timerWd, 1, (FUNCPTR) timerInt, 0);

	LOGDBG(("comLib:h2timerLib:h2timerInit: ---end\n"));


	/* record errors msg */
	h2recordErrMsgs("h2timerInit", "h2timerLib", M_h2timerLib,
	    sizeof(h2timerLibH2errMsgs)/sizeof(H2_ERROR),
	    h2timerLibH2errMsgs);

	h2timerInited = TRUE;
	return OK;
}


/******************************************************************************
*
*   h2timerEnd	-  Cleanup timers
*
*   Returns: OK or ERROR
*/
STATUS
h2timerEnd(void)
{
	int nTimer;

	if (h2timerInited != TRUE) return OK;

	/* Stop global watch dog */
	wdDelete (timerWd);

	semTake(timerMutex, WAIT_FOREVER);
	for (nTimer = 0; nTimer < NMAX_TIMERS; nTimer++) {
		semDelete(timerTab[nTimer].semSync);
		timerTab[nTimer].semSync = NULL;
	}
	semGive(timerMutex);

	/* Free global semaphore */
	semDelete(timerMutex);

	h2timerInited = FALSE;
	return OK;
}


/*****************************************************************************
*
*   h2timerAlloc  -  Allocate a timer
*
*   Description:
*   Allocate memory and initialize a timer structure
*
*   Returns: the new timer id or NULL
*/

H2TIMER_ID
h2timerAlloc(void)
{
	int nTimer;			/* index of the allocted timer */
	H2TIMER_ID timerId;		/* Timer identifier */

	if (h2timerInited != TRUE) {
		errnoSet(S_h2timerLib_TIMER_NOT_INIT);
		return NULL;
	}

	/* Start manipulating shared data */
	semTake(timerMutex, WAIT_FOREVER);

	timerId = timerTab;

	/* Find a free timer */
	for (nTimer = 0; timerId->flagInit == H2TIMER_FLG_INIT; nTimer++) {
		/* No free timer : error */
		if (nTimer == NMAX_TIMERS) {
			errnoSet (S_h2timerLib_TOO_MUCH_TIMERS);
			semGive(timerMutex);
			return (NULL);
		}
		timerId = (H2TIMER_ID) timerId + 1;
	} /* for */

	/* Fill the new structure */
	timerId->status = STOPPED_TIMER;
	timerId->flagInit = H2TIMER_FLG_INIT;

	/* Done with shared data */
	semGive(timerMutex);

	LOGDBG(("comLib:h2timerLib:h2timerAlloc: alloc %#x\n", timerId));
	return (timerId);
}

/******************************************************************************
*
*   h2timerStart  -  Start a timer
*
*   Description:
*   Starts a timer with a given delay
*   Warning: the period should be a multiple or a divisor of the
*	     maximum delay DELAY_MAX
*
*   Retourns: OK or ERROR
*/
STATUS
h2timerStart(H2TIMER_ID timerId,
    int period,
    int delay)
{
	/* Check if the timer is initialized */
	if (timerId->flagInit != H2TIMER_FLG_INIT) {
		errnoSet (S_h2timerLib_TIMER_NOT_INIT);
		return (ERROR);
	}

	/* You can only start a stopped timer */
	if (timerId->status != STOPPED_TIMER) {
		errnoSet (S_h2timerLib_NOT_STOPPED_TIMER);
		return (ERROR);
	}

	/* Check period's validity XXX */
	if ((period <= 0) ||
	    ((period > MAX_DELAY) && ((period%MAX_DELAY) != 0)) ||
	    ((period < MAX_DELAY) && ((MAX_DELAY%period) != 0))) {
		logMsg ("h2timerStart: "
		    "period (%d) must be a multiple or a divisor of %d\n",
		    period, MAX_DELAY);
		errnoSet (S_h2timerLib_BAD_PERIOD);
		return (ERROR);
	}

	/* Check delay */
	if (delay < 0 || delay >= MAX_DELAY) {
		errnoSet (S_h2timerLib_BAD_DELAY);
		return (ERROR);
	}

	/* Fill the timer structure */
	timerId->period = period;
	timerId->delay = delay;
	timerId->count = 0;
	timerId->status = WAIT_DELAY;

	LOGDBG(("h2timerStart %d %d\n", period, delay));
	return (OK);
}

/******************************************************************************
*
*   h2timerPause  -  Wait for timer expiration
*
*   Description:
*   Waits for timer expiration, given by freeing the synchronisation variable
*
*   Returns: OK or ERROR
*/
STATUS
h2timerPause(H2TIMER_ID timerId)
{
	/* Check for timer initialization */
	if (timerId->flagInit != H2TIMER_FLG_INIT) {
		errnoSet (S_h2timerLib_TIMER_NOT_INIT);
		return (ERROR);
	}

	/* Wait until the semaphore is released */
	LOGDBG(("h2timerPause: got lock\n"));
	semTake(timerId->semSync, WAIT_FOREVER);

	/*
	 * XXXX
	 * Flush the semaphore to make sure next call will block
	 */
	while (semTake(timerId->semSync, NO_WAIT) == OK)
		;
	/* Reset timer */
	timerId->count = 0;
	return (OK);

}

/******************************************************************************
*
*   h2timerPauseReset  -  Wait timer expiration and reset it
*
*   Description:
*   Waits for the timer expiration and then set it's value to 0 to keep
*   synchronisation
*
*   Returns: OK or ERROR
*/
STATUS
h2timerPauseReset(H2TIMER_ID timerId)
{
	/*  Check timer initialisation */
	if (timerId->flagInit != H2TIMER_FLG_INIT) {
		errnoSet (S_h2timerLib_TIMER_NOT_INIT);
		return (ERROR);
	}

	/* Wait until the semaphore is release */
	semTake (timerId->semSync, WAIT_FOREVER);

	timerId->count = 0;
	return (OK);
}

/******************************************************************************
*
*   h2timerStop	 -  Stop a timer
*
*
*   Returns: OK or ERROR
*/

STATUS
h2timerStop(H2TIMER_ID timerId)
{
	/* Check timer initialization */
	if (timerId->flagInit != H2TIMER_FLG_INIT) {
		errnoSet (S_h2timerLib_TIMER_NOT_INIT);
		return (ERROR);
	}

	/* Move to stopped state */
	timerId->status = STOPPED_TIMER;
	return (OK);
}

/******************************************************************************
*
*   h2timerChangePeriod	 -  Change the period of a timer
*
*
*   Returns: OK or ERROR
*/
STATUS
h2timerChangePeriod(H2TIMER_ID timerId, int period)
{
	/* Check timer initialization */
	if (timerId->flagInit != H2TIMER_FLG_INIT) {
		errnoSet (S_h2timerLib_TIMER_NOT_INIT);
		return (ERROR);
	}

	/* Check that the timer is running */
	if (timerId->status == STOPPED_TIMER) {
		errnoSet (S_h2timerLib_STOPPED_TIMER);
		return (ERROR);
	}

	/* Change period */
	timerId->period = period;
	/* Adjust current count if needed */
	timerId->count = timerId->count % period;
	return (OK);
}

/******************************************************************************
*
*   h2timerFree	 -  Free a timer structure
*
*   Retourns: OK or ERROR
*/

STATUS
h2timerFree(H2TIMER_ID timerId)
{
	/* Check that the timer is initialized */
	if (timerId->flagInit != H2TIMER_FLG_INIT) {
		errnoSet (S_h2timerLib_TIMER_NOT_INIT);
		return (ERROR);
	}

	/* Stop the timer and mark it not initialized */
	timerId->status = STOPPED_TIMER;
	timerId->flagInit = FALSE;
	return (OK);
}

/*****************************************************************************
*
*   timerInt  -	 Handler of the watchdog interrupt
*
*   Description:
*   Check for expired timers, release their synchronisation semaphores
*
*   Return: Nothing
*/
static void
timerInt(int arg)
{
	int nTimer;		  /* timer index */
	H2TIMER_ID timerId;	  /* Timer array pointer */

	LOGDBG(("comLib:h2timerLib:timerInt: delayCount %d\n", delayCount));

	/* Bump delay counter */
	if (++delayCount >= MAX_DELAY)
		delayCount = 0;

	/* Restart the watchdog */
	wdStart (timerWd, 1, (FUNCPTR) timerInt, 0);

	/* Loop on all timers */
	timerId = timerTab;
	for (nTimer = 0; nTimer < NMAX_TIMERS; nTimer++) {
		/* Only check initialized timers */
		if (timerId->flagInit == H2TIMER_FLG_INIT) {
			/* Timer running */
			if (timerId->status == RUNNING_TIMER) {
				LOGDBG((" -- count(%d) = %d\n", nTimer,
					timerId->count));

				/* bump counter */
				if (++timerId->count >= timerId->period) {
					/* reset it */
					timerId->count = 0;
					LOGDBG((" -- timer %d triggered\n",
						nTimer));

					/* Release the semaphore */
					semGive (timerId->semSync);

				}
			} else if (timerId->status >= WAIT_DELAY) {

				/* Waiting initial delay */

				/* If delay is reached */
				if (delayCount == timerId->delay) {
					/* reset the counter */
					timerId->count = 0;

					/* Start running */
					LOGDBG((" -- timer %d starting\n",
						nTimer));
					timerId->status = RUNNING_TIMER;
					semGive (timerId->semSync);
				}
			}
		}

		/* next */
		timerId++;
	}
}
