/*
 * Copyright (c) 2004 
 *      Autonomous Systems Lab, Swiss Federal Institute of Technology.
 * Copyright (c) 1990, 2003 CNRS/LAAS
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

#include <rtai_sem.h>

#include "taskLib.h"
#include "h2devLib.h"
#include "errnoLib.h"
#include "h2semLib.h"

/* #define COMLIB_DEBUG_H2SEMLIB */

#ifdef COMLIB_DEBUG_H2SEMLIB
# define LOGDBG(x)	logMsg x
#else
# define LOGDBG(x)
#endif

static struct {
   /* option bits */
   unsigned allocated:1;/* initialized */
   unsigned unpriv:1;	/* created by a PORTLIB_UNPRIVILEGED task */

   /* actual sem */
   SEM s;
} semPool[H2_DEV_MAX][MAX_SEM];

/* true if task `tid' had PORTLIB_UNPRIVILEGED bit set */
static inline int isunprivtask(long tid)
{
   return 0;
#if 0 /* XXX disable this for now */
   int opt;

   if (taskOptionsGet(tid, &opt) == ERROR) return 0;
   return (opt & PORTLIB_UNPRIVILEGED)? 1 : 0;
#endif
}

extern int	sysClkTickDuration(void);


/*----------------------------------------------------------------------*/

/**
 ** Create an array of semaphores
 **/
STATUS
h2semInit(int num, int *pSemId)
{
   key_t key;
   int i;

   if (num < 0 || num >= H2_DEV_MAX) return ERROR;

   /* Check that device doesn't exist yet */
   key = h2devGetKey(H2_DEV_TYPE_SEM, num, FALSE, NULL);

   /* Initialize semaphores to SEM_UNALLOCATED */
   for(i=0; i<MAX_SEM; i++) {
      semPool[num][i].allocated = 0;
   }
   
   *pSemId = num;
   return OK;
}


/*----------------------------------------------------------------------*/

void
h2semEnd(void)
{
   int i,j;

   /* Free all semaphores - do this in reverse orser so that sem#0
    * remains allocated until the very end of the job (h2devFree needs
    * it) */

   for (i = H2_DEV_MAX-1; i >= 0; i--) {
      if (H2DEV_TYPE(i) == H2_DEV_TYPE_SEM) {

	 /* This is ok even for i==0, but beware... */
	 h2devFree(i);

	 /* Free semaphores */
	 for(j=0; j<MAX_SEM; j++)
	    if (semPool[i][j].allocated) {
	       rt_sem_delete(&semPool[i][j].s);
	       semPool[i][j].allocated = 0;
	    }
      }
   } /* for */
}

/*----------------------------------------------------------------------*/

/**
 ** allocate semaphore 0 of an array
 **/
STATUS
h2semCreate0(int semId, int value)
{
   rt_sem_init(&semPool[semId][0].s, value);
   semPool[semId][0].unpriv = isunprivtask(0);
   semPool[semId][0].allocated = 1;
   return OK;
}

    
/*----------------------------------------------------------------------*/

/**
 ** allocation d'un semaphore
 **/
