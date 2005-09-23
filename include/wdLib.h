/* $LAAS$ */
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

#ifndef _WDLIB_H
#define _WDLIB_H

#ifdef __cplusplus
extern "C" {
#endif

/* -- ERRORS CODES ----------------------------------------------- */

#include "h2errorLib.h"
#define M_wdLib          34

#define S_wdLib_ID_ERROR            H2_ENCODE_ERR(M_wdLib, 1)
#define S_wdLib_NOT_ENOUGH_MEMORY   H2_ENCODE_ERR(M_wdLib, 2)

#define WD_LIB_H2_ERR_MSGS { \
    {"ID_ERROR",          H2_DECODE_ERR(S_wdLib_ID_ERROR)},	\
    {"NOT_ENOUGH_MEMORY", H2_DECODE_ERR(S_wdLib_NOT_ENOUGH_MEMORY)},	\
}
extern const H2_ERROR wdLibH2errMsgs[];   /* = WD_LIB_H2_ERR_MSGS */ 


/* -- STRUCTURES ----------------------------------------------- */

#define WDLIB_MAGIC     (M_wdLib << 16)

typedef struct wdog *WDOG_ID;

struct wdog {
    int magic;
    FUNCPTR pRoutine;
    long parameter;
    int delay;
    WDOG_ID next;
};

/* -- PROTOTYPES ----------------------------------------------- */

extern STATUS wdLibInit ( void );
extern WDOG_ID wdCreate ( void );
extern STATUS wdDelete ( WDOG_ID wdId );
extern STATUS wdStart ( WDOG_ID wdId, int delay, FUNCPTR pRoutine, 
			long parameter );
extern STATUS wdCancel ( WDOG_ID wdId );

#ifdef __cplusplus
};
#endif

#endif
