/*
 * Copyright (c) 1996, 2003-2004 CNRS/LAAS
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

#include "pocolibs-config.h"
__RCSID("$LAAS$");

#include <portLib.h>

#if defined(__RTAI__) && defined(__KERNEL__)
# include <linux/slab.h>
#else
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
#endif
#include <sys/types.h>
#include <inttypes.h>

#include <h2devLib.h>
#include <posterLib.h>
#include <errnoLib.h>

#include "posterLibPriv.h"

#if defined(__RTAI__) && defined(__KERNEL__)
# define malloc(x)     kmalloc(x, GFP_KERNEL)
# define free(x)       kfree(x)
#endif

static STATUS posterInit(void);
#define POSTER_INIT if (posterInit() == ERROR) return ERROR

static POSTER_STR *allPosters = NULL;

/*----------------------------------------------------------------------*/

STATUS 
posterCreate(char *name, int size, POSTER_ID *pPosterId)
{
    POSTER_STR *p;
    STATUS res;

    POSTER_INIT;

    if (pPosterId == NULL) {
	return ERROR;
    }
    /* Allocate posterId */
    p = (POSTER_STR *)malloc(sizeof(POSTER_STR));
    if (p == NULL) {
	errnoSet(S_posterLib_MALLOC_ERROR);
	return ERROR;
    }
    
#ifdef POSTERLIB_ONLY_LOCAL
    p->type = POSTER_ACCESS_LOCAL;
    p->funcs = &posterLocalFuncs;

#else
    /* Si POSTER_HOST est defini, creation remote, sinon creation locale */
    if (getenv("POSTER_HOST") != NULL) {
	p->type = POSTER_ACCESS_REMOTE;
	p->funcs = &posterRemoteFuncs;
    } else {
	p->type = POSTER_ACCESS_LOCAL;
	p->funcs = &posterLocalFuncs;
    }
#endif /* POSTERLIB_ONLY_LOCAL */
    
    /* Init endianness value to local value 
       (will be changed in remote create procedure if necessary) */
    p->endianness = H2_LOCAL_ENDIANNESS;

    /* create poster (! remote func could modified p->endianness !) */
    res = p->funcs->create(name, size, &(p->posterId));
    if (res != OK) {
	/* errno positionne par la fonction specifique */
	free(p);
	return ERROR;
    }

    /* record poster */
    strcpy(p->name, name);
    *pPosterId = (POSTER_ID)p;

    /* Add to list */
    p->next = allPosters;
    allPosters = p;

    return OK;
}

/*----------------------------------------------------------------------*/

STATUS
posterMemCreate(char *name, int busSpace, void *pPool, 
		int size, POSTER_ID *pPosterId)
{
    logMsg("posterMemCreate: Not supported under Unix\n");
    return(ERROR);
}

/*----------------------------------------------------------------------*/

STATUS 
posterDelete(POSTER_ID posterId)
{
    STATUS res;
    POSTER_STR *p = (POSTER_STR *)posterId;

    if (p == NULL) {
#ifdef DEBUG
	    fprintf(stderr, "posterDelete: bad posterId: NULL\n");
#endif
	    errnoSet(S_posterLib_POSTER_CLOSED);
	    return ERROR;
    }
    POSTER_INIT;
    res = p->funcs->delete(p->posterId);
    /* Don't free the POSTER_STR struct. It should stay around, so that 
       anything that call posterRead/Write on a closed poster gets
       the error from the actual local or remote function */
    return res;
}

/*----------------------------------------------------------------------*/

STATUS
posterFind(char *name, POSTER_ID *pPosterId)
{
    POSTER_STR *p;
    POSTER_ID id;

    POSTER_INIT;

    if (pPosterId == NULL) {
	return ERROR;
    }

    /* Look in already known posters first */
    for (p = allPosters; p != NULL; p = p->next) {
	if (strcmp(p->name, name) == 0) {
	    /* found */
		*pPosterId = (POSTER_ID)p;

	    return OK;
	}
    }

    /* Allocation posterId */
    p = (POSTER_STR *)malloc(sizeof(POSTER_STR));
    if (p == NULL) {
	errnoSet(S_posterLib_MALLOC_ERROR);
	return ERROR;
    }
    /* Recherche locale d'abord */
    if (posterLocalFuncs.find(name, &id) == OK) {
	p->type = POSTER_ACCESS_LOCAL;
	p->funcs = &posterLocalFuncs;
	p->posterId = id;
	/* get endianness from local h2dev */
	posterLocalFuncs.getEndianness(id, &p->endianness);
	strcpy(p->name, name);
	/* Add to list */
	p->next = allPosters;
	allPosters = p;
	/* Return value */
	*pPosterId = (POSTER_ID)p;
	return OK;
    } 

#ifndef POSTERLIB_ONLY_LOCAL
    /* Reset errno */
    errnoSet(0);
    
    /* Puis recheche remote */
    if (posterRemoteFuncs.find(name, &id) == OK) {
	p->type = POSTER_ACCESS_REMOTE;
	p->funcs = &posterRemoteFuncs;
	p->posterId = id;
	/* get endianness from REMOTE_POSTER_STR 
	   (itself filled in by  remotePosterFind) */
	posterRemoteFuncs.getEndianness(id, &p->endianness);
	strcpy(p->name, name);
	/* Add to global list */
	p->next = allPosters;
	allPosters = p;
	/* Return value */
	*pPosterId = (POSTER_ID)p;
	return OK;
    }
#endif /* POSTERLIB_ONLY_LOCAL */

    /* Pas trouve' - errno a deja ete positionne' */
    free(p);
    return ERROR;
}

