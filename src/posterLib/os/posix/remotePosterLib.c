/*
 * Copyright (c) 1996, 2004 CNRS/LAAS
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

#include <sys/types.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <rpc/rpc.h>

#include <portLib.h>
#include <h2devLib.h>
#include <errnoLib.h>
#include <posterLib.h>

#include "posterLibPriv.h"
#include "remotePosterLibPriv.h"

/* Global variables */
static const char *posterHost;

static STATUS remotePosterInit(void);
static STATUS remotePosterCreate(char *name, int size, 
    POSTER_ID *pPosterId);
static STATUS remotePosterMemCreate(char *name, int busSpace, 
    void *pPool, int size, 
    POSTER_ID *pPosterId);
static int remotePosterWrite(POSTER_ID posterId, int offset, void *buf, 
    int nbytes);
static STATUS remotePosterFind(char *posterName, POSTER_ID *pPosterId);
static int remotePosterRead(POSTER_ID posterId, int offset, void *buf, 
    int nbytes);
static STATUS remotePosterTake(POSTER_ID posterId, POSTER_OP op);
static STATUS remotePosterGive(POSTER_ID posterId);
static void * remotePosterAddr(POSTER_ID posterId);
static STATUS remotePosterDelete(POSTER_ID posterId);
static STATUS remotePosterIoctl(POSTER_ID posterId, int code, void *parg);
static STATUS remotePosterShow(void);
static STATUS remotePosterSetEndianness(POSTER_ID posterId, 
    H2_ENDIANNESS endianness);
static STATUS remotePosterGetEndianness(POSTER_ID posterId, 
    H2_ENDIANNESS *endianness);

const POSTER_FUNCS posterRemoteFuncs = {
	remotePosterInit,
	remotePosterCreate,
	remotePosterMemCreate,
	remotePosterDelete,
	remotePosterFind,
	remotePosterWrite,
	remotePosterRead,
	remotePosterTake,
	remotePosterGive,
	remotePosterAddr,
	remotePosterIoctl,
	remotePosterShow,
	remotePosterSetEndianness,
	remotePosterGetEndianness
};


/*****************************************************************************
*
* remotePosterInit - Initialization routine 
*
* Description : 
*
* Returns : OK or ERROR
*/
static STATUS 
remotePosterInit(void)
{
	if (posterHost == NULL) 
		posterHost = getenv("POSTER_HOST");
	return OK;
}

/******************************************************************************
*
*  posterCreate  -  Poster creation
*
*  Description: this function triggers the creation of the poster 
*               in the posterServ task on the remote host (described by
*               POSTER_HOST).
*
*  Returns :  OK or ERROR
*/

static STATUS 
remotePosterCreate(char *name,		/* Name of the device to create */
    int size,				/* Poster size in bytes */
    POSTER_ID *pPosterId)		/* where to store the resulting Id */
{
	POSTER_CREATE_PAR param;
	POSTER_CREATE_RESULT *res;
	REMOTE_POSTER_ID remPosterId;
	CLIENT *client;
	pthread_key_t key;
	
	param.name = name;
	param.length = size;
	param.endianness = H2_LOCAL_ENDIANNESS;
	
	if (posterHost == NULL) {
		errnoSet(S_remotePosterLib_POSTER_HOST_NOT_DEFINED);
		return ERROR;
	}
	/* look for thread-specific client connexion */
	if (clientKeyFind(posterHost, &key) == -1) {
		return(ERROR);
	}
	client = clientCreate(key, posterHost);
	if (client == NULL) {
		errnoSet(S_remotePosterLib_BAD_RPC);
		clnt_pcreateerror("remotePosterLib/cnt_create");
		return(ERROR);
	}

	res = poster_create_1(&param, client);
	if (res == NULL) {
		clnt_perror(client, "poster_create_1");
		return(ERROR);
	}
	
	if (res->status != POSTER_OK) {
		errnoSet(res->status);
		xdr_free((xdrproc_t)xdr_POSTER_CREATE_RESULT, (char *)res);
		return(ERROR);
	}
	
	remPosterId = (REMOTE_POSTER_ID)malloc(sizeof(REMOTE_POSTER_STR));
	if (remPosterId == NULL) {
		errnoSet(S_remotePosterLib_BAD_ALLOC);
		xdr_free((xdrproc_t)xdr_POSTER_CREATE_RESULT, (char *)res);
		return(ERROR);
	}
	
	remPosterId->vxPosterId = (void *)res->id;
	remPosterId->hostname = posterHost;
	remPosterId->key = key;
	remPosterId->pid = getpid();
	remPosterId->endianness = H2_LOCAL_ENDIANNESS;
	
	xdr_free((xdrproc_t)xdr_POSTER_CREATE_RESULT, (char *)res);
	
	/* Allocate a data cache */
	remPosterId->dataSize = size;
	remPosterId->dataCache = malloc(size);
	if (remPosterId->dataCache == NULL) {
		errnoSet(S_remotePosterLib_BAD_ALLOC);
		return(ERROR);
	}
	*pPosterId = (POSTER_ID)remPosterId;
	return OK;
}


