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

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/param.h>
#include <sys/utsname.h>
#include <sys/ipc.h>
#include <sys/shm.h>


#include <rtai_sem.h>
#include <rtai_nam2num.h>

#include "portLib.h"
#include "errnoLib.h"
#include "semLib.h"

#define NAME_FIRST_SEM 0

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
   SEM *s;
  long int name;
};

extern int	sysClkTickDuration(void);
#ifdef PORTLIB_DEBUG_SEMLIB
extern SEM_ID wdMutex;
#endif

/* not static because it is used in h2semLib */
long int semLibGetNewName(void);

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

    sem = malloc(sizeof(struct SEM_ID));
    if (sem == NULL) {
	errnoSet(S_semLib_ALLOC_ERROR);
	LOGDBG(("portLib:semLib:semBCreate: S_semLib_ALLOC_ERROR\n"));
	return NULL;
    }

    /* we could use a binary semaphore but it would not be able to
     * represent the SEM_UNALLOCATED state. Using counting semaphores
     * is just ok as well. */
    sem->name = semLibGetNewName();
    if (sem->name == -1)
      {
	errnoSet(S_semLib_ALLOC_ERROR);
	LOGDBG(("portLib:semLib:semBCreate: S_semLib_GNN_ERROR\n"));
	return NULL;
      }

    sem->s = rt_sem_init(sem->name, initialState);

    if (sem->s == NULL) {
	errnoSet(S_semLib_ALLOC_ERROR);
	LOGDBG(("portLib:semLib:semBCreate: S_semLib_SEMINIT_ERROR, num=%d\n",
		sem->name));
	return NULL;
    }

    sem->type = BIN_SEM;

    LOGDBG(("portLib:semLib:semBCreate: new binary sem %d, type %s\n",
	    sem->name, SEM_TYPE_STR(sem->type)));


    return sem;
}

/*
 * Create and initialize a counting semaphore 
 */
SEM_ID
semCCreate(int options, int initialCount)
{
    SEM_ID sem;

    sem = malloc(sizeof(struct SEM_ID));
    if (sem == NULL) {
	errnoSet(S_semLib_ALLOC_ERROR);
	LOGDBG(("portLib:semLib:semCCreate: S_semLib_ALLOC_ERROR\n"));
	return NULL;
    }

    sem->name = semLibGetNewName();
    if (sem->name == -1)
      {
	errnoSet(S_semLib_ALLOC_ERROR);
	LOGDBG(("portLib:semLib:semCCreate: S_semLib_GNN_ERROR\n"));
	return NULL;
      }

    sem->s = rt_sem_init(sem->name, initialCount);

    if (sem->s == NULL) {
	errnoSet(S_semLib_ALLOC_ERROR);
	LOGDBG(("portLib:semLib:semCCreate: S_semLib_SEMINIT_ERROR, num=%d\n",
		sem->name));
	return NULL;
    }

    sem->type = CNT_SEM;

    LOGDBG(("portLib:semLib:semCCreate: new counting sem %d, type %s\n",
	    sem->name, SEM_TYPE_STR(sem->type)));

    return sem;
}

/*
 * Create and initialize a mutex semaphore 
 */
SEM_ID
semMCreate(int options)
{
    SEM_ID sem;

    sem = malloc(sizeof(struct SEM_ID));
    if (sem == NULL) {
	errnoSet(S_semLib_ALLOC_ERROR);
	LOGDBG(("portLib:semLib:semMCreate: S_semLib_ALLOC_ERROR\n"));
	return NULL;
    }

    sem->name = semLibGetNewName();
    if (sem->name == -1)
      {
	errnoSet(S_semLib_ALLOC_ERROR);
	LOGDBG(("portLib:semLib:semMCreate: S_semLib_GNN_ERROR\n"));
	return NULL;
      }

    sem->s = rt_typed_sem_init(sem->name, 1, RES_SEM);

    if (sem->s == NULL) {
	errnoSet(S_semLib_LXRT_ERROR);
	LOGDBG(("portLib:semLib:semMCreate: S_semLib_SEMINIT_ERROR, num=%d\n",
		sem->name));
	return NULL;
    }

    sem->type = RES_SEM;

    LOGDBG(("portLib:semLib:semMCreate: new mutex %d, type %s\n",
	    sem->name, SEM_TYPE_STR(sem->type)));

    return sem;
}

/*
 * Destroy a semaphore
 */
