/*
 * Copyright (c) 1990, 2003 CNRS/LAAS
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
   Permet la creation/deletion de timers, utilises pour la synchronisation
   de taches. Ces fonctions offrent la possibilite de partager le temps
   de la CPU en tranches, dont les les instants de debut sont programmables 
   par l'utilisateur.
*/

#include "pocolibs-config.h"
__RCSID("$LAAS$");

/* #define DEBUG_H2TIMER */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "portLib.h"
#include "errnoLib.h"
#include "wdLib.h"
#include "h2timerLib.h"

static  int delayCount;                /* Compteur global de synchronisation */
static pthread_mutex_t timerMutex = PTHREAD_MUTEX_INITIALIZER;
static WDOG_ID timerWd;			/* watchdog du compteur */
static H2TIMER timerTab[NMAX_TIMERS]; /* Tableau de timers */
static BOOL h2timerInited = FALSE;

static void timerInt(int arg); /* routine traitement interruption timer */ 

/******************************************************************************
*
*   h2timerInit  -  Initialisation de la bibliotheque de routines de timer
*
*   Description:
*   Initialisation de la bibliotheque de routines de timer. Lance le wdog 
*   de comptage des slots de temps.
* 
*   Retourne: OK ou ERROR
*/
STATUS 
h2timerInit(void)
{
    int nTimer;
    
    pthread_mutex_lock(&timerMutex);

    /* Cree le watchdog du timer et le semaphore de protection */
    if ((timerWd = wdCreate ()) == NULL) {
	pthread_mutex_unlock(&timerMutex);
	return ERROR;
    }

    /* Reset du tableau de timers */
    memset (timerTab, 0, sizeof(timerTab));

    /* Allouer la variable de synchronisation pour chaque timer */
    for (nTimer = 0; nTimer < NMAX_TIMERS; nTimer++) {
	/* Initialiser le semaphore */
	timerTab[nTimer].semSync = semBCreate(SEM_Q_PRIORITY, SEM_EMPTY);
	if (timerTab[nTimer].semSync == NULL) {
	    fprintf(stderr, "h2timerInit: erreur semBCreate\n");
	    errnoSet(errno);
	    return ERROR;
	}
    }
    /* Reset le compteur global de delay */
    delayCount = 0;
    
    /* Liberer le semaphore de protection */
    pthread_mutex_unlock(&timerMutex);
    
    /* Lance le watch dog global du timer */
    wdStart (timerWd, 1, (FUNCPTR) timerInt, 0);

    h2timerInited = TRUE;
    return OK;
}

/*****************************************************************************
*
*   h2timerAlloc  -  Allocation d'un timer h2
*
*   Description:
*   Alloue un timer type h2. Initialise la structure de donnees associee
*   au timer.
*
*   Retourne: l'id du timer ou NULL;
*/

H2TIMER_ID 
h2timerAlloc(void)
{
    int nTimer;                     /* Numero du timer alloue */
    H2TIMER_ID timerId;             /* Identificateur du timer */
  
    /* Prendre le semaphore de protection */
    pthread_mutex_lock(&timerMutex);

    if (!h2timerInited) {
	    errnoSet(S_h2timerLib_TIMER_NOT_INIT);
	    return NULL;
    }
    /* Initialiser le ptr vers debut du tableau */
    timerId = timerTab;

    /* Allouer un timer */
    for (nTimer = 0; timerId->flagInit == H2TIMER_FLG_INIT; nTimer++) {
	/* Retourner erreur si pas trouve timer libre */
	if (nTimer == NMAX_TIMERS) {
	    errnoSet (S_h2timerLib_TOO_MUCH_TIMERS);
	    pthread_mutex_unlock(&timerMutex);
	    return (NULL);
	}

	/* Position suivante */
	timerId = (H2TIMER_ID) timerId + 1;
    } /* for */

    /* Remplir la structure */
    timerId->status = STOPPED_TIMER;
    timerId->flagInit = H2TIMER_FLG_INIT;

    /* Liberer le semaphore et retourner */
    pthread_mutex_unlock(&timerMutex);
    return (timerId);
}

