/* $LAAS$ */
/*
 * Copyright (c) 1998, 2003-2004 CNRS/LAAS
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

#ifndef _H2DEVLIB_H
#define _H2DEVLIB_H

#include "h2timeLib.h"
#include "h2semLib.h"
#include "h2rngLib.h"
#include "h2endianness.h"

/**
 ** Structures de device H2 pour Unix
 ** Le tableau des devices h2 est stocké dans un tableau en memoire 
 ** partagee 
 **/

/* Mode par defaut des structures IPC */
#define PORTLIB_MODE 0666

/* Semaphore */
typedef struct H2_SEM_STR {
    int semId;
    int semNum;
} H2_SEM_STR;

/*
 * XXX Attention, dans les 3 types de devices suivants sont memorises 
 * XXX sous le nom taskId, des identificateurs differents
 */

/* Mailbox */
typedef struct H2_MBOX_STR {
    int taskId;				/* identificateur h2dev */
    int size;				/* taille du mbox */
    H2SEM_ID semExcl;			/* Semaphore excution mutuelle */
    H2SEM_ID semSigRd;			/* Semaphore signalisation lecture */
    H2RNG_ID rngId;			/* Id global du ring buffer */
} H2_MBOX_STR;

/* Poster */
typedef struct H2_POSTER_STR {
    int taskId;				/* pid Unix */
    unsigned char *pPool;		/* addresse globale poster */
    H2SEM_ID semId;			/* semaphore de synchro */
    int flgFresh;			/* Flag donnees disponibles */
    H2TIME date;			/* date derniere modif */
    int size;				/* taille du poster */
    int op;				/* operation en cours */
    H2_ENDIANNESS endianness;
} H2_POSTER_STR;

/* Tache */
typedef struct H2_TASK_STR {
    int taskId;				/* identificateur taskLib */
    int semId;
} H2_TASK_STR;

/* Memoire partagee */
typedef struct H2_MEM_STR {
    int shmId;				/* id segment memoire partagee */
    int size;				/* taille */
} H2_MEM_STR;

/* Types de devices */
typedef enum {
    H2_DEV_TYPE_NONE,
    H2_DEV_TYPE_H2DEV,
    H2_DEV_TYPE_SEM,
    H2_DEV_TYPE_MBOX,
    H2_DEV_TYPE_POSTER,
    H2_DEV_TYPE_TASK,
    H2_DEV_TYPE_MEM
} H2_DEV_TYPE;

/* Nombre de types definis */
#define H2DEV_MAX_TYPES 7

/* longueur max d'un nom */
#define H2_DEV_MAX_NAME 32

/* devices */
typedef struct H2_DEV_STR {
    H2_DEV_TYPE type;
    char name[H2_DEV_MAX_NAME];
    long uid;
    union {
	H2_SEM_STR sem;
	H2_MBOX_STR mbox;
	H2_POSTER_STR poster;
	H2_TASK_STR task;
	H2_MEM_STR mem;
    } data;
} H2_DEV_STR;

/* Nombre max de h2dev */
#define H2_DEV_MAX 100

/* Timeout sur h2devFind */
#define H2DEV_TIMEOUT 100

/* Nom externe */
#define H2_DEV_NAME ".h2dev"

/* Code du module */
#define M_h2devLib   505

/* Codes d'erreur */
#define S_h2devLib_BAD_DEVICE_TYPE 		((M_h2devLib << 16) | 0)
#define S_h2devLib_DUPLICATE_DEVICE_NAME	((M_h2devLib << 16) | 1)
#define S_h2devLib_TOO_MANY_DEVICES             ((M_h2devLib << 16) | 2)
#define S_h2devLib_FIND_DEVICE_TIMEOUT          ((M_h2devLib << 16) | 3)
#define S_h2devLib_NOT_OWNER                    ((M_h2devLib << 16) | 4)
#define S_h2devLib_NOT_INITIALIZED 		((M_h2devLib << 16) | 5)
#define S_h2devLib_BAD_HOME_DIR			((M_h2devLib << 16) | 6)
#define S_h2devLib_FULL				((M_h2devLib << 16) | 7)
#define S_h2devLib_BAD_PARAMETERS		((M_h2devLib << 16) | 8)
#define S_h2devLib_NOT_FOUND			((M_h2devLib << 16) | 9)

extern H2_DEV_STR *h2Devs;

#define H2DEV_NAME(dev) h2Devs[dev].name
#define H2DEV_TYPE(dev) h2Devs[dev].type
#define H2DEV_UID(dev)  h2Devs[dev].uid

#define H2DEV_SEM_SEM_ID(dev) h2Devs[dev].data.sem.semId
#define H2DEV_SEM_SEM_NUM(dev) h2Devs[dev].data.sem.semNum

#define H2DEV_MBOX_STR(dev) (&(h2Devs[dev].data.mbox))
#define H2DEV_MBOX_SEM_ID(dev) h2Devs[dev].data.mbox.semSigRd
#define H2DEV_MBOX_SEM_EXCL_ID(dev) h2Devs[dev].data.mbox.semExcl
#define H2DEV_MBOX_TASK_ID(dev) h2Devs[dev].data.mbox.taskId
#define H2DEV_MBOX_RNG_ID(dev) h2Devs[dev].data.mbox.rngId

#define H2DEV_POSTER_SEM_ID(dev) h2Devs[dev].data.poster.semId
#define H2DEV_POSTER_POOL(dev) h2Devs[dev].data.poster.pPool
#define H2DEV_POSTER_TASK_ID(dev) h2Devs[dev].data.poster.taskId
#define H2DEV_POSTER_FLG_FRESH(dev) h2Devs[dev].data.poster.flgFresh
#define H2DEV_POSTER_DATE(dev) (&(h2Devs[dev].data.poster.date))
#define H2DEV_POSTER_SIZE(dev) h2Devs[dev].data.poster.size
#define H2DEV_POSTER_OP(dev) h2Devs[dev].data.poster.op
#define H2DEV_POSTER_ENDIANNESS(dev) h2Devs[dev].data.poster.endianness

#define H2DEV_TASK_TID(dev) h2Devs[dev].data.task.taskId
#define H2DEV_TASK_PID(dev) h2Devs[dev].data.task.pid
#define H2DEV_TASK_SEM_ID(dev) h2Devs[dev].data.task.semId

#define H2DEV_MEM_SHM_ID(dev) h2Devs[dev].data.mem.shmId
#define H2DEV_MEM_SIZE(dev) h2Devs[dev].data.mem.size

/*
 * Prototypes
 */
extern int h2devAlloc ( char *name, H2_DEV_TYPE type );
extern STATUS h2devAttach ( void );
extern STATUS h2devEnd ( void );
extern int h2devFind ( char *name, H2_DEV_TYPE type );
extern STATUS h2devFree ( int dev );
extern long h2devGetKey ( int type, int dev, BOOL create, int *pFd );
extern int h2devGetSemId ( void );
extern STATUS h2devInit ( int smMemSize );
extern STATUS h2devShow ( void );

#endif
