/*
 * Copyright (c) 1999, 2003 CNRS/LAAS
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

#include <pthread.h>
#include <stdlib.h>
#include <errno.h>

#include "portLib.h"
#include "wdLib.h"
#include "errnoLib.h"
#include "objLib.h"
#include "tickLib.h"
#include "semLib.h"

/*
 * Create and initialize a binary semaphore 
 */
SEM_ID
semBCreate(int options, SEM_B_STATE initialState)
{
    sem_t *sem;
    int status;

    sem = (sem_t *)malloc(sizeof(sem_t));
    if (sem == NULL) {
	return NULL;
    }
    status = sem_init(sem, 0, initialState);
    if (status != 0) {
	errnoSet(errno);
	return NULL;
    }
    return sem;
}

/*
 * Create and initialize a counting semaphore 
 */
SEM_ID
semCCreate(int options, int initialCount)
{
    sem_t *sem;
    int status;

    sem = (sem_t *)malloc(sizeof(sem_t));
    if (sem == NULL) {
	return NULL;
    }
    status = sem_init(sem, 0, initialCount);
    if (status != 0) {
	errnoSet(errno);
	return NULL;
    }
    return sem;
}

#ifdef notyet
/*
 * Create and initialize a mutex semaphore 
 */
SEM_ID
semMCreate(int options)
{
    /* Could be a mutex lock */
    return NULL;
}
#endif

/*
 * Destroy a semaphore
 */
STATUS
semDelete(SEM_ID semId)
{
    int status;
    
    status = sem_destroy(semId);
    if (status != 0) {
	errnoSet(errno);
	return(ERROR);
    }
    free(semId);
    return OK;
}

/*
 * Give a semaphore (V operation)
 */
STATUS
semGive(SEM_ID semId)
{
    int status;
    
    status = sem_post(semId);
    if (status != 0) {
	errnoSet(errno);
	return(ERROR);
    }
    return(OK);
}

/*
 * Dummy handler for watchdog
 */
static 
void semHandler(int sig)
{
}

/*
 * Take a semaphore (P operation)
 */
STATUS
semTake(SEM_ID semId, int timeout)
{
    WDOG_ID timer;
    unsigned long ticks;

    switch (timeout) {
      case WAIT_FOREVER:
	while (1) {
	    if (sem_wait(semId) < 0) {
		if (errno == EINTR) {
		    continue;
		} else {
		    errnoSet(errno);
		    return(ERROR);
		}
	    } else {
		break;
	    }
	}/* while */
	break;
	
      case 0:
	if (sem_trywait(semId) < 0) {
	    if (errno == EAGAIN) {
		errnoSet(S_objLib_OBJ_TIMEOUT);
		return ERROR;
	    } else {
		return ERROR;
	    }
	}
	break;
	
      default:
	timer = wdCreate();
	ticks = tickGet();
	wdStart(timer, timeout, (FUNCPTR)semHandler, 0);

	while (1) {
	    if (sem_wait(semId) < 0) {
		if (errno == EINTR) {
		    if (tickGet() - ticks > timeout) {
			errnoSet(S_objLib_OBJ_TIMEOUT);
			wdDelete(timer);
			return ERROR;
		    } else {
			continue;
		    }
		} else {
		    errnoSet(errno);
		    wdDelete(timer);
		    return(ERROR);
		}
	    } else {
		break;
	    }
	} /* while */
	wdDelete(timer);
	break;
    } /* switch */
    return OK;
}

/*
 * unblock every task pended on a semaphore 
 */
STATUS
semFlush(SEM_ID semId)
{
    while (sem_trywait(semId) < 0 && errno == EAGAIN) {
	sem_post(semId);
    }
    if (sem_post(semId) != 0) {
	return ERROR;
    } 
    return OK;
}

