/* $LAAS$ */
/*
 * Copyright (c) 1991, 2003 CNRS/LAAS
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

/* Codes d'erreur */
#define M_mboxLib 508

#define   S_mboxLib_MBOX_CLOSED         ((M_mboxLib << 16) | 0)
#define   S_mboxLib_NOT_OWNER           ((M_mboxLib << 16) | 1)
#define   S_mboxLib_BAD_IOCTL_CODE      ((M_mboxLib << 16) | 2)
#define   S_mboxLib_MBOX_FULL           ((M_mboxLib << 16) | 3)
#define   S_mboxLib_NAME_IN_USE         ((M_mboxLib << 16) | 4)
#define   S_mboxLib_TOO_BIG             ((M_mboxLib << 16) | 5)
#define   S_mboxLib_SMALL_BLOCK         ((M_mboxLib << 16) | 6)
#define   S_mboxLib_SHORT_MESSAGE       ((M_mboxLib << 16) | 7)


/* Prototypes */
extern STATUS mboxCreate ( char *name, int len, MBOX_ID *pMboxId );
extern STATUS mboxDelete ( MBOX_ID mboxId );
extern STATUS mboxEnd ( int taskId );
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
};
#endif

#endif
