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

/***
 *** Fonctions d'entrees/sorties a la mode stdio pour XES
 ***/

#include "pocolibs-config.h"
__RCSID("$LAAS$");

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#include "portLib.h"
#include "errnoLib.h"

#define XES_STDIO_C /* protection contre les redefinitions */
#include "xes.h"

/* Structure contenant les descripteurs d'E/S prives d'un thread */
typedef struct {
    FILE *in;
    FILE *out;
} XES_PRIVATE_STREAMS;

static pthread_key_t xesKey;

static STATUS xesStdioRegisterStreams(FILE *in, FILE *out);
#ifdef notused
static void xesClose(void *p);
#endif

/*----------------------------------------------------------------------*/

/** 
 ** xesStdioInit - Initialisation des E/S standard pour XES
 **
 ** Creation de la clef d'acces aux donnees privees du thread
 **/
STATUS
xesStdioInit(void)
{
    int retval;
    
    retval = pthread_key_create(&xesKey, NULL);
    if (retval != 0) {
	errnoSet(retval);
	return ERROR;
    }
    /* Pour le thread principal, enregistre stdin/stdout */
    return xesStdioRegisterStreams(stdin, stdout);
} 

/*----------------------------------------------------------------------*/

STATUS 
xesStdioSet(int fd)
{
    FILE *in, *out;

    /* Ouverture du stream d'entree */
    in = fdopen(fd, "r");
    if (in == NULL) {
	errnoSet(errno);
	return ERROR;
    }
    setbuf(in, NULL);

    /* Ouverture du stream de sortie */
    out = fdopen(fd, "w");
    if (out == NULL) {
	errnoSet(errno);
	return ERROR;
    }
    setbuf(out, NULL);

    return xesStdioRegisterStreams(in, out);
}

/*----------------------------------------------------------------------*/

FILE *
xesStdinOfThread(void)
{
    XES_PRIVATE_STREAMS *streams;
    
    /* Recupere le stream */
    streams = (XES_PRIVATE_STREAMS *)pthread_getspecific(xesKey);
    if (streams == NULL) {
	return stdin;
    } else {
	return streams->in;
    }
}
    
/*----------------------------------------------------------------------*/

FILE *
xesStdoutOfThread(void)
{
     XES_PRIVATE_STREAMS *streams;
    
    /* Recupere le stream */
     streams = (XES_PRIVATE_STREAMS *)pthread_getspecific(xesKey);
    if (streams == NULL) {
	return stdout;
    } else {
	return streams->out;
    }
}

/*----------------------------------------------------------------------*/

int 
xesVPrintf(const char *fmt, va_list args)
{
    FILE *f = xesStdoutOfThread();

    return vfprintf(f, fmt, args);
}

/*----------------------------------------------------------------------*/

int
xesPrintf(const char *fmt, ...)
{
    va_list args;
    int n;

    va_start(args, fmt);
    n = xesVPrintf(fmt, args);
    va_end(args);
    return n;
}

/*----------------------------------------------------------------------*/

int
xesScanf(const char *fmt, ...)
{
    FILE *f = xesStdinOfThread();
    va_list args;
    int n;

    va_start(args, fmt);
    n = vfscanf(f, fmt, args);
    va_end(args);
    return n;
}

/*----------------------------------------------------------------------*/

int 
h2scanf(const char *fmt, void *addr)
{
    FILE *f = xesStdinOfThread();
    static char buf[1024];
    int n;

    while (fgets(buf, sizeof(buf), f) == NULL) {
	if (errno != EINTR) {
	    return ERROR;
	}
    } /* while */
    n = sscanf(buf, fmt, addr);
    return(n < 0 ? 0 : n);
}
/*----------------------------------------------------------------------*/

static STATUS
xesStdioRegisterStreams(FILE *in, FILE *out)
{
    XES_PRIVATE_STREAMS *privateStreams;
    int retval;

    /* Allocation d'une structure privee */
    privateStreams = (XES_PRIVATE_STREAMS *)
		     malloc(sizeof(XES_PRIVATE_STREAMS));
    if (privateStreams == NULL) {
	return ERROR;
    }
    /* Enregistre les streams dans la valeur specifique */
    privateStreams->in = in;
    privateStreams->out = out;

    retval = pthread_setspecific(xesKey, privateStreams);
    if (retval != 0) {
	errnoSet(retval);
	return ERROR;
    }
    return OK;
}

/*----------------------------------------------------------------------*/

#ifdef notused
static void
xesClose(void *p)
{
    FILE *f = (FILE *)p;

    fclose(f);
}
#endif
