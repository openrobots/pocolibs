/*
 * Copyright (c) 1990, 2003, 2010,2012 CNRS/LAAS
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
/***
 *** Serveur de posters pour VxWorks
 *** base' sur RPC
 ***
 *** Fonctions de service RPC
 ***/

#include "pocolibs-config.h"

#include <sys/types.h>
#define PORTMAP
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <portLib.h>
#include <h2initGlob.h>
#include <errnoLib.h>
#include <h2errorLib.h>
#include <posterLib.h>
#include <h2endianness.h>
#include <h2timeLib.h>

#include "posters.h"   /* generated by rpcgen */

#include <h2devLib.h>
#include "posterLibPriv.h"
#include "remPosterId.h"

#if defined(HAVE_RPCGEN_C)
#define SVC(f) f##_svc
#else
#define SVC(f) f
#endif

int verbose = 0;
int _rpcsvcdirty = 0;
#ifdef DEBUG
/* reference created by Solaris rpcgen if DEBUG is defined */
int _rpcpmstart = 0;
#endif
/*----------------------------------------------------------------------*/

/**
 ** Fonction principale de la tache de service
 **/
extern void poster_serv_1(struct svc_req *rqstp, SVCXPRT *transp);

STATUS 
posterServ(void)
{
    SVCXPRT *transp;
 
    if (h2initGlob(0) == ERROR)
	    return(ERROR);
 
    (void) pmap_unset(POSTER_SERV, POSTER_VERSION);
    
    transp = svctcp_create(RPC_ANYSOCK, 0, 0);
    if (transp == NULL) {
        fprintf(stderr, "posterServ: cannot create tcp service.");
        return(ERROR);
    }
    if (!svc_register(transp, POSTER_SERV, POSTER_VERSION, poster_serv_1,
		      IPPROTO_TCP)) {
	fprintf(stderr, "posterServ: unable to register (POSTER_SERV tcp).");
        return(ERROR);
    }
    svc_run();
    svc_unregister(POSTER_SERV, POSTER_VERSION);
    return(ERROR);
}

/*----------------------------------------------------------------------*/

/**
 ** Services proprement dit
 **/

bool_t
SVC(poster_find_1)(char **nom, POSTER_FIND_RESULT *res, struct svc_req *clnt)
{
    POSTER_ID id;

    /* look for the poster locally only */
    if (posterLocalFuncs.find(*nom, &id) == ERROR) {
	res->status = errnoGet();
	if (verbose) {
	    fprintf(stderr, "posterServ error: find(%s) ", *nom);
	    h2printErrno(res->status);
	}
	return 1;
    }

    /* get the poster length */
    if (posterLocalFuncs.ioctl(id, FIO_GETSIZE, &res->length) == ERROR) {
	res->status = errnoGet();
	if (verbose) {
	    fprintf(stderr, "posterServ error: find: posterIoctl ");
	    h2printErrno(res->status);
	}
	return 1;
    } 

    /* get the endianness */
    if (posterLocalFuncs.getEndianness(id, 
	    (H2_ENDIANNESS *)&res->endianness) == ERROR) {
	res->status = errnoGet();
	if (verbose) {
	    fprintf(stderr, "posterServ error: find: getEndianness ");
	    h2printErrno(res->status);
	}
	return 1;
    } 

    /* fill the data to be returned to the client */
    res->id = remposterIdAlloc(id);
    res->status = POSTER_OK;
    return 1;
}

/*----------------------------------------------------------------------*/

bool_t
SVC(poster_create_1)(POSTER_CREATE_PAR *param, POSTER_CREATE_RESULT *res, struct svc_req *clnt)
{
    POSTER_ID id;

    /* create the poster locally */
    if (posterLocalFuncs.create(param->name, param->length, &id) == ERROR) {
	res->status = errnoGet();
	if (verbose) {
	    fprintf(stderr, "posterServ error: create: ");
	    h2printErrno(res->status);
	}
	return 1;
    } 

    /* set correct endianness in h2dev and in POSTER_STR */
    if (posterLocalFuncs.setEndianness(id, param->endianness)
	== ERROR) {
      res->status = errnoGet();
      if (verbose) {
	fprintf(stderr, "posterServ error: create: setEndianness ");
	h2printErrno(res->status);
      }
    }
 
    res->id = remposterIdAlloc(id);
    /* OK */
    res->status = POSTER_OK;
    
    return 1;
}

/*----------------------------------------------------------------------*/

bool_t
SVC(poster_resize_1)(POSTER_RESIZE_PAR *param, int *res, struct svc_req *clnt)
{
    POSTER_ID p = (POSTER_ID)remposterIdLookup(param->id);
    size_t size = param->size; /* copy here to make sure sizeof is correct */

    *res = posterLocalFuncs.ioctl(p, FIO_RESIZE, &size);
    if (*res == ERROR) {
	*res = errnoGet();
	if (verbose) {
	    fprintf(stderr, "posterServ error: resize ");
	    h2printErrno(*res);
	}
    }

    return 1;
}

/*----------------------------------------------------------------------*/

bool_t
SVC(poster_write_1)(POSTER_WRITE_PAR *param, int *res, struct svc_req *clnt)
{
    POSTER_ID p = (POSTER_ID)remposterIdLookup(param->id);

    *res = posterLocalFuncs.write(p, param->offset,
		      param->data.data_val, param->length);
    if (*res == ERROR) {
	*res = errnoGet();
	if (verbose) {
	    fprintf(stderr, "posterServ error: write ");
	    h2printErrno(*res);
	}
    }
    return 1;
}

/*----------------------------------------------------------------------*/
    
