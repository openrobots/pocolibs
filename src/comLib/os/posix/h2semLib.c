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

#include "pocolibs-config.h"
__RCSID("$LAAS$");

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include "portLib.h"
#include "wdLib.h"
#include "tickLib.h"
#include "h2devLib.h"
#include "errnoLib.h"
#include "h2semLib.h"

#ifdef VALGRIND_SUPPORT
#include <valgrind/memcheck.h>
#endif

/***
 *** Semaphores a` la mode VxWorks pour Unix
 ***
 *** Utilise les semaphore des IPC systeme V
 ***/

#if defined(sun) || defined(__NetBSD__) || (defined(__linux) && defined(_SEM_SEMUN_UNDEFINED))
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};
#endif

static 
void h2semHandler(int sig)
{
}

/*----------------------------------------------------------------------*/

/**
 ** Allocation d'un tableau de semaphores
 **/
STATUS
h2semInit(int num, int *pSemId)
{
    key_t key;
    int i;
    union semun semun;    
    unsigned short tabval[MAX_SEM];
    int semId;
    struct semid_ds ds;
   
    /* Cree le tableau de semaphores */
    key = h2devGetKey(H2_DEV_TYPE_SEM, num, FALSE, NULL);
    if (key == ERROR) {
	return(ERROR);
    }
    if ((semId = semget(key, MAX_SEM, IPC_CREAT | PORTLIB_MODE)) == -1) {
	errnoSet(errno);
	return(ERROR);
    }

    /* (re)Definit son proprietaire 
     * Pour que tous les tableaux appartiennent a celui qui a fait le
     * h2 init */
    semun.buf = &ds;
    if (semctl(semId, 0, IPC_STAT, semun) == -1) {
	errnoSet(errno);
	return ERROR;
    }
    ds.sem_perm.uid = H2DEV_UID(0);
    if (semctl(semId, 0, IPC_SET, semun) == -1) {
	errnoSet(errno);
	return ERROR;
    }
    /* Initialise tous les semaphores */
    for (i = 0; i < MAX_SEM; i++) {
	tabval[i] = SEM_UNALLOCATED;	/* valeur initiale */
    }
    semun.array = tabval;
    if (semctl(semId, 0, SETALL, semun) == -1) {
	semun.val = 0;
	semctl(semId, 0, IPC_RMID, semun);
	errnoSet(errno);
	return(ERROR);
    }
    *pSemId = semId;
    return OK;
}


/*----------------------------------------------------------------------*/

void
h2semEnd(void)
{
    union semun semun;
    int i;
    
    for (i = 0; i < H2_DEV_MAX; i++) {
	if (H2DEV_TYPE(i) == H2_DEV_TYPE_SEM) {
	    /* Libere le tableau de semaphores */
	    semun.val = 0;
	    semctl(H2DEV_SEM_SEM_ID(i), 0, IPC_RMID, semun);
	    h2devFree(i);
	} 
    } /* for */
}

/*----------------------------------------------------------------------*/

/**
 ** allocation du semaphore 0 d'un tableau
 **/
STATUS
h2semCreate0(int semId, int value)
{
    union semun semun;    

    /* reset le semaphore */
    semun.val = value;
    if (semctl(semId, 0, SETVAL, semun) == -1) {
	errnoSet(errno);
	return ERROR;
    }
    return OK;
}

    
/*----------------------------------------------------------------------*/

/**
 ** allocation d'un semaphore
 **/
