/*
 * Copyright (c) 1990, 2003-2005 CNRS/LAAS
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
/*****************************************************************************/
/*   LABORATOIRE D'AUTOMATIQUE ET D'ANALYSE DE SYSTEMES - LAAS / CNRS        */
/*   PROJET HILARE II - BIBLIOTHEQUE DE ROUTINES CLIENT/SERVEUR (csLib.c)    */
/*****************************************************************************/

/* VERSION ACTUELLE / HISTORIQUE DES MODIFICATIONS :
   version 1.1, juin90, Ferraz de Camargo, 1ere version;
*/

/* DESCRIPTION :
   Ensemble de routines de communication adequates a une architecture 
   logicielle du type "client/serveur".
*/

#include "pocolibs-config.h"

#include <string.h>
#include <stdlib.h>

#include "portLib.h"
#include "errnoLib.h"
#include "gcomLib.h"
#include "csLib.h"
static const H2_ERROR csLibH2errMsgs[] = CS_LIB_H2_ERR_MSGS;


/*---------- ROUTINES DISPONIBLES A L'UTILISATEUR ---------------------------*/

/*---------- PREMIERE PARTIE: MANIPULATION DES BOITES AUX LETTRES -----------*/


/* INITIALISATION DES BOITES AUX LETTRES D'UNE TACHE : 

   L'appel de cette fonction permet la creation de deux boites aux lettres:
   l'une destinee a la reception de requetes envoyees par les clients de la
   tache appelante et l'autre dediee a recevoir les repliques des serveurs
   de cette tache.

   STATUS csMboxInit (mboxBaseName, rcvMboxSize, replyMboxSize)
      char *mboxBaseName;     * Nom de base des boites aux lettres crees *
      int rcvMboxSize;        * Taille de la boite aux lettres de requetes *
      int replyMboxSize;      * Taille de la boite aux lettres de repliques *

   Retourne : OK ou ERROR

   Remarques:
      1. Cette fonction doit etre appelee imperativement avant l'appel des 
         routines "csServInit" ou "csClientInit", montrees par la suite;
      2. Si les arguments rcvMboxSize ou replyMboxSize sont nuls, les boites
         aux lettres correspondentes ne sont pas creees;
*/


/* ATTENDRE L'ARRIVEE D'UNE REQUETE ET/OU REPLIQUE :

   Cette routine permet a une tache de se suspendre, dans l'attente de 
   arrivee soit d'une requete (emise par un client), soit d'une replique 
   (emise par un serveur).

   int csMboxWait (timeout, mboxMask)
     int timeout;             * Timeout d'attente (1u = 1tick) *
     int mboxMask;            * RCV_MBOX, REPLY_MBOX, RCV_MBOX | REPLY_MBOX *

   Retourne: Masque avec les boites aux lettres qui ont recu des messages
             ou ERROR

   Remarque: les valeurs susceptibles d'etre retournees sont:
      mask = RCV_MBOX, si 1 ou + requetes sont dans mbox reception
      mask = REPLY_MBOX, si 1 ou + repliques sont dans mbox repliques
      mask = RCV_MBOX | REPLY_MBOX, si au moins une requete ou replique
      mask = 0: timeout et pas de message
      mask = ERROR, erreur pendant l'execution
*/


/* ETAT D'UNE (DES) BOITE(S) AUX LETTRES :
   
   Cette fonction permet d'obtenir l'etat soit de la boite aux lettres de 
   reception de requetes, soit de celle de repliques.

   int csMboxStatus (mask)
     int mask;                * RCV_MBOX, REPLY_MBOX, RCV_MBOX | REPLY_MBOX *

   Retourne: masque avec l'etat des boites aux lettres (identique a celle de
   csMboxWait) ou ERROR
*/


/* FERMER TOUTES LES BOITES AUX LETTRES :

   Cette routine libere les boites aux lettres allouees par csMboxInit.

   STATUS csMboxEnd ()
   
   Retourne: OK ou ERROR
*/


/*------------- DEUXIEME PARTIE : ROUTINES POUR LES SERVEURS ----------------*/


/* INITIALISATION D'UN SERVEUR :

   Cette fonction permet a une tache de s'initialiser comme serveuse.

   STATUS csServInit (maxRqstDataSize, maxReplyDataSize, pServId)
      int maxRqstDataSize;      * Taille max en bytes des requetes *
      int maxReplyDataSize;     * Taille max en bytes des repliques *
      SERV_ID *pServId;         * Ou` mettre l'id de ce serveur *

   Retourne : OK ou ERROR
*/

/* INSTALLATION DE LA ROUTINE DE TRAITEMENT D'UNE REQUETE :

   Cette fonction permet a un serveur d'installer la routine de traitement
   associee a une requete d'un type donne. Remarque: des exemples de type
   de requete pour l'agent pilotage sont "move", "stop", "concatenate", etc.

   STATUS csServFuncInstall (servId, rqstType, rqstFunc)
      SERV_ID servId;           * Identificateur du serveur *
      int rqstType;             * Type de la requete *
      FUNCPTR rqstFunc;         * Fonction de traitement de cette requete *

   Retourne: OK ou ERROR
*/


