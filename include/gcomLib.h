/*
 * Copyright (c) 1991, 2003-2005, 2012, 2025 CNRS/LAAS
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

/*-------------- INDIQUER L'INCLUSION DE CE FICHIER -------------------------*/

#ifndef   GCOM_LIB_H
#define   GCOM_LIB_H

/*****************************************************************************/
/* LABORATOIRE D'AUTOMATIQUE ET D'ANALYSE DE SYSTEMES - LAAS / CNRS          */
/* PROJET HILARE II - EN-TETE DU GESTIONNAIRE DE COMMUNICATIONS  (gcomLib.h) */
/*****************************************************************************/


/* DESCRIPTION :
   Fichier d'en-tete du module de gestion des communications
*/

#include "mboxLib.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -- CONSTANTS ----------------------------------------------- */

/* Flag d'initialisation */
#define   GCOM_FLAG_INIT                  0x12348765

/* Nombre max de sends paralelles */
#define   MAX_SEND                        80

/* Status des cellules de sends */
#define   FREE                            0
#define   WAITING_INTERMED_REPLY          1
#define   INTERMED_REPLY_TIMEOUT          2
#define   WAITING_FINAL_REPLY             3
#define   FINAL_REPLY_TIMEOUT             4
#define   FINAL_REPLY_OK                  5

/* Masques d'identification des mailboxes d'une tache */
#define   RCV_MBOX                        1
#define   REPLY_MBOX                      2

/* Type de blocage pour le send */
#define   NO_BLOCK                        1
#define   BLOCK_ON_INTERMED_REPLY         2
#define   BLOCK_ON_FINAL_REPLY            3

/* Types des lettres de replique */
#define   INTERMED_REPLY                  1
#define   FINAL_REPLY                     2


/* -- ERRORS CODES ----------------------------------------------- */

#include "h2errorLib.h"
#define   M_gcomLib                     (511)

/* Erreurs */
#define   S_gcomLib_ERR_MBOX_MASK         H2_ENCODE_ERR(M_gcomLib,  1)
#define   S_gcomLib_NOT_A_LETTER          H2_ENCODE_ERR(M_gcomLib,  2)
#define   S_gcomLib_SMALL_LETTER          H2_ENCODE_ERR(M_gcomLib,  3)
#define   S_gcomLib_SMALL_DATA_STR        H2_ENCODE_ERR(M_gcomLib,  4)
#define   S_gcomLib_TOO_MANY_SENDS        H2_ENCODE_ERR(M_gcomLib,  5)
#define   S_gcomLib_ERR_SEND_ID           H2_ENCODE_ERR(M_gcomLib,  6)
#define   S_gcomLib_TOO_MANY_LETTERS      H2_ENCODE_ERR(M_gcomLib,  7)
#define   S_gcomLib_LETTER_NOT_OWNER      H2_ENCODE_ERR(M_gcomLib,  8)
#define   S_gcomLib_INVALID_BLOCK_MODE    H2_ENCODE_ERR(M_gcomLib,  9)
#define   S_gcomLib_REPLY_LETTER_TYPE     H2_ENCODE_ERR(M_gcomLib,  10)
#define   S_gcomLib_MALLOC_FAILED         H2_ENCODE_ERR(M_gcomLib,  11)

#define GCOM_LIB_H2_ERR_MSGS { \
   {"ERR_MBOX_MASK",         H2_DECODE_ERR(S_gcomLib_ERR_MBOX_MASK)},  \
   {"NOT_A_LETTER",          H2_DECODE_ERR(S_gcomLib_NOT_A_LETTER)},	\
   {"SMALL_LETTER",          H2_DECODE_ERR(S_gcomLib_SMALL_LETTER)},	\
   {"SMALL_DATA_STR",        H2_DECODE_ERR(S_gcomLib_SMALL_DATA_STR)},  \
   {"TOO_MANY_SENDS",        H2_DECODE_ERR(S_gcomLib_TOO_MANY_SENDS)},  \
   {"ERR_SEND_ID",           H2_DECODE_ERR(S_gcomLib_ERR_SEND_ID)},  \
   {"TOO_MANY_LETTERS",      H2_DECODE_ERR(S_gcomLib_TOO_MANY_LETTERS)},  \
   {"LETTER_NOT_OWNER",      H2_DECODE_ERR(S_gcomLib_LETTER_NOT_OWNER)},  \
   {"INVALID_BLOCK_MODE",    H2_DECODE_ERR(S_gcomLib_INVALID_BLOCK_MODE)},  \
   {"REPLY_LETTER_TYPE",     H2_DECODE_ERR(S_gcomLib_REPLY_LETTER_TYPE)},  \
   {"MALLOC_FAILED",         H2_DECODE_ERR(S_gcomLib_MALLOC_FAILED)},  \
  }