static STATUS 
remotePosterMemCreate(char *name,	/* Device name to be created */
    int busSpace,			/* Address space of the memoy pool */
    void *pPool,			/* Effective address within the pool */
    int size,				/* Poster size, in bytes */
    POSTER_ID *pPosterId)		/* Where to store the result */
{
	fprintf(stderr, "posterMemCreate: not suppored on Unix\n");
	return(ERROR);
}

/*****************************************************************************
*
*  posterWrite  -  Write data to a poster
*
*  Returns : number of bytes really written or ERROR
*/

static int 
remotePosterWrite (POSTER_ID posterId,	/* Id of the poster */ 
    int offset,				/* Offset relative to the start 
					   of the poster */
    void *buf,				/* message to write */
    int nbytes)				/* number of bytes to write */
{
	POSTER_WRITE_PAR param;
	int *res;
	REMOTE_POSTER_ID remPosterId = (REMOTE_POSTER_ID)posterId;
	CLIENT *client = clientCreate(remPosterId->key, remPosterId->hostname);
	
	if (remPosterId->pid != getpid()) {
		errnoSet(S_remotePosterLib_NOT_OWNER);
		return(ERROR);
	}
	if (client == NULL) {
		errnoSet(S_remotePosterLib_BAD_RPC);
		return ERROR;
	}
	param.id = (int)(long)remPosterId->vxPosterId;
	param.offset = offset;
	param.length = nbytes;
	param.data.data_val = buf;
	param.data.data_len = nbytes;
	
	res = poster_write_1(&param, client);
	
	if (res == NULL) {
		clnt_perror(client, "remotePosterWrite");
		return(ERROR);
	}
	if (*res != nbytes) {
		errnoSet(*res);
		return(ERROR);
	}
	return(*res);
}



/******************************************************************************
*
*   posterFind  -  Find a poster by name
* 
*   Description:  this function looks through POSTER_PATH for a 
*                 posterServ process providing the named poster
*
*   Returns :
*   OK or ERROR
*/

