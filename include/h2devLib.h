/*
 * Copyright (c) 1998, 2003-2010 CNRS/LAAS
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

#include <sys/types.h>

#include "h2rngLib.h"
#include "h2timeLib.h"
#include "h2semLib.h"
#include "h2endianness.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 ** H2 device structures
 ** The array of h2 devices is stored in shared memory. 
 **/

/* Default access mode for IPC data */
#define PORTLIB_MODE 0666

/* Semaphore */
typedef struct H2_SEM_STR {
    int semId;
    int semNum;
} H2_SEM_STR;

/*
 * XXX Warning, in the 3 following device types, 3 different identifiers
 * XXX are stored under the 'taskId' name. Don't mix them!
 */

/* Mailbox */
typedef struct H2_MBOX_STR {
    long taskId;			/* h2dev id */
    int size;				/* mbox size */
    H2SEM_ID semExcl;			/* mutex for this mailbox */
    H2SEM_ID semSigRd;			/* signalling semaphore  */
    H2RNG_ID rngId;			/* global Id of the ring buffer */
} H2_MBOX_STR;

/* Poster statistics */
typedef struct H2_POSTER_STAT_STR {
	unsigned int read_ops;
	unsigned int write_ops;
	unsigned int read_bytes;
	unsigned int write_bytes;
} H2_POSTER_STAT_STR;
		
/* Poster */
typedef struct H2_POSTER_STR {
    int taskId;				/* Unix pid */
    unsigned char *pPool;		/* global address of the data */
    H2SEM_ID semId;			/* synchronization semaphore */
    int flgFresh;			/* available data flag */
    H2TIMESPEC date;			/* last modification date */
    int size;				/* poster size */
    int op;				/* current operation */
    H2_ENDIANNESS endianness;
    H2_POSTER_STAT_STR stats;		/* statistics */
} H2_POSTER_STR;

/* Task */
typedef struct H2_TASK_STR {
    long taskId;			/* taskLib id */
    int semId;
} H2_TASK_STR;

/* Shared memory */
typedef struct H2_MEM_STR {
    int shmId;				/* IPC SHM identifier */
    int size;				/* size */
} H2_MEM_STR;

/* Device types */
typedef enum {
    H2_DEV_TYPE_NONE,
    H2_DEV_TYPE_H2DEV,
    H2_DEV_TYPE_SEM,
    H2_DEV_TYPE_MBOX,
    H2_DEV_TYPE_POSTER,
    H2_DEV_TYPE_TASK,
    H2_DEV_TYPE_MEM
} H2_DEV_TYPE;

/* Number of defined device types */
#define H2DEV_MAX_TYPES 7

/* Maximum length of a device name */
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

/* Default Maximum number of h2 devices */
#define H2_DEV_MAX_DEFAULT 120

/* Timeout on h2devFind */
#define H2DEV_TIMEOUT 100

/* External name for h2 devices */
#define H2_DEV_NAME ".h2dev"

/* -- ERRORS CODES ----------------------------------------------- */

#include "h2errorLib.h"
#define M_h2devLib   505

/* Error codes */
#define S_h2devLib_BAD_DEVICE_TYPE 		H2_ENCODE_ERR(M_h2devLib, 0)
#define S_h2devLib_DUPLICATE_DEVICE_NAME	H2_ENCODE_ERR(M_h2devLib, 1)
#define S_h2devLib_TOO_MANY_DEVICES             H2_ENCODE_ERR(M_h2devLib, 2)
#define S_h2devLib_FIND_DEVICE_TIMEOUT          H2_ENCODE_ERR(M_h2devLib, 3)
#define S_h2devLib_NOT_OWNER                    H2_ENCODE_ERR(M_h2devLib, 4)
#define S_h2devLib_NOT_INITIALIZED 		H2_ENCODE_ERR(M_h2devLib, 5)
#define S_h2devLib_BAD_HOME_DIR			H2_ENCODE_ERR(M_h2devLib, 6)
#define S_h2devLib_FULL				H2_ENCODE_ERR(M_h2devLib, 7)
#define S_h2devLib_BAD_PARAMETERS		H2_ENCODE_ERR(M_h2devLib, 8)
#define S_h2devLib_NOT_FOUND			H2_ENCODE_ERR(M_h2devLib, 9)

#define H2_DEV_LIB_H2_ERR_MSGS { \
  {"BAD_DEVICE_TYPE", 		H2_DECODE_ERR(S_h2devLib_BAD_DEVICE_TYPE)}, \
  {"DUPLICATE_DEVICE_NAME", 	H2_DECODE_ERR(S_h2devLib_DUPLICATE_DEVICE_NAME)}, \
  {"TOO_MANY_DEVICES",          H2_DECODE_ERR(S_h2devLib_TOO_MANY_DEVICES)}, \
  {"FIND_DEVICE_TIMEOUT",       H2_DECODE_ERR(S_h2devLib_FIND_DEVICE_TIMEOUT)}, \
  {"NOT_OWNER",                 H2_DECODE_ERR(S_h2devLib_NOT_OWNER)}, \
  {"NOT_INITIALIZED",  		H2_DECODE_ERR(S_h2devLib_NOT_INITIALIZED)}, \
  {"BAD_HOME_DIR", 		H2_DECODE_ERR(S_h2devLib_BAD_HOME_DIR)}, \
  {"FULL", 			H2_DECODE_ERR(S_h2devLib_FULL)}, \
  {"BAD_PARAMETERS", 		H2_DECODE_ERR(S_h2devLib_BAD_PARAMETERS)}, \
  {"NOT_FOUND", 		H2_DECODE_ERR(S_h2devLib_NOT_FOUND)}, \
  }


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

#define H2DEV_POSTER_STATS(dev) h2Devs[dev].data.poster.stats
#define H2DEV_POSTER_READ_OPS(dev) h2Devs[dev].data.poster.stats.read_ops
#define H2DEV_POSTER_WRITE_OPS(dev) h2Devs[dev].data.poster.stats.write_ops
#define H2DEV_POSTER_READ_BYTES(dev) h2Devs[dev].data.poster.stats.read_bytes
#define H2DEV_POSTER_WRITE_BYTES(dev) h2Devs[dev].data.poster.stats.write_bytes

#define H2DEV_TASK_TID(dev) h2Devs[dev].data.task.taskId
#define H2DEV_TASK_PID(dev) h2Devs[dev].data.task.pid
#define H2DEV_TASK_SEM_ID(dev) h2Devs[dev].data.task.semId

#define H2DEV_MEM_SHM_ID(dev) h2Devs[dev].data.mem.shmId
#define H2DEV_MEM_SIZE(dev) h2Devs[dev].data.mem.size

/*
 * Prototypes
 */
extern int h2devRecordH2ErrMsgs(void);
extern int h2devAlloc ( const char *name, H2_DEV_TYPE type );
extern STATUS h2devAttach ( int *h2devMax );
extern int h2devSize ( void );
extern STATUS h2devEnd ( void );
extern int h2devFind ( const char *name, H2_DEV_TYPE type );
extern STATUS h2devFree ( int dev );
extern STATUS h2devClean ( const char *name );
extern long h2devGetKey ( int type, int dev, BOOL create, int *pFd );
extern int h2devGetSemId ( void );
extern STATUS h2devInit ( int smMemSize, int h2devMax, int posterServFlag );
extern STATUS h2devShow ( void );

#ifdef __cplusplus
}
#endif

#endif
