/*
 * Copyright (c) 1996, 2003-2004,2009 CNRS/LAAS
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

/***
 *** Poster lower layer for a local Unix machine , 
 *** using SYSV shared memory segments
 ***/
#include "pocolibs-config.h"

#include <sys/types.h>
#include <string.h>
#include <unistd.h>

#include "portLib.h"
#include "taskLib.h"
#include "smMemLib.h"
#include "smObjLib.h"
#include "errnoLib.h"
#include "h2semLib.h"
#include "h2devLib.h"
#include "posterLib.h"
#include "h2timeLib.h"

#include "posterLibPriv.h"

#ifdef VALGRIND_SUPPORT
#include <valgrind/memcheck.h>
#endif

static STATUS localPosterCreate ( const char *name, int size, POSTER_ID *pPosterId );
static STATUS localPosterMemCreate ( const char *name, int busSpace, void *pPool,
				     int size, POSTER_ID *pPosterId );
static STATUS localPosterDelete ( POSTER_ID posterId );
static STATUS localPosterFind ( const char *name, POSTER_ID *pPosterId );
static int localPosterWrite ( POSTER_ID posterId, int offset, void *buf, 
			      int nbytes );
static int localPosterRead ( POSTER_ID posterId, int offset, void *buf, 
			     int nbytes );
static STATUS localPosterTake ( POSTER_ID posterId, POSTER_OP op );
static STATUS localPosterGive ( POSTER_ID posterId );
static void * localPosterAddr ( POSTER_ID posterId );
static STATUS localPosterIoctl ( POSTER_ID posterId, int code, void *parg );
static STATUS localPosterShow ( void );
static STATUS localPosterSetEndianness(POSTER_ID posterId, 
				       H2_ENDIANNESS endianness);
static STATUS localPosterGetEndianness(POSTER_ID posterId, 
				       H2_ENDIANNESS *endianness);
static STATUS localPosterStats(void);

const POSTER_FUNCS posterLocalFuncs = {
    NULL,
    localPosterCreate,
    localPosterMemCreate,
    localPosterDelete,
    localPosterFind,
    localPosterWrite,
    localPosterRead,
    localPosterTake,
    localPosterGive,
    localPosterAddr,
    localPosterIoctl,
    localPosterShow,
    localPosterSetEndianness,
    localPosterGetEndianness,
    localPosterStats
};

/*----------------------------------------------------------------------*/


static STATUS
localPosterCreate(const char *name, int size, POSTER_ID *pPosterId)
{
    long dev;
    unsigned char *pool;
    
    if (pPosterId != NULL) {
	*pPosterId = NULL;
    }

    /* Allocation d'un h2dev */
    dev = h2devAlloc(name, H2_DEV_TYPE_POSTER);
    if (dev < 0) {
	return(ERROR);
    }
    /* Allocation memoire partagee */
    pool = smMemMalloc(size);
    if (pool == NULL) {
	errnoSet(S_posterLib_MALLOC_ERROR);
	h2devFree(dev);
	return ERROR;
    }
    /* Creation SEM */
    H2DEV_POSTER_SEM_ID(dev) = h2semAlloc(H2SEM_EXCL);
    if (H2DEV_POSTER_SEM_ID(dev) == ERROR) {
	smMemFree(pool);
	h2devFree(dev);
	return(ERROR);
    }    
    /* Memorise l'adresse globale */
    H2DEV_POSTER_POOL(dev) = smObjLocalToGlobal(pool);
   
    /* Memorise la taille */
    H2DEV_POSTER_SIZE(dev) = size;
    /* Memorise le pid du createur */
    H2DEV_POSTER_TASK_ID(dev) = getpid();
    /* Indiquer que les donnees ne sont pas fraiches */
    H2DEV_POSTER_FLG_FRESH(dev) = FALSE;
    /* Init endianness of the data of the poster to local value
       (will be changed in remote create procedure if necessary) */
    H2DEV_POSTER_ENDIANNESS(dev) = H2_LOCAL_ENDIANNESS;

    H2DEV_POSTER_READ_OPS(dev) = 0;
    H2DEV_POSTER_WRITE_OPS(dev) = 0;
    H2DEV_POSTER_READ_BYTES(dev) = 0;
    H2DEV_POSTER_WRITE_BYTES(dev) = 0;


    if (pPosterId != NULL) {
	*pPosterId = (POSTER_ID)dev;
    }
    return(OK);

} /* posterCreate */