/* RECEVOIR ET EXECUTER UNE REQUETE :

   Cette fonction permet de lire une requete qui arrive dans la boite aux 
   lettres de reception, de verifier son type et d'executer automatiquement
   la fonction de traitement que lui est associee (fonction installee au
   moyen de csServFuncInstall).

   STATUS csServRqstExec (servId)
      SERV_ID servId;           * Identificateur du serveur *

   Retourne: OK ou ERROR
*/


/* PRENDRE LES PARAMETRES D'UNE REQUETE A EXECUTER :

   Cette fonction doit etre appelee dans la routine de traitement d'une
   requete, de maniere a pouvoir lire ses arguments (si existent!).

   STATUS csServRqstParamsGet (servId, rqstId, rqstDataAdrs, rqstDataSize,
                               decodFunc)
      SERV_ID servId;           * Identificateur du serveur *
      int rqstId;               * Id de la requete en execution *
      char *rqstDataAdrs;       * Adrs struct. ou` stocker params requete *
      int rqstDataSize;         * Taille de cette structure *
      FUNCPTR decodFunc;        * Fonction de decodage *

   Retourne: OK ou ERROR
*/


/* ENVOI D'UNE REPLIQUE AU CLIENT :

   Cette fonction permet d'envoyer une replique au client. Deux types de 
   replique sont possibles: une replique intermediaire et une replique finale.

   STATUS csServReplySend (servId, rqstId, replyType, replyBilan,
                           replyDataAdrs, replyDataSize, codFunc)
      SERV_ID servId;           * Identificateur du serveur *
      int rqstId;               * Identificateur de la requete *
      int replyType;            * INTERMED_REPLY ou FINAL_REPLY *
      int replyBilan;           * Bilan: OK ou code d'erreur *
      char *replyDataAdrs;      * Adresse base de la replique a envoyer *
      int replyDataSize;        * Taille de la replique a envoyer *
      FUNCPTR codFunc;          * Fonction de codage du message *

  Retourne : OK ou ERROR
*/


/* LIBERER L'IDENTIFICATEUR D'UNE REQUETE RECUE :

   Cette fonction permet de liberer l'identificateur d'une requete recue
   par le serveur. Attention: l'identificateur d'une requete recue est
   automatiquement libere quand de l'envoi de la replique finale au moyen de
   csServReplySend (). Ainsi, il est conseille d'EVITER d'utiliser la fonction
   csServRqstIdFree.

   STATUS csServRqstIdFree (servId, rqstId)
       SERV_ID servId;          * Identificateur du serveur *
       int rqstId;              * Identificateur de la requete *

   Retourne: OK ou ERROR
*/


/* LIBERER LES OBJETS ALLOUES PAR UN SERVEUR :

   STATUS csServEnd (servId)
      SERV_ID servId;           * Id du serveur *

   Cette fonction permet de liberer tous les objets alloues par un serveur.

   Retourne: OK ou ERROR
*/


/*------------- TROISIEME PARTIE : ROUTINES POUR LES CLIENTS ----------------*/


/* INITIALISATION D'UN CLIENT :

   Cette fonction permet d'initialiser une tache comme cliente d'une
   autre tache (serveuse).

   STATUS csClientInit (servMboxName, maxRqstSize, maxIntermedReplySize, 
	                maxFinalReplySize, pClientId)
      char *servMboxName;        * Nom boite aux lettres du serveur *
      int maxRqstSize;           * Taille max des requetes *
      int maxIntermedReplySize;  * Taille max des repliques intermed. *
      int maxFinalReplySize;     * Taille max des repliques finales *
      CLIENT_ID *pClientId;      * Id du client initialise *
 
   Retourne : OK ou ERROR 
*/


/* ENVOI D'UNE REQUETE VERS UN SERVEUR :

   Cette fonction permet a un client d'envoyer une requete vers un serveur.
   Remarque: L'envoi des requetes est toujours non-bloquant.
   
   STATUS csClientRqstSend (clientId, rqstType, rqstDataAdrs, rqstDataSize, 
                            codeFunc, intermedFlag, intermedReplyTout, 
                            finalReplyTout, pRqstId)
      CLIENT_ID clientId;        * Id du client *
      int rqstType;              * Type de la requete *
      char *rqstDataAdrs;        * Adrs. struct. donnees de la requete *
      int rqstDataSize;          * Taille de la requete a envoyer *
      FUNCPTR codeFunc;          * Fonction de codage *
      BOOL intermedFlag;         * TRUE (si attend repl interm), FALSE sinon *
      int intermedReplyTout;     * Timeout attente replique intermediaire *
      int finalReplyTout;        * Timeout attente replique finale *
      int *pRqstId;              * Ou` mettre l'id de la requete *

   Retourne : OK ou ERROR
*/