STATUS
semDelete(SEM_ID semId)
{
    int status;
    
    LOGDBG(("portLib:semLib:semDelete: deleting sem %d, type %s\n",
	    semId->name, SEM_TYPE_STR(semId->type)));

    status = rt_sem_delete(semId->s);
    if (status == 0xffff) {
       errnoSet(S_semLib_NOT_A_SEM);
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

#if PORTLIB_DEBUG_SEMLIB
    if (semId != wdMutex) {
	LOGDBG(("portLib:semLib:semGive: signaling sem %d, type %s\n",
		semId->name, SEM_TYPE_STR(semId->type)));
    }
#endif
    status = rt_sem_signal(semId->s);
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

#ifdef PORTLIB_DEBUG_SEMLIB
   if (semId != wdMutex) {
       LOGDBG(("portLib:semLib:semTake: waiting sem %d, type %s, timeout %d\n",
	       semId->name, SEM_TYPE_STR(semId->type), timeout));
   }
#endif
    switch (timeout) {
       case WAIT_FOREVER:
	  status = rt_sem_wait(semId->s);
	  break;
	
       case NO_WAIT:
	  status = rt_sem_wait_if(semId->s);
	  break;
	
       default:
	  status = rt_sem_wait_timed(semId->s,
				     timeout * sysClkTickDuration());
	  break;
    } /* switch */

    switch(status) {
       case 0:
	  if (timeout == WAIT_FOREVER) break;

	  LOGDBG(("portLib:semLib:semTake: timeout sem %d, type %s\n",
		  semId->name, SEM_TYPE_STR(semId->type)));

	  if (timeout == NO_WAIT)
	     errnoSet(S_semLib_RESOURCE_BUSY);
	  else
	     errnoSet(S_semLib_TIMEOUT);
	  return ERROR;

       case 0xffff:
	  errnoSet(S_semLib_NOT_A_SEM);
	  return ERROR;
    }


    LOGDBG(("portLib:semLib:semTake: got sem %d, type %s\n",
	    semId->name, SEM_TYPE_STR(semId->type)));

    return OK;
}

/*
 * unblock every task pended on a semaphore 
 */
STATUS
semFlush(SEM_ID semId)
{
   while(rt_sem_wait_if(semId->s) == 0)
      rt_sem_signal(semId->s);

   if (rt_sem_signal(semId->s) == 0xffff) {
      errnoSet(S_semLib_NOT_A_SEM);
      return ERROR;
   }

   return OK;
}

/*
  compute the "name" of the allocated semaphore.
  If there is no more evailable name return -1
  or a number between 0 to 254.
*/
long int semLibGetNewName(void)
{
  unsigned long int semNum = NAME_FIRST_SEM + 1;

  static char initialized = 0;	/* 1=initialized */
  
  static SEM *semNumSem=NULL;

  static key_t key;
  static int shmid = -1;
  static unsigned long int *semNumPt=NULL;

  if (initialized == 0)
    {
      int shmExist=0;
      struct utsname uts;
      char *home;
      char semLibFileName[MAXPATHLEN];
      int fd;

      LOGDBG(("semLibGetNewName: initializing\n"));

      /* perhaps sem named "NAME_FIRST_SEM" already exists */
      semNumSem = rt_get_adr(0);
      if (semNumSem != NULL)
	{
	  LOGDBG(("semLibGetNewName: sem 0 already exist, ok\n"));
	}
      else
	{
	  semNumSem = rt_typed_sem_init(NAME_FIRST_SEM, 0, RES_SEM);
	  if (semNumSem == NULL)
	    {
	      LOGDBG(("semLibGetNewName: rt_typed_sem_init error\n"));
	      return -1;
	    }
	}

      /* get a token */
      home = getenv("H2DEV_DIR");
      if (home != NULL) 
	{
	  /* Verifie l'existence et le droit d'ecriture */
	  if (access(home, R_OK|W_OK|X_OK) < 0) 
	    {
	      home = NULL;
	    }
	}
      if (home == NULL) 
	{
	  /* Sinon, essaye HOME */
	  home = getenv("HOME");
	}
      if (home == NULL) 
	{
	  return -1;
	}
      if (uname(&uts) == -1) 
	{
	  return -1;
	}
      /* Test sur la longueur de la chaine */
      if (strlen(home)+strlen(uts.nodename)+strlen(".semLib")+3 > 
	  MAXPATHLEN) 
	{
	  return -1;
	}
      sprintf(semLibFileName, "%s/%s-%s", home, ".semLib", uts.nodename);

      fd = open(semLibFileName, O_WRONLY | O_CREAT , 0666);
      if (fd < 0) 
	{
	  return -1;
	}
      close(fd);

      key = ftok(semLibFileName, 1);
      if (key == -1)
	{
	  LOGDBG(("semLibGetNewName: ftok\n"));
	  return -1;
	}

      shmid = shmget(key, sizeof(long int),
		     IPC_CREAT  | IPC_EXCL | 0666); 
      if (shmid == -1)
	{
	  if (errno == EEXIST)
	    {
	      LOGDBG(("semLibGetNewName: shmget already exist, ok\n"));
	      shmid = shmget(key, sizeof(long int),
			     IPC_CREAT  | 0666);
	      if (shmid == -1)
		{
		  LOGDBG(("semLibGetNewName: shmget error\n"));
		  perror("semLibGetNewName:shmget error : ");
		  return -1;
		}
	      else
		{
		  shmExist = 1;
		}
	    }
	  else
	    {
	      LOGDBG(("semLibGetNewName: shmget error\n"));
	      perror("semLibGetNewName:shmget error : ");
	    }
	}

      semNumPt = (unsigned long int *)shmat(shmid, NULL, 0);
      if (semNumPt == NULL) 
	{
	  LOGDBG(("semLibGetNewName: shmat\n"));
	  return -1;
	}

      if (shmExist)
	{
	  semNum = *semNumPt + 1;
	  *semNumPt = semNum;
	}
      else
	{
	  semNum = *semNumPt = NAME_FIRST_SEM + 1;
	}

      /* initialization done */
      initialized = 1;      
      rt_sem_signal(semNumSem);

      LOGDBG(("semLibGetNewName: initialized\n"));
    }
  else
    {
      rt_sem_wait(semNumSem);
      (*semNumPt)++;
      semNum = *semNumPt;
      rt_sem_signal(semNumSem);
    }
      
  return (semNum);
}