/*----------------------------------------------------------------------*/

static STATUS 
localPosterMemCreate(
     const char *name,          /* Nom du device a creer */
     int busSpace,		/* espace d'adressage de l'addresse pPool */
     void *pPool,		/* adresse Pool de memoire pour le poster */
     int size,                  /* Taille poster - en bytes */
     POSTER_ID *pPosterId)      /* Ou` mettre l'id du poster */
{
    logMsg("posterMemCreate: pas encore supportee sur Unix\n");
    return(ERROR);
}

/*----------------------------------------------------------------------*/

static STATUS
localPosterDelete(POSTER_ID posterId)
{
    long dev = (long)posterId;
    unsigned char *pool;
    uid_t uid = getuid();

    if (dev < 0 || dev > H2_DEV_MAX || 
	H2DEV_TYPE(dev) != H2_DEV_TYPE_POSTER) {
	errnoSet(S_posterLib_POSTER_CLOSED);
	return(ERROR);
    }

    if (uid != H2DEV_UID(dev) && uid != H2DEV_UID(0)) {
	errnoSet(S_posterLib_NOT_OWNER);
	return ERROR;
    }
    /* Passer en local l'adresse du poster */
    pool = smObjGlobalToLocal(H2DEV_POSTER_POOL(dev));

    /* Liberer l'espace */
    smMemFree(pool);
    
    /* Destruction du semaphore */
    h2semDelete(H2DEV_POSTER_SEM_ID(dev));
    /* Liberation du device */
    h2devFree(dev);

    return(OK);

} /* posterDelete */

/*----------------------------------------------------------------------*/

static STATUS
localPosterFind(const char *name, POSTER_ID *pPosterId)
{
    long p;

    /* Recherche du device h2 correspondant */
    p = h2devFind(name, H2_DEV_TYPE_POSTER);
    if (p == ERROR) {
	return(ERROR);
    }
    /* Memorise le resultat */
    *pPosterId = (POSTER_ID)p;
    
#ifdef VALGRIND_SUPPORT
    // valgrind can't know if somebody has ever
    // written on this poster, so ...
    VALGRIND_MAKE_READABLE( localPosterAddr(*pPosterId), H2DEV_POSTER_SIZE(p));
#endif

     return(OK);

} /* posterFind */

/*----------------------------------------------------------------------*/

static int
localPosterWrite(POSTER_ID posterId, int offset, void *buf, int nbytes)
{
    long dev = (long)posterId;

    /* Prise du semaphore d'exclusion mutuelle */
    if (localPosterTake(posterId, POSTER_WRITE) == ERROR) {
	return(ERROR);
    }

    /* Ecrire les donnees dans le poster */
    memcpy((char *)localPosterAddr(posterId) + offset, buf, nbytes);
    
    /* Store statistics */
    H2DEV_POSTER_WRITE_OPS(dev)++;
    H2DEV_POSTER_WRITE_BYTES(dev) += nbytes;    

    /* liberer le semaphore d'exclusion mutuelle */
    localPosterGive(posterId);
    
    return(nbytes);

} /* posterWrite */

/*----------------------------------------------------------------------*/

static int 
localPosterRead(POSTER_ID posterId, int offset, void *buf, int nbytes)
{
    long dev = (long)posterId;
    int nRd;

    /* Calculer le nombre d'octets a lire */
    nRd = MIN(nbytes, H2DEV_POSTER_SIZE(dev) - offset);
    if (nRd <= 0) {
        errnoSet(S_posterLib_BAD_FORMAT);
	return 0;
    }
    /* Prendre le semaphore d'exclusion mutuelle */
    if (localPosterTake(posterId, POSTER_READ) == ERROR) {
	return(ERROR);
    }
    /* Copier les donnees */
    memcpy(buf, (char *)localPosterAddr(posterId) + offset, nbytes);

    /* statistics */
    H2DEV_POSTER_READ_OPS(dev)++;
    H2DEV_POSTER_READ_BYTES(dev) += nbytes;

    /* Liberer le semaphore */
    localPosterGive(posterId);
    
    return(nbytes);

} /* posterRead */

/*----------------------------------------------------------------------*/