bool_t
SVC(poster_read_1)(POSTER_READ_PAR *param, POSTER_READ_RESULT *res, struct svc_req *clnt)
{
    POSTER_ID p = (POSTER_ID)remposterIdLookup(param->id);

    /* tread data_len == -1 as 'read whole poster'. This can only come from a
     * remotePosterTake(), as far as posterLib API is concerned */
    if (param->length == -1) {
      /* do not lock? there is a race in any case until the actual read(),
       * however the read() will return the correct number of bytes read in any
       * case.
       */
      param->length = H2DEV_POSTER_SIZE((long)p);
    }
    res->data.data_len = param->length;
    res->data.data_val = malloc(param->length);

    res->data.data_len = posterLocalFuncs.read(p, param->offset,
                                              res->data.data_val, param->length);
    if (res->data.data_len == ERROR) {
	res->status = errnoGet();
	if (verbose) {
	    fprintf(stderr, "posterServ error: read ");
	    h2printErrno(res->status);
	}
    } else {
	res->status = POSTER_OK;
    }
    return 1;
}

/*----------------------------------------------------------------------*/
    
bool_t
SVC(poster_delete_1)(int *id, int * res, struct svc_req *clnt)
{
    *res = posterLocalFuncs.delete((POSTER_ID)remposterIdLookup(*id));
    if (*res == ERROR) {
	*res = errnoGet();
	if (verbose) {
	    fprintf(stderr, "posterServ error: delete ");
	    h2printErrno(*res);
	}
    }
    remposterIdRemove(*id);
    return 1;
}

/*----------------------------------------------------------------------*/

bool_t
SVC(poster_ioctl_1)(POSTER_IOCTL_PAR *param, POSTER_IOCTL_RESULT *res, struct svc_req *clnt)
{
    POSTER_ID p = (POSTER_ID)remposterIdLookup(param->id);
    H2TIME date;
    int fresh;

    switch (param->cmd) {
    case FIO_GETDATE:
    case FIO_NMSEC:
	    memset(&date, 0, sizeof(H2TIME));
	    if (posterLocalFuncs.ioctl(p, param->cmd, &date) == ERROR) {
		    res->status = errnoGet();
		    if (verbose) {
			    fprintf(stderr, "posterServ error: ioctl ");
			    h2printErrno(res->status);
		    }
	    } else {
		    res->status = POSTER_OK;
		    res->ntick = date.ntick;
		    res->msec = date.msec;
		    res->sec = date.sec;
		    res->minute = date.minute;
		    res->hour = date.hour;
		    res->day = date.day;
		    res->date = date.date;
		    res->month = date.month;
		    res->year = date.year;
	    }
	    break;
    case FIO_FRESH:
	    if (posterLocalFuncs.ioctl(p, FIO_FRESH, &fresh) == ERROR) {
		    res->status = errnoGet();
	    } else {
		    res->status = POSTER_OK;
		    res->ntick = fresh;
	    }
	    break;
    default:
	    res->status = S_posterLib_BAD_IOCTL_CODE;
	    break;
    }
    return 1;
}

bool_t
SVC(poster_list_1)(void *unused, POSTER_LIST_RESULT *res, struct svc_req *clnt)
{
    POSTER_LIST *list = NULL, *l;
    int i, h2devMax;

    if (h2devAttach(&h2devMax) == ERROR) {
	    res = NULL;
	    return 1;
    }
    for (i = 0; i < h2devMax; i++) {
	if (H2DEV_TYPE(i) == H2_DEV_TYPE_POSTER) {
	    l = malloc(sizeof(struct POSTER_LIST));
	    l->next = list;
	    snprintf(l->name, H2_DEV_MAX_NAME, "%s", H2DEV_NAME(i));
	    l->id = i;
	    l->size = H2DEV_POSTER_SIZE(i);
	    l->fresh = H2DEV_POSTER_FLG_FRESH(i);
	    l->tv_sec = H2DEV_POSTER_DATE(i)->tv_sec;
	    l->tv_nsec = H2DEV_POSTER_DATE(i)->tv_nsec;
	    list = l;
	}
    } /* for */
    res->list = list;
    return 1;
}

int
poster_serv_1_freeresult(SVCXPRT *transp, xdrproc_t xdr_result, caddr_t res)
{
	/* printf("%s\n", __func__); */
	xdr_free(xdr_result, res);
	return 1;
}
/*----------------------------------------------------------------------*/

static RETSIGTYPE
sighandler(int sig)
{
    switch (sig) {

      case SIGINT:
      case SIGTERM:
	(void) svc_unregister(POSTER_SERV, POSTER_VERSION);
        exit(0);
    }
}

int 
main(int argc, char *argv[])
{
    int c, bg = 0, err = 0;
    CLIENT *clnt;

    while ((c = getopt(argc, argv, "bv")) != EOF) {
	switch (c) {
	  case 'b':
	    bg++;
	    break;
	  case 'v':
	    verbose++;
	    break;
	  case '?':
	    err++;
	    break;
	}
    }
    if (err) {
	fprintf(stderr, "usage: %s [-b]\n", argv[0]);
	exit(2);
    } 
    /* Test if service is already registered on localhost */
    if ((clnt = clnt_create("localhost",
		POSTER_SERV, POSTER_VERSION, "tcp")) != NULL) {
	    fprintf(stderr, "posterServ already running\n");
	    clnt_destroy(clnt);
	    exit(1);
    }
    /*
     * Lancement en background
     */
    if (bg) {
	if (fork()) {
	    printf("posterServ\n"
		   "Copyright (C) LAAS/CNRS 1992-2011\n");
	    exit(0);
	}
    }
    (void)signal(SIGINT, sighandler);
    (void)signal(SIGTERM, sighandler);

    if (posterServ() == ERROR) {
	fprintf(stderr, "posterServ() exited.\n");
	h2printErrno(errnoGet());
    }
    return 0;
}
