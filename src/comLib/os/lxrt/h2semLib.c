/*
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

/*
  there is only one semaphore per device
*/

#include "pocolibs-config.h"
__RCSID("$LAAS$");

#include <stdlib.h>

#include <rtai_sem.h>

#include "portLib.h"

#include "errnoLib.h"

#include "h2devLib.h"
#include "h2semLib.h"		/* include semLib.h */
const H2_ERROR h2semLibH2errMsgs[] = H2_SEM_LIB_H2_ERR_MSGS;

#define COMLIB_DEBUG_H2SEMLIB

#ifdef COMLIB_DEBUG_H2SEMLIB
#include <string.h>
#include "taskLib.h"
# define LOGDBG(x)	logMsg x
#else
# define LOGDBG(x)
#endif

/* to get unique number for the LXRT sem name */
extern long int semLibGetNewName(void);

/* for h2semTake timeout */
extern int	sysClkTickDuration(void);

/*----------------------------------------------------------------------*/

/**
 ** Allocation d'un tableau de semaphores
 **/
STATUS
h2semInit(int num, SEM *pSemId[])
{
  int semNum;

  if ((num < 0) || (num >= H2_DEV_MAX))
    return ERROR;

  /* Check that device doesn't exist yet */
  /* how ?? */
  
  /* Initialize semaphores to SEM_UNALLOCATED */
  for(semNum=0; semNum<MAX_SEM; semNum++) 
    {
      /* non allocated */
      pSemId[semNum] = (SEM *)NULL;
    }

  return OK;
}


/*----------------------------------------------------------------------*/

void
h2semEnd(void)
{
  int devNum,semNum;
  
  for (devNum=0; devNum < H2_DEV_MAX; devNum++)
    {
      if (H2DEV_TYPE(devNum) == H2_DEV_TYPE_SEM)
	{
	  h2devFree(devNum);

	  for (semNum = 0; semNum < MAX_SEM; semNum++)
	    {
	      /* free semaphore */
	      if (H2DEV_SEM_SEM_ID(devNum)[semNum] != (SEM *)NULL)
		{
		  rt_sem_delete(H2DEV_SEM_SEM_ID(devNum)[semNum]);
		  /* check status !!! */
		  H2DEV_SEM_SEM_ID(devNum)[semNum] = (SEM *)NULL;
		}
	    }
	}
    } /* for H2_DEV_MAX */
}

/*----------------------------------------------------------------------*/

/**
 ** allocation du semaphore 0 d'un tableau
 **/
STATUS
h2semCreate0(int devNum, int value)
{
  long int name;

  name = semLibGetNewName();
  if (name == -1)
    {
      LOGDBG(("comLib:h2semLib:h2semCreate: GNN_ERROR\n"));
      return ERROR;
    }

  H2DEV_SEM_SEM_ID(devNum)[0] = rt_sem_init(name, value);
  if (H2DEV_SEM_SEM_ID(devNum)[0] == (SEM *)NULL)
    {
      LOGDBG(("comLib:h2semLib:h2semCreate: SEMINIT_ERROR, num=%d\n",
	      name));
      return ERROR;
    }


  /* check status */

  return OK;
}

    
/*----------------------------------------------------------------------*/

/**
 ** allocation d'un semaphore
 **/