static STATUS
localPosterTake(POSTER_ID posterId, POSTER_OP op)
{
    long dev = (long)posterId;

    if (dev < 0 || dev >= H2_DEV_MAX 
	|| H2DEV_TYPE(dev) != H2_DEV_TYPE_POSTER) {
	errnoSet(S_posterLib_POSTER_CLOSED);
	return(ERROR);
    }
    if (op == POSTER_WRITE && H2DEV_POSTER_TASK_ID(dev) != getpid()) {
	errnoSet(S_posterLib_NOT_OWNER);
	return ERROR;
    }
    if (h2semTake(H2DEV_POSTER_SEM_ID(dev), WAIT_FOREVER) == FALSE) {
	return ERROR;
    }
    H2DEV_POSTER_OP(dev) = op;
    return OK;

} /* localPosterTake */

/*----------------------------------------------------------------------*/

static STATUS
localPosterGive(POSTER_ID posterId)
{
    long dev = (long)posterId;
    H2TIMESPEC date;

    if (dev < 0 || dev >= H2_DEV_MAX 
	|| H2DEV_TYPE(dev) != H2_DEV_TYPE_POSTER) {
	errnoSet(S_posterLib_POSTER_CLOSED);
	return(ERROR);
    }
    
    if (H2DEV_POSTER_OP(dev) == POSTER_WRITE) {

	/* Marquer les donnes comme fraiches */
	H2DEV_POSTER_FLG_FRESH(dev) = TRUE;

	/* Lire la date */
	if (h2GetTimeSpec(&date) == ERROR) {
	    h2semGive(H2DEV_POSTER_SEM_ID(dev));
	    return ERROR;
	}
	/* La copier dans le device */
	memcpy(H2DEV_POSTER_DATE(dev), &date, sizeof(H2TIMESPEC));
    }

    return(h2semGive(H2DEV_POSTER_SEM_ID(dev)));

} /* localPosterGive */

/*----------------------------------------------------------------------*/

static void *
localPosterAddr(POSTER_ID posterId)
{
    long dev = (long)posterId;

    if (dev < 0 || dev >= H2_DEV_MAX 
	|| H2DEV_TYPE(dev) != H2_DEV_TYPE_POSTER) {
	errnoSet(S_posterLib_POSTER_CLOSED);
	return(NULL);
    }
    
    return smObjGlobalToLocal(H2DEV_POSTER_POOL(dev));
    
} /* posterAddr */


/*----------------------------------------------------------------------*/

static STATUS
localPosterSetEndianness(POSTER_ID posterId, H2_ENDIANNESS endianness)
{
    long dev = (long)posterId;

    if (dev < 0 || dev >= H2_DEV_MAX 
	|| H2DEV_TYPE(dev) != H2_DEV_TYPE_POSTER) {
	errnoSet(S_posterLib_POSTER_CLOSED);
	return ERROR;
    }

    H2DEV_POSTER_ENDIANNESS(dev) = endianness;
    return OK;
    
} /* posterSetEndianness */

/*----------------------------------------------------------------------*/

static STATUS
localPosterGetEndianness(POSTER_ID posterId, H2_ENDIANNESS *endianness)
{
    long dev = (long)posterId;

    if (dev < 0 || dev >= H2_DEV_MAX 
	|| H2DEV_TYPE(dev) != H2_DEV_TYPE_POSTER) {
	errnoSet(S_posterLib_POSTER_CLOSED);
	return ERROR;
    }

    *endianness = H2DEV_POSTER_ENDIANNESS(dev);
    return OK;
    
} /* posterGetEndianness */


/*----------------------------------------------------------------------*/

