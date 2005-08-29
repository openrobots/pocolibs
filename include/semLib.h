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

#ifndef _SEMLIB_H
#define _SEMLIB_H

#include "portLib.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Code du module */
#define M_semLib (22 << 16)

/* codes d'erreur */
#define S_semLib_TOO_MANY_SEM (M_semLib | 1)
#define S_semLib_ALLOC_ERROR (M_semLib | 2)
#define S_semLib_NOT_A_SEM (M_semLib | 3)
#define S_semLib_TIMEOUT (M_semLib | 4)
#define S_semLib_RESOURCE_BUSY (M_semLib | 5)
#define S_semLib_LXRT_ERROR (M_semLib | 6) 

/* Valeur initiale des semaphores binaires */
typedef enum SEM_B_STATE {
    SEM_EMPTY = 0,
    SEM_FULL = 1,
    SEM_UNALLOCATED = 32767
} SEM_B_STATE;

/* Flags des semaphores */
enum {
    SEM_Q_FIFO,
    SEM_Q_PRIORITY
};

#ifndef __cplusplus
/* semaphore definition - see semLib.c for actual definition */
typedef struct SEM_ID *SEM_ID;
#else
struct SEM_ID_STR;
typedef struct SEM_ID_STR *SEM_ID;
#endif

/* Prototypes */
/** Creates and initializes a binary semaphore
 * @arg options semaphore options, unused on Posix systems
 * @arg initialState the initial state of the semaphore (one of the SEM_B_STATE value)
 * @return the semaphore ID or NULL if an error has occured
 * Call errnoGet() to get the error code
 */
extern SEM_ID semBCreate ( int options, SEM_B_STATE initialState );

/** Creates and initializes a counting semaphore
 * @arg options semaphore options, unused on Posix systems
 * @arg initialCount the initial count of the semaphore
 * @return the semaphore ID or NULL if an error has occured
 * Call errnoGet() to get the error code
 */
extern SEM_ID semCCreate ( int options, int initialCount );

/** Creates and initializes a mutex semaphore
 * @arg options creation options, unused on Posix systems
 * @return the semaphore ID or NULL if an error has occured. 
 * The error code is returned by errnoGet()
 */
extern SEM_ID semMCreate ( int options );

extern STATUS semDelete ( SEM_ID semId );
extern STATUS semGive ( SEM_ID semId );
extern STATUS semTake ( SEM_ID semId, int timeout );
extern STATUS semFlush ( SEM_ID semId );

#ifdef __cplusplus
};
#endif

#endif