H2SEM_ID
h2semAlloc(int type)
{
   int unpriv;
   int i, j = 0, dev;
   BOOL trouve = FALSE;

   /* Check type */
   if (type != H2SEM_SYNC && type != H2SEM_EXCL) {
      errnoSet(S_h2semLib_BAD_SEM_TYPE);
      return ERROR;
   }

   /* unprivileged tasks can create semaphores, but this involves
    * doing some privileged operations. We remove the bit it it is set
    * and restore it at the end. */
   unpriv = isunprivtask(0);
   if (unpriv) taskOptionsSet(0, PORTLIB_UNPRIVILEGED, 0);

   /* Lock devices */
   if (h2semTake(0, WAIT_FOREVER) != TRUE) {
      LOGDBG(("comLib:h2semAlloc: cannot wait sem#0\n"));
      if (unpriv) taskOptionsSet(0, 0, PORTLIB_UNPRIVILEGED);
      return ERROR;
   }

   /* Look for an empty sem in arrays */
   for (i = 0; i < H2_DEV_MAX; i++) {
      if (H2DEV_TYPE(i) == H2_DEV_TYPE_SEM) {
	 /* Create a semaphore */
	 for (j = 0; j < MAX_SEM; j++) {
	    if (!semPool[i][j].allocated) {
	       trouve = TRUE;
	       break;
	    }
	 } /* for */
	 if (trouve) {
	    break;
	 }
      }
   } /* for */

   if (trouve) {
      LOGDBG(("comLib:h2semAlloc: allocating sem #%d\n", i*MAX_SEM+j));

      /* initialize semaphore */
      rt_sem_init(&semPool[i][j].s,
		  type == H2SEM_SYNC ? SEM_EMPTY : SEM_FULL);
      semPool[i][j].unpriv = unpriv;
      semPool[i][j].allocated = 1;
      h2semGive(0);
      if (unpriv) taskOptionsSet(0, 0, PORTLIB_UNPRIVILEGED);
      return(i*MAX_SEM+j);
   }
    
   h2semGive(0);

   /* no more free semaphores, allocate a new array in a new device */
   dev = h2devAlloc("h2semLib", H2_DEV_TYPE_SEM);
   if (dev == ERROR) {
      if (unpriv) taskOptionsSet(0, 0, PORTLIB_UNPRIVILEGED);
      return ERROR;
   }
   if (h2semTake(0, WAIT_FOREVER) != TRUE) {
      LOGDBG(("comLib:h2semAlloc: cannot wait sem#0\n"));

      if (unpriv) taskOptionsSet(0, 0, PORTLIB_UNPRIVILEGED);
      return ERROR;
   }
   
   if (h2semInit(dev, &(H2DEV_SEM_SEM_ID(dev))) == ERROR) {
      h2semGive(0);

      if (unpriv) taskOptionsSet(0, 0, PORTLIB_UNPRIVILEGED);
      return ERROR;
   }
   if (h2semCreate0(H2DEV_SEM_SEM_ID(dev), 
		    type == H2SEM_SYNC ? SEM_EMPTY : SEM_FULL) == ERROR) {
      h2semGive(0);
      if (unpriv) taskOptionsSet(0, 0, PORTLIB_UNPRIVILEGED);
      return ERROR;
   }

   semPool[dev][0].unpriv = unpriv;
   h2semGive(0);
   if (unpriv) taskOptionsSet(0, 0, PORTLIB_UNPRIVILEGED);

   LOGDBG(("comLib:h2semAlloc: allocated sem #%d\n", dev*MAX_SEM));
   return dev*MAX_SEM;
}

/*----------------------------------------------------------------------*/

/**
 ** Destroy a semaphore
 **/
STATUS 
h2semDelete(H2SEM_ID sem)
{
   int dev;
    
   LOGDBG(("comLib:h2semDelete: deleting sem #%d\n", sem));
   
   /* Compute device number */
   dev = sem / MAX_SEM;
   sem = sem % MAX_SEM;

   /* user-space applications cannot destroy kernel semaphores */
   if (isunprivtask(0) && !semPool[dev][sem].unpriv) {
      errnoSet(S_h2semLib_PERMISSION_DENIED);
      return ERROR;
   }

   /* reset semaphore */
   semPool[dev][sem].allocated = 0;
   rt_sem_delete(&semPool[dev][sem].s);

   return OK;
}

/*----------------------------------------------------------------------*/
/**
 ** Wait for a semaphore
 **/

BOOL
h2semTake(H2SEM_ID sem, int timeout)
{
   int status;
   int dev;

   LOGDBG(("comLib:h2semTake: waiting sem #%d, timeout %d\n", sem, timeout));

   dev = sem / MAX_SEM;
   sem = sem % MAX_SEM;

   if (!semPool[dev][sem].allocated) {
      LOGDBG(("comLib:h2semTake: sem #%d does not exist\n", dev*MAX_SEM+sem));
      errnoSet(S_h2semLib_NOT_A_SEM);
      return ERROR;
   }

   /* user-space applications and kernel cannot share semaphores */
   if (isunprivtask(0) != semPool[dev][sem].unpriv) {
      LOGDBG(("comLib:h2semTake: trying to take %sprivileged sem #%d "
	      "from %sprivileged task\n", semPool[dev][sem].unpriv?"un":"",
	      dev*MAX_SEM+sem,  isunprivtask(0)?"":"un"));
      errnoSet(S_h2semLib_PERMISSION_DENIED);
      return ERROR;
   }

   /* For comLib, timeout = 0 is blocking mode */
   if (timeout == 0) 
      timeout = WAIT_FOREVER;

   switch (timeout) {
      case WAIT_FOREVER:
	 status = rt_sem_wait(&semPool[dev][sem].s);
	 break;
	
      case 0:
	 status = rt_sem_wait_if(&semPool[dev][sem].s);
	 break;
	
      default:
	 status = rt_sem_wait_timed(&semPool[dev][sem].s,
				    timeout * sysClkTickDuration());
	 break;
   } /* switch */

   switch(status) {
      case 0:
	 if (timeout == WAIT_FOREVER) break;

	 errnoSet(S_h2semLib_TIMEOUT);
	 return FALSE;

      case 0xffff:
	 LOGDBG(("comLib:h2semTake: sem #%d not a sem\n", dev*MAX_SEM+sem));
	 errnoSet(S_h2semLib_NOT_A_SEM);
	 return ERROR;
   }

   LOGDBG(("comLib:h2semTake: got sem #%d\n", dev*MAX_SEM+sem));
   return TRUE;
}

