/*
 * Copyright (c) 1998, 2003-2005 CNRS/LAAS
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

#ifndef _H2EVNLIB_H
#define _H2EVNLIB_H

#ifdef __cplusplus
extern "C" {
#endif

/* -- ERRORS CODES ----------------------------------------------- */

#include "h2errorLib.h"
#define   M_h2evnLib                    507

#define  S_h2evnLib_BAD_TASK_ID     H2_ENCODE_ERR(M_h2evnLib, 0)

#define H2_EVN_LIB_H2_ERR_MSGS { \
    {"BAD_TASK_ID",     H2_DECODE_ERR(S_h2evnLib_BAD_TASK_ID)},  \
}

/* -- PROTOTYPES ----------------------------------------------- */

extern int h2evnRecordH2ErrMsgs(void);
extern void h2evnClear ( void );
extern STATUS h2evnSignal ( int taskId );
extern BOOL h2evnSusp ( int timeout );
 
#ifdef __cplusplus
};
#endif

#endif
