/* $LAAS$ */
/*
 * Copyright (c) 1999, 2003 CNRS/LAAS
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

/* DESCRIPTION :
   Fichier d'en-tete de la bibliotheque de fonctions de gestion des timers
   de synchronisation.
*/

#ifndef _H2TIMERLIB_H
#define _H2TIMERLIB_H

#include "semLib.h"

/* Flag d'indication d'initialisation */
#define  H2TIMER_FLG_INIT                 0x44444444

/* Nombre max de timers */
#define  NMAX_TIMERS                      20

/* Temps max du retard */
#define  MAX_DELAY                        20

/* Status des timers */
#define  STOPPED_TIMER                    0
#define  WAIT_DELAY                       1
#define  RUNNING_TIMER                    2

/* Code du module */
#define   M_h2timerLib                  (504 << 16)

/* Codes des erreurs */
#define  S_h2timerLib_TIMER_NOT_INIT      (M_h2timerLib | 0)
#define  S_h2timerLib_BAD_DELAY           (M_h2timerLib | 1)
#define  S_h2timerLib_TOO_MUCH_TIMERS     (M_h2timerLib | 2)
#define  S_h2timerLib_STOPPED_TIMER       (M_h2timerLib | 3)
#define  S_h2timerLib_NOT_STOPPED_TIMER   (M_h2timerLib | 4)
#define  S_h2timerLib_BAD_PERIOD          (M_h2timerLib | 5)

/* Definition de type d'un timer h2 */
typedef struct {
    int flagInit;			/* Indication d'initialisation */
    int status;				/* Etat du timer */
    int periode;			/* Periode du timer */
    int delay;				/* Retard pour starter timer */
    int count;				/* Compteur du timer */
    SEM_ID semSync;			/* semaphore de synchronisation */
} H2TIMER, *H2TIMER_ID;
 
#include "h2timerLibProto.h"

#endif