/*----------------------------------------------------------------------*/

int
posterWrite(POSTER_ID posterId, int offset, void *buf, int nbytes)
{
    POSTER_STR *p = (POSTER_STR *)posterId;

    POSTER_INIT;
    return p->funcs->write(p->posterId, offset, buf, nbytes);
}

/*----------------------------------------------------------------------*/

int
posterRead(POSTER_ID posterId, int offset, void *buf, int nbytes)
{
    POSTER_STR *p = (POSTER_STR *)posterId;

    POSTER_INIT;
    return p->funcs->read(p->posterId, offset, buf, nbytes);
}

/*----------------------------------------------------------------------*/

STATUS
posterTake(POSTER_ID posterId, POSTER_OP op) 
{
    POSTER_STR *p = (POSTER_STR *)posterId;

    POSTER_INIT;
    return p->funcs->take(p->posterId, op);
}

/*----------------------------------------------------------------------*/

STATUS 
posterGive(POSTER_ID posterId)
{
    POSTER_STR *p = (POSTER_STR *)posterId;

    POSTER_INIT;
    return p->funcs->give(p->posterId);
}

/*----------------------------------------------------------------------*/

void *
posterAddr(POSTER_ID posterId)
{
    POSTER_STR *p = (POSTER_STR *)posterId;

    return p->funcs->addr(p->posterId);
}

/*----------------------------------------------------------------------*/

STATUS
posterEndianness(POSTER_ID posterId, H2_ENDIANNESS *endianness)
{
    POSTER_STR *p = (POSTER_STR *)posterId;
    *endianness = p->endianness;
    return OK;
}

/*----------------------------------------------------------------------*/
/* only used internally */
/* Set endianness in internal device structure */
STATUS
posterSetEndianness(POSTER_ID posterId, H2_ENDIANNESS endianness)
{
    POSTER_STR *p = (POSTER_STR *)posterId;

    p->endianness = endianness;
    return p->funcs->setEndianness(p->posterId, endianness);
}

/*----------------------------------------------------------------------*/
/* only used internally */
/* Get endianness from internal device structure */
STATUS
posterGetEndianness(POSTER_ID posterId, H2_ENDIANNESS *endianness)
{
    POSTER_STR *p = (POSTER_STR *)posterId;

    return p->funcs->getEndianness(p->posterId, endianness);
}

/*----------------------------------------------------------------------*/

STATUS
posterIoctl(POSTER_ID posterId, int code, void *parg)
{
    POSTER_STR *p = (POSTER_STR *)posterId;

    POSTER_INIT;
    return p->funcs->ioctl(p->posterId, code, parg);
}

/*----------------------------------------------------------------------*/

STATUS
posterShow(void)
{
    POSTER_INIT;
    if (posterLocalFuncs.show() == ERROR) {
	return ERROR;
    }
    /* For now, no way to list remote posters, sigh */
    return OK;
}

/*----------------------------------------------------------------------*/
#if 0
/*
 * Chargement et initialiasation d'une bibliotheque 
 */
static POSTER_FUNCS *
loadAndInitPosterLib(char *libname)
{
    void *posterLibHandle; 
    POSTER_FUNCS *f;

    posterLibHandle = dlopen(libname, RTLD_LAZY);
    if (posterLibHandle == NULL) {
	fprintf(stderr, "loadAndInitPosterLib: %s\n", dlerror());
	errnoSet(S_posterLib_DLOPEN);
	return NULL;
    }
    f = (POSTER_FUNCS *)dlsym(posterLibHandle, POSTER_FUNC_NAME);
    if (f == NULL) {
	fprintf(stderr, "loadAndInitPosterLib: %s\n", dlerror());
	errnoSet(S_posterLib_DLSYM);
	return NULL;
    }
    if (f->init != NULL) {
	if ((*f->init)() != OK) {
	    dlclose(posterLibHandle);
	    return NULL;
	}
    }
    return f;
}
#endif

/*
 * Global initialisation of the poster library
 */
static STATUS
posterInit(void)
{
	static BOOL posterInitDone = FALSE;

	if (posterInitDone) {
		return OK;
	}
#ifndef POSTERLIB_ONLY_LOCAL
	/* Remote posters specific init */
	if (posterRemoteFuncs.init() != OK) {
		return ERROR;
	}
#endif
	posterInitDone = TRUE;
	return OK;
}