/******************************************************************************
*
*   h2timerStart  -  Demarrer le timer de synchronisation
*
*   Description:
*   Demarre le timer de synchronisation, avec un retard donne 
*   par l'utilisateur.
*   Attention: la periode doit etre un multiple ou sous-multiple
*              de MAX_DELAY
*
*   Retourne: OK ou ERROR
*/
STATUS 
h2timerStart(H2TIMER_ID timerId,
	     int periode,
	     int delay)
{
    /* Verifier si timer initialise */
    if (timerId->flagInit != H2TIMER_FLG_INIT) {
	errnoSet (S_h2timerLib_TIMER_NOT_INIT);
	return (ERROR);
    }
    
    /* Seulement demarrer si arrete' */
    if (timerId->status != STOPPED_TIMER) {
	errnoSet (S_h2timerLib_NOT_STOPPED_TIMER);
	return (ERROR);
    }
    
    /* Verifier la validite de la periode */
    if ((periode <= 0) || 
	((periode > MAX_DELAY) && ((periode%MAX_DELAY) != 0)) ||
	((periode < MAX_DELAY) && ((MAX_DELAY%periode) != 0))) {
        printf ("h2timerStart: period (%d) must be a multiple or a divisor of %d\n", 
		periode, MAX_DELAY);
	errnoSet (S_h2timerLib_BAD_PERIOD);
	return (ERROR);
    }
    
    /* Verifier validite du retard */
    if (delay < 0 || delay >= MAX_DELAY) {
	errnoSet (S_h2timerLib_BAD_DELAY);
	return (ERROR);
    }
    
    /* Remplir la structure du timer */
    timerId->periode = periode;
    timerId->delay = delay;
    timerId->count = 0;
    timerId->status = WAIT_DELAY;

#ifdef DEBUG_H2TIMER
    printf("h2timerStart %d %d\n", periode, delay);
#endif
    return (OK);
}

/******************************************************************************
*
*   h2timerPause  -  Attente comptage final timer
*
*   Description:
*   Attent synchronisation, faite par la liberation du semaphore de
*   synchronisation.
*
*   Retourne: OK ou ERROR
*/
STATUS
h2timerPause(H2TIMER_ID timerId)
{
    /* Verifier si timer initialise */
    if (timerId->flagInit != H2TIMER_FLG_INIT) {
	errnoSet (S_h2timerLib_TIMER_NOT_INIT);
	return (ERROR);
    }

    /* Attente liberation du semaphore de sync du timer */
#ifdef DEBUG_H2TIMER
    printf("h2timerPause: got lock\n");
#endif
    semTake(timerId->semSync, WAIT_FOREVER);

    /*
     * XXXX
     * vide le semaphore pour  garantir qu'il bloquera au prochain appel
     * (parce que semSync n'est pas un semaphore binaire)
     */
    while (semTake(timerId->semSync, NO_WAIT) == OK) 
	;
    return (OK);

}

/******************************************************************************
*
*   h2timerPauseReset  -  Attente comptage final timer et reset
*
*   Description:
*   Attent synchronisation, faite par la liberation du semaphore de
*   synchronisation. remise a` zero ensuite pour garder synchro
*
*   Retourne: OK ou ERROR
*/
STATUS 
h2timerPauseReset(H2TIMER_ID timerId)
{
  /* Verifier si timer initialise */
    if (timerId->flagInit != H2TIMER_FLG_INIT) {
	errnoSet (S_h2timerLib_TIMER_NOT_INIT);
	return (ERROR);
    }
    
    /* Attente liberation du semaphore de sync du timer */
    semTake (timerId->semSync, WAIT_FOREVER);

    timerId->count = 0;
    return (OK);
}

/******************************************************************************
*
*   h2timerStop  -  Arreter timer
*
*   Description:
*   Arreter le timer donne par son identificateur
*
*   Retourne: OK ou ERROR
*/