H2SEM_ID
h2semAlloc(int type)
{
  unsigned int devNum=0, semNum=0;
  unsigned int trouve=0;
  int dev=0;
  long int name;
#ifdef COMLIB_DEBUG_H2SEMLIB
  char task_name[12]={"oups\0"};
  long int task_id;

  task_id = taskIdSelf();

  if(task_id != 0)
    strcpy(task_name, taskName(task_id));
#endif   

  /* Check type */
  if (type != H2SEM_SYNC && type != H2SEM_EXCL) 
    {
      errnoSet(S_h2semLib_BAD_SEM_TYPE);
      return ERROR;
    }

  /* Lock devices */
  if (h2semTake(0, WAIT_FOREVER) != TRUE)
    {
      LOGDBG(("comLib:h2semAlloc: cannot wait sem#0\n"));
      return ERROR;
    }

  /* Look for an empty sem in arrays */
  for (devNum = 0; devNum < H2_DEV_MAX; devNum++)
    {
      if (H2DEV_TYPE(devNum) == H2_DEV_TYPE_SEM)
	{
	  /* Create a semaphore */
	  for (semNum = 0; semNum < MAX_SEM; semNum++)
	    {
	      if (H2DEV_SEM_SEM_ID(devNum)[semNum] == (SEM *)NULL)
		{
		  trouve = TRUE;
		  break;
		}
	    } /* for */
	 if (trouve)
	   {
	     break;
	   }
	}
    } /* for */

   if (trouve)
     {
       LOGDBG(("comLib:h2semAlloc: allocating sem #%d(%d,%d), (task \"%s\")\n",
	       devNum*MAX_SEM+semNum, devNum, semNum, task_name));

       /* initialize semaphore */
       name = semLibGetNewName();
       if (name == -1)
	 {
	   LOGDBG(("comLib:h2semAlloc: GNN_ERROR\n"));
	   return ERROR;
	 }

       H2DEV_SEM_SEM_ID(devNum)[semNum] = 
	 rt_sem_init(name, type == H2SEM_SYNC ? SEM_EMPTY : SEM_FULL);

       if (H2DEV_SEM_SEM_ID(devNum)[semNum] == (SEM *)NULL) 
	 {
	   LOGDBG(("comLib:h2semAlloc: SEMINIT_ERROR, num=%d\n",
		   name));
	   return ERROR;
	 }

       h2semGive(0);
       return(devNum*MAX_SEM+semNum);
     }

   h2semGive(0);

   /* no more free semaphores, allocate a new array in a new device */
   dev = h2devAlloc("h2semLib", H2_DEV_TYPE_SEM);
   if (dev == ERROR)
     {
       return ERROR;
     }
   if (h2semTake(0, WAIT_FOREVER) != TRUE) 
     {
       LOGDBG(("comLib:h2semAlloc: cannot wait sem#0\n"));

       return ERROR;
     }
   
   if (h2semInit(dev, &(H2DEV_SEM_SEM_ID(dev)[0])) == ERROR)
     {
       h2semGive(0);

       return ERROR;
     }
   if (h2semCreate0(dev, 
		    type == H2SEM_SYNC ? SEM_EMPTY : SEM_FULL) == ERROR) 
     {
       h2semGive(0);
       return ERROR;
     }

   h2semGive(0);

   LOGDBG(("comLib:h2semAlloc: allocated sem #%d\n", dev*MAX_SEM));
   return dev*MAX_SEM;

}

/*----------------------------------------------------------------------*/

/**
 ** Destruction d'un semaphore
 **/
STATUS 
h2semDelete(H2SEM_ID sem)
{
   int dev;
   int status;

   LOGDBG(("comLib:h2semDelete: deleting sem #%d (task \"titi\")\n", 
	   sem));

   /* Compute device number */
   dev = sem / MAX_SEM;
   sem = sem % MAX_SEM;

   /* reset semaphore */
   status = rt_sem_delete(H2DEV_SEM_SEM_ID(dev)[sem]);
   if (status == 0xffff)
     {
       errnoSet(S_h2semLib_NOT_A_SEM);
       return ERROR;
     }

   H2DEV_SEM_SEM_ID(dev)[sem] = (SEM *)NULL;
   
   return OK;
}

/*----------------------------------------------------------------------*/
/**
 ** Prise d'un semaphore
 **/

BOOL
h2semTake(H2SEM_ID sem, int timeout)
{
  int status;
  int dev;
#ifdef COMLIB_DEBUG_H2SEMLIB
  char task_name[12]={"oups\0"};
  long int task_id;

  task_id = taskIdSelf();

  if(task_id != 0)
    strcpy(task_name, taskName(task_id));
#endif   

  dev = sem / MAX_SEM;
  sem = sem % MAX_SEM;

  if (H2DEV_SEM_SEM_ID(dev)[sem] == (SEM *)NULL) {
    LOGDBG(("comLib:h2semTake: sem (%d,%d) does not exist\n",
	    dev, sem));
    errnoSet(S_h2semLib_NOT_A_SEM);
    return ERROR;
  }

  /* For comLib, timeout = 0 is blocking mode */
  if (timeout == 0) 
    timeout = WAIT_FOREVER;

  switch (timeout)
    {
    case WAIT_FOREVER:
      LOGDBG(("comLib:h2semTake: WAIT_FOREVER sem #%d(%d,%d), (task \"%s\")\n",
	      dev*MAX_SEM+sem, dev, sem, task_name));

      status = rt_sem_wait(H2DEV_SEM_SEM_ID(dev)[sem]);
      break;
	
    case NO_WAIT:
      LOGDBG(("comLib:h2semTake: NO_WAIT sem #%d(%d,%d), (task \"titi\")\n",
	      dev*MAX_SEM+sem, dev, sem));

      status = rt_sem_wait_if(H2DEV_SEM_SEM_ID(dev)[sem]);
      break;
	
    default:
      LOGDBG(("comLib:h2semTake: waiting sem #%d(%d,%d), timeout %d, (task \"titi\")\n", dev*MAX_SEM+sem, dev, sem, timeout));


      status = rt_sem_wait_timed(H2DEV_SEM_SEM_ID(dev)[sem],
				 timeout * sysClkTickDuration());
      break;
    } /* switch */

  switch(status) 
    {
    case 0:
      if (timeout == WAIT_FOREVER) 
	break;
      
      LOGDBG(("comLib:h2semTake: sem (%d,%d) TIMEOUT\n", dev, sem));
      errnoSet(S_h2semLib_TIMEOUT);
      return FALSE;
      
    case 0xffff:
      LOGDBG(("comLib:h2semTake: sem (%d,%d) not a sem\n", dev, sem));
      errnoSet(S_h2semLib_NOT_A_SEM);
      return ERROR;
    }
  
  LOGDBG(("comLib:h2semTake: got sem #%d(%d,%d), (task \"%s\")\n", 
	  dev*MAX_SEM+sem, dev, sem, task_name));
  return TRUE;
}

