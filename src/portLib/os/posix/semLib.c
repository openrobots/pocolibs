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

#ifndef NO_POSIX_SEMAPHORES
#include <semaphore.h>
#else
#include "tw_sem.h"
#endif

/* semaphore structure */
struct SEM_ID {
   enum { SEM_T_BIN, SEM_T_CNT, SEM_T_MTX } type;
   union {
      sem_t sem;
      pthread_mutex_t mtx;
   } v;
};


/*
 * Create and initialize a binary semaphore 
 */
SEM_ID
semBCreate(int options, SEM_B_STATE initialState)
{
    SEM_ID sem;
    int status;

    sem = malloc(sizeof(struct SEM_ID));
    if (sem == NULL) {
	return NULL;
    }
    sem->type = SEM_T_BIN;
    status = sem_init(&sem->v.sem, 0, initialState);
    if (status != 0) {
        free(sem);
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
    SEM_ID sem;
    int status;

    sem = malloc(sizeof(struct SEM_ID));
    if (sem == NULL) {
	return NULL;
    }
    sem->type = SEM_T_CNT;
    status = sem_init(&sem->v.sem, 0, initialCount);
    if (status != 0) {
        free(sem);
	errnoSet(errno);
	return NULL;
    }
    return sem;
}

/*
 * Create and initialize a mutex semaphore 
 */
SEM_ID
semMCreate(int options)
{
    SEM_ID sem;
    int status;

    sem = malloc(sizeof(struct SEM_ID));
    if (sem == NULL) {
       return NULL;
    }
    sem->type = SEM_T_MTX;
    status = pthread_mutex_init(&sem->v.mtx, NULL);
    if (status != 0) {
       free(sem);
       errnoSet(status);
       return NULL;
    }

    return sem;
}

/*
 * Destroy a semaphore
 */
STATUS
semDelete(SEM_ID semId)
{
    int status;

    switch(semId->type) {
       case SEM_T_MTX:
         status = pthread_mutex_destroy(&semId->v.mtx);
         break;

       default:
         status = sem_destroy(&semId->v.sem);
         break;
    }

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
    
    switch(semId->type) {
       case SEM_T_MTX:
         status = pthread_mutex_unlock(&semId->v.mtx);
         break;

       default:          
         status = sem_post(&semId->v.sem);
         break;
    }
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
    int status, e;

    switch (timeout) {
      case WAIT_FOREVER:
	while (1) {
	   switch(semId->type) {
	      case SEM_T_MTX:
		 status = pthread_mutex_lock(&semId->v.mtx);
		 break;

	      default:
		 status = sem_wait(&semId->v.sem);
		 break;
	   }
	   if (status != 0) {
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
	 switch(semId->type) {
	    case SEM_T_MTX:
	       e = EBUSY;
	       status = pthread_mutex_trylock(&semId->v.mtx);
	       break;

	    default:
	       e = EAGAIN;
	       status = sem_trywait(&semId->v.sem);
	       break;
	 }
	 if (status != 0) {
	    if (errno == e) {
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
	   switch(semId->type) {
	      case SEM_T_MTX:
		 status = pthread_mutex_lock(&semId->v.mtx);
		 break;

	      default:
		 status = sem_wait(&semId->v.sem);
		 break;
	   }
	   if (status != 0) {
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
   switch(semId->type) {
      case SEM_T_MTX:
	 return ERROR; /* one should not flush a mutex? */
	 break;

      default:
	 while (sem_trywait(&semId->v.sem) < 0 && errno == EAGAIN) {
	    sem_post(&semId->v.sem);
	 }
	 if (sem_post(&semId->v.sem) != 0) {
	    return ERROR;
	 }
	 return OK;
   }
}