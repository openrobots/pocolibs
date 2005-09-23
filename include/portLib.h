/* $LAAS$ */
/*
 * Copyright (c) 1998, 2003-2004 CNRS/LAAS
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

#ifndef _PORTLIB_H
#define _PORTLIB_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 ** General definitions found on other some systems
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

/* Various generic function pointer types */
typedef int (*FUNCPTR) ();
typedef int (*INTFUNCPTR)(); 
typedef void (*VOIDFUNCPTR)();
typedef float (*FLTFUNCPTR)();
typedef double (*DBLFUNCPTR)();
typedef void *(*VOIDPTRFUNCPTR)();

/* Macro used to create endless loops */
#ifndef FOREVER
#define FOREVER while (TRUE)
#endif

/* Some other useful macros  */
#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#ifdef __cplusplus
};
#endif

/* XXX Does this belong here ?? */
/* Number of tick per second of the clock */
#include <sysLib.h>
#define NTICKS_PER_SEC sysClkRateGet()
/* Duration of one tick in micro-seconds */
#define TICK_US (1000000/NTICKS_PER_SEC)

#include "logLib.h" /* older versions used to define logMsg() here */

#ifdef __cplusplus
extern "C" {
#endif

/* -- ERRORS CODES ----------------------------------------------- */
#include "h2errorLib.h"
#define M_portLib			(1)

#define  S_portLib_NO_MEMORY		H2_ENCODE_ERR(M_portLib, 1)
#define  S_portLib_NO_SUCH_TASK		H2_ENCODE_ERR(M_portLib, 2)
#define  S_portLib_NOT_IN_A_TASK	H2_ENCODE_ERR(M_portLib, 3)
#define  S_portLib_INVALID_TASKID	H2_ENCODE_ERR(M_portLib, 4)
#define  S_portLib_NOT_IMPLEMENTED	H2_ENCODE_ERR(M_portLib, 5)

#define PORT_LIB_H2_ERR_MSGS { \
   {"NO_MEMORY",	H2_DECODE_ERR(S_portLib_NO_MEMORY)},	\
   {"NO_SUCH_TASK",	H2_DECODE_ERR(S_portLib_NO_SUCH_TASK)},	\
   {"NOT_IN_A_TASK",	H2_DECODE_ERR(S_portLib_NOT_IN_A_TASK)},  \
   {"INVALID_TASKID",	H2_DECODE_ERR(S_portLib_INVALID_TASKID)},  \
   {"NOT_IMPLEMENTED",	H2_DECODE_ERR(S_portLib_NOT_IMPLEMENTED)}  \
  }

extern const H2_ERROR portLibH2errMsgs[]; /* = PORT_LIB_H2_ERR_MSGS */

/* -- PROTOTYPES ----------------------------------------------- */
int portRecordH2ErrMsgs();
extern STATUS osInit(int);
extern void   osExit(void);

#ifdef __cplusplus
};
#endif

#endif /* _PORTLIB_H */