/*----------------------------------------------------------------------*/

/**
 ** Liberation d'un semaphore 
 **/
STATUS
h2semGive(H2SEM_ID sem)
{
  int dev;
  int status;
  
#ifdef COMLIB_DEBUG_H2SEMLIB
  char task_name[12]={"oups\0"};
  long int task_id;

  task_id = taskIdSelf();

  if(task_id != 0)
    strcpy(task_name, taskName(task_id));
#endif   

  dev = sem / MAX_SEM;
  sem = sem % MAX_SEM;

  LOGDBG(("comLib:h2semGive: giving sem #%d(%d,%d), (task \"%s\")\n",
	  dev*MAX_SEM+sem, dev, sem, task_name));

  if (H2DEV_SEM_SEM_ID(dev)[sem] == (SEM *)NULL)
    {
      LOGDBG(("comLib:h2semGive: sem #%d(%d,%d) NULL\n", 
	      dev*MAX_SEM+sem, dev,sem));
      errnoSet(S_h2semLib_NOT_A_SEM);
      return ERROR;
    }
  
  status = rt_sem_signal(H2DEV_SEM_SEM_ID(dev)[sem]);

  if (status == 0xffff)
    {
      LOGDBG(("comLib:h2semGive: sem (%d,%d) not a sem\n", dev,sem));
      errnoSet(S_h2semLib_NOT_A_SEM);
      return ERROR;
    }

  LOGDBG(("comLib:h2semGive: gived sem (%d,%d)\n", dev, sem));
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

  dev = sem / MAX_SEM;
  sem = sem % MAX_SEM;
  
  LOGDBG(("comLib:h2semFlush: flushing sem (%d,%d)\n", dev, sem));

  if (H2DEV_SEM_SEM_ID(dev)[sem] == (SEM *)NULL)
    {
      LOGDBG(("comLib:h2semFlush: sem (%d,%d) not a sem\n", dev, sem));
      errnoSet(S_h2semLib_NOT_A_SEM);
      return ERROR;
    }

  while(rt_sem_wait_if(H2DEV_SEM_SEM_ID(dev)[sem]) == 0)
      rt_sem_signal(H2DEV_SEM_SEM_ID(dev)[sem]);

  if (rt_sem_signal(H2DEV_SEM_SEM_ID(dev)[sem]) == 0xffff)
    {
      LOGDBG(("comLib:h2semFlush: sem #%d not a sem\n", 
	      dev*MAX_SEM+sem));
      errnoSet(S_h2semLib_NOT_A_SEM);
      return ERROR;
    }

  LOGDBG(("comLib:h2semFlush: sem flushed (%d,%d)\n", dev, sem));  
  return OK;  
}

/*----------------------------------------------------------------------*/

STATUS
h2semShow(H2SEM_ID sem)
{
  int dev;
  
  dev = sem / MAX_SEM;
  sem = sem % MAX_SEM;
  
  if (H2DEV_SEM_SEM_ID(dev)[sem] == NULL) 
    {
      logMsg("h2semLib: semaphore %3d: SEM_UNALLOCATED\n", (int)sem);
    } 
  else
    {
      logMsg("h2semLib: semaphore %3d: allocated\n", (int)sem);
    }

   return OK;

}

/*----------------------------------------------------------------------*/
STATUS
h2semSet(H2SEM_ID sem, int value)
{
  /* this is a hack - there's a race condition */
  if (value == 1) {
    h2semFlush(sem);
    return h2semGive(sem);
  }
  errnoSet(S_h2semLib_NOT_IMPLEMENTED);
  return ERROR;
}
