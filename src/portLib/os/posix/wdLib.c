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
#include "config.h"
__RCSID("$LAAS$");

#include <signal.h>
#include <stdlib.h>

#include "portLib.h"

#include "errnoLib.h"
#include "memLib.h"
#include "objLib.h"
#include "wdLib.h"

WDOG_ID wdList = NULL;

static pthread_mutex_t wdMutex = PTHREAD_MUTEX_INITIALIZER;

STATUS
wdLibInit(void)
{
    return OK;
}

WDOG_ID
wdCreate(void)
{
    WDOG_ID wd;
    sigset_t set, old;

    wd = (WDOG_ID)malloc(sizeof(struct wdog));
    if (wd == NULL) {
	errnoSet(S_memLib_NOT_ENOUGH_MEMORY);
	return NULL;
    }
    wd->delay = -1;
    wd->pRoutine = NULL;
    wd->magic = M_wdLib;

    /* Block clock signal */
    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
    sigprocmask(SIG_BLOCK, &set, &old);

    pthread_mutex_lock(&wdMutex);

    /* Insert into wdList */
    wd->next = wdList;
    wdList = wd;
    pthread_mutex_unlock(&wdMutex);
    sigprocmask(SIG_SETMASK, &old, NULL); /* unblock clock sig */
    return wd;
}

STATUS
wdDelete(WDOG_ID wdId)
{
    WDOG_ID wd;
    sigset_t set, old;

    if (wdId == NULL || wdId->magic != M_wdLib) {
	errnoSet(S_objLib_OBJ_ID_ERROR);
	return ERROR;
    }
    /* Block clock signal */
    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
    sigprocmask(SIG_BLOCK, &set, &old);

    /* Remove from wdList */
    pthread_mutex_lock(&wdMutex);
    if (wdId == wdList) {
	wdList = wdId->next;
    } else {
	for (wd = wdList; wd->next != NULL; wd = wd->next) {
	    if (wd->next == wdId) {
		wd->next = wdId->next;
		break;
	    }
	} /* for */
    }
    free(wdId);
    pthread_mutex_unlock(&wdMutex);
    sigprocmask(SIG_SETMASK, &old, NULL); /* unblock clock sig */
    return OK;
}

STATUS
wdStart(WDOG_ID wdId, int delay, FUNCPTR pRoutine, long parameter)
{
    sigset_t set, old;

    if (wdId == NULL || wdId->magic != M_wdLib) {
	errnoSet(S_objLib_OBJ_ID_ERROR);
	return ERROR;
    }
    /* Block clock signal */
    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
    sigprocmask(SIG_BLOCK, &set, &old);

    /* XXX */
    /* mutex_lock n'est pas aync signal safe, or wdStart() doit l'etre */
    /* Il faut trouver une autre solution */
    pthread_mutex_lock(&wdMutex);
    wdId->delay = delay;
    wdId->pRoutine = pRoutine;
    wdId->parameter = parameter;
    pthread_mutex_unlock(&wdMutex);

    sigprocmask(SIG_SETMASK, &old, NULL); /* unblock clock sig */
    return OK;
}

STATUS
wdCancel(WDOG_ID wdId)
{
    sigset_t set, old;

    if (wdId == NULL || wdId->magic != M_wdLib) {
	errnoSet(S_objLib_OBJ_ID_ERROR);
	return ERROR;
    }
    /* Block clock signal */
    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
    sigprocmask(SIG_BLOCK, &set, &old);

    pthread_mutex_lock(&wdMutex);
    wdId->delay = 0;
    wdId->pRoutine = NULL;
    pthread_mutex_unlock(&wdMutex);

    sigprocmask(SIG_SETMASK, &old, NULL); /* unblock clock sig */
    return OK;
}



	
