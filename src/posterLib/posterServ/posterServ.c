/*
 * Copyright (c) 1990, 2003, 2010 CNRS/LAAS
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

POSTER_FIND_RESULT *
SVC(poster_find_1)(char **nom, struct svc_req *clnt)
{
    POSTER_ID id;
    static POSTER_FIND_RESULT res;

    /* look for the poster locally only */
    if (posterLocalFuncs.find(*nom, &id) == ERROR) {
	res.status = errnoGet();
	if (verbose) {
	    fprintf(stderr, "posterServ error: find(%s) ", *nom);
	    h2printErrno(res.status);
	}
	return(&res);
    }

    /* get the poster length */
    if (posterLocalFuncs.ioctl(id, FIO_GETSIZE, &res.length) == ERROR) {
	res.status = errnoGet();
	if (verbose) {
	    fprintf(stderr, "posterServ error: find: posterIoctl ");
	    h2printErrno(res.status);
	}
	return(&res);
    } 

    /* get the endianness */
    if (posterLocalFuncs.getEndianness(id, 
	    (H2_ENDIANNESS *)&res.endianness) == ERROR) {
	res.status = errnoGet();
	if (verbose) {
	    fprintf(stderr, "posterServ error: find: getEndianness ");
	    h2printErrno(res.status);
	}
	return(&res);
    } 

    /* fill the data to be returned to the client */
    res.id = (int)id;
    res.status = POSTER_OK;
    return(&res);
}

/*----------------------------------------------------------------------*/

POSTER_CREATE_RESULT *
SVC(poster_create_1)(POSTER_CREATE_PAR *param, struct svc_req *clnt)
{
    static POSTER_CREATE_RESULT res;
    
    /* create the poster locally */
    if (posterLocalFuncs.create(param->name, param->length, 
	    (POSTER_ID*)&(res.id)) == ERROR) {
	res.status = errnoGet();
	if (verbose) {
	    fprintf(stderr, "posterServ error: create: ");
	    h2printErrno(res.status);
	}
	return(&res);
    } 

    /* set correct endianness in h2dev and in POSTER_STR */
    if (posterLocalFuncs.setEndianness((POSTER_ID)(res.id), param->endianness)
	== ERROR) {
      res.status = errnoGet();
      if (verbose) {
	fprintf(stderr, "posterServ error: create: setEndianness ");
	h2printErrno(res.status);
      }
    }
 
    /* OK */
    res.status = POSTER_OK;
    
    return(&res);
}

/*----------------------------------------------------------------------*/

int *
SVC(poster_write_1)(POSTER_WRITE_PAR *param, struct svc_req *clnt)
{
    static int res;
    POSTER_ID p = (POSTER_ID)param->id;

    res = posterLocalFuncs.write(p, param->offset,
		      param->data.data_val, param->length);
    if (res == ERROR) {
	res = errnoGet();
	if (verbose) {
	    fprintf(stderr, "posterServ error: write ");
	    h2printErrno(res);
	}
    }
    /* xdr_free((xdrproc_t)xdr_POSTER_WRITE_PAR, (char *)param); */
    return(&res);
}

/*----------------------------------------------------------------------*/
    
POSTER_READ_RESULT *
SVC(poster_read_1)(POSTER_READ_PAR *param, struct svc_req *clnt)
{
    static POSTER_READ_RESULT res;
    POSTER_ID p = (POSTER_ID)param->id;

    xdr_free((xdrproc_t)xdr_POSTER_READ_RESULT, (char *)&res);
    
    res.data.data_len = param->length;
    res.data.data_val = malloc(param->length);

    if (posterLocalFuncs.read(p, param->offset, 
	    res.data.data_val, param->length) == ERROR) {
	res.status = errnoGet();
	if (verbose) {
	    fprintf(stderr, "posterServ error: read ");
	    h2printErrno(res.status);
	}
    } else {
	res.status = POSTER_OK;
    }
    return(&res);
}

/*----------------------------------------------------------------------*/
    
int *
SVC(poster_delete_1)(int *id, struct svc_req *clnt)
{
    static int res;

    res = posterLocalFuncs.delete((POSTER_ID)(*id));
    if (res == ERROR) {
	res = errnoGet();
	if (verbose) {
	    fprintf(stderr, "posterServ error: delete ");
	    h2printErrno(res);
	}
    }
    return(&res);
}

/*----------------------------------------------------------------------*/

POSTER_IOCTL_RESULT *
SVC(poster_ioctl_1)(POSTER_IOCTL_PAR *param, struct svc_req *clnt)
{
    static POSTER_IOCTL_RESULT res;
    POSTER_ID p = (POSTER_ID)param->id;
    H2TIME date;

    memset(&date, 0, sizeof(H2TIME));
    if (posterLocalFuncs.ioctl(p, param->cmd, &date) == ERROR) {
	res.status = errnoGet();
	if (verbose) {
	    fprintf(stderr, "posterServ error: ioctl ");
	    h2printErrno(res.status);
	}
    } else {
	res.status = POSTER_OK;
	res.ntick = date.ntick;
	res.msec = date.msec;
	res.sec = date.sec;
	res.minute = date.minute;
	res.hour = date.hour;
	res.day = date.day;
	res.date = date.date;
	res.month = date.month;
	res.year = date.year;
    }
    return &res;
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
		   "Copyright (C) LAAS/CNRS 1992-2010\n");
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
