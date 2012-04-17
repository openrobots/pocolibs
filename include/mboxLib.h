/*
 * Copyright (c) 1991, 2003,2012 CNRS/LAAS
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

#ifndef _MBOXLIB_H
#define _MBOXLIB_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int MBOX_ID;

/* Codes des fonctions ioctl */
#define   FIO_NBYTES                    1
#define   FIO_NMSGS                     2
#define   FIO_GETNAME                   3
#define   FIO_FLUSH                     4
#define   FIO_SIZE                      5

/* Indication de "tous les mailboxes " */
#define   ALL_MBOX                      0

/* -- ERRORS CODES ----------------------------------------------- */

#include "h2errorLib.h"
#define M_mboxLib 508

#define   S_mboxLib_MBOX_CLOSED         H2_ENCODE_ERR(M_mboxLib, 0)
#define   S_mboxLib_NOT_OWNER           H2_ENCODE_ERR(M_mboxLib, 1)
#define   S_mboxLib_BAD_IOCTL_CODE      H2_ENCODE_ERR(M_mboxLib, 2)
#define   S_mboxLib_MBOX_FULL           H2_ENCODE_ERR(M_mboxLib, 3)
#define   S_mboxLib_NAME_IN_USE         H2_ENCODE_ERR(M_mboxLib, 4)
#define   S_mboxLib_TOO_BIG             H2_ENCODE_ERR(M_mboxLib, 5)
#define   S_mboxLib_SMALL_BLOCK         H2_ENCODE_ERR(M_mboxLib, 6)
#define   S_mboxLib_SHORT_MESSAGE       H2_ENCODE_ERR(M_mboxLib, 7)

#define MBOX_LIB_H2_ERR_MSGS { \
   {"MBOX_CLOSED",         H2_DECODE_ERR(S_mboxLib_MBOX_CLOSED)},  \
   {"NOT_OWNER",           H2_DECODE_ERR(S_mboxLib_NOT_OWNER)},  \
   {"BAD_IOCTL_CODE",      H2_DECODE_ERR(S_mboxLib_BAD_IOCTL_CODE)},  \
   {"MBOX_FULL",           H2_DECODE_ERR(S_mboxLib_MBOX_FULL)},  \
   {"NAME_IN_USE",         H2_DECODE_ERR(S_mboxLib_NAME_IN_USE)},  \
   {"TOO_BIG",             H2_DECODE_ERR(S_mboxLib_TOO_BIG)},  \
   {"SMALL_BLOCK",         H2_DECODE_ERR(S_mboxLib_SMALL_BLOCK)},  \
   {"SHORT_MESSAGE",       H2_DECODE_ERR(S_mboxLib_SHORT_MESSAGE)},  \
  }

/* -- PROTOTYPES ----------------------------------------------- */
extern STATUS mboxCreate ( char *name, int len, MBOX_ID *pMboxId );
extern STATUS mboxResize ( MBOX_ID mboxId, int size );
extern STATUS mboxDelete ( MBOX_ID mboxId );
extern STATUS mboxEnd ( long taskId );
extern STATUS mboxFind ( char *name, MBOX_ID *pMboxId );
extern STATUS mboxInit ( char *procName );
extern STATUS mboxIoctl ( MBOX_ID mboxId, int codeFunc, void *pArg );
extern BOOL mboxPause ( MBOX_ID mboxId, int timeout );
extern int mboxRcv ( MBOX_ID mboxId, MBOX_ID *pFromId, char *buf, int maxbytes, int timeout );
extern STATUS mboxSend ( MBOX_ID toId, MBOX_ID fromId, char *buf, int nbytes );
extern void mboxShow ( void );
extern STATUS mboxSkip ( MBOX_ID mboxId );
extern int mboxSpy ( MBOX_ID mboxId, MBOX_ID *pFromId, int *pNbytes, char *buf, int maxbytes );

#ifdef __cplusplus
}
#endif

#endif