/*----------------------------------------------------------------------*/

/**
 ** Signal a semaphore 
 **/
STATUS
h2semGive(H2SEM_ID sem)
{
   int dev;

   LOGDBG(("comLib:h2semGive: signaling sem #%d\n", sem));

   dev = sem / MAX_SEM;
   sem = sem % MAX_SEM;

   /* user-space applications and kernel cannot share semaphores */
   if (isunprivtask(0) != semPool[dev][sem].unpriv) {
      LOGDBG(("comLib:h2semGive: trying to give %sprivileged sem #%d "
	      "from %sprivileged task\n", semPool[dev][sem].unpriv?"un":"",
	      dev*MAX_SEM+sem,  isunprivtask(0)?"":"un"));
      errnoSet(S_h2semLib_PERMISSION_DENIED);
      return ERROR;
   }

   if (rt_sem_signal(&semPool[dev][sem].s) == 0xffff) {
      LOGDBG(("comLib:h2semGive: sem #%d not a sem\n", dev*MAX_SEM+sem));
      errnoSet(S_h2semLib_NOT_A_SEM);
      return ERROR;
   }

   return OK;
}

/*----------------------------------------------------------------------*/

/**
 ** Flush d'un semaphore 
 **/
BOOL
h2semFlush(H2SEM_ID sem)
{
   int dev;

   LOGDBG(("comLib:h2semFlush: flushing sem #%d\n", sem));

   dev = sem / MAX_SEM;
   sem = sem % MAX_SEM;
    
   /* user-space applications and kernel cannot share semaphores */
   if (isunprivtask(0) != semPool[dev][sem].unpriv) {
      LOGDBG(("comLib:h2semFlush: trying to flush %sprivileged sem #%d "
	      "from %sprivileged task\n", semPool[dev][sem].unpriv?"un":"",
	      dev*MAX_SEM+sem,  isunprivtask(0)?"":"un"));
      errnoSet(S_h2semLib_PERMISSION_DENIED);
      return ERROR;
   }

   while(rt_sem_wait_if(&semPool[dev][sem].s) == 0)
      rt_sem_signal(&semPool[dev][sem].s);

   if (rt_sem_signal(&semPool[dev][sem].s) == 0xffff) {
      LOGDBG(("comLib:h2semFlush: sem #%d not a sem\n", dev*MAX_SEM+sem));
      errnoSet(S_h2semLib_NOT_A_SEM);
      return ERROR;
   }

   return OK;
}

/*----------------------------------------------------------------------*/

STATUS
h2semShow(H2SEM_ID sem)
{
   int val, dev, sem1;

   dev = sem / MAX_SEM;
   sem1 = sem % MAX_SEM;

   if (!semPool[dev][sem].allocated) {
      printk("h2semLib: semaphore %3d: SEM_UNALLOCATED\n", (int)sem);
   } else {
      switch(val = semPool[dev][sem].s.count) {
	 case 0:
	    printk("h2semLib: semaphore %3d: SEM_EMPTY\n", (int)sem);
	    break;

	 case 1:
	    printk("h2semLib: semaphore %3d: SEM_FULL\n", (int)sem);
	    break;

	 default:
	    printk("h2semLib: semaphore %3d: count=%d\n", (int)sem, val);
	    break;
      }
   }

   return OK;
}
