/*
 * Copyright (c) 1996, 2004, 2010,2012 CNRS/LAAS
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
static STATUS remotePosterCreate(const char *name, int size, 
    POSTER_ID *pPosterId);
static STATUS remotePosterMemCreate(const char *name, int busSpace, 
    void *pPool, int size, 
    POSTER_ID *pPosterId);
static int remotePosterWrite(POSTER_ID posterId, int offset, void *buf, 
    int nbytes);
static STATUS remotePosterFind(const char *posterName, POSTER_ID *pPosterId);
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
static STATUS remotePosterShowHost(const char *host);

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
remotePosterCreate(const char *name,	/* Name of the device to create */
    int size,				/* Poster size in bytes */
    POSTER_ID *pPosterId)		/* where to store the resulting Id */
{
	POSTER_CREATE_PAR param;
	POSTER_CREATE_RESULT *res;
	REMOTE_POSTER_ID remPosterId;
	CLIENT *client;
	pthread_key_t key;
	enum clnt_stat s;
	
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
		clnt_pcreateerror(posterHost);
		return(ERROR);
	}

	param.name = strdup(name);
	param.length = size;
	param.endianness = H2_LOCAL_ENDIANNESS;
	
	res = (POSTER_CREATE_RESULT *)malloc(sizeof(POSTER_CREATE_RESULT));
	if (res == NULL)
		return ERROR;

	s = poster_create_1(&param, res, client);
	free(param.name);
	if (s != RPC_SUCCESS) {
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
	
	remPosterId->vxPosterId = res->id;
	remPosterId->hostname = posterHost;
	remPosterId->key = key;
	remPosterId->pid = getpid();
	remPosterId->endianness = H2_LOCAL_ENDIANNESS;
	
	free(res);
	
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
remotePosterMemCreate(const char *name,	/* Device name to be created */
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
*  posterResize  -  Resize a poster, called from posterIoctl only
*
*  Returns : OK or ERROR
*/

static STATUS
remotePosterResize (REMOTE_POSTER_ID remPosterId,	/* Id of the poster */
                    size_t size,			/* new size */
                    CLIENT *client)			/* rpc client */
{
	POSTER_RESIZE_PAR param;
	void *cache;
	int res;
	enum clnt_stat s;

	/* check owner */
	if (remPosterId->pid != getpid()) {
		errnoSet(S_remotePosterLib_NOT_OWNER);
		return ERROR;
	}

	/* optimize if size does not change */
	if (size == remPosterId->dataSize)
		return OK;

	/* create new local cache now - do not realloc() in case the remote
         * fails */
	cache = malloc(size);
	if (!cache) {
		errnoSet(S_posterLib_MALLOC_ERROR);
		return ERROR;
	}

	/* resize remote poster - if this fails, forget about new cache */
	param.id = remPosterId->vxPosterId;
	param.size = (int)size;
	s = poster_resize_1(&param, &res, client);
	if (s != RPC_SUCCESS) {
		free(cache);
		return ERROR;
	}
	if (res != OK) {
		free(cache);
		errnoSet(res);
		return ERROR;
	}
	/* update local cache */
	if (size > remPosterId->dataSize)
		memcpy(cache, remPosterId->dataCache, remPosterId->dataSize);
	else
		memcpy(cache, remPosterId->dataCache, size);
	remPosterId->dataSize = size;

	free(remPosterId->dataCache);
	remPosterId->dataCache = cache;

	return OK;
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
	int res;
	REMOTE_POSTER_ID remPosterId = (REMOTE_POSTER_ID)posterId;
	CLIENT *client = clientCreate(remPosterId->key, remPosterId->hostname);
	enum clnt_stat s;

	if (remPosterId->pid != getpid()) {
		errnoSet(S_remotePosterLib_NOT_OWNER);
		return(ERROR);
	}
	if (client == NULL) {
		errnoSet(S_remotePosterLib_BAD_RPC);
		return ERROR;
	}
	if (nbytes == 0)
		return 0;
	param.id = remPosterId->vxPosterId;
	param.offset = offset;
	param.length = nbytes;
	param.data.data_val = buf;
	param.data.data_len = nbytes;
	
	s = poster_write_1(&param, &res, client);
	
	if (s != RPC_SUCCESS) {
		clnt_perror(client, "remotePosterWrite");
		return(ERROR);
	}
	if (res == ERROR) {
		errnoSet(S_posterLib_BAD_FORMAT);
		return(ERROR);
	}
	return(res);
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
posterFindPath(const char *posterName, REMOTE_POSTER_ID *pPosterId)
{
	POSTER_FIND_RESULT *res = NULL;
	char *host;
	CLIENT *client;
	char *pp, *tmp = NULL, *h, *n;
	char *posterPath = getenv("POSTER_PATH");
	pthread_key_t key;
	enum clnt_stat s;
	
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
		h = strdup(host);
		n = strdup(posterName);
		client = clientCreate(key, h);
		if (client != NULL) {
			res = (POSTER_FIND_RESULT *)malloc(sizeof(POSTER_FIND_RESULT));
			if (res == NULL) {
				errnoSet(S_posterLib_MALLOC_ERROR);
				/* XXX leaks some resources here */
				return ERROR;
			}
			s = poster_find_1(&n, res, client);
			if (s == RPC_SUCCESS && res->status == POSTER_OK) {
				/* Allocate a cache stucture  */
				*pPosterId = (REMOTE_POSTER_ID)
				    malloc(sizeof(REMOTE_POSTER_STR));
				if (*pPosterId == NULL) {
					return(ERROR);
				}
				(*pPosterId)->vxPosterId = res->id;
				(*pPosterId)->key = key;
				(*pPosterId)->hostname = h;
				(*pPosterId)->dataSize = res->length;
				(*pPosterId)->dataCache = malloc(res->length);
				/* record endianness in REMOTE_POSTER_STR */
				(*pPosterId)->endianness = res->endianness;
				(*pPosterId)->pid = -1;
				free(n);
				free(pp);
				free(res);
				return(OK);
			} else {
			  /* clnt_destroy(client); XXX */
			}
		}
		free(h);
		free(n);
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
remotePosterFind (const char *posterName, POSTER_ID *pPosterId)
{
	POSTER_FIND_RESULT *res = NULL;
	REMOTE_POSTER_ID remPosterId;
	CLIENT *client = NULL;
	char *rpc_posterName;
	pthread_key_t key;
	enum clnt_stat s = RPC_FAILED;
	
	if (posterHost != NULL) {
		if (clientKeyFind(posterHost, &key) == -1) {
			fprintf(stderr,
			    "remotePosterFind: %s: clientKeyFind failed\n",
			    posterHost);
			return ERROR;
		}
		client = clientCreate(key, posterHost);
		if (client != NULL) {
			rpc_posterName = strdup(posterName);
			res = (POSTER_FIND_RESULT *)malloc(sizeof(POSTER_FIND_RESULT));
			if (res == NULL) {
				errnoSet(S_posterLib_MALLOC_ERROR);
				return ERROR;
			}
			s = poster_find_1(&rpc_posterName, res, client);
			free(rpc_posterName);
		}
	}
	/* search along POSTER_PATH */
	if (s != RPC_SUCCESS) {
		if (posterFindPath(posterName, &remPosterId) == ERROR) {
			return ERROR;
		}
		*pPosterId = (POSTER_ID)remPosterId;
		return OK;
	}
	/* res != NULL */
	if (res->status != OK) {
		errnoSet(res->status);
		free(res);
		clientRemove(key);
		return ERROR;
	}
	/* res->status == OK */
	remPosterId = (REMOTE_POSTER_ID)malloc(sizeof(REMOTE_POSTER_STR));
	if (remPosterId == NULL)
		return(ERROR);
	remPosterId->vxPosterId = res->id;
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
        size_t len;
	enum clnt_stat s;

	if (client == NULL) {
		errnoSet(S_remotePosterLib_BAD_RPC);
		return ERROR;
	}
	if (nbytes <= 0)
		return 0;
	param.id = remPosterId->vxPosterId;
	param.length = nbytes;
	param.offset = offset;
	
	res = (POSTER_READ_RESULT *)calloc(1, sizeof(POSTER_READ_RESULT));
	if (res == NULL) {
		errnoSet(S_posterLib_MALLOC_ERROR);
		return ERROR;
	}
	s = poster_read_1(&param, res, client);
    
	if (s != RPC_SUCCESS) {
		errnoSet(S_remotePosterLib_BAD_RPC);
		xdr_free((xdrproc_t)xdr_POSTER_READ_RESULT, (char *)res);
		free(res);
		return(ERROR);
	}

	if (res->status != POSTER_OK) {
		errnoSet(res->status);
		xdr_free((xdrproc_t)xdr_POSTER_READ_RESULT, (char *)res);
		free(res);
		return(ERROR);
	}
	if (res->data.data_val == NULL) {
		fprintf(stderr, "remotePosterRead: returning NULL data_val\n");
		errnoSet(S_remotePosterLib_BAD_RPC);
		xdr_free((xdrproc_t)xdr_POSTER_READ_RESULT, (char *)res);
		free(res);
		return(ERROR);
	}
	
        len = res->data.data_len;
        if (len > nbytes) len = nbytes;
	memcpy(buf, res->data.data_val, len);
	
	xdr_free((xdrproc_t)xdr_POSTER_READ_RESULT, (char *)res);
	free(res);
	return len;
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
	enum clnt_stat s;
	
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
	
	param.id = remPosterId->vxPosterId;
	param.length = -1 /* whole poster (in case the size was changed) */;
	param.offset = 0;
	
	res = (POSTER_READ_RESULT *)malloc(sizeof(POSTER_READ_RESULT));
	if (res == NULL) {
		errnoSet(S_posterLib_MALLOC_ERROR);
		return ERROR;
	}
	s = poster_read_1(&param, res, client);
	
	if (s != RPC_SUCCESS) {
		free(res);
		return(ERROR);
	}
	
	if (res->status != POSTER_OK 
	    && res->status != S_posterLib_POSTER_CLOSED 
	    && res->status != S_posterLib_EMPTY_POSTER) {
		errnoSet(res->status);
		free(res);
		return(ERROR);
	}

        /* update cache size if needed */
        if (res->status == POSTER_OK &&
            remPosterId->dataSize < res->data.data_len) {
          void *c = realloc(remPosterId->dataCache, res->data.data_len);
          if (!c) {
            free(res);
            errnoSet(S_posterLib_MALLOC_ERROR);
            return ERROR;
          }
          remPosterId->dataSize = res->data.data_len;
          remPosterId->dataCache = c;
        }

	remPosterId->op = op;
	if (res->status == S_posterLib_POSTER_CLOSED 
	    || res->status == S_posterLib_EMPTY_POSTER) {
		switch (op) {
		case POSTER_READ:
		case POSTER_IOCTL:
			errnoSet(res->status);
			free(res);
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
	free(res);
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
	int res;
	REMOTE_POSTER_ID remPosterId = (REMOTE_POSTER_ID)posterId;
	CLIENT *client = clientCreate(remPosterId->key, remPosterId->hostname);
	enum clnt_stat s;
	
	if (remPosterId->op == POSTER_WRITE) {
		if (client == NULL) {
			errnoSet(S_remotePosterLib_BAD_RPC);
			return ERROR;
		}
		/* copy local cache back to the server */
		param.id = remPosterId->vxPosterId;
		param.offset = 0;
		param.length = remPosterId->dataSize;
		param.data.data_val = remPosterId->dataCache;
		param.data.data_len = remPosterId->dataSize;
		
		s = poster_write_1(&param, &res, client);
		
		if (s != RPC_SUCCESS) {
			clnt_perror(client, "remotePosterGive");
			return(ERROR);
		}
		if (res != remPosterId->dataSize) {
			errnoSet(res);
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
	int pres;
	enum clnt_stat s;
	
	s = poster_delete_1(&(remPosterId->vxPosterId), &pres, client);
		
	/* Mark remote poster Id as deleted */
	remPosterId->dataSize = 0;

	if (s != RPC_SUCCESS) {
		/* RPC error */
		errnoSet(S_remotePosterLib_BAD_RPC);
		return ERROR;
	}
	if (pres != OK) {
		/* Error on the remote side */
		errnoSet(pres);
		return(ERROR);
	} 
	return OK;
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
	enum clnt_stat s;
	
	/* This can be handled locally */
	if (code == FIO_GETSIZE) {
		*(u_long *)parg = (u_long)remPosterId->dataSize;
		return OK;
	}
	if (code == FIO_RESIZE) {
		return remotePosterResize(remPosterId, *(size_t *)parg, client);
	}

	param.id = remPosterId->vxPosterId;
	param.cmd = code;
	
	res = (POSTER_IOCTL_RESULT *)malloc(sizeof(POSTER_IOCTL_RESULT));
	if (res == NULL) {
		errnoSet(S_posterLib_MALLOC_ERROR);
		return ERROR;
	}
	s = poster_ioctl_1(&param, res, client);
	if (s != RPC_SUCCESS) {
		errnoSet(S_remotePosterLib_BAD_RPC);
		free(res);
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
		free(res);
		return ERROR;
	}
	free(res);
	return OK;
}

static STATUS
remotePosterShowHost(const char *host)
{
    POSTER_LIST_RESULT *res = NULL;
    POSTER_LIST *l;
    CLIENT *client = NULL;
    H2TIME h2time;
    H2TIMESPEC h2ts;
    pthread_key_t key;
    char *h;
    enum clnt_stat s;

    if (clientKeyFind(host, &key) == -1) {
	fprintf(stderr, "remotePosterShow: %s: clientKeyFind failed\n",
		host);
	return ERROR;
    }
    h = strdup(host);
    client = clientCreate(key, h);
    if (client != NULL) {
	res = (POSTER_LIST_RESULT *)malloc(sizeof(POSTER_LIST_RESULT));
	if (res == NULL) {\
	    errnoSet(S_posterLib_MALLOC_ERROR);
	    return ERROR;
	}
	s = poster_list_1(NULL, res, client);
	if (s != RPC_SUCCESS) {
	    clnt_perror(client, "remotePosterList");
	    errnoSet(S_remotePosterLib_BAD_RPC);
	    return ERROR;
	}
	l = res->list;
	while (l) {
	    logMsg("%-32s %8s:%-3d %8d", l->name, host, l->id, l->size);
	    if (l->fresh) {
		h2ts.tv_sec = l->tv_sec;
		h2ts.tv_nsec = l->tv_nsec;
		h2timeFromTimespec(&h2time, &h2ts);
		logMsg(" %02dh:%02dmin%02ds %lu\n", h2time.hour, h2time.minute,
		       h2time.sec, h2time.ntick);
	    } else
		logMsg(" EMPTY_POSTER!\n");
	    
	    l = l->next;
	}
	xdr_free((xdrproc_t)xdr_POSTER_LIST_RESULT, (char *)res);
	free(res);
    }
    free(h);
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
    char *host, *pp, *tmp = NULL;
    char *posterPath = getenv("POSTER_PATH");

    if (posterHost != NULL) {
	remotePosterShowHost(posterHost);
    }
    if (posterPath == NULL || *posterPath == '\0')
	return OK;
    pp = strdup(posterPath);
    for (host = strtok_r(pp, ":", &tmp); host != NULL; 
	 host = strtok_r(NULL, ":", &tmp)) {
	remotePosterShowHost(host);
    }
    free(pp);
    return OK;
}
