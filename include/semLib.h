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

#ifndef _SEMLIB_H
#define _SEMLIB_H

#ifndef NO_POSIX_SEMAPHORES
#include <semaphore.h>
#else
#include "tw_sem.h"
#endif

/* Code du module */
#define M_semLib (22 << 16)

/* codes d'erreur */
#define S_semLib_TOO_MANY_SEM (M_semLib | 1)
#define S_semLib_ALLOC_ERROR (M_semLib | 2)
#define S_semLib_NOT_A_SEM (M_semLib | 3)
#define S_semLib_TIMEOUT (M_semLib | 4)
 

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

/* Type de semaphore */
typedef sem_t *SEM_ID;

/* Prototypes */
extern SEM_ID semBCreate ( int options, SEM_B_STATE initialState );
extern SEM_ID semCCreate ( int options, int initialCount );
extern STATUS semDelete ( SEM_ID semId );
extern STATUS semGive ( SEM_ID semId );
extern STATUS semTake ( SEM_ID semId, int timeout );
extern STATUS semFlush ( SEM_ID semId );

#endif