/* RECEPTION D'UNE REPLIQUE :

   Cette fonction permet de recevoir (de maniere bloquante ou non-bloquante)
   soit la replique intermediaire, soit la replique finale.

   int csClientReplyRcv (clientId, rqstId, block, intermedReplyDataAdrs, 
	                 intermedReplyDataSize, intermedReplyDecodFunc,
			 finalReplyDataAdrs, finalReplyDataSize, 
			 finalReplyDecodFunc)
     CLIENT_ID clientId;              * Identificateur du client *
     int rqstId;                      * Id. de la requete *
     int block;                       * NO_BLOCK, BLOCK_ON_INTERMED_REPLY,
                                        BLOCK_ON_FINAL_REPLY *
     char *intermedReplyDataAdrs;     * Adresse donees replique intermed. *
     int intermedReplyDataSize;       * Taille de la replique intermediaire *
     FUNCPTR intermedReplyDecodFunc;  * Fonction decodage replique intermed *
     char *finalReplyDataAdrs;        * Adresse donnees replique finale *
     int finalReplyDataSize;          * Taille de la replique finale *
     FUNCPTR finalReplyDecodFunc;     * Fonction decodage replique finale *

   Retourne : bilan de la replique ou ERROR
*/

/* LIBERATION D'UN IDENTIFICATEUR DE REQUETE ENVOYEE :

   Cette fonction permet de liberer l'identificateur d'une requete envoyee
   au prealable a un serveur.
   Remarque: l'identificateur de requete est automatiquement libere au moment
   de la reception de la replique finale. Donc, EVITER d'utiliser la
   fonction csClientRqstIdFree.

   int csClientRqstIdFree (clientId, rqstId)
      CLIENT_ID clientId;          * Identificateur du client *
      int rqstId;                  * Id. de la requete *

   Retourne : OK ou ERROR
*/


/* LIBERATION DES OBJETS ALLOUES PAR UN CLIENT :

   Cette fonction permet de liberer les objets alloues par un client.
 
   STATUS csClientEnd (clientId)
      CLIENT_ID clientId;          * Identificateur du client *

   Retourne : OK ou ERROR 
*/




/*---------- PREMIERE PARTIE: MANIPULATION DES BOITES AUX LETTRES -----------*/


/******************************************************************************
*
*  csMboxInit - Initialisation des boites aux lettres d'un agent
*
*  Description:
*  Cette routine permet d'initialiser les boites aux lettres de requetes
*  (cas "serveur") et de repliques (cas "client") d'un agent. Cette fonction 
*  doit etre appelee imperativement avant l'appel des routines "csServInit" ou
*  "csClientInit".
*
*  Retourne : OK ou ERROR
*/

STATUS 
csMboxInit(char *mboxBaseName, /* Nom de base des boites aux lettres crees */ 
	   int rcvMboxSize,    /* Taille de la boite aux lettres de requetes */
	   int replyMboxSize)  /* Taille de la boite aux lettres de repliques*/
{
    /* record error msgs */
    h2recordErrMsgs("csMboxInit", "csLib", M_csLib, 			
		    sizeof(csLibH2errMsgs)/sizeof(H2_ERROR), 
		    csLibH2errMsgs);

    /* Initialiser le module gcom */
    return gcomInit(mboxBaseName, rcvMboxSize, replyMboxSize);
}


/******************************************************************************
*
*  csMboxWait - Attendre l'arrivee d'une requete et/ou replique
*
*  Description:
*  Cette routine permet a une tache de se suspendre, dans l'attente de 
*  arrivee soit d'une requete (emise par un client), soit d'une replique 
*  (emise par un serveur).
*
*  Retourne: Masque avec les boites aux lettres qui ont recu des messages
*            ou ERROR
*
*  Remarque: les valeurs possibles d'etre retournees sont:
*      mask = RCV_MBOX, si 1 ou + requetes sont dans mbox reception
*      mask = REPLY_MBOX, si 1 ou + repliques sont dans mbox repliques
*      mask = RCV_MBOX | REPLY_MBOX, si au moins une requete ou replique
*      mask = 0: timeout et pas de message
*      mask = ERROR, erreur pendant l'execution
*/

int 
csMboxWait(int timeout,     /* Timeout d'attente (1u = 1tick) */
	   int mboxMask)    /* RCV_MBOX, REPLY_MBOX, RCV_MBOX | REPLY_MBOX*/
{
    /* Appeler la routine de pause de gcom */
    return gcomMboxPause(timeout, mboxMask);
}


/******************************************************************************
*
*  csMboxStatus - Etat d'une (des) boite(s) aux lettres
*
*  Description:
*  Cette fonction permet d'obtenir l'etat soit de la boite aux lettres de 
*  reception de requetes, soit de celle de repliques.
*
*  Retourne: masque avec l'etat des boites aux lettres ou ERROR
*/

int 
csMboxStatus(int mask)        /* RCV_MBOX, REPLY_MBOX, RCV_MBOX | REPLY_MBOX*/
{
    /* Appel direct de la routine d'etat de gcom */
    return gcomMboxStatus(mask);
}


/******************************************************************************
*
*  csMboxEnd - Fermer toutes les boites aux lettres 
*
*  Description:
*  Cette routine libere les objets alloues par les boites aux lettres.
*
*  Retourne: OK ou ERROR
*/

STATUS 
csMboxEnd(void)
{
    /* Appeler directement la routine de finalisation de gcom */
    return gcomEnd ();
}



/*------------- DEUXIEME PARTIE : ROUTINES POUR LES SERVEURS ----------------*/


/******************************************************************************
*
*  csServInit - Initialisation d'un serveur
*
*  Description:
*  Cette fonction permet a une tache de s'initialiser comme serveur.
*
*  Retourne : OK ou ERROR
*/

