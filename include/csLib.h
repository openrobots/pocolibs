/*
 * Copyright (c) 1991-2005 CNRS/LAAS
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

#ifndef   CS_LIB_H
#define   CS_LIB_H


/*****************************************************************************/
/*    LABORATOIRE D'AUTOMATIQUE ET D'ANALYSE DE SYSTEMES - LAAS / CNRS       */
/*    PROJET HILARE II - BIBLIOTHEQUE DE ROUTINES SERVEUR/CLIENT             */
/*    FICHIER D'EN-TETE "csLib.h"                                            */
/*****************************************************************************/


/* VERSION ACTUELLE / HISTORIQUE DES MODIFICATIONS :
   version 1.1, juil90, Ferraz de Camargo, 1ere version;
*/

/* DESCRIPTION :
   Fichier d'en-tete du module de routines de communication type
   "serveur/client".
*/

#include "gcomLib.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Nombre max de types de requetes traitees par un serveur */
#define  NMAX_RQST_TYPE                25

/* Nombre max d'identificateurs de requete (cote serveur) */
#define  SERV_NMAX_RQST_ID             20

/* Nombre max d'identificateurs de requete (cote client) */
#define  CLIENT_NMAX_RQST_ID           20 /* XXXX 8 */

/* Flag d'indication d'initialisation d'un serveur */
#define  CS_SERV_INIT_FLAG             0x99887766

/* Flag d'indication d'initialisation d'un client */
#define  CS_CLIENT_INIT_FLAG           0x66554433

/* -- ERRORS CODES ----------------------------------------------- */

#include "h2errorLib.h"
#define   M_csLib                      512

/* Codes d'erreur */
#define  S_csLib_NOT_A_SERV            H2_ENCODE_ERR(M_csLib, 0)
#define  S_csLib_INVALID_RQST_TYPE     H2_ENCODE_ERR(M_csLib, 1)
#define  S_csLib_TOO_MANY_RQST_IDS     H2_ENCODE_ERR(M_csLib, 2)
#define  S_csLib_BAD_RQST_ID           H2_ENCODE_ERR(M_csLib, 3)
#define  S_csLib_BAD_REPLY_BILAN       H2_ENCODE_ERR(M_csLib, 4)
#define  S_csLib_NOT_A_CLIENT          H2_ENCODE_ERR(M_csLib, 5)
#define  S_csLib_BAD_INTERMED_FLAG     H2_ENCODE_ERR(M_csLib, 6)
#define  S_csLib_BAD_BLOCK_TYPE        H2_ENCODE_ERR(M_csLib, 7)

#define CS_LIB_H2_ERR_MSGS { \
   {"NOT_A_SERV",            H2_DECODE_ERR(S_csLib_NOT_A_SERV)},  \
   {"INVALID_RQST_TYPE",     H2_DECODE_ERR(S_csLib_INVALID_RQST_TYPE)},	\
   {"TOO_MANY_RQST_IDS",     H2_DECODE_ERR(S_csLib_TOO_MANY_RQST_IDS)},  \
   {"BAD_RQST_ID",           H2_DECODE_ERR(S_csLib_BAD_RQST_ID)},  \
   {"BAD_REPLY_BILAN",       H2_DECODE_ERR(S_csLib_BAD_REPLY_BILAN)},  \
   {"NOT_A_CLIENT",          H2_DECODE_ERR(S_csLib_NOT_A_CLIENT)},  \
   {"BAD_INTERMED_FLAG",     H2_DECODE_ERR(S_csLib_BAD_INTERMED_FLAG)},  \
   {"BAD_BLOCK_TYPE",        H2_DECODE_ERR(S_csLib_BAD_BLOCK_TYPE)},  \
     }

/*------------ Structures de donnees d'un serveur -----------------------*/

/* Def. type struct. parametres associees requete recue par le serveur */
typedef struct {
  BOOL rqstIdFlag;                          /* Flag rqstId pris */
  MBOX_ID clientMboxId;                     /* Id du mbox du client */
  int clientSendId;                         /* Id d'envoi du client */
} SERV_RQST;