static STATUS 
posterFindPath(char *posterName, REMOTE_POSTER_ID *pPosterId)
{
	POSTER_FIND_RESULT *res = NULL;
	char *host;
	CLIENT *client;
	char *pp, *tmp;
	char *posterPath = getenv("POSTER_PATH");
	pthread_key_t key;
	
	if (posterPath == NULL || *posterPath == '\0') {
		errnoSet(S_h2devLib_NOT_FOUND);
		return ERROR;
	}
	pp = strdup(posterPath);
	for (host = strtok_r(pp, ":", &tmp); host != NULL; 
	     host = strtok_r(NULL, ":", &tmp)) {
#ifdef DEBUG
		fprintf(stderr, "posterFind: searching on %s\n", host);
#endif
		if (clientKeyFind(host, &key) == -1) {
			fprintf(stderr, 
			    "posterFindPath: %s: clientKeyFind failed\n", 
			    host);
			continue;
		}
		client = clientCreate(key, host);
		if (client != NULL) {
			res = poster_find_1(&posterName, client);
			if (res != NULL && res->status == POSTER_OK) {
				/* Allocate a cache stucture  */
				*pPosterId = (REMOTE_POSTER_ID)
				    malloc(sizeof(REMOTE_POSTER_STR));
				if (*pPosterId == NULL) {
					return(ERROR);
				}
				(*pPosterId)->vxPosterId = (void *)(long)(res->id);
				(*pPosterId)->key = key;
				(*pPosterId)->hostname = strdup(host);
				(*pPosterId)->dataSize = res->length;
				(*pPosterId)->dataCache = malloc(res->length);
				/* record endianness in REMOTE_POSTER_STR */
				(*pPosterId)->endianness = res->endianness;
				(*pPosterId)->pid = -1;
				free(pp);
				xdr_free((xdrproc_t)xdr_POSTER_FIND_RESULT, 
				    (char *)res);
				return(OK);
			} else {
				clnt_destroy(client);
			}
		}
	} /* for */
	free(pp);
	if (res != NULL) 
		errnoSet(res->status);
	else
		errnoSet(S_h2devLib_NOT_FOUND);
	xdr_free((xdrproc_t)xdr_POSTER_FIND_RESULT, (char *)res);
	return(ERROR);
}


static STATUS 
remotePosterFind (char *posterName, POSTER_ID *pPosterId)
{
	POSTER_FIND_RESULT *res = NULL;
	REMOTE_POSTER_ID remPosterId;
	CLIENT *client = NULL;
	pthread_key_t key;
	
	if (posterHost != NULL) {
		if (clientKeyFind(posterHost, &key) == -1) {
			fprintf(stderr,
			    "remotePosterFind: %s: clientKeyFind failed\n",
			    posterHost);
			return ERROR;
		}
		client = clientCreate(key, posterHost);
		if (client != NULL) {
			res = poster_find_1(&posterName, client);
		}
	}
	/* search along POSTER_PATH */
	if (res == NULL) {
		if (posterFindPath(posterName, &remPosterId) == ERROR) {
			return ERROR;
		}
		*pPosterId = (POSTER_ID)remPosterId;
		return OK;
	}
	/* res != NULL */
	if (res->status != OK) {
		errnoSet(res->status);
		xdr_free((xdrproc_t)xdr_POSTER_FIND_RESULT, (char *)res);
		clientRemove(key);
		return ERROR;
	}
	/* res->status == OK */
	remPosterId = (REMOTE_POSTER_ID)malloc(sizeof(REMOTE_POSTER_STR));
	if (remPosterId == NULL)
		return(ERROR);
	remPosterId->vxPosterId = (void *)(long)(res->id);
	remPosterId->key = key;
	remPosterId->hostname = posterHost;
	/* record endianness in REMOTE_POSTER_STR */
	remPosterId->endianness = res->endianness;
	/* the found poster cannot be written */
	remPosterId->pid = -1;
	/* Allocate the cache structure */
	remPosterId->dataSize = res->length;
	remPosterId->dataCache = malloc(res->length);
	xdr_free((xdrproc_t)xdr_POSTER_FIND_RESULT, (char *)res);
	*pPosterId = (POSTER_ID)remPosterId;
	return (OK);
}

/******************************************************************************
*
*  posterRead  -  read a poster
*
*  Description : sends a read request to the posterServ process
*                and store the received data
*
*  Returns : number of bytes read or ERROR
*/