STATUS 
csServInit(int maxRqstDataSize, /* Taille max des requetes */
	   int maxReplyDataSize,/* Taille max des repliques */ 
	   SERV_ID *pServId)    /* Ou` mettre l'id de ce serveur */
{
    return csServInitN(maxRqstDataSize, maxReplyDataSize, 
		       NMAX_RQST_TYPE, pServId);
}

STATUS 
csServInitN(int maxRqstDataSize,	/* Taille max des requetes */
	   int maxReplyDataSize,	/* Taille max des repliques */ 
	   int nbRqstFunc,		/* Nombre max de requetes */
	   SERV_ID *pServId)		/* Ou` mettre l'id de ce serveur */
{
    SERV_ID servId;               /* Id. du serveur */
    LETTER_ID letterId;           /* Lettre: requetes, repliques */
    
    /* Allouer une zone memoire pour structure du serveur */
    if ((servId = (SERV_ID) malloc(sizeof (CS_SERV))) == NULL)
	return ERROR;
    
    /* Nettoyer cette zone memoire */
    memset((char *) servId, 0, sizeof (CS_SERV));

    /* Allouer le tableau initial de fonctions de traitement */
    servId->rqstFuncTab = (FUNCPTR *)malloc(nbRqstFunc*sizeof(FUNCPTR *));
    if (servId->rqstFuncTab == NULL) {
	free(servId);
	return ERROR;
    }
    servId->nbRqstFunc = nbRqstFunc;
    
    /* Allouer une lettre pour la reception des messages */
    if (gcomLetterAlloc(maxRqstDataSize, &letterId) != OK) {
	free(servId->rqstFuncTab);
	free(servId);
	return ERROR;
    }
    
    /* Stocker l'identificateur de la lettre de reception */
    servId->rcvLetter = letterId;
    
    /* Allouer une lettre pour les repliques */
    if (gcomLetterAlloc(maxReplyDataSize, &letterId) != OK) {
	free(servId->rqstFuncTab);
	free(servId);
	return ERROR;
    }
    /* Stocker l'identificateur de la lettre de replique */
    servId->replyLetter = letterId;
    
    /* Garder l'identificateur du serveur */
    *pServId = servId;
    
    /* Indiquer l'initialisation de ce serveur */
    servId->initFlag = CS_SERV_INIT_FLAG;
    return OK;
}


/******************************************************************************
*
*  csServFuncInstall - Installation routine de traitement d'une requete
*
*  Description:
*  Cette fonction permet a un serveur d'installer la routine de traitement
*  associee a une requete donnee.
*
*  Retourne: OK ou ERROR
*/

STATUS 
csServFuncInstall(SERV_ID servId,  /* Identificateur du serveur */
		  int rqstType,    /* Type de la requete */
		  FUNCPTR rqstFunc)/* Fonction de traitement de la requete */
{
    /* Retourner ERROR, si ce serveur n'est pas initialise */
    if (servId->initFlag != CS_SERV_INIT_FLAG) {
	errnoSet(S_csLib_NOT_A_SERV);
	return ERROR;
    }

    /* Verifier validite du type de la requete */
    if (rqstType < 0 || rqstType >= servId->nbRqstFunc) {
	errnoSet(S_csLib_INVALID_RQST_TYPE);
	return ERROR;
    }

    /* Installer la fonction de traitement de cette requete */
    servId->rqstFuncTab[rqstType] = rqstFunc;
    return OK;
}


/******************************************************************************
*
*  csServRqstExec - Recevoir et executer une requete
*
*  Decription:
*  Cette fonction permet de lire une requete qui arrive dans la boite aux 
*  lettres de reception, de verifier son type et d'executer automatiquement
*  la fonction de traitement que lui est associee (fonction installee au
*  moyen de csServFuncInstall).
*
*  Retourne: OK ou ERROR
*/

STATUS 
csServRqstExec(SERV_ID servId)  /* Identificateur du serveur */
{
    int rqstType;                 /* Type de la requete */
    FUNCPTR servFunc;             /* Fonction de traitement de la requete */
    int rqstId;                   /* Identificateur de la requete */
    SERV_RQST *pRqst;             /* Ptr structure de donnees requete */
    
    /* Retourner ERROR, si ce serveur n'est pas initialise */
    if (servId->initFlag != CS_SERV_INIT_FLAG) {
	errnoSet (S_csLib_NOT_A_SERV);
	return ERROR;
    }
    
    /* Initialiser le pointeur vers struct. param. caracteristiques requete */
    pRqst = servId->rqstTab;
    
    /* Allouer un id de reception de requete */
    for (rqstId = 0; ; rqstId++) {
	/* Verifier s'il n'y a plus d'identificateur libre */
	if (rqstId == SERV_NMAX_RQST_ID) {
	    errnoSet(S_csLib_TOO_MANY_RQST_IDS);
	    return ERROR;
	}

	/* Verifier si l'id de requete est libre */
	if (pRqst->rqstIdFlag == FALSE)
	    break;

	/* Actualiser le pointeur de requete */
	pRqst = (SERV_RQST *) pRqst + 1;
    } /* for */

    /* Recevoir la requete */
    if (gcomLetterRcv(servId->rcvLetter, &pRqst->clientMboxId,
		      &pRqst->clientSendId, 0) != TRUE) {
	return ERROR;
    }
    /* Obtenir le type de la requete */
    if ((rqstType = gcomLetterType(servId->rcvLetter)) == ERROR) {
	return ERROR;
    }
    /* Verifier la validite du type de la requete */
    if ((rqstType < 0 && rqstType >= servId->nbRqstFunc) ||
	(servFunc = servId->rqstFuncTab[rqstType]) == (FUNCPTR) NULL) {
	return csServReplySend(servId, rqstId, FINAL_REPLY, 
			       S_csLib_INVALID_RQST_TYPE, (char *) NULL, 0,
			       (FUNCPTR) NULL);
    }
    /* Signaler la prise de cet identificateur de requete */
    pRqst->rqstIdFlag = TRUE;
    
    /* Garder l'identificateur de la requete en execution */
    servId->inExecRqstId = rqstId;
    
    /* Sinon, executer la fonction de traitement associee a cette requete */
    servFunc (servId, rqstId);
    return OK;
}


