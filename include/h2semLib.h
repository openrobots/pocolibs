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

#ifndef _H2SEMLIB_H
#define _H2SEMLIB_H

#include "semLib.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Nombre max de semaphores par Id(SEMMSL) */
#define MAX_SEM 20

/* Code du module */
#define M_h2semLib (503 << 16)

/* Types de semaphores */
#define   H2SEM_SYNC         0          /* Semaphore de synchronisation */
#define   H2SEM_EXCL         1          /* Semaphore d'exclusion mutuelle */

/* codes d'erreur */
#define S_h2semLib_TOO_MANY_SEM (M_h2semLib | 1)
#define S_h2semLib_ALLOC_ERROR (M_h2semLib | 2)
#define S_h2semLib_NOT_A_SEM (M_h2semLib | 3)
#define S_h2semLib_TIMEOUT (M_h2semLib | 4)
#define S_h2semLib_BAD_SEM_TYPE (M_h2semLib | 5)
#define S_h2semLib_PERMISSION_DENIED (M_h2semLib | 6)


typedef int H2SEM_ID;


/** Creates a new H2 semaphore of the given type
 * @return ERROR on error, the semaphore id on success
 * In case of an error, errnoGet() returns the according 
 * error code
 */
extern H2SEM_ID h2semAlloc (int type );
extern STATUS h2semCreate0 ( int semId, int value );
extern STATUS h2semDelete ( H2SEM_ID sem );
extern void h2semEnd ( void );
extern BOOL h2semFlush ( H2SEM_ID sem );
extern STATUS h2semGive ( H2SEM_ID sem );
extern STATUS h2semInit ( int num, int *pSemId );
extern STATUS h2semShow ( H2SEM_ID sem );
extern BOOL h2semTake ( H2SEM_ID sem, int timeout );
extern STATUS h2semSet ( H2SEM_ID sem, int value );


#ifdef __cplusplus
};
#endif

#endif
