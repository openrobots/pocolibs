/*
 * Copyright (c) 1996, 2003 CNRS/LAAS
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
static char *posterHost;

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
* remotePosterInit - Routine d'initialisation de la bibliotheque
*
* Description : 
*
* Retourne : OK ou ERROR
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
*  posterCreate  -  Creation d'un poster
*
*  Description: Cette function provoque la creation du poster par la tache
*               posterServ sur VxWorks. (sur le rack POSTER_HOST)
*
*  Retourne :  OK ou ERROR
*/

static STATUS 
remotePosterCreate(char *name,		/* Nom du device a creer */
    int size,				/* Taille poster - en bytes */
    POSTER_ID *pPosterId)		/* Ou` mettre l'id du poster */
{
	POSTER_CREATE_PAR param;
	POSTER_CREATE_RESULT *res;
	REMOTE_POSTER_ID remPosterId;
	CLIENT *client;
	
	param.name = name;
	param.length = size;
	param.endianness = H2_LOCAL_ENDIANNESS;
	
	if (posterHost == NULL) {
		errnoSet(S_remotePosterLib_POSTER_HOST_NOT_DEFINED);
		return ERROR;
	}
	
	client = clnt_create(posterHost, POSTER_SERV, POSTER_VERSION, "tcp");
	if (client == NULL) {
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
	
	remPosterId->vxPosterId = (void *)(long)res->id;
	remPosterId->client = client;
	remPosterId->pid = getpid();
	remPosterId->endianness = H2_LOCAL_ENDIANNESS;
	
	xdr_free((xdrproc_t)xdr_POSTER_CREATE_RESULT, (char *)res);
	
	/* Allocation du cache des donne'es */
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
remotePosterMemCreate(char *name,	/* Nom du device a creer */
    int busSpace,			/* espace d'adressage de 
					   l'addresse pPool */
    void *pPool,			/* adresse Pool de memoire pour 
					   le poster */
    int size,				/* Taille poster - en bytes */
    POSTER_ID *pPosterId)		/* Ou` mettre l'id du poster */
{
	fprintf(stderr, "posterMemCreate: not suppored on Unix\n");
	return(ERROR);
}

/*****************************************************************************
*
*  posterWrite  -  Ecrire sur un poster
*
*  Retourne : nombre de bytes effectivement ecrit ou ERROR.
*/

static int 
remotePosterWrite (POSTER_ID posterId,	/* Identificateur du poster */ 
    int offset,				/* Offset par rapport debut poster */
    void *buf,				/* Message a ecrire */
    int nbytes)				/* Nombre de bytes a ecrire */
{
	POSTER_WRITE_PAR param;
	int *res;
	REMOTE_POSTER_ID remPosterId = (REMOTE_POSTER_ID)posterId;
	
	if (remPosterId->pid != getpid()) {
		errnoSet(S_remotePosterLib_NOT_OWNER);
		return(ERROR);
	}
	param.id = (uint64_t)(long)remPosterId->vxPosterId;
	param.offset = offset;
	param.length = nbytes;
	param.data.data_val = buf;
	param.data.data_len = nbytes;
	
	res = poster_write_1(&param, remPosterId->client);
	
	if (res == NULL) {
		clnt_perror(remPosterId->client, "remotePosterWrite");
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
*   posterFind  -  Chercher un poster par son nom
* 
*   Description:  Cette fonction permet de s'initialiser comme client
*                 du serveur de posters ainsi que la mbox. Il associe en
*                 outre un numero au nom du poster.
*
*   Retourne :
*   OK ou ERROR
*/

static STATUS 
posterFindPath(char *posterName, REMOTE_POSTER_ID *pPosterId)
{
	POSTER_FIND_RESULT *res = NULL;
	char *host;
	CLIENT *client;
	char *pp, *tmp;
	char *posterPath = getenv("POSTER_PATH");
	
	if (posterPath == NULL || *posterPath == '\0') {
		errnoSet(S_h2devLib_NOT_FOUND);
		return ERROR;
	}
	pp = strdup(posterPath);
	for (host = strtok_r(pp, ":", &tmp); host != NULL; 
	     host = strtok_r(NULL, ":", &tmp)) {
		client = clnt_create(host, POSTER_SERV, POSTER_VERSION, "tcp");
#ifdef DEBUG
		fprintf(stderr, "posterFind: searching on %s\n", host);
#endif
		if (client != NULL) {
			res = poster_find_1(&posterName, client);
			if (res != NULL && res->status == POSTER_OK) {
				/* Allocation stucture cache */
				*pPosterId = (REMOTE_POSTER_ID)
				    malloc(sizeof(REMOTE_POSTER_STR));
				if (*pPosterId == NULL) {
					return(ERROR);
				}
				(*pPosterId)->vxPosterId = (void *)(long)(res->id);
				(*pPosterId)->client = client;
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
	POSTER_FIND_RESULT *res;
	REMOTE_POSTER_ID remPosterId;
	CLIENT *client=NULL;
	
	if (posterHost != NULL) {
		client = clnt_create(posterHost, POSTER_SERV, 
		    POSTER_VERSION, "tcp");
		if (client == NULL) {
			clnt_pcreateerror("remotePosterLib/cnt_create");
			return ERROR;
		}
		res = poster_find_1(&posterName, client);
	} else {
		res = NULL;
	}
	
	/* Recherche dans POSTER_PATH */
	if (res == NULL) {
		if (client !=  NULL)
			clnt_destroy(client);
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
		clnt_destroy(client);
		return ERROR;
	}
	/* res->status == OK */
	remPosterId = (REMOTE_POSTER_ID)malloc(sizeof(REMOTE_POSTER_STR));
	if (remPosterId == NULL)
		return(ERROR);
	remPosterId->vxPosterId = (void *)(long)(res->id);
	remPosterId->client = client;
	/* record endianness in REMOTE_POSTER_STR */
	remPosterId->endianness = res->endianness;
	/* On ne pourra pas ecrire dans un poster trouve' */
	remPosterId->pid = -1;
	/* Allocation de la structure cache */
	remPosterId->dataSize = res->length;
	remPosterId->dataCache = malloc(res->length);
	xdr_free((xdrproc_t)xdr_POSTER_FIND_RESULT, (char *)res);
	*pPosterId = (POSTER_ID)remPosterId;
	return (OK);
}

/******************************************************************************
*
*  posterRead  -  Lire un poster
*
*  Description : Envoi une requete de lecture de poster au serveur de posters
*                et transmet les donne'es lues.
*
*  Retourne : nombre de bytes lus ou ERROR.
*/


static int 
remotePosterRead(POSTER_ID posterId,   /* Identificateur du poster a lire */
    int offset,			       /* Offset a partir du debut du poster */
    void *buf,			       /* Buffer ou` mettre les informations */
    int nbytes)			       /* Nombre de bytes a lire */	
{
	POSTER_READ_RESULT *res;
	static POSTER_READ_PAR param;
	REMOTE_POSTER_ID remPosterId = (REMOTE_POSTER_ID)posterId;
	
	
	param.id = (uint64_t)(long)(remPosterId->vxPosterId);
	param.length = nbytes;
	param.offset = offset;
	
	res = poster_read_1(&param, remPosterId->client);
    
	if (res == NULL)
		return(ERROR);

	if (res->status != POSTER_OK) {
		errnoSet(res->status);
		xdr_free((xdrproc_t)xdr_POSTER_READ_RESULT, (char *)res);
		return(ERROR);
	}
	
	memcpy(buf, res->data.data_val, nbytes);
	
	xdr_free((xdrproc_t)xdr_POSTER_READ_RESULT, (char *)res);
	return (nbytes);
}

/******************************************************************************
*
*   posterTake - Prendre l'acces a un poster
*
*   Retourne : OK ou ERROR
*
*   Sur UNIX, recopie du poster dans le cache des donne'es
*/
static STATUS 
remotePosterTake(POSTER_ID posterId, POSTER_OP op)
{
	POSTER_READ_RESULT *res;
	static POSTER_READ_PAR param;
	REMOTE_POSTER_ID remPosterId = (REMOTE_POSTER_ID)posterId;
	
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
	
	param.id = (uint64_t)(long)(remPosterId->vxPosterId);
	param.length = remPosterId->dataSize;
	param.offset = 0;
	
	res = poster_read_1(&param, remPosterId->client);
	
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
*   posterGive - Liberer l'acces a un poster
*
*   Retourne : OK ou ERROR
*
*   Sur UNIX, recopie du cache des donne'es vers le poster
*/
static STATUS
remotePosterGive(POSTER_ID posterId)
{
	POSTER_WRITE_PAR param;
	int *res;
	REMOTE_POSTER_ID remPosterId = (REMOTE_POSTER_ID)posterId;
	
	
	if (remPosterId->op == POSTER_WRITE) {
		/* recopie du cache local dans le poster */
		param.id = (uint64_t)(long)remPosterId->vxPosterId;
		param.offset = 0;
		param.length = remPosterId->dataSize;
		param.data.data_val = remPosterId->dataCache;
		param.data.data_len = remPosterId->dataSize;
		
		res = poster_write_1(&param, remPosterId->client);
		
		if (res == NULL) {
			clnt_perror(remPosterId->client, "remotePosterGive");
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
*   posterAddr - retourne l'adresse des donne'es du poster
*
*   Retourne : adresse ou NULL
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
*   Retourne : 
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
*   Retourne : 
*/
static STATUS
remotePosterGetEndianness(POSTER_ID posterId, H2_ENDIANNESS *endianness)
{
	*endianness = ((REMOTE_POSTER_ID)posterId)->endianness;
	return OK;
}

/******************************************************************************
*
*   posterDelete  -  Deleter un poster
*
*   Retourne : OK ou ERROR
*/

static STATUS 
remotePosterDelete(POSTER_ID posterId)
{
	REMOTE_POSTER_ID remPosterId = (REMOTE_POSTER_ID)posterId;
	
	int *pres = poster_delete_1((int *)&(remPosterId->vxPosterId), 
	    remPosterId->client);
	
	if (pres != NULL) {
		return(*pres);
	} else {
		return(ERROR);
	}
}



/*****************************************************************************
*
*   posterIoctl  -  Demande de renseignements sur le poster
*
*   Description:
*   Analogue a la fonction ioctl sous UNIX, cette routine permet la demande
*   de renseignements sur un poster.
*
*   Retourne : OK ou ERROR
*/

static STATUS 
remotePosterIoctl(POSTER_ID posterId,	/* Identificateur du poster */
    int code,				/* Code de la fonction a executer */
    void *parg)				/* Adresse argument entree/sortie */
{
	REMOTE_POSTER_ID remPosterId = (REMOTE_POSTER_ID)posterId;
	
	POSTER_IOCTL_PAR param;
	POSTER_IOCTL_RESULT *res;
	
	param.id = (uint64_t)(long)remPosterId->vxPosterId;
	param.cmd = code;
	res = poster_ioctl_1(&param, remPosterId->client);
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
	}
	return OK;
}



/*****************************************************************************
*
*  posterShow  -  Montre l'etat des devices poster du systeme
*
*  Description :
*  L'etat des devices poster est envoye a la sortie standard.
*
*  Retourne : OK ou ERROR.
*/

static STATUS 
remotePosterShow(void)

{
	fprintf(stderr, "remotePosterShow: not implemented\n");
	return (ERROR);
}