/******************************************************************************
*
*  csServRqstParamsGet - Prendre les parametres d'une requete a executer
*
*  Description:
*  Cette fonction doit etre appelee dans la routine de traitement d'une
*  requete, de maniere a pouvoir lire ses arguments (si existent!).
*
*  Retourne: OK ou ERROR
*/

STATUS 
csServRqstParamsGet(SERV_ID servId, /* Identificateur du serveur */
		    int rqstId, /* Id de la requete en execution */
		    char *rqstDataAdrs, /* Adrs ou` stocker params requete */
		    int rqstDataSize, /* Taille de cette zone */
		    FUNCPTR decodFunc) /* Fonction de decodage des donnees */
{
    SERV_RQST *pRqst;               /* Ptr vers struct. requete */
    
    /* Retourner ERROR, si ce serveur n'est pas initialise */
    if (servId->initFlag != CS_SERV_INIT_FLAG) {
	errnoSet(S_csLib_NOT_A_SERV);
	return ERROR;
    }
    
    /* Obtenir le ptr vers struct de la requete */
    pRqst = &servId->rqstTab[rqstId];
    
    /* Verifier validite du numero de la requete */
    if (rqstId < 0 || rqstId >= SERV_NMAX_RQST_ID ||
	pRqst->rqstIdFlag != TRUE || rqstId != servId->inExecRqstId) {
	errnoSet(S_csLib_BAD_RQST_ID);
	return ERROR;
    }

    /* Lire directement les parametres */
    if (gcomLetterRead (servId->rcvLetter, rqstDataAdrs, rqstDataSize, 
			decodFunc) == ERROR)
	return ERROR;
    
    /* Signaler la bonne execution */
    return OK;
}


/******************************************************************************
*
*  csServReplySend - Envoi d'une replique au client
*
*  Description:
*  Cette fonction permet d'envoyer une replique au client. 
*
*  Retourne : OK ou ERROR
*/

STATUS 
csServReplySend(SERV_ID servId,      /* Identificateur du serveur */
		int rqstId, 	     /* Identificateur de la requete */
		int replyType, 	     /* INTERMED_REPLY ou FINAL_REPLY */
		int replyBilan,      /* Bilan: OK ou code d'erreur */
		char *replyDataAdrs, /* Adresse  de la replique a envoyer */
		int replyDataSize,   /* Taille de la replique a envoyer */
		FUNCPTR codFunc)     /* Fonction de codage des donnees */
{
    SERV_RQST *pRqst;               /* Ptr vers struct de la requete */
    
    /* Retourner ERROR, si ce serveur n'est pas initialise */
    if (servId->initFlag != CS_SERV_INIT_FLAG) {
	errnoSet(S_csLib_NOT_A_SERV);
	return ERROR;
    }
    
    /* Obtenir le ptr vers struct de la requete */
    pRqst = &servId->rqstTab[rqstId];
    
    /* Verifier la validite de l'identificateur de la requete */
    if (rqstId < 0 || rqstId >= SERV_NMAX_RQST_ID 
	|| pRqst->rqstIdFlag != TRUE) {
	errnoSet(S_csLib_BAD_RQST_ID);
	return ERROR;
    }
    
    /* Verifier coherence entre le bilan de la replique et son type */
    if (replyType == INTERMED_REPLY && replyBilan != OK) {
	errnoSet(S_csLib_BAD_REPLY_BILAN);
	return ERROR;
    }
    
    /* Ecrire la lettre de replique */
    if (gcomLetterWrite(servId->replyLetter, replyBilan, replyDataAdrs,
			replyDataSize, codFunc) != OK) {
	return ERROR;
    }
    /* Envoyer la lettre de replique */
    if (gcomLetterReply(pRqst->clientMboxId, pRqst->clientSendId, 
			replyType, servId->replyLetter) != OK) {
	return ERROR;
    }

    /* Si c'etait la replique finale, liberer l'identificateur de requete */
    if (replyType == FINAL_REPLY)
	pRqst->rqstIdFlag = FALSE;
    
    /* C'est bien */
    return OK;
}


/******************************************************************************
*
*  csServRqstIdFree - Liberer un id. de requete du serveur
*
*  Description:
*  Cette fonction permet de liberer l'identificateur d'une requete recue
*  par le serveur.
*
*  Retourne: OK ou ERROR
*/