STATUS
h2timerStop(H2TIMER_ID timerId)
{
    /* Verifier si timer initialise */
    if (timerId->flagInit != H2TIMER_FLG_INIT) {
	errnoSet (S_h2timerLib_TIMER_NOT_INIT);
	return (ERROR);
    }
    
    /* Arreter le timer */
    timerId->status = STOPPED_TIMER;
    return (OK);
}

/******************************************************************************
*
*   h2timerChangePeriod  -  Modifier la periode du timer
*
*   Description:
*   Modifier la peridoe d'un timer.
*
*   Retourne: OK ou ERROR
*/
STATUS
h2timerChangePeriod(H2TIMER_ID timerId, int periode)
{
    /* Verifier si timer initialise */
    if (timerId->flagInit != H2TIMER_FLG_INIT) {
	errnoSet (S_h2timerLib_TIMER_NOT_INIT);
	return (ERROR);
    }
    
    /* Verifier que le timer n'est pas a l'arret */
    if (timerId->status == STOPPED_TIMER) {
	errnoSet (S_h2timerLib_STOPPED_TIMER);
	return (ERROR);
    }
    
    /* Modification de la periode */
    timerId->periode = periode;
    timerId->count = timerId->count % periode;
    return (OK);
}

/******************************************************************************
*
*   h2timerFree  -  Liberer un timer alloue
*
*   Description:
*   Libere le timer donne par son identificateur
*
*   Retourne: OK ou ERROR
*/

STATUS 
h2timerFree(H2TIMER_ID timerId)
{
    /* Verifier si timer initialise */
    if (timerId->flagInit != H2TIMER_FLG_INIT) {
	errnoSet (S_h2timerLib_TIMER_NOT_INIT);
	return (ERROR);
    }
    
    /* Arreter le timer */
    timerId->status = STOPPED_TIMER;
    timerId->flagInit = FALSE;
    return (OK);
}

/*****************************************************************************
*
*   timerInt  -  Trait. interruption timer de synchronisation
*
*   Description:
*   Libere le semaphore de synchronisation et recommence un nouveau cycle
*
*   Retourne: Neant
*/
static void
timerInt(int arg)
{
    int nTimer;               /* Numero d'un timer */
    H2TIMER_ID timerId;       /* Ptr vers tableau de timers */
    

#ifdef DEBUG_H2TIMER
    printf("timerInt: delayCount %d\n", delayCount);
#endif
    /* Incremente le compteur de delay */
    if (++delayCount >= MAX_DELAY)
	delayCount = 0;
    
    /* Reinitialiser le watch dog */
    wdStart (timerWd, 1, (FUNCPTR) timerInt, 0);

    /* Initialiser le pointer vers debut tableau de timers */
    timerId = timerTab;
    
    /* Traiter les timers initialises */
    for (nTimer = 0; nTimer < NMAX_TIMERS; nTimer++) {
	/* Verifier si timer initialise */
	if (timerId->flagInit == H2TIMER_FLG_INIT) {
	    /* Verifier si le timer est en comptage */
	    if (timerId->status == RUNNING_TIMER) {
#ifdef DEBUG_H2TIMER
		printf(" -- count(%d) = %d\n", nTimer, timerId->count);
#endif
		/* Incrementer le compteur */
		if (++timerId->count == timerId->periode) {
		    /* Reseter le compteur */
		    timerId->count = 0;
#ifdef DEBUG_H2TIMER		    
		    printf(" -- timer % declenche'\n", nTimer);
#endif
		    /* Liberer le semaphore */
		    semGive (timerId->semSync);
		    
		}
	    } else if (timerId->status == WAIT_DELAY) {
		
		/* Verifier si attend delay */

		/* Verifier si le delai est ecoule */
		if (delayCount == timerId->delay) {
		    /* Reseter le compteur de periode */
		    timerId->count = 0;
		    
		    /* Demarrer le timer */
#ifdef DEBUG_H2TIMER
		    printf(" -- timer %d starting\n", nTimer);
#endif
		    timerId->status = RUNNING_TIMER;
		    semGive (timerId->semSync);
		}
	    }
	}
	
	/* Actualiser le pointeur */
	timerId++;
    }
}

    
