/* $LAAS$ */
/*
 * Copyright (c) 1998, 2005 CNRS/LAAS
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
#include <portLib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -- STRUCTURES ------------------------------------------ */

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
#define   FIO_FRESH                  4

/* bus address space for poster storage */
#define POSTER_LOCAL_MEM   0		/* local memory of one process
					 * - not exportable */
#define POSTER_SM_MEM      1		/* VxMP like shared memory */
#define POSTER_VME_A32     2		/* VME A32 space */
#define POSTER_VME_A24     3		/* VME A24 space */
#define POSTER_REMOTE	   4            /* poster accessed using posterServ */

typedef void *POSTER_ID;

#define POSTER_MAGIC 0x89012345


/* -- PROTOTYPES ------------------------------------------ */

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
extern char* posterName(POSTER_ID posterId);

/* for posterServ only ! */
extern STATUS posterSetEndianness(POSTER_ID posterId, H2_ENDIANNESS endianness);


/* -- ERRORS CODES ----------------------------------------------- */

#include "h2errorLib.h"

/* Module codes */
#define M_posterLib  				510
#define M_remotePosterLib			516

/* Error codes */
#define S_posterLib_POSTER_CLOSED   H2_ENCODE_ERR(M_posterLib, 0)
#define S_posterLib_NOT_OWNER       H2_ENCODE_ERR(M_posterLib, 1)
#define S_posterLib_EMPTY_POSTER    H2_ENCODE_ERR(M_posterLib, 3)
#define S_posterLib_BAD_IOCTL_CODE  H2_ENCODE_ERR(M_posterLib, 4)
#define S_posterLib_BAD_OP          H2_ENCODE_ERR(M_posterLib, 5)
#define S_posterLib_DLOPEN	    H2_ENCODE_ERR(M_posterLib, 6)
#define S_posterLib_DLSYM	    H2_ENCODE_ERR(M_posterLib, 7)

#define S_posterLib_DUPLICATE_POSTER H2_ENCODE_ERR(M_posterLib, 10)
#define S_posterLib_INVALID_PATH     H2_ENCODE_ERR(M_posterLib, 11)
#define S_posterLib_MALLOC_ERROR     H2_ENCODE_ERR(M_posterLib, 12)
#define S_posterLib_SHMGET_ERROR     H2_ENCODE_ERR(M_posterLib, 13)
#define S_posterLib_SHMAT_ERROR      H2_ENCODE_ERR(M_posterLib, 14)
#define S_posterLib_SEMGET_ERROR     H2_ENCODE_ERR(M_posterLib, 15)
#define S_posterLib_SEMOP_ERROR      H2_ENCODE_ERR(M_posterLib, 16)
#define S_posterLib_BAD_FORMAT       H2_ENCODE_ERR(M_posterLib, 17)

#define POSTER_LIB_H2_ERR_MSGS { \
    {"POSTER_CLOSED",   H2_DECODE_ERR(S_posterLib_POSTER_CLOSED)},  \
    {"NOT_OWNER",       H2_DECODE_ERR(S_posterLib_NOT_OWNER)},      \
    {"EMPTY_POSTER",    H2_DECODE_ERR(S_posterLib_EMPTY_POSTER)},   \
    {"BAD_IOCTL_CODE",  H2_DECODE_ERR(S_posterLib_BAD_IOCTL_CODE)}, \
    {"BAD_OP",          H2_DECODE_ERR(S_posterLib_BAD_OP)},	    \
    {"DLOPEN",          H2_DECODE_ERR(S_posterLib_DLOPEN)},	    \
    {"DLSYM",           H2_DECODE_ERR(S_posterLib_DLSYM)},	    \
    {"DUPLICATE_POSTER", H2_DECODE_ERR(S_posterLib_DUPLICATE_POSTER)},  \
    {"INVALID_PATH",     H2_DECODE_ERR(S_posterLib_INVALID_PATH)},  \
    {"MALLOC_ERROR",     H2_DECODE_ERR(S_posterLib_MALLOC_ERROR)},  \
    {"SHMGET_ERROR",     H2_DECODE_ERR(S_posterLib_SHMGET_ERROR)},  \
    {"SHMAT_ERROR",      H2_DECODE_ERR(S_posterLib_SHMAT_ERROR)},   \
    {"SEMGET_ERROR",     H2_DECODE_ERR(S_posterLib_SEMGET_ERROR)},  \
    {"SEMOP_ERROR",      H2_DECODE_ERR(S_posterLib_SEMOP_ERROR)}, \
    {"BAD_FORMAT",       H2_DECODE_ERR(S_posterLib_BAD_FORMAT)}   \
  }

/* Remote posters error codes  */
#define S_remotePosterLib_POSTER_CLOSED    H2_ENCODE_ERR(M_remotePosterLib, 0)
#define S_remotePosterLib_NOT_OWNER        H2_ENCODE_ERR(M_remotePosterLib, 1)
#define S_remotePosterLib_ERR_TIME_READ    H2_ENCODE_ERR(M_remotePosterLib, 2)
#define S_remotePosterLib_EMPTY_POSTER     H2_ENCODE_ERR(M_remotePosterLib, 3)
#define S_remotePosterLib_BAD_IOCTL_CODE   H2_ENCODE_ERR(M_remotePosterLib, 4)
#define S_remotePosterLib_BAD_OP           H2_ENCODE_ERR(M_remotePosterLib, 5)

#define S_remotePosterLib_BAD_RPC          H2_ENCODE_ERR(M_remotePosterLib, 10)
#define S_remotePosterLib_BAD_ALLOC        H2_ENCODE_ERR(M_remotePosterLib, 11)
#define S_remotePosterLib_CORRUPT_DATA     H2_ENCODE_ERR(M_remotePosterLib, 12)
#define S_remotePosterLib_BAD_PARAMS       H2_ENCODE_ERR(M_remotePosterLib, 13)
#define S_remotePosterLib_POSTER_HOST_NOT_DEFINED H2_ENCODE_ERR(M_remotePosterLib,14)

#define REMOTE_POSTER_LIB_H2_ERR_MSGS { \
    {"POSTER_CLOSED",    H2_DECODE_ERR(S_remotePosterLib_POSTER_CLOSED)},  \
    {"NOT_OWNER",        H2_DECODE_ERR(S_remotePosterLib_NOT_OWNER)},  \
    {"ERR_TIME_READ",    H2_DECODE_ERR(S_remotePosterLib_ERR_TIME_READ)}, \
    {"EMPTY_POSTER",     H2_DECODE_ERR(S_remotePosterLib_EMPTY_POSTER)},  \
    {"BAD_IOCTL_CODE",   H2_DECODE_ERR(S_remotePosterLib_BAD_IOCTL_CODE)},  \
    {"BAD_OP",           H2_DECODE_ERR(S_remotePosterLib_BAD_OP)},  \
\
    {"BAD_RPC",          H2_DECODE_ERR(S_remotePosterLib_BAD_RPC)},  \
    {"BAD_ALLOC",        H2_DECODE_ERR(S_remotePosterLib_BAD_ALLOC)},  \
    {"CORRUPT_DATA",     H2_DECODE_ERR(S_remotePosterLib_CORRUPT_DATA)},  \
    {"BAD_PARAMS",       H2_DECODE_ERR(S_remotePosterLib_BAD_PARAMS)},  \
    {"POSTER_HOST_NOT_DEFINED", H2_DECODE_ERR(S_remotePosterLib_POSTER_HOST_NOT_DEFINED)},  \
      }

#ifdef __cplusplus
};
#endif

#endif /* _POSTERLIB_H */