STATUS 
csServRqstIdFree(SERV_ID servId,	/* Identificateur du serveur */
		 int rqstId)		/* Identificateur de la requete */
{
    SERV_RQST *pRqst;               /* Ptr vers struct de la requete */
    
    /* Retourner ERROR, si ce serveur n'est pas initialise */
    if (servId->initFlag != CS_SERV_INIT_FLAG) {
	errnoSet(S_csLib_NOT_A_SERV);
	return ERROR;
    }

    /* Obtenir le ptr vers struct de la requete */
    pRqst = &servId->rqstTab[rqstId];
    
    /* Verifier la validite de l'identificateur de la requete */
    if (rqstId < 0 || rqstId >= SERV_NMAX_RQST_ID 
	|| pRqst->rqstIdFlag != TRUE) {
	errnoSet (S_csLib_BAD_RQST_ID);
	return ERROR;
    }
     
    /* Liberer l'id de la requete */
    pRqst->rqstIdFlag = FALSE;
    return OK;
}


/******************************************************************************
*
*  csServEnd - Liberer les objets alloues par un serveur
*
*  Description:
*  Cette fonction permet de liberer tous les objets alloues par un serveur.
*
*  Retourne: OK ou ERROR
*/

STATUS 
csServEnd(SERV_ID servId)    /* Id du serveur */
{
    /* Retourner ERROR, si ce serveur n'est pas initialise */
    if (servId->initFlag != CS_SERV_INIT_FLAG) {
	errnoSet(S_csLib_NOT_A_SERV);
	return ERROR;
    }

    /* Liberer la lettre pour la reception des messages */
    gcomLetterDiscard(servId->rcvLetter);
    
    /* Liberer la lettre des repliques */
    gcomLetterDiscard(servId->replyLetter);

    /* Indiquer que le serveur a ete libere */
    servId->initFlag = FALSE;

    /* Liberer la zone memoire occupee par le serveur */
    free((char *) servId);
    return OK;
}



/*------------- TROISIEME PARTIE : ROUTINES POUR LES CLIENTS ----------------*/


/******************************************************************************
*
*  csClientInit  -  Initialisation d'un client
*
*  Description:
*  Cette fonction permet d'initialiser une tache comme cliente d'une
*  autre tache (serveuse).
* 
*  Retourne : OK ou ERROR 
*/

STATUS 
csClientInit(char * servMboxName,	/* Nom boite aux lettres du serveur */ 
	     int maxRqstSize,		/* Taille max des requetes */ 
	     int maxIntermedReplySize,	/* Taille max des repliques 
					   intermed. */ 
	     int maxFinalReplySize,	/* Taille max des repliques finales */ 
	     CLIENT_ID *pClientId)	/* Id du client initialise */ 
{
    CLIENT_ID clientId;			/* Id. du client */
    int rqstId;				/* Id de la requete */
    CLIENT_RQST *pRqst;			/* Ptr vers struct. requete */

    /* Allouer de la memoire pour la structure de donnees du client */
    if ((clientId = (CLIENT_ID) malloc (sizeof (CS_CLIENT))) == NULL) {
	return ERROR;
    }

    /* Nettoyer cette zone memoire */
    memset((char *) clientId, 0, sizeof (CS_CLIENT));

    /* Obtenir l'identificateur de la boite aux lettres du serveur */
    if (gcomMboxFind(servMboxName, &clientId->servMboxId) != OK) {
	return ERROR;
    }
    /* Allouer une lettre pour l'envoi de requetes */
    if (gcomLetterAlloc(maxRqstSize, &clientId->sendLetter) != OK) {
	return ERROR;
    }
  
    /* Init. ptr vers struct. requete */
    pRqst = clientId->rqstTab;

    /* Allouer les lettres de replique intermed. et finale */
    for (rqstId = 0; rqstId < CLIENT_NMAX_RQST_ID; rqstId++) {
	/* Allouer, si necessaire, une lettre de replique intermediaire */
	if (maxIntermedReplySize != 0 &&
	    gcomLetterAlloc(maxIntermedReplySize, 
			    &pRqst->intermReply) != OK) {
	    return ERROR;
	}

	/* Allouer une lettre de replique finale */
	if (gcomLetterAlloc(maxFinalReplySize, 
			    &pRqst->finalReply) != OK) {
	    return ERROR;
	}

	/* Actualiser le pointeur vers struct. requete */
	pRqst = (CLIENT_RQST *) pRqst + 1;
    }
    
    /* Garder l'id du client */
    *pClientId = clientId;
    
    /* Indiquer que la structure de donnees du client a ete initialisee */
    clientId->initFlag = CS_CLIENT_INIT_FLAG;
    return OK;
}


/******************************************************************************
*
*  csClientRqstSend - Envoi d'une requete vers le serveur
*
*  Description:
*  Cette fonction permet a un client d'envoyer une requete vers un serveur,
*  dont l'id a ete obtenu au prealable au moyen de csClientInit.
*
*  Retourne : OK ou ERROR
*
*  Remarque: L'envoi des requetes est toujours non-bloquant.
*/

