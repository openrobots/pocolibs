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

#ifndef u_long
#include <sys/types.h>
#endif

#include "mboxLib.h"

/* Flag d'initialisation */
#define   GCOM_FLAG_INIT                  0x12348765

/* Nombre max. de lettres */
#define   MAX_LETTER                      617 /* 15 modules *
					       (2*CLIENT_NMAX_RQST_ID + 1) + 2
					       letters !! */

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

#define   M_gcomLib                     (511 << 16)

/* Erreurs */
#define   S_gcomLib_ERR_MBOX_MASK         (M_gcomLib | 1)
#define   S_gcomLib_NOT_A_LETTER          (M_gcomLib | 2)
#define   S_gcomLib_SMALL_LETTER          (M_gcomLib | 3)
#define   S_gcomLib_SMALL_DATA_STR        (M_gcomLib | 4)
#define   S_gcomLib_TOO_MANY_SENDS        (M_gcomLib | 5)
#define   S_gcomLib_ERR_SEND_ID           (M_gcomLib | 6)
#define   S_gcomLib_TOO_MANY_LETTERS      (M_gcomLib | 7)
#define   S_gcomLib_LETTER_NOT_OWNER      (M_gcomLib | 8)
#define   S_gcomLib_INVALID_BLOCK_MODE    (M_gcomLib | 9)
#define   S_gcomLib_REPLY_LETTER_TYPE     (M_gcomLib | 10)
#define   S_gcomLib_MALLOC_FAILED         (M_gcomLib | 11)

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
  int status;                      /* Status du send */
  u_long time;                     /* Temps du send */
  int finalReplyTout;              /* Timeout replique finale */
  int intermedReplyTout;           /* Timeout replique intermediaire */
  LETTER_ID finalReplyLetter;      /* Adresse lettre de replique finale */
  LETTER_ID intermedReplyLetter;   /* Adresse lettre de replique intermed. */
} SEND;


#include "gcomLibProto.h"

/*------------- fin de chargement du fichier --------------------------------*/
#endif