/* -- STRUCTURES ----------------------------------------------- */

/* En-tete d'une lettre */
typedef struct {
  int sendId;                 /* Identificateur de send */
  int type;                   /* Type de lettre (replique) */
  int dataType;               /* Type de donnees d'une lettre */
  int dataSize;               /* Taille des donnees */
} LETTER_HDR, *LETTER_HDR_ID;

/* Descripteur d'une lettre */
typedef struct {
  int flagInit;               /* Flag d'initialisation */
  int size;                   /* Taille de la lettre */
  LETTER_HDR_ID pHdr;         /* Pointeur vers en-tete de la lettre */
} LETTER, *LETTER_ID;

/* Structure de donnees d'un identif. de send */
typedef struct {
  int status;                      /* Send status */
  unsigned long time;              /* Send time */
  int finalReplyTout;              /* Timeout final reply */
  int intermedReplyTout;           /* Timeout intermediate reply */
  LETTER_ID finalReplyLetter;      /* Final reply address */
  LETTER_ID intermedReplyLetter;   /* Intermediate reply address */
} SEND;


/* -- PROTOTYPES ----------------------------------------------- */

typedef int (*GCOM_CODINGFUNC)(char *, int, char *, int);

BOOL gcomLetterRcv (LETTER_ID letter, MBOX_ID *pOrigMboxId, int *pSendId, 
    int timeout);
STATUS gcomEnd (void);
STATUS gcomInit (const char *procName, int rcvMboxSize, int replyMboxSize);
STATUS gcomUpdate (int rcvMboxSize, int replyMboxSize );
STATUS gcomLetterAlloc (int sizeLetter, LETTER_ID *pLetterId);
STATUS gcomLetterDiscard (LETTER_ID letterId);
STATUS gcomLetterReply (MBOX_ID mboxId, int sendId, int replyLetterType, 
    LETTER_ID letterReply);
STATUS gcomLetterWrite (LETTER_ID letterId, int dataType, char *pData, 
    int dataSize, GCOM_CODINGFUNC codingFunc);
STATUS gcomMboxFind (const char *procName, MBOX_ID *pMboxId);
STATUS gcomMboxName (MBOX_ID mboxId, char *pName);
STATUS gcomSendIdFree (int sendId);
int gcomLetterList (LETTER_ID *pList, int maxList);
int gcomLetterRead (LETTER_ID letterId, char *pData, int maxDataSize, 
    GCOM_CODINGFUNC decodingFunc);
int gcomLetterSend (MBOX_ID mboxId, LETTER_ID sendLetter, 
    LETTER_ID intermedReplyLetter, LETTER_ID finalReplyLetter, int block, 
    int *pSendId, int finalReplyTout, int intermedReplyTout);
int gcomLetterType (LETTER_ID letterId);
int gcomMboxPause (int timeout, int mask);
int gcomMboxStatus (int mask);
int gcomReplyStatus (int sendId);
int gcomReplyWait (int sendId, int replyLetterType);
int gcomSendIdList (int *pList, int maxList);
void gcomMboxShow (void);
void gcomReplyLetterBySendId (int sendId, LETTER_ID *pIntermedReplyLetter, 
    LETTER_ID *pFinalReplyLetter);
void gcomSelect(char *sendIdTable);


#ifdef __cplusplus
}
#endif

#endif /* GCOM_LIB_H */
