/*
 * Copyright (c) 1990, 2003,2009 CNRS/LAAS
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
static const H2_ERROR h2semLibH2errMsgs[] = H2_SEM_LIB_H2_ERR_MSGS;

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

/**
 ** Record errors messages
 **/
int
h2semRecordH2ErrMsgs(void)
{
    return h2recordErrMsgs("h2semRecordH2ErrMsg", "h2semLib", M_h2semLib,
			   sizeof(h2semLibH2errMsgs)/sizeof(H2_ERROR),
			   h2semLibH2errMsgs);
}

/*----------------------------------------------------------------------*/

/**
 ** Allocation d'un tableau de semaphores
 **/
STATUS
#if USE_POSIX_SEMAPHORES
h2semInit(int num)
#else
h2semInit(int num, int *pSemId)
#endif
{
#if USE_POSIX_SEMAPHORES
    int i;

    /* initialize all semaphores to empty, except number #0, because number #0
     * will be used in all cases after this function is called. This is not
     * really clean, since there is some hidden assumptions here, but given the
     * current API of h2semCreate0() it's simpler. */
    H2DEV_SEM_EMPTY(num)[0] = 0;
    for (i = 1; i < MAX_SEM; i++) {
      H2DEV_SEM_EMPTY(num)[i] = 1;
    }
#else
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
#endif
    return OK;
}


/*----------------------------------------------------------------------*/

void
h2semEnd(void)
{
#if USE_POSIX_SEMAPHORES
    int i, j;

    /* make sure not to clean device #0, sem #0 */
    for (i = 0; i < H2_DEV_MAX; i++) {
      if (H2DEV_TYPE(i) == H2_DEV_TYPE_SEM) {
	if (i>0) h2devFree(i);

	/* destroy all allocated semaphores */
	for(j=(i>0?0:1); j<MAX_SEM; j++) if (!H2DEV_SEM_EMPTY(i)[j])
	    sem_destroy(&H2DEV_SEM_SEM_PTR(i)[j]);
      }
    }
#else
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
#endif
}

/*----------------------------------------------------------------------*/

/**
 ** allocation du semaphore 0 d'un tableau
 **/
