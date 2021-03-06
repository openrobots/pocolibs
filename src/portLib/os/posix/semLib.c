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

#include <errno.h>
#ifdef USE_SEM_OPEN
#include <fcntl.h>
#endif
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "portLib.h"
#include "wdLib.h"
#include "errnoLib.h"
#include "tickLib.h"

#include "semLib.h"
const H2_ERROR semLibH2errMsgs[]   = SEM_LIB_H2_ERR_MSGS;

#include <semaphore.h>

#ifdef USE_SEM_OPEN
#define TEMPLATE "/tmp/semaphoreXXXXXXXXXX"
#endif

typedef struct ticket_lock {
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	unsigned long queue_head, queue_tail;
} ticket_lock_t;

#define TICKET_LOCK_INITIALIZER { \
	PTHREAD_COND_INITIALIZER, \
	PTHREAD_MUTEX_INITIALIZER \
}


/* semaphore structure */
struct SEM_ID {
   enum { SEM_T_BIN, SEM_T_CNT, SEM_T_MTX } type;
   union {
      sem_t *sem;
      ticket_lock_t ticket;
   } v;
#ifndef USE_SEM_OPEN
   sem_t sem_storage;
#else
   char name[32]; /* /tmp/semaphoreXXXXXXXXXX */
#endif
   int options;
};

/*
 * Record errors messages
 */
int
semRecordH2ErrMsgs()
{
    return h2recordErrMsgs("semRecordH2ErrMsg", "semLib", M_semLib, 
			   sizeof(semLibH2errMsgs)/sizeof(H2_ERROR), 
			   semLibH2errMsgs);
}

/*
 * simple ticket lock (mutex with fifo)
 * http://stackoverflow.com/questions/3050083/linux-synchronization-with-fifo-waiting-queue
 * from caf : http://stackoverflow.com/users/134633/caf
 *
 */
static int
ticket_init(ticket_lock_t *ticket)
{
	int result;
	result = pthread_mutex_init(&ticket->mutex, NULL);
	if (result != 0)
		return result;
	result = pthread_cond_init(&ticket->cond, NULL);
	if (result != 0) {
		pthread_mutex_destroy(&ticket->mutex);
		return result;
	}
	ticket->queue_head = ticket->queue_tail = 0;
	return 0;
}

static int
ticket_destroy(ticket_lock_t *ticket)
{
	int error;

	error = pthread_mutex_destroy(&ticket->mutex);
	if (error != 0)
		return error;
	return pthread_cond_destroy(&ticket->cond);
}

static int
ticket_lock(ticket_lock_t *ticket)
{
	unsigned long queue_me;
	int error;
	
	error = pthread_mutex_lock(&ticket->mutex);
	if (error != 0)
		return error;
	queue_me = ticket->queue_tail++;
	while (queue_me != ticket->queue_head) {
		error = pthread_cond_wait(&ticket->cond, &ticket->mutex);
		if (error != 0)
			break;
	}
	pthread_mutex_unlock(&ticket->mutex);
	return error;
}

static int
ticket_trylock(ticket_lock_t *ticket)
{
	unsigned long queue_me;
	int error = 0;

	error = pthread_mutex_trylock(&ticket->mutex);
	if (error != 0)
		return error;
	queue_me = ticket->queue_tail++;
	if (queue_me != ticket->queue_head)
		error = EBUSY;
	pthread_mutex_unlock(&ticket->mutex);
	return error;
}

static int
ticket_unlock(ticket_lock_t *ticket)
{
	int error;

	error = pthread_mutex_lock(&ticket->mutex);
	if (error != 0)
		return error;
	ticket->queue_head++;
	error = pthread_cond_broadcast(&ticket->cond);
	pthread_mutex_unlock(&ticket->mutex);
	return error;
}

#ifdef USE_SEM_OPEN
static int
_sem_init(SEM_ID sem, unsigned int value)
{ 
	int fd, saved_errno;

	strncpy(sem->name, TEMPLATE, sizeof(sem->name));
	fd = mkstemp(sem->name);
	if (fd == -1)
		return -1;
	
	close(fd);
	sem->v.sem = sem_open(sem->name, O_CREAT|O_EXCL, 0700, value);
	if (sem->v.sem == (sem_t *)SEM_FAILED) {
		saved_errno = errno;
		printf("sem_open returns SEM_FAILED errno %d\n", errno);
		unlink(sem->name);
		errno = saved_errno;
		return -1;
	}
	return 0;
}
#endif

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
#ifndef USE_SEM_OPEN
    sem->v.sem = &sem->sem_storage;
    status = sem_init(sem->v.sem, 0, initialState);
#else
    status = _sem_init(sem, initialState);
#endif
    if (status != 0) {
	errnoSet(errno);
        free(sem);
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
#ifndef USE_SEM_OPEN
    sem->v.sem = &sem->sem_storage;
    status = sem_init(sem->v.sem, 0, initialCount);
#else
    status = _sem_init(sem, initialCount);
#endif
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
    status = ticket_init(&sem->v.ticket);
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
         status = ticket_destroy(&semId->v.ticket);
         break;

       default:
#ifndef USE_SEM_OPEN
         status = sem_destroy(semId->v.sem);
#else
	 sem_unlink(semId->name);
	 status = sem_close(semId->v.sem);
#endif
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
         status = ticket_unlock(&semId->v.ticket);
         break;

       default:          
	 status = sem_post(semId->v.sem);
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
		 status = ticket_lock(&semId->v.ticket);
		 break;

	      default:
		 status = sem_wait(semId->v.sem);
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
	       status = ticket_trylock(&semId->v.ticket);
	       break;

	    default:
	       e = EAGAIN;
	       status = sem_trywait(semId->v.sem);
	       break;
	 }
	 if (status != 0) {
	    if (errno == e) {
		errnoSet(S_semLib_TIMEOUT);
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
		 status = ticket_lock(&semId->v.ticket);
		 break;

	      default:
		 status = sem_wait(semId->v.sem);
		 break;
	   }
	   if (status != 0) {
		if (errno == EINTR) {
		    if (tickGet() - ticks > timeout) {
			errnoSet(S_semLib_TIMEOUT);
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
	 while (sem_trywait(semId->v.sem) < 0 && errno == EAGAIN) {
	    sem_post(semId->v.sem);
	 }
	 if (sem_post(semId->v.sem) != 0) {
	    return ERROR;
	 }
	 return OK;
   }
}