STATUS 
csClientRqstSend(CLIENT_ID clientId,    /* Id du client */ 
		 int rqstType, 		/* Type de la requete */ 
		 char *rqstDataAdrs, 	/* Adrs. donnees de la requete */ 
		 int rqstDataSize, 	/* Taille de la requete a envoyer */ 
		 FUNCPTR codFunc, 	/* Fonction de codage des donnees */ 
		 BOOL intermedFlag, 	/* TRUE (si attend intermed.), 
					   FALSE sinon */ 
		 int intermedReplyTout, /* Timeout attente replique 
					   intermediaire */  
		 int finalReplyTout, 	/* Timeout attente replique finale */ 
		 int *pRqstId)		/* Ou` mettre l'id de la requete */ 
{
    int rqstId;				/* Identificateur de la requete */
    LETTER_ID intermedReplyLetter;	/* Lettre de replique */
    CLIENT_RQST *pRqst;			/* Ptr vers struct requete */
    
    /* Verifier si le client a ete initialise */
    if (clientId->initFlag != CS_CLIENT_INIT_FLAG) {
	errnoSet(S_csLib_NOT_A_CLIENT);
	return ERROR;
    }
    
    /* Init. le ptr vers structure caracteristique requete */
    pRqst = clientId->rqstTab;
    
    /* Obtenir un id de requete libre */
    for (rqstId = 0; ; rqstId++) {
	/* Retourner, s'il n'y a plus de requete disponible */
	if (rqstId == CLIENT_NMAX_RQST_ID) {
	    errnoSet(S_csLib_TOO_MANY_RQST_IDS);
	    return ERROR;
	}
	
	/* Verifier si requete libre */
	if (pRqst->rqstIdFlag == FALSE)
	    break;
	
	/* Incrementer le ptr vers requete suivante */
	pRqst = (CLIENT_RQST *) pRqst + 1;
    } /* for */

    /* Garder le flag d'attente de la replique intermediaire */
    pRqst->intermedFlag = intermedFlag;
    
    /* Si n'attend pas de replique intermediaire ... */
    if (intermedFlag == FALSE) {
	intermedReplyLetter = (LETTER_ID) NULL;
    } else if (intermedFlag == TRUE) {
	/* Si attend une replique intermediaire ... */
	intermedReplyLetter = pRqst->intermReply;
    } else {
	/* Sinon, erreur ... */
      errnoSet(S_csLib_BAD_INTERMED_FLAG);
      return ERROR;
    } 

    /* Former la lettre a envoyer */
    if (gcomLetterWrite(clientId->sendLetter, rqstType, rqstDataAdrs, 
			rqstDataSize, codFunc) != OK)
	return ERROR;
    
    /* Envoyer la requete vers le serveur */
    if (gcomLetterSend(clientId->servMboxId, clientId->sendLetter, 
		       intermedReplyLetter, pRqst->finalReply, NO_BLOCK, 
		       &pRqst->sendId, intermedReplyTout,
		       finalReplyTout) == ERROR) {
	return ERROR;
    }
    
    /* Stocker l'id d'envoi */
    *pRqstId = rqstId;
    
    /* Indiquer que cet identificateur a ete pris */
    pRqst->rqstIdFlag = TRUE;
    return OK;
}


     
/******************************************************************************
*
*   csClientReplyRcv  -  Reception d'une replique
*
*   Description:
*   Cette fonction permet de recevoir (de maniere bloquante ou non-bloquante)
*   soit la replique intermediaire, soit la replique finale.
*
*   Retourne : bilan de la replique ou ERROR
*/