static int 
remotePosterRead(POSTER_ID posterId,   /* Id of the poster to read */
    int offset,			       /* offset from start of poster */
    void *buf,			       /* buffer to store the data */
    int nbytes)			       /* number of bytes to read */	
{
	POSTER_READ_RESULT *res;
	POSTER_READ_PAR param;
	REMOTE_POSTER_ID remPosterId = (REMOTE_POSTER_ID)posterId;
	CLIENT *client = clientCreate(remPosterId->key, remPosterId->hostname);
	
	if (client == NULL) {
		errnoSet(S_remotePosterLib_BAD_RPC);
		return ERROR;
	}
	param.id = (int)(long)(remPosterId->vxPosterId);
	param.length = nbytes;
	param.offset = offset;
	
	res = poster_read_1(&param, client);
    
	if (res == NULL)
		return(ERROR);

	if (res->status != POSTER_OK) {
		errnoSet(res->status);
		xdr_free((xdrproc_t)xdr_POSTER_READ_RESULT, (char *)res);
		return(ERROR);
	}
	if (res->data.data_val == NULL) {
		fprintf(stderr, "remotePosterRead: returning NULL data_val\n");
		errnoSet(S_remotePosterLib_BAD_RPC);
		xdr_free((xdrproc_t)xdr_POSTER_READ_RESULT, (char *)res);
		return(ERROR);
	}
	
	memcpy(buf, res->data.data_val, nbytes);
	
	xdr_free((xdrproc_t)xdr_POSTER_READ_RESULT, (char *)res);
	return (nbytes);
}

/******************************************************************************
*
*   posterTake - take access control to a poster
*
*   Returns : OK or ERROR
*
*   On remote posters, copy the poster contents into the data cache
*/
static STATUS 
remotePosterTake(POSTER_ID posterId, POSTER_OP op)
{
	POSTER_READ_RESULT *res;
	POSTER_READ_PAR param;
	REMOTE_POSTER_ID remPosterId = (REMOTE_POSTER_ID)posterId;
	CLIENT *client = clientCreate(remPosterId->key, remPosterId->hostname);
	
	if (client == NULL) {
		errnoSet(S_remotePosterLib_BAD_RPC);
		return ERROR;
	}
	switch(op) {
	case POSTER_READ:
	case POSTER_IOCTL:
		break;
	case POSTER_WRITE:
		if (remPosterId->pid != getpid()) {
			errnoSet(S_remotePosterLib_NOT_OWNER);
			return(ERROR);
		}
		break;
	default:
		errnoSet(S_remotePosterLib_BAD_OP);
		return(ERROR);
	} /* switch */
	
	param.id = (int)(long)(remPosterId->vxPosterId);
	param.length = remPosterId->dataSize;
	param.offset = 0;
	
	res = poster_read_1(&param, client);
	
	if (res == NULL)
		return(ERROR);
	
	if (res->status != POSTER_OK 
	    && res->status != S_posterLib_POSTER_CLOSED 
	    && res->status != S_posterLib_EMPTY_POSTER) {
		xdr_free((xdrproc_t)xdr_POSTER_READ_RESULT, (char *)res);
		errnoSet(res->status);
		return(ERROR);
	}
	remPosterId->op = op;
	if (res->status == S_posterLib_POSTER_CLOSED 
	    || res->status == S_posterLib_EMPTY_POSTER) {
		switch (op) {
		case POSTER_READ:
		case POSTER_IOCTL:
			errnoSet(res->status);
			xdr_free((xdrproc_t)xdr_POSTER_READ_RESULT, 
			    (char *)res);
			return(ERROR);
		case POSTER_WRITE:
			memset(remPosterId->dataCache, 0, 
			    remPosterId->dataSize);
			break;
		} /* switch */
	} else {
		memcpy(remPosterId->dataCache, res->data.data_val, 
		    remPosterId->dataSize);
	}
	xdr_free((xdrproc_t)xdr_POSTER_READ_RESULT, (char *)res);
	return(OK);
}

