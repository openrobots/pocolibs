/* $LAAS$ */
/*
 * Copyright (c) 1998, 2003 CNRS/LAAS
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

#ifndef _H2LIB_H
#define _H2LIB_H

#include <pthread.h>

/**
 ** Definitions generales compatibles VxWorks
 **/

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

typedef int BOOL;

#ifndef OK
#define OK 0
#endif
#ifndef ERROR 
#define ERROR (-1)
#endif

typedef int STATUS;

#ifndef WAIT_FOREVER
#define WAIT_FOREVER (-1)
#endif

#ifndef NO_WAIT
#define NO_WAIT 0
#endif

/* pointeurs sur fonctions */
typedef int (*FUNCPTR) ();
typedef int (*INTFUNCPTR)(); 
typedef void (*VOIDFUNCPTR)();
typedef float (*FLTFUNCPTR)();
typedef double (*DBLFUNCPTR)();
typedef void *(*VOIDPTRFUNCPTR)();

/* macro pour les boucles */
#ifndef FOREVER
#define FOREVER while (TRUE)
#endif

/* macros utiles */
#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif


/* Nombre de ticks par seconde */
#include <sysLib.h>
#define NTICKS_PER_SEC sysClkRateGet()
/* Duree d'un tick en micro-seconde */
#define TICK_US (1000000/NTICKS_PER_SEC)

/* En attendant l'implementation d'une logTask */
#define logMsg printf

/* Sun n'a pas snprintf partout */
#ifdef __sun
#define snprintf mySnPrintf
#endif

/*
 * Prototypes
 */
extern STATUS osInit(int clkRate);

#endif