int 
csClientReplyRcv(CLIENT_ID clientId,	/* Identificateur du client */ 
		 int rqstId,		/* Id. de la requete */ 
		 int block,		/* NO_BLOCK, BLOCK_ON_INTERMED_REPLY,
					   BLOCK_ON_FINAL_REPLY */ 
		 char *intermedReplyDataAdrs, /* Adresse donees replique 
						 intermed. */ 
		 int intermedReplyDataSize, /* Taille de la replique 
					       intermediaire */
		 FUNCPTR intermedReplyDecodFunc, /* Fonction decodage replique
						    intermed */
		 char *finalReplyDataAdrs, /* Adresse donnees replique 
					      finale */	
		 int finalReplyDataSize, /* Taille de la replique finale */ 
		 FUNCPTR finalReplyDecodFunc) /* Fonction decodage replique 
						 finale */   
{
    int status;                         /* Etat de la replique */
    int servErrno;                      /* Errno envoye par le serveur */
    CLIENT_RQST *pRqst;                 /* Ptr vers struct. requete */

    /* Verifier si le client a ete initialise */
    if (clientId->initFlag != CS_CLIENT_INIT_FLAG) {
	errnoSet(S_csLib_NOT_A_CLIENT);
	return ERROR;
    }
    
    /* Obtenir le pointeur vers struct. caracteristique requete */
    pRqst = &clientId->rqstTab[rqstId];
    
    /* Verifier l'identificateur de la requete */
    if (rqstId < 0 || rqstId >= CLIENT_NMAX_RQST_ID 
	|| pRqst->rqstIdFlag != TRUE) {
	errnoSet(S_csLib_BAD_RQST_ID);
	return ERROR;
    }
    
    /* Lire l'etat de la replique */
    status = gcomReplyStatus(pRqst->sendId);

    /* Verifier si l'on doit attendre la replique intermediaire */
    if (block == BLOCK_ON_INTERMED_REPLY) {
	/* Ne bloquer que si dans attente de la replique intermediaire */
	if (status == WAITING_INTERMED_REPLY) {
	    status = gcomReplyWait(pRqst->sendId, INTERMED_REPLY);
	}
    /* Sinon, verifier si l'on doit attendre la replique finale */
    } else if (block == BLOCK_ON_FINAL_REPLY) {
	/* Ne bloquer que si dans attente d'une replique */
	if (status == WAITING_INTERMED_REPLY || 
	    status == WAITING_FINAL_REPLY) {
	    status = gcomReplyWait(pRqst->sendId, FINAL_REPLY); 
	}
	/* Erreur si mauvaise option de blocage */
    }  else if (block != NO_BLOCK) {
	/* Indiquer l'erreur et retourner */
	errnoSet(S_csLib_BAD_BLOCK_TYPE);
	return ERROR;
    }

    /* Verifier si a recu la lettre intermediaire */
    if (status == WAITING_FINAL_REPLY && pRqst->intermedFlag == TRUE) {
	/* Lire la lettre de replique intermediaire */
	if (gcomLetterRead(pRqst->intermReply, intermedReplyDataAdrs,
			    intermedReplyDataSize, intermedReplyDecodFunc) 
	    == ERROR) {
	    return ERROR;
	}
	
	/* Retourner l'etat de la replique */
	return status;
    }

    /* Verifier si la reponse finale a ete recue */
    if (status == FINAL_REPLY_OK) {
	/* Liberer l'id de la requete */
	pRqst->rqstIdFlag = FALSE;
	
	/* Lire la lettre de replique finale */
	if (gcomLetterRead(pRqst->finalReply, finalReplyDataAdrs,
			   finalReplyDataSize, finalReplyDecodFunc) == ERROR) {
	    return ERROR;
	}
	/* Obtenir le bilan de la replique */
	if ((servErrno = gcomLetterType (pRqst->finalReply)) != OK) {
	    errnoSet(servErrno);
	    return ERROR;
	}
    }

    /* Retourner l'etat de la replique */
    return status;
}  


/******************************************************************************
*
*   csClientRqstIdFree - Liberation de id de requete
*
*   Description:
*   Cette fonction permet de liberer l'identificateur d'une requete envoyee
*   au prealable a un serveur.
*
*   Retourne : OK ou ERROR
*/

int 
csClientRqstIdFree(CLIENT_ID clientId,	/* Identificateur du client */
		   int rqstId)		/* Id. de la requete */
{
    CLIENT_RQST *pRqst;                 /* Ptr vers struct. requete */

    /* Verifier si le client a ete initialise */
    if (clientId->initFlag != CS_CLIENT_INIT_FLAG) {
	errnoSet(S_csLib_NOT_A_CLIENT);
	return ERROR;
    }

    /* Obtenir le pointeur vers struct. caracteristique requete */
    pRqst = &clientId->rqstTab[rqstId];
    
    /* Verifier l'identificateur de la requete */
    if (rqstId < 0 || rqstId >= CLIENT_NMAX_RQST_ID 
	|| pRqst->rqstIdFlag != TRUE) {
	errnoSet(S_csLib_BAD_RQST_ID);
	return ERROR;
    }
    
    /* Liberer l'id de l'envoi */
    if (gcomSendIdFree(pRqst->sendId) != OK) {
	return ERROR;
    }
    /* Liberer l'id de la requete */
    pRqst->rqstIdFlag = FALSE;
    return OK;
}


/******************************************************************************
*
*  csClientEnd - Liberation des objets alloues par un client
*
*  Description:
*  Cette fonction permet de liberer les objets alloues par un client
* 
*  Retourne : OK ou ERROR 
*/

STATUS 
csClientEnd(CLIENT_ID clientId)		/* Identificateur du client */
{
    int rqstId;				/* Id d'une requete */
    CLIENT_RQST *pRqst;			/* Ptr vers struct. requete */

    /* Verifier si le client a ete initialise */
    if (clientId == NULL || clientId->initFlag != CS_CLIENT_INIT_FLAG) {
	errnoSet(S_csLib_NOT_A_CLIENT);
	return ERROR;
    }

    /* Liberer la lettre pour l'envoi de requetes */
    gcomLetterDiscard(clientId->sendLetter);
    
    /* Init. ptr vers struct. requete */
    pRqst = clientId->rqstTab;

    /* Liberer les lettres de replique intermed. et finale */
    for (rqstId = 0; rqstId < CLIENT_NMAX_RQST_ID; rqstId++) {
	/* Liberer, si necessaire, la lettre de replique intermediaire */
	if (pRqst->intermReply != (LETTER_ID) NULL) {
	    gcomLetterDiscard(pRqst->intermReply);
	}
	/* Liberer la lettre de replique finale */
	gcomLetterDiscard(pRqst->finalReply);
	
	/* Actualiser le pointeur vers struct. requete */
	pRqst = (CLIENT_RQST *) pRqst + 1;
    } /* for */
  
    /* Indiquer que la structure de donnees du client a ete liberee */
    clientId->initFlag = FALSE;
    
  /* Liberer le pool de memoire et retourner */
    free((char *) clientId);
    return OK;
}
