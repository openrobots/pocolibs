/* $LAAS$ */
/*
 * Copyright (c) 1998, 2003 CNRS/LAAS
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

#ifndef _POSTERLIB_H
#define _POSTERLIB_H

#include "h2endianness.h"

/* type of operation in posterTake() */
typedef enum {
	POSTER_READ,
	POSTER_WRITE,
	POSTER_IOCTL
} POSTER_OP;

/*  ioctl operation codes */
#define   FIO_GETDATE                            1
#define   FIO_NMSEC                              2
#define   FIO_GETSIZE				 3

/* bus address space for poster storage */
#define POSTER_LOCAL_MEM   0		/* local memory of one process
					 * - not exportable */
#define POSTER_SM_MEM      1		/* VxMP like shared memory */
#define POSTER_VME_A32     2		/* VME A32 space */
#define POSTER_VME_A24     3		/* VME A24 space */
#define POSTER_REMOTE	   4            /* poster accessed using posterServ */

typedef void *POSTER_ID;

#define POSTER_MAGIC 0x89012345

/* Module codes */
#define M_posterLib  				510
#define M_remotePosterLib			516

/* Error codes */
#define S_posterLib_POSTER_CLOSED (M_posterLib << 16 | 0)
#define S_posterLib_NOT_OWNER     (M_posterLib << 16 | 1)
#define S_posterLib_EMPTY_POSTER   (M_posterLib << 16 | 3)
#define S_posterLib_BAD_IOCTL_CODE (M_posterLib << 16 | 4)
#define S_posterLib_BAD_OP           (M_posterLib << 16 | 5)
#define S_posterLib_DLOPEN		(M_posterLib << 16 | 6)
#define S_posterLib_DLSYM		(M_posterLib << 16 | 7)

#define S_posterLib_DUPLICATE_POSTER (M_posterLib << 16 | 10)
#define S_posterLib_INVALID_PATH   (M_posterLib << 16 | 11)
#define S_posterLib_MALLOC_ERROR  (M_posterLib << 16 | 12)
#define S_posterLib_SHMGET_ERROR   (M_posterLib << 16 | 13)
#define S_posterLib_SHMAT_ERROR   (M_posterLib << 16 | 14)
#define S_posterLib_SEMGET_ERROR  (M_posterLib << 16 | 15)
#define S_posterLib_SEMOP_ERROR   (M_posterLib << 16 | 16)

/* Remote posters error codes  */
#define S_remotePosterLib_POSTER_CLOSED        (M_remotePosterLib << 16 | 0)
#define S_remotePosterLib_NOT_OWNER            (M_remotePosterLib << 16 | 1)
#define S_remotePosterLib_ERR_TIME_READ        (M_remotePosterLib << 16 | 2)
#define S_remotePosterLib_EMPTY_POSTER         (M_remotePosterLib << 16 | 3)
#define S_remotePosterLib_BAD_IOCTL_CODE       (M_remotePosterLib << 16 | 4)
#define S_remotePosterLib_BAD_OP               (M_remotePosterLib << 16 | 5)

#define S_remotePosterLib_BAD_RPC              (M_remotePosterLib << 16 | 10)
#define S_remotePosterLib_BAD_ALLOC            (M_remotePosterLib << 16 | 11)
#define S_remotePosterLib_CORRUPT_DATA         (M_remotePosterLib << 16 | 12)
#define S_remotePosterLib_BAD_PARAMS           (M_remotePosterLib << 16 | 13)
#define S_remotePosterLib_POSTER_HOST_NOT_DEFINED (M_remotePosterLib << 16| 14)

/*
 * Prototypes
 */

extern STATUS posterCreate ( char *name, int size, POSTER_ID *pPosterId );
extern STATUS posterMemCreate ( char *name, int busSpace, void *pPool, int size, POSTER_ID *pPosterId );
extern STATUS posterDelete ( POSTER_ID dev );
extern STATUS posterFind ( char *name, POSTER_ID *pPosterId );
extern int posterWrite ( POSTER_ID posterId, int offset, void *buf, int nbytes );
extern int posterRead ( POSTER_ID posterId, int offset, void *buf, int nbytes );
extern STATUS posterTake ( POSTER_ID posterId, POSTER_OP op );
extern STATUS posterGive ( POSTER_ID posterId );
extern void * posterAddr ( POSTER_ID posterId );
extern STATUS posterIoctl(POSTER_ID posterId, int code, void *parg);
extern STATUS posterEndianness(POSTER_ID posterId, H2_ENDIANNESS *endianness);

/* for posterServ only ! */
extern STATUS posterSetEndianness(POSTER_ID posterId, H2_ENDIANNESS endianness);

#endif