H2SEM_ID
h2semAlloc(int type)
{
    int i, j, dev;
    union semun semun;    
    unsigned short tabval[MAX_SEM];
#ifdef VALGRIND_SUPPORT
    VALGRIND_MAKE_READABLE(tabval, MAX_SEM * sizeof(unsigned short));
#endif
     BOOL trouve = FALSE;

    /* Verification du type */
    if (type != H2SEM_SYNC && type != H2SEM_EXCL) {
        errnoSet(S_h2semLib_BAD_SEM_TYPE);
        return ERROR;
    }
    /* Verrouille l'acces aux structures */
    h2semTake(0, WAIT_FOREVER);

    /* Recherche d'un semaphore libre dans un tableau */
    j = -1;				/* stupid gcc warning killer */
    for (i = 0; i < H2_DEV_MAX; i++) {
	if (H2DEV_TYPE(i) == H2_DEV_TYPE_SEM) {
	    /* Allocation d'un semaphore dans le tableau */
	    semun.array = tabval;
	    if (semctl(H2DEV_SEM_SEM_ID(i), 0, GETALL, semun) == -1) {
		errnoSet(errno);
		h2semGive(0);
		return ERROR;
	    }

           
	    for (j = 0; j < MAX_SEM; j++) {
		if (tabval[j] == SEM_UNALLOCATED) {
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
	/* initialise le semaphore */
	semun.val = type == H2SEM_SYNC ? SEM_EMPTY : SEM_FULL;
	semctl(H2DEV_SEM_SEM_ID(i), j, SETVAL, semun);
	h2semGive(0);
	return(i*MAX_SEM+j);
    }
    
    h2semGive(0);
    /* plus de semaphores libre, allocation d'un nouveau tableau */
    /* allocation d'un nouveau device */
    dev = h2devAlloc("h2semLib", H2_DEV_TYPE_SEM);
    if (dev == ERROR) {
	return ERROR;
    }
    h2semTake(0, WAIT_FOREVER);
    /* Allocation d'un nouveau tableau */
    if (h2semInit(dev, &(H2DEV_SEM_SEM_ID(dev))) == ERROR) {
	h2semGive(0);
	return ERROR;
    }
    if (h2semCreate0(H2DEV_SEM_SEM_ID(dev), 
		     type == H2SEM_SYNC ? SEM_EMPTY : SEM_FULL) == ERROR) {
	h2semGive(0);
	return ERROR;
    }
    h2semGive(0);
    return dev*MAX_SEM;
}

/*----------------------------------------------------------------------*/

/**
 ** Destruction d'un semaphore
 **/
STATUS 
h2semDelete(H2SEM_ID sem)
{
    union semun semun;
    int dev;
    
    /* Calcul du device */
    dev = sem / MAX_SEM;
    sem = sem % MAX_SEM;
    /* reset le semaphore */
    semun.val = SEM_UNALLOCATED;
    semctl(H2DEV_SEM_SEM_ID(dev), sem, SETVAL, semun);
    
    return(OK);
}

/*----------------------------------------------------------------------*/
/**
 ** Prise d'un semaphore
 **/

BOOL
h2semTake(H2SEM_ID sem, int timeout)
{
    struct  sembuf op;
    WDOG_ID timer;
    unsigned long ticks;
    int dev;

    dev = sem / MAX_SEM;
    sem = sem % MAX_SEM;

    op.sem_num = sem;
    op.sem_op = -1;
    /* Dans comLib timeout = 0 signifie bloquant */
    if (timeout == 0) 
	timeout = WAIT_FOREVER;

    switch (timeout) {
      case WAIT_FOREVER:
	op.sem_flg = 0;
	while (1) {
	    if (semop(H2DEV_SEM_SEM_ID(dev), &op, 1) < 0) {
		if (errno == EINTR) {
		    continue;
		} else {
		    return FALSE;
		}
	    } else {
		break;
	    }
	} /* while */
	break;
	
      case 0:				
	/* Dans comLib, ce cas n'est pas possible */
	/* On laisse le code ici, pour reference, au cas ou */
	op.sem_flg = IPC_NOWAIT;
	if (semop(H2DEV_SEM_SEM_ID(dev), &op, 1) < 0) {
	    if (errno == EAGAIN) {
		errnoSet(S_h2semLib_TIMEOUT);
	    } else { 
		errnoSet(errno);
	    }
	    return FALSE;
	}
	break;
      default:
	/* Arme un timer pour gerer le timeout */
	timer=wdCreate();
	ticks = tickGet();
	wdStart(timer, timeout, (FUNCPTR)h2semHandler, 0);

	op.sem_flg = 0;
	
	while (1) {
	    if (semop(H2DEV_SEM_SEM_ID(dev), &op, 1) < 0) {
		if (errno == EINTR) {

		    if (tickGet() - ticks > timeout) {
			errnoSet(S_h2semLib_TIMEOUT);
			wdDelete(timer);
			return FALSE;
		    } else {
			continue;
		    }
		} else {
		    errnoSet(errno);
		    wdDelete(timer);
		    return FALSE;
		}
	    } else {
		break;
	    }
	} /* while */
	wdDelete(timer);
	break;
    } /* switch */
    
    return TRUE;
}

/*----------------------------------------------------------------------*/

/**
 ** Liberation d'un semaphore 
 **/
STATUS
h2semGive(H2SEM_ID sem)
{
    struct  sembuf op;
    int dev;

    dev = sem / MAX_SEM;
    sem = sem % MAX_SEM;

    op.sem_num = sem;
    op.sem_op = 1;
    op.sem_flg = 0;
    if (semop(H2DEV_SEM_SEM_ID(dev), &op, 1) < 0) {
	return(ERROR);
    }
    return(OK);
}

/*----------------------------------------------------------------------*/

/**
 ** Flush d'un semaphore 
 **/
BOOL
h2semFlush(H2SEM_ID sem)
{
    return h2semSet(sem, 0) == OK ? TRUE : FALSE;
}

/*----------------------------------------------------------------------*/

STATUS
h2semShow(H2SEM_ID sem)
{
    int val, dev, sem1;
    union semun semun;

    dev = sem / MAX_SEM;
    sem1 = sem % MAX_SEM;

    semun.val = 0;
    switch ((val = semctl(H2DEV_SEM_SEM_ID(dev), sem1, GETVAL, semun))) {
      case SEM_EMPTY:
	printf("%3d: SEM_EMPTY\n", (int)sem);
	break;
      case SEM_FULL:
	printf("%3d: SEM_FULL\n", (int)sem);
	break;
      case SEM_UNALLOCATED:
	printf("%3d: SEM_UNALLOCATED\n", (int)sem);
	break;
      default:
	printf("%3d: %d\n", (int)sem, val);
	break;
    } /* switch */
    return OK;
}

/*----------------------------------------------------------------------*/
STATUS
h2semSet(H2SEM_ID sem, int value)
{
    union semun semun;
    int dev;

    dev = sem / MAX_SEM;
    sem = sem % MAX_SEM;
    
    semun.val = value;
    if (semctl(H2DEV_SEM_SEM_ID(dev), sem, SETVAL, semun) == -1) {
	return ERROR;
    }
    return OK;
}