/******************************************************************************
* 
*   posterGive - release access control to a poster
*
*   Retourns : OK or ERROR
*
*   On remote posters, copy back the cached data into the remote server
*/
static STATUS
remotePosterGive(POSTER_ID posterId)
{
	POSTER_WRITE_PAR param;
	int *res;
	REMOTE_POSTER_ID remPosterId = (REMOTE_POSTER_ID)posterId;
	CLIENT *client = clientCreate(remPosterId->key, remPosterId->hostname);
	
	
	if (remPosterId->op == POSTER_WRITE) {
		if (client == NULL) {
			errnoSet(S_remotePosterLib_BAD_RPC);
			return ERROR;
		}
		/* copy local cache back to the server */
		param.id = (int)(long)remPosterId->vxPosterId;
		param.offset = 0;
		param.length = remPosterId->dataSize;
		param.data.data_val = remPosterId->dataCache;
		param.data.data_len = remPosterId->dataSize;
		
		res = poster_write_1(&param, client);
		
		if (res == NULL) {
			clnt_perror(client, "remotePosterGive");
			return(ERROR);
		}
		if (*res != remPosterId->dataSize) {
			errnoSet(*res);
			return(ERROR);
		}
	}
	return(OK);
}

/*****************************************************************************
*
*   posterAddr - returns a local memory address of the poster
*
*   Returns : address or NULL
*/
static void *
remotePosterAddr(POSTER_ID posterId)
{
	return(((REMOTE_POSTER_ID)posterId)->dataCache);
}

/*****************************************************************************
 *
*   posterSetEndianness - 
*
*   Returns : OK 
*/
static STATUS
remotePosterSetEndianness(POSTER_ID posterId, H2_ENDIANNESS endianness)
{
	((REMOTE_POSTER_ID)posterId)->endianness = endianness; 
	return OK;
}

/*****************************************************************************
*
*   posterGetEndianness - 
*
*   Returns : 
*/
static STATUS
remotePosterGetEndianness(POSTER_ID posterId, H2_ENDIANNESS *endianness)
{
	*endianness = ((REMOTE_POSTER_ID)posterId)->endianness;
	return OK;
}

/******************************************************************************
*
*   posterDelete  -  Delete a poster
*
*   Returns : OK or ERROR
*/

static STATUS 
remotePosterDelete(POSTER_ID posterId)
{
	REMOTE_POSTER_ID remPosterId = (REMOTE_POSTER_ID)posterId;
	CLIENT *client = clientCreate(remPosterId->key, remPosterId->hostname);
	
	int *pres = poster_delete_1((int *)&(remPosterId->vxPosterId), client);
	
	if (pres != NULL) {
		/* Mark remote poster Id as deleted */
		remPosterId->dataSize = 0;
		return(*pres);
	} else {
		return(ERROR);
	}
}



/*****************************************************************************
*
*   posterIoctl  -  Ask information about a poster
*
*   Description:
*   This functions access some control information about posters
*
*   Returns : OK or ERROR
*/

static STATUS 
remotePosterIoctl(POSTER_ID posterId,	/* poster Id */
    int code,				/* code of the control function */
    void *parg)				/* address of in/out arguments */
{
	REMOTE_POSTER_ID remPosterId = (REMOTE_POSTER_ID)posterId;
	
	POSTER_IOCTL_PAR param;
	POSTER_IOCTL_RESULT *res;
	CLIENT *client = clientCreate(remPosterId->key, remPosterId->hostname);
	
	/* This can be handled locally */
	if (code == FIO_GETSIZE) {
		*(u_long *)parg = (u_long)remPosterId->dataSize;
		return OK;
	}
	
	param.id = (int)(long)remPosterId->vxPosterId;
	param.cmd = code;
	
	res = poster_ioctl_1(&param, client);
	if (res == NULL) {
		return ERROR;
	}
	if (res->status != POSTER_OK) {
		errnoSet(res->status);
		return ERROR;
	}
	switch (code) {
	case FIO_GETDATE:
		memcpy(parg, &(res->ntick), sizeof(H2TIME));
		break;
	case FIO_NMSEC:
		*(u_long *)parg = res->ntick;
		break;
	default:
		errnoSet(S_posterLib_BAD_IOCTL_CODE);
		return ERROR;
	}
	return OK;
}



/*****************************************************************************
*
*  posterShow  -  show the list of posters in the system
*
*  Description :
*  A list of existing posters is written to stdout
*
*  Returns : OK or ERROR.
*/

static STATUS 
remotePosterShow(void)

{
	fprintf(stderr, "remotePosterShow: not implemented\n");
	return (ERROR);
}
