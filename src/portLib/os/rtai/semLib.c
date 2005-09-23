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

#include <linux/slab.h>
#include <rtai_sem.h>

#include "portLib.h"
#include "wdLib.h"
#include "errnoLib.h"
#include "tickLib.h"
#include "semLib.h"

/* #define PORTLIB_DEBUG_SEMLIB */

#ifdef PORTLIB_DEBUG_SEMLIB
# define LOGDBG(x)	logMsg x
#else
# define LOGDBG(x)
#endif

#define SEM_TYPE_STR(x) \
   ((x)==BIN_SEM?"BIN":((x)==CNT_SEM?"CNT":((x)==RES_SEM?"RES":"UNK")))

/* semaphore structure */
struct SEM_ID {
   int type;
   SEM s;
};

extern int	sysClkTickDuration(void);

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
 * Create and initialize a binary semaphore 
 */
SEM_ID
semBCreate(int options, SEM_B_STATE initialState)
{
    SEM_ID sem;

    sem = kmalloc(sizeof(struct SEM_ID), GFP_KERNEL);
    if (sem == NULL) {
	return NULL;
    }

    /* we could use a binary semaphore but it would not be able to
     * represent the SEM_UNALLOCATED state. Using counting semaphores
     * is just ok as well. */
    rt_sem_init(&sem->s, initialState);
    sem->type = BIN_SEM;

    LOGDBG(("portLib:semLib:semBCreate: new binary sem 0x%p, type %s\n",
	    sem, SEM_TYPE_STR(sem->type)));

    return sem;
}

/*
 * Create and initialize a counting semaphore 
 */
SEM_ID
semCCreate(int options, int initialCount)
{
    SEM_ID sem;

    sem = kmalloc(sizeof(struct SEM_ID), GFP_KERNEL);
    if (sem == NULL) {
	return NULL;
    }

    rt_sem_init(&sem->s, initialCount);
    sem->type = CNT_SEM;

    LOGDBG(("portLib:semLib:semCCreate: new counting sem 0x%p, type %s\n",
	    sem, SEM_TYPE_STR(sem->type)));

    return sem;
}

/*
 * Create and initialize a mutex semaphore 
 */
SEM_ID
semMCreate(int options)
{
    SEM_ID sem;

    sem = kmalloc(sizeof(struct SEM_ID), GFP_KERNEL);
    if (sem == NULL) {
	return NULL;
    }

    rt_typed_sem_init(&sem->s, 1, RES_SEM);
    sem->type = RES_SEM;

    LOGDBG(("portLib:semLib:semMCreate: new mutex 0x%p, type %s\n",
	    sem, SEM_TYPE_STR(sem->type)));
    return sem;
}

/*
 * Destroy a semaphore
 */
STATUS
semDelete(SEM_ID semId)
{
    int status;
    
    if (semId->type == RES_SEM) {
       LOGDBG(("portLib:semLib:semDelete: checking mutex 0x%p state\n",
	       semId));
       if (rt_sem_wait_if(&semId->s) > 0) {
	  rt_sem_signal(&semId->s);
       } else {
	  errnoSet(S_semLib_RESOURCE_BUSY);
	  return(ERROR);
       }
    }

    LOGDBG(("portLib:semLib:semDelete: deleting sem 0x%p, type %s\n",
	    semId, SEM_TYPE_STR(semId->type)));

    status = rt_sem_delete(&semId->s);
    if (status == 0xffff) {
       errnoSet(S_semLib_NOT_A_SEM);
       return(ERROR);
    }
    kfree(semId);
    return OK;
}

/*
 * Give a semaphore (V operation)
 */
STATUS
semGive(SEM_ID semId)
{
    int status;
    
   LOGDBG(("portLib:semLib:semTake: signaling sem 0x%p, type %s\n",
	   semId, SEM_TYPE_STR(semId->type)));

    status = rt_sem_signal(&semId->s);
    if (status == 0xffff) {
	errnoSet(S_semLib_NOT_A_SEM);
	return(ERROR);
    }
    return(OK);
}


/*
 * Take a semaphore (P operation)
 */
STATUS
semTake(SEM_ID semId, int timeout)
{
   int status;

   LOGDBG(("portLib:semLib:semTake: waiting sem 0x%p, type %s, timeout %d\n",
	   semId, SEM_TYPE_STR(semId->type), timeout));

    switch (timeout) {
       case WAIT_FOREVER:
	  status = rt_sem_wait(&semId->s);
	  break;
	
       case NO_WAIT:
	  status = rt_sem_wait_if(&semId->s);
	  break;
	
       default:
	  status = rt_sem_wait_timed(&semId->s,
				     timeout * sysClkTickDuration());
	  break;
    } /* switch */

    switch(status) {
       case 0:
	  if (timeout == WAIT_FOREVER) break;

	  LOGDBG(("portLib:semLib:semTake: timeout sem 0x%p, type %s\n",
		  semId, SEM_TYPE_STR(semId->type)));

	  if (timeout == NO_WAIT)
	     errnoSet(S_semLib_RESOURCE_BUSY);
	  else
	     errnoSet(S_semLib_TIMEOUT);
	  return ERROR;

       case 0xffff:
	  errnoSet(S_semLib_NOT_A_SEM);
	  return ERROR;
    }


    LOGDBG(("portLib:semLib:semTake: got sem 0x%p, type %s\n",
	    semId, SEM_TYPE_STR(semId->type)));

    return OK;
}

/*
 * unblock every task pended on a semaphore 
 */
STATUS
semFlush(SEM_ID semId)
{
   while(rt_sem_wait_if(&semId->s) == 0)
      rt_sem_signal(&semId->s);

   if (rt_sem_signal(&semId->s) == 0xffff) {
      errnoSet(S_semLib_NOT_A_SEM);
      return ERROR;
   }

   return OK;
}