STATUS
#if USE_POSIX_SEMAPHORES
h2semCreate0(sem_t *semId, int value)
#else
h2semCreate0(int semId, int value)
#endif
{
#if USE_POSIX_SEMAPHORES
    if (sem_init(semId, 1, value) == -1) {
      errnoSet(errno);
      return ERROR;
    }
#else
    union semun semun;

    /* reset le semaphore */
    semun.val = value;
    if (semctl(semId, 0, SETVAL, semun) == -1) {
	errnoSet(errno);
	return ERROR;
    }
#endif
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
#if USE_SVR4_SEMAPHORES
    union semun semun;
    unsigned short tabval[MAX_SEM];
#endif
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
#if USE_POSIX_SEMAPHORES
	    for (j = 0; j < MAX_SEM; j++) {
	      if (H2DEV_SEM_EMPTY(i)[j]) {
		trouve = TRUE;
		break;
	      }
	    }
#else
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
#endif
	    if (trouve) {
		break;
	    }
	}
    } /* for */

    if (trouve) {
	/* initialise le semaphore */
#if USE_POSIX_SEMAPHORES
	if (type == H2SEM_SYNC) {
	  sem_init(&H2DEV_SEM_SEM_PTR(i)[j], 1, 0);
	} else {
	  sem_init(&H2DEV_SEM_SEM_PTR(i)[j], 1, 1);
	}
	H2DEV_SEM_EMPTY(i)[j] = 0;
#else
	semun.val = type == H2SEM_SYNC ? SEM_EMPTY : SEM_FULL;
	semctl(H2DEV_SEM_SEM_ID(i), j, SETVAL, semun);
#endif
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
    if (
      h2semInit(dev
#if USE_SVR4_SEMAPHORES
		, &(H2DEV_SEM_SEM_ID(dev))
#endif
	) == ERROR
      ) {
	h2semGive(0);
	return ERROR;
    }
    if (h2semCreate0(
#if USE_POSIX_SEMAPHORES
	  H2DEV_SEM_SEM_PTR(dev)
#else
	  H2DEV_SEM_SEM_ID(dev)
#endif
	  , type == H2SEM_SYNC ? SEM_EMPTY : SEM_FULL) == ERROR) {
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
#if USE_SVR4_SEMAPHORES
    union semun semun;
#endif
    int dev;

    /* Calcul du device */
    dev = sem / MAX_SEM;
    sem = sem % MAX_SEM;
    /* reset le semaphore */
#if USE_POSIX_SEMAPHORES
    sem_destroy(&H2DEV_SEM_SEM_PTR(dev)[sem]);
    H2DEV_SEM_EMPTY(dev)[sem] = 1;
#else
    semun.val = SEM_UNALLOCATED;
    semctl(H2DEV_SEM_SEM_ID(dev), sem, SETVAL, semun);
#endif

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
	  if (
#if USE_POSIX_SEMAPHORES
	    sem_wait(&H2DEV_SEM_SEM_PTR(dev)[sem]) < 0
#else
	    semop(H2DEV_SEM_SEM_ID(dev), &op, 1) < 0
#endif
	    ) {
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
	if (
#if USE_POSIX_SEMAPHORES
	  sem_trywait(&H2DEV_SEM_SEM_PTR(dev)[sem]) < 0
#else
	  semop(H2DEV_SEM_SEM_ID(dev), &op, 1) < 0
#endif
	  ) {
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
	    if (
#if USE_POSIX_SEMAPHORES
	      sem_wait(&H2DEV_SEM_SEM_PTR(dev)[sem]) < 0
#else
	      semop(H2DEV_SEM_SEM_ID(dev), &op, 1) < 0
#endif
	      ) {
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
#if USE_POSIX_SEMAPHORES
    int dev;

    dev = sem / MAX_SEM;
    sem = sem % MAX_SEM;

    if (sem_trywait(&H2DEV_SEM_SEM_PTR(dev)[sem]) < 0)
      if (errno != EAGAIN) return ERROR;

    if (sem_post(&H2DEV_SEM_SEM_PTR(dev)[sem]) < 0)
      return ERROR;

    return OK;
#else
    return h2semSet(sem, 1);
#endif
}

/*----------------------------------------------------------------------*/

/**
 ** Flush d'un semaphore
 **/
BOOL
h2semFlush(H2SEM_ID sem)
{
#if USE_POSIX_SEMAPHORES
    int dev;

    dev = sem / MAX_SEM;
    sem = sem % MAX_SEM;

    while (!sem_trywait(&H2DEV_SEM_SEM_PTR(dev)[sem]))
      /* empty body */;

    return TRUE;
#else
    return h2semSet(sem, 0) == OK ? TRUE : FALSE;
#endif
}

/*----------------------------------------------------------------------*/

STATUS
h2semShow(H2SEM_ID sem)
{
    int val, dev, sem1;
#if USE_SVR4_SEMAPHORES
    union semun semun;
#endif

    dev = sem / MAX_SEM;
    sem1 = sem % MAX_SEM;

#if USE_POSIX_SEMAPHORES
    if (H2DEV_SEM_EMPTY(dev)[sem])
      val = SEM_UNALLOCATED;
    else if (sem_getvalue(&H2DEV_SEM_SEM_PTR(dev)[sem], &val))
      return ERROR;

    switch(val) {
      case 0:
	printf("%3d: SEM_EMPTY\n", (int)sem);
	break;
      case 1:
	printf("%3d: SEM_FULL\n", (int)sem);
	break;
      case SEM_UNALLOCATED:
	printf("%3d: SEM_UNALLOCATED\n", (int)sem);
	break;

      default:
	printf("%3d: %d\n", (int)sem, val);
	break;
    }
#else
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
#endif

    return OK;
}

/*----------------------------------------------------------------------*/
STATUS
h2semSet(H2SEM_ID sem, int value)
{
#if USE_SVR4_SEMAPHORES
    union semun semun;
#endif
    int dev;

    dev = sem / MAX_SEM;
    sem = sem % MAX_SEM;

#if USE_POSIX_SEMAPHORES
    if (value)
      h2semGive(sem);
    else
      h2semFlush(sem);
#else
    semun.val = value;
    if (semctl(H2DEV_SEM_SEM_ID(dev), sem, SETVAL, semun) == -1) {
	return ERROR;
    }
#endif
    return OK;
}