static STATUS
localPosterIoctl(POSTER_ID posterId, int code, void *parg)
{
    long dev = (long)posterId;
    STATUS retval;
    H2TIME h2time;

    if (dev < 0 || dev >= H2_DEV_MAX 
	|| H2DEV_TYPE(dev) != H2_DEV_TYPE_POSTER) {
	errnoSet(S_posterLib_POSTER_CLOSED);
	return ERROR;
    }
    /* Prendre l'acces au poster */
    if (localPosterTake(posterId, POSTER_IOCTL) == ERROR) {
	return ERROR;
    }
    /* Executer la fonction demandee */
    retval = OK;
    switch (code) {
      case FIO_FRESH:
        *(int *) parg = H2DEV_POSTER_FLG_FRESH(dev);
        break;

      case FIO_GETDATE:
	/* Verifier si on a deja ecrit sur le poster */
	if (H2DEV_POSTER_FLG_FRESH(dev) != TRUE) {
	    errnoSet(S_posterLib_EMPTY_POSTER);
	    retval = ERROR;
	}
	/* Copier la date du poster */
	h2timeFromTimespec(&h2time, (H2TIMESPEC *)H2DEV_POSTER_DATE(dev));
	memcpy(parg, (char *)&h2time, sizeof(H2TIME));
	break;
	
      case FIO_NMSEC:
	/* Verifier si on a deja ecrit sur le poster */
	if (H2DEV_POSTER_FLG_FRESH(dev) != TRUE) {
	    errnoSet(S_posterLib_EMPTY_POSTER);
	    retval = ERROR;
	}
	/* Nombre de millisecondes depuis la derniere ecriture */
	if (h2timespecInterval(H2DEV_POSTER_DATE(dev), 
		(unsigned long *)parg) == ERROR) {
	    retval = ERROR;
	}
	break;

      case FIO_GETSIZE:
	/* Size of the poster */
	*(size_t *)parg = H2DEV_POSTER_SIZE(dev);
	break;

      case FIO_GETSTATS:
	/* statistics */
	memcpy(parg, (char *)&(H2DEV_POSTER_STATS(dev)),
	       sizeof(H2_POSTER_STAT_STR));
	H2DEV_POSTER_READ_OPS(dev) = 0;
	H2DEV_POSTER_READ_BYTES(dev) = 0;
	H2DEV_POSTER_WRITE_OPS(dev) = 0;
	H2DEV_POSTER_WRITE_BYTES(dev) = 0;
	break;

      default:
	errnoSet(S_posterLib_BAD_IOCTL_CODE);
	retval = ERROR;
    } /* switch */
    localPosterGive(posterId);
    return retval;
}

/*----------------------------------------------------------------------*/

static STATUS
localPosterShow(void)
{
    int i;
    H2TIMESPEC *date;
    H2TIME h2time;

    if (h2devAttach() == ERROR) {
	return ERROR;
    }
    logMsg("\n");
    logMsg("NAME                              Id/host      Size T(last write)\n");
    logMsg("-------------------------------- -------- --------- -------------\n");
    for (i = 0; i < H2_DEV_MAX; i++) {
	if (H2DEV_TYPE(i) == H2_DEV_TYPE_POSTER) {
	    logMsg("%-32s %8d %8d", H2DEV_NAME(i), i,
		   H2DEV_POSTER_SIZE(i));
	    if (H2DEV_POSTER_FLG_FRESH(i)) {
		date = H2DEV_POSTER_DATE(i);
		h2timeFromTimespec(&h2time, date);
		logMsg(" %02dh:%02dmin%02ds %lu\n", h2time.hour, h2time.minute,
		    h2time.sec, h2time.ntick);
	    } else {
		logMsg(" EMPTY_POSTER!\n");
	    }
	}
    } /* for */
    logMsg("\n");
    return OK;
}
/*----------------------------------------------------------------------*/

static STATUS
localPosterStats(void)
{
    int i;

    if (h2devAttach() == ERROR) {
	return ERROR;
    }
    logMsg("\n");
    logMsg("NAME                                ReadOps   WriteOps  ReadBytes WriteBytes\n");
    logMsg("-------------------------------- ---------- ---------- ---------- ----------\n");
    for (i = 0; i < H2_DEV_MAX; i++) {
	if (H2DEV_TYPE(i) == H2_DEV_TYPE_POSTER) {
	    logMsg("%-32s %10d %10d %10d %10d\n", H2DEV_NAME(i),
		   H2DEV_POSTER_READ_OPS(i),
		   H2DEV_POSTER_WRITE_OPS(i),
		   H2DEV_POSTER_READ_BYTES(i),
		   H2DEV_POSTER_WRITE_BYTES(i));
	    H2DEV_POSTER_READ_OPS(i) = 0;
	    H2DEV_POSTER_WRITE_OPS(i) = 0;
	    H2DEV_POSTER_READ_BYTES(i) = 0;
	    H2DEV_POSTER_WRITE_BYTES(i) = 0;
	}
    } /* for */
    logMsg("\n");
    return OK;
}
/*----------------------------------------------------------------------*/