/* Def. type de la structure principale d'un serveur */
typedef struct {
  int initFlag;				/* Flag d'initialisation */
  LETTER_ID rcvLetter;			/* Lettre de reception */
  LETTER_ID replyLetter;		/* Lettre de replique */
  int inExecRqstId;			/* id requete en execution */
  int nbRqstFunc;			/* nombre de fonctions de traitement */
  FUNCPTR *rqstFuncTab;			/* tab fonctions de traitement */
  SERV_RQST rqstTab[SERV_NMAX_RQST_ID];	/* Tab. parametres requetes */
} CS_SERV, *SERV_ID;


/*------------ Structures de donnees d'un client -----------------------*/

/* Def. type struct. parametres associees requete envoyee par le client */
typedef struct {
  BOOL rqstIdFlag;                          /* Flag rqstId pris */
  BOOL intermedFlag;                        /* Flag attente rep intermed */
  int sendId;                               /* Id. d'envoi de la requete */
  LETTER_ID intermReply;                    /* Lettre de replique interm.*/
  LETTER_ID finalReply;                     /* Lettre de replique finale */ 
} CLIENT_RQST;

/* Def. de type de la structure d'un client */
typedef struct {
  int initFlag;                               /* Flag d'initialisation */
  MBOX_ID servMboxId;                         /* Id. du mbox du serveur */
  LETTER_ID sendLetter;                       /* Lettre a envoyer */
  CLIENT_RQST rqstTab[CLIENT_NMAX_RQST_ID];   /* Tab. parametres requetes */
} CS_CLIENT, *CLIENT_ID;


extern STATUS csClientEnd ( CLIENT_ID clientId );
extern STATUS csClientInit ( char *servMboxName, int maxRqstSize, 
    int maxIntermedReplySize, int maxFinalReplySize, CLIENT_ID *pClientId );
extern int csClientReplyRcv ( CLIENT_ID clientId, int rqstId, int block, 
    char *intermedReplyDataAdrs, int intermedReplyDataSize, 
    FUNCPTR intermedReplyDecodFunc, char *finalReplyDataAdrs, 
    int finalReplyDataSize, FUNCPTR finalReplyDecodFunc );
extern int csClientRqstIdFree ( CLIENT_ID clientId, int rqstId );
extern STATUS csClientRqstSend ( CLIENT_ID clientId, int rqstType, 
    char *rqstDataAdrs, int rqstDataSize, FUNCPTR codFunc, BOOL intermedFlag, 
    int intermedReplyTout, int finalReplyTout, int *pRqstId );
extern STATUS csMboxEnd ( void );
extern STATUS csMboxInit ( char *mboxBaseName, int rcvMboxSize, 
    int replyMboxSize );
extern int csMboxStatus ( int mask );
extern int csMboxWait ( int timeout, int mboxMask );
extern STATUS csServEnd ( SERV_ID servId );
extern STATUS csServFuncInstall ( SERV_ID servId, int rqstType, 
    FUNCPTR rqstFunc );
extern STATUS csServInit ( int maxRqstDataSize, int maxReplyDataSize, 
    SERV_ID *pServId );
extern STATUS csServInitN ( int maxRqstDataSize, int maxReplyDataSize, 
    int nbRqstFunc, SERV_ID *pServId );
extern STATUS csServReplySend ( SERV_ID servId, int rqstId, int replyType, 
    int replyBilan, char *replyDataAdrs, int replyDataSize, FUNCPTR codFunc );
extern STATUS csServRqstExec ( SERV_ID servId );
extern STATUS csServRqstIdFree ( SERV_ID servId, int rqstId );
extern STATUS csServRqstParamsGet ( SERV_ID servId, int rqstId, 
    char *rqstDataAdrs, int rqstDataSize, FUNCPTR decodFunc );

#ifdef __cplusplus
}
#endif

#endif /* CS_LIB_H */
