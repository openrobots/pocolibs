/*
 * Copyright (c) 1990, 2003-2004, 2012, 2014, 2024-2025 CNRS/LAAS
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

#include "pocolibs-config.h"

#include <sys/types.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "portLib.h"
#include "taskLib.h"
#include "tickLib.h"
#include "h2semLib.h"
#include "errnoLib.h"
#include "h2devLib.h"

#include "gcomLib.h"
static const H2_ERROR gcomLibH2errMsgs[] = GCOM_LIB_H2_ERR_MSGS;

#ifdef COMLIB_DEBUG_GCOMLIB
# define LOGDBG(x)     logMsg x
#else
# define LOGDBG(x)
#endif

/**
 ** Variables statiques
 **/
static MBOX_ID *rcvMboxTab = NULL;    /* Tableau mboxes reception */
static MBOX_ID *replyMboxTab = NULL;  /* Tableau mboxes replique */
static SEND **sendTab = NULL;         /* Tableau de sends */

static void gcomDispatch (MBOX_ID replyMbox);
static int gcomVerifStatus(int sendId);
static int gcomVerifTout (int sendId, int *pTimeout);

/* Retrieve the h2 device index of a task, used as an index in local arrays */
#define MY_TASK_DEV_IDX (H2DEV_INDEX(taskGetUserData(0)))


static pthread_once_t gcom_once = PTHREAD_ONCE_INIT;

static void
gcomAllocTabs(void)
{
    int h2devMax = h2devSize();

    rcvMboxTab = calloc(h2devMax, sizeof(MBOX_ID));
    if (rcvMboxTab == NULL)
        return;

    replyMboxTab = calloc(h2devMax, sizeof(MBOX_ID));
    if (replyMboxTab == NULL)
        return;
    sendTab = calloc(h2devMax, sizeof(MBOX_ID *));
    if (sendTab == NULL)

        return;
}

/******************************************************************************
*
*  gcomInit  -  Initialisation du gestionnaire de communications
*
*  Description :
*  Cette fonction doit etre appelee pendant la phase d'initialisation,
*  avant d'employer les utilitaires gcom.
*
*  Retourne : OK ou ERROR
*/

STATUS
gcomInit(const char *procName, int rcvMboxSize, int replyMboxSize)
{
    char replyMboxName[H2_DEV_MAX_NAME];
    int myTaskNum;
    int retval;
    
    /* record error msgs */
    h2recordErrMsgs("gcomInit", "gcomLib", M_gcomLib, 			
		    sizeof(gcomLibH2errMsgs)/sizeof(H2_ERROR), 
		    gcomLibH2errMsgs);

    /* Appeler la routine d'initialisation des mailboxes */
    if (mboxInit(procName) == ERROR) {
        LOGDBG(("gcomInit:mboxInit failed %d\n", errnoGet()));
	return ERROR;
    }

    /* Nom du mailbox de replique */
    strncpy (replyMboxName, procName, H2_DEV_MAX_NAME - 1);
    strcat (replyMboxName, "R");

    /* Allouer et Initialiser les tableaux */
    /* In the case we have several threads in the same address space,
       only do the allocation once */
    retval = pthread_once(&gcom_once, gcomAllocTabs);
    if (retval != 0) {
        errnoSet(retval);
        LOGDBG(("gcomInit:pthread_once failed %d\n", errnoGet()));
        goto failed;
    }
    myTaskNum = MY_TASK_DEV_IDX;
    sendTab[myTaskNum] = (SEND *)calloc(MAX_SEND, sizeof(SEND));
    if (sendTab[myTaskNum] == NULL)
	    goto failed;

    LOGDBG(("comLib:gcomInit: arrays allocated for mbox %d (`%s')\n",
	    myTaskNum, procName));

    /* Creation mailbox de reception */
    if (rcvMboxSize != 0) {
	if (mboxCreate(procName, rcvMboxSize, 
		       &rcvMboxTab[myTaskNum]) == ERROR) {
		int e = errnoGet();
		free(sendTab[myTaskNum]);
		sendTab[myTaskNum] = NULL;
                LOGDBG(("gcomInit:mboxCreate rcv failed %d\n", e));
		errnoSet(e);
		return ERROR;
	}
    }

    /* Creation mailbox de replique */
    if (replyMboxSize != 0) {
	if (mboxCreate(replyMboxName, replyMboxSize, 
		       &replyMboxTab[myTaskNum]) == ERROR) {
		int e = errnoGet();
		mboxDelete(rcvMboxTab[myTaskNum]);
		free(sendTab[myTaskNum]);
		sendTab[myTaskNum] = NULL;
                LOGDBG(("gcomInit:mboxCreate reply failed %d\n", e));
		errnoSet(e);
		return ERROR;
	}
    }
    return OK;
failed:
    free(rcvMboxTab);
    free(replyMboxTab);
    free(sendTab);
    errnoSet(S_gcomLib_MALLOC_FAILED);
    return ERROR;
}


/******************************************************************************
*
*  gcomUpdate  -  Resize mailboxes
*
*  Description :
*  This function resizes task mailboxes
*
*  Retourne : OK ou ERROR
*/

STATUS
gcomUpdate(int rcvMboxSize, int replyMboxSize)
{
    int myIndice;

    myIndice = MY_TASK_DEV_IDX;
    if (rcvMboxSize > 0) {
        if (mboxResize(rcvMboxTab[myIndice], rcvMboxSize) != OK) {
            return ERROR;
        }
    }
    if (replyMboxSize > 0) {
        if (mboxResize(replyMboxTab[myIndice], replyMboxSize) != OK) {
            return ERROR;
        }
    }

    return OK;
}


/*****************************************************************************
*
*  gcomEnd  -  Routine de finalisation
*
*  Description:
*  Liberer tous les objets alloues par une tache.
*
*  Retourne:
*  OK ou ERROR
*/

STATUS
gcomEnd(void)
{
    int taskIndice;
    int bilan = OK;               /* Bilan de la routine */

    taskIndice = MY_TASK_DEV_IDX;

    /* Liberer les mailboxes alloues */
    if (mboxEnd(0) == ERROR) {
	bilan = ERROR;
    }

    /* Liberer les tableaux de send et de lettres */
    free(sendTab[taskIndice]);
    sendTab[taskIndice] = NULL;
    /* Retourner le bilan */
    return (bilan);
}

/******************************************************************************
*
*   gcomSelect - Select sur les lettre de reponse en cours
*
*   Description:
*   Cette fonction permet de tester l'etat des requetes en cours
*   Elle travaille a partir d'un masque contenant 1 pour les lettre a tester
*   pour chaque id de lettre a tester elle renvoie dans le masque l'une
*   des valeurs suivantes:
*   FINAL_REPLY_OK
*   WAITING_INTERMED_REPLY
*   INTERMED_REPLY_TIMEOUT
*   WAITING_FINAL_REPLY
*   FINAL_REPLY_TIMEOUT
*
*/

void 
gcomSelect(char *sendIdTable)
{
    int i;

    gcomDispatch(replyMboxTab[MY_TASK_DEV_IDX]);
    
    for (i=0; i<MAX_SEND; i++) {
	if (sendIdTable[i] == 1) {
	    sendIdTable[i] = gcomVerifStatus(i);
	}
    }
}

/*****************************************************************************
*
*  gcomMboxFind  -  Chercher l'adresse d'un mailbox
*
*  Description:
*  Cette fonction permet de retrouver l'adresse du mailbox de reception
*  d'un processus dont le nom est fourni comme argument.
*
*  Retourne : OK ou ERROR
*/

STATUS 
gcomMboxFind(const char *procName, MBOX_ID *pMboxId)
{
    return mboxFind(procName, pMboxId);
}

/******************************************************************************
*
*  gcomMboxPause  -  Attendre l'arrivee d'un message
*
*  Description:
*  Quand on appelle cette fonction, la tache reste bloquee jusqu'a l'arrivee
*  d'un message sur un mbox donnee.
*
*     int timeout;               Periode de timeout 
*     int mask;                  Masque de choix des mboxes 
*
*  Retourne : Masque du mbox avec message ou ERROR
*/

int 
gcomMboxPause(int timeout, int mask)
{
    int n1, n2;                  /* Nombre de bytes dans les mailboxes */
    int pauseStat;               /* Status de la pause */
    int myIndice;

    myIndice = MY_TASK_DEV_IDX;
    /* En fonction de la masque, trouver le mbox ou` attendre */
    switch (mask) {
      case RCV_MBOX:
	/* Attendre l'arrivee d'un message sur le mbox de reception */
	if ((pauseStat = mboxPause(rcvMboxTab[myIndice], timeout)) != TRUE) {
	    return pauseStat;
	}
	return RCV_MBOX;
	    
      case REPLY_MBOX:
	/* Attendre l'arrivee d'un message sur le mbox de repliques */
	if ((pauseStat = mboxPause(replyMboxTab[myIndice], timeout)) != TRUE) {
	    return pauseStat;
	}
	return REPLY_MBOX;

      case RCV_MBOX | REPLY_MBOX:
	/* Attendre l'arrivee du message sur les deux mboxes */
	break;

      default:
	/* Erreur definition du mbox */
	errnoSet(S_gcomLib_ERR_MBOX_MASK);
	return ERROR;
    }

    /* Attendre l'arrivee d'un message et verifier l'etat des mailboxes */
    if (mboxPause(ALL_MBOX, timeout) == ERROR ||
	mboxIoctl(rcvMboxTab[myIndice], FIO_NBYTES, &n1) == ERROR ||
	mboxIoctl(replyMboxTab[myIndice], FIO_NBYTES, &n2) == ERROR) {
	return ERROR;
    }
    /* Former la masque de sortie et retourner */
    if (!n1)
	mask = mask & ~RCV_MBOX;
    if (!n2)
	mask = mask & ~REPLY_MBOX;
    return (mask);
}

/*****************************************************************************
*
*  gcomMboxStatus  -  Etat des mboxes 
*
*  Description :
*  Cette fonction verifie s'il y a des lettres dans le (s) mailbox (es)
*  donne (s). On choisit le mailbox a verifier par moyen du remplissage
*  d'une masque de selection.
*
*  Retourne : masque des mboxes avec lettre ou ERROR
*/

int 
gcomMboxStatus(int mask)
{
    int n1, n2;                  /* Nombre de bytes dans les mailboxes */
    int myIndice;
    
    myIndice = MY_TASK_DEV_IDX;
    /* En fonction de la masque, trouver le mbox a verifier */
    switch (mask) {
      case RCV_MBOX:
	/*  Verifier le mbox de reception */
	if (mboxIoctl(rcvMboxTab[myIndice], FIO_NBYTES, &n1) == ERROR)
	    return ERROR;
	return ((!n1) ? 0 : RCV_MBOX);
	
      case REPLY_MBOX:
	if (mboxIoctl(replyMboxTab[myIndice], FIO_NBYTES, &n2) == ERROR)
	    return ERROR;   
	return ((!n2) ? 0 : REPLY_MBOX);
	
      case RCV_MBOX | REPLY_MBOX:
	/* Attendre l'arrivee du message sur les deux mboxes */
	if (mboxIoctl (rcvMboxTab[myIndice], FIO_NBYTES, &n1) == ERROR ||
	    mboxIoctl (replyMboxTab[myIndice], FIO_NBYTES, &n2) == ERROR)
	    return ERROR;
	
	/* Former la masque de sortie et retourner */
	if (!n1)
	    mask = mask & ~RCV_MBOX;
	if (!n2)
	    mask = mask & ~REPLY_MBOX;
	return mask;
	
      default:
	/* Erreur definition du mbox */
	errnoSet(S_gcomLib_ERR_MBOX_MASK);
	return ERROR;
    } /* switch */
}

/******************************************************************************
*
*  gcomMboxName  -  Obtenir le nom d'un mailbox, a partir de son id
*
*  Description:
*  Cette fonction permet d'obtenir le nom d'un mailbox, etant donne son id.
*
*  Retourne : OK ou ERROR
*/

STATUS 
gcomMboxName(MBOX_ID mboxId, char *pName)
{
    return (mboxIoctl (mboxId, FIO_GETNAME, pName));
}

/******************************************************************************
*
*  gcomMboxShow  -  Impression de l'etat des mboxes sur sortie standard
*
*  Description:
*  On visualise l'etat actuel des mailboxes sur la sortie standard
*
*  Retourne: Neant
*/

void 
gcomMboxShow (void)
{
    /* Montre l'etat des mailboxes sur la sortie standard */
    mboxShow ();
}

/******************************************************************************
*
*  gcomLetterAlloc  -  Allouer une lettre
*
*  Description:
*  Cette fonction permet la prise d'une lettre de taille donnee, pour etre
*  employee comme support de donnees pour les messages entre les taches.
*  C'est une fonction qui doit etre appelee pendant la phase d'initialisation.
*
*     int sizeLetter;                 Taille de la lettre 
*     LETTER_ID *pLetterId;           Ou` mettre l'id de la lettre
*
*  Retourne : OK ou ERROR
*/

STATUS
gcomLetterAlloc(int sizeLetter, LETTER_ID *pLetterId)
{
    LETTER_ID letter;                  /* Lettre allouee */

    letter = malloc(sizeof(LETTER));
    if (letter == NULL) {
            errnoSet (S_gcomLib_TOO_MANY_LETTERS);
            return (ERROR);
    }
    memset(letter, 0, sizeof(LETTER));

    /* Allouer de la memoire pour la lettre */
    if ((letter->pHdr = (LETTER_HDR_ID) calloc (1, (size_t) sizeLetter + 
					      sizeof (LETTER_HDR))) == NULL)
	return (ERROR);
    
    /* Remplir les champs de la lettre */
    letter->size = sizeLetter + sizeof (LETTER_HDR);
    letter->flagInit = GCOM_FLAG_INIT;
    
    /* Garder l'identificateur de la lettre et retourner */
    *pLetterId = letter;
    return (OK);
}

/******************************************************************************
*
*  gcomLetterWrite  -  Ecriture d'une lettre
*
*  Description:
*  Par moyen de cette fonction, on ecrit un message sur une lettre donnee.
*  
*     LETTER_ID letterId;           Lettre a ecrire 
*     int dataType;                 Type des donnees de la lettre
*     char *pData;                  Adresse initiale donnees a ecrire
*     int dataSize;                 Taille des donnees
*     GCOM_CODINGFUNC codingFunc;   Letter encoding function
*
*  Retourne : OK ou ERROR
*/

STATUS 
gcomLetterWrite(LETTER_ID letterId, int dataType, char *pData, 
		int dataSize, GCOM_CODINGFUNC codingFunc)
{
    LETTER_HDR_ID pText;            /* Pointeur vers le texte de la lettre */
    
    /* Verifier si la lettre a ete cree */
    if (letterId->flagInit != GCOM_FLAG_INIT) {
	errnoSet (S_gcomLib_NOT_A_LETTER);
	return (ERROR);
    }
    
    /* Obtenir pointeur vers texte */
    pText = letterId->pHdr;
    
    /* Garder le type de donnee */
    pText->dataType = dataType;
    
    /* Verifier si le codage n'a pas ete demande */
    if (codingFunc == NULL) {
	/* Verifier si la lettre comporte le message */
	if (letterId->size < dataSize + sizeof (LETTER_HDR)) {
	    errnoSet (S_gcomLib_SMALL_LETTER);
	    return (ERROR);
	}
	
	/* Garder la taille des donnees */
	pText->dataSize = dataSize;
	
	/* Copier directement les donnees sur la lettre */
	memcpy ((char *) pText + sizeof (LETTER_HDR), pData, dataSize);
	return (OK);
    }

    /* Appeler la fonction de codage */
    if ((pText->dataSize = 
	 codingFunc (pData, dataSize, (char *) pText + sizeof (LETTER_HDR),
		     letterId->size - sizeof (LETTER_HDR))) == ERROR)
	return (ERROR);
    return (OK);
}

/******************************************************************************
*
*  gcomLetterType - Type des donnees d'une lettre
*
*  Description:
*  Cette fonction permet d'obtenir le type des donnees vehiculees par la lettre
*
*  Retourne : type des donnees de la lettre ou ERROR
*/

int 
gcomLetterType(LETTER_ID letterId)
{
    /* Verifier si la lettre est initialisee */
    if (letterId->flagInit != GCOM_FLAG_INIT) {
	errnoSet (S_gcomLib_NOT_A_LETTER);
	return (ERROR);
    }

    /* Retourner le type de la lettre */
    return (letterId->pHdr->dataType);
}

/******************************************************************************
*
*  gcomLetterRead - Lecture d'une lettre
*
*  Description:
*  Cette fonction permet de lire le contenu d'une lettre qui a ete recue.
*  Optionnellement, une fonction de decodage peut etre aussi employee.
*
*     LETTER_ID letterId;          Lettre a lire 
*     char *pData;                 Ou` mettre les donnees 
*     int maxDataSize;             Taille max du message a lire 
*     GCOM_CODINGFUNC decodingFunc Decoding function
*
*  Retourne : taille du message en bytes ou ERROR
*/

int 
gcomLetterRead(LETTER_ID letterId, char *pData, int maxDataSize, 
	       GCOM_CODINGFUNC decodingFunc)
{
    int dataSize;                   /* Taille des donnees de la lettre */
    LETTER_HDR_ID pText;            /* Pointeur vers le texte de la lettre */
    
    /* Verifier si la lettre est initialisee */
    if (letterId->flagInit != GCOM_FLAG_INIT) {
	errnoSet (S_gcomLib_NOT_A_LETTER);
	return (ERROR);
    }
    
    /* Obtenir le pointeur vers le texte de la lettre */
    pText = letterId->pHdr;
    
    /* Verifier s'il faut pas decoder le message */
    if (decodingFunc == NULL) {
	/* Obtenir la taille des donnees recues */
	dataSize = pText->dataSize;
	
	/* Verifier si structure donnees utilisateur comporte message */
	if (maxDataSize < dataSize) {
	    errnoSet (S_gcomLib_SMALL_DATA_STR);
	    return (ERROR);
	}
	
	/* Copier les donnees dans la structure de l'utilisateur */
	memcpy (pData, (char *) pText + sizeof (LETTER_HDR), dataSize);
	
	/* Retourner la taille des donnees recues */
	return (dataSize);
    }
    
    /* Sinon, decoder la lettre recue */
    return (decodingFunc ((char *) pText + sizeof (LETTER_HDR),
			  pText->dataSize, pData, maxDataSize));
}

/******************************************************************************
 *
 *  gcomLetterDiscard  -  Liberer une lettre
 *
 *  Description:
 *  Libere les ressources prises par une lettre.
 * 
 *  Retourne : OK
 */

STATUS 
gcomLetterDiscard(LETTER_ID letterId)
{
    /* Verifier si la lettre est initialisee */
    if (letterId->flagInit != GCOM_FLAG_INIT) {
	errnoSet (S_gcomLib_NOT_A_LETTER);
	return (ERROR);
    }
    /* Liberer la memoire */
    free (letterId->pHdr);
    free(letterId);
    return (OK);
}

/*****************************************************************************
*
*  gcomLetterList  -  Obtention de la liste de lettres prises
*
*  Description:
*  Cette fonction possibilite l'obtention de la liste de lettres prises
*  par la tache. 
*
*  Retourne: le nombre de lettres prises, dans le limite de la
*  liste fournie par l'utilisateur.
*/

int 
gcomLetterList(LETTER_ID *pList, int maxList)
{
    /* Not implemented in this version */
    return 0;
}

/******************************************************************************
*
*  gcomLetterSend  -  Envoi d'une lettre
*
*  Description:
*  Cette fonction permet qu'une lettre soit envoyee vers la boite
*  aux lettres de reception d'une tache destinataire. On peut attendre
*  l'arrivee de deux lettres de reponse: une finale et une autre intermediaire
*  La lettre de reponse finale est toujours obligatoire, tandis que la
*  reponse intermediaire est optionelle. Des periodes de timeout peuvent
*  etre definies pour l'attente des deux types de reponse.
*  
*     MBOX_ID mboxId;                  Mailbox de destination 
*     LETTER_ID sendLetter;            Lettre a envoyer 
*     LETTER_ID intermedReplyLetter;   Lettre de replique intermediaire 
*     LETTER_ID finalReplyLetter;      Lettre finale de replique 
*     int block;                       NO_BLOCK, BLOCK_ON_FINAL_REPLY,
*				       BLOCK_ON_INTERMED_REPLY 
*     int *pSendId;                    Ou` mettre id d'envoi (si pas bloq.) 
*     int intermedReplyTout;           Timeout attente replique intermed. 
*     int finalReplyTout;              Timeout attente replique finale 
*
*  Retourne: bilan de l'envoi ou ERROR
*/

int 
gcomLetterSend(MBOX_ID mboxId, LETTER_ID sendLetter, 
	       LETTER_ID intermedReplyLetter, LETTER_ID finalReplyLetter,
	       int block, int *pSendId, 
	       int intermedReplyTout, int finalReplyTout)
{
    SEND *pTabSend;        /* Pointeur vers tableau de sends de la tache */
    int sendId;            /* Identificateur du send */
    LETTER_HDR_ID pText;   /* Pointeur vers le texte de la lettre */
    int status;            /* Etat du reply */
    int timeout = NO_WAIT;		/* Temps avant timeout */
    int myIndice;
    
    myIndice = MY_TASK_DEV_IDX;

    /* Verifier le type de blocage demande */
    if ((block != NO_BLOCK && block != BLOCK_ON_FINAL_REPLY &&
	 block != BLOCK_ON_INTERMED_REPLY) || 
	(block == BLOCK_ON_INTERMED_REPLY && intermedReplyLetter == NULL)) {
	errnoSet (S_gcomLib_INVALID_BLOCK_MODE);
	return (ERROR);
    }
  
    /* Verifier si les lettres de send et de reply existent */
    if (sendLetter->flagInit != GCOM_FLAG_INIT ||
	finalReplyLetter->flagInit != GCOM_FLAG_INIT ||
	(intermedReplyLetter != (LETTER_ID) NULL &&
	 intermedReplyLetter->flagInit != GCOM_FLAG_INIT)) {
	errnoSet (S_gcomLib_NOT_A_LETTER);
	return (ERROR);
    }
    
    /* Calculer le pointeur vers le debut du tableau de sends */
    pTabSend = &sendTab[myIndice][0];
    
    /* Chercher une position libre dans le tableau de sends */
    for (sendId = 0; pTabSend->status != FREE; sendId++) {
	/* Retourner ERROR, s'il n'y en a pas */
	if (sendId >= MAX_SEND) {
	    errnoSet (S_gcomLib_TOO_MANY_SENDS);
	    return (ERROR);
	}
	
	/* Incrementer le pointeur */
	pTabSend = (SEND *) pTabSend + 1;
    } /* for */

    /* Obtenir le pointeur vers le texte de la lettre */
    pText = sendLetter->pHdr;
    
    /* Inscrire l'id du send */
    pText->sendId = sendId;
    
    /* Verifier si une replique intermediaire est demandee */
    if (intermedReplyLetter != (LETTER_ID) NULL) {
	pTabSend->status = WAITING_INTERMED_REPLY;
	pTabSend->intermedReplyTout = intermedReplyTout;
	pTabSend->intermedReplyLetter = intermedReplyLetter;
    } else {
	pTabSend->status = WAITING_FINAL_REPLY;
	pTabSend->intermedReplyLetter = (LETTER_ID) NULL;
    }
    
    /* Lire le temps */
    pTabSend->time = tickGet();
    
    /* Charger le champ de timeout de la replique finale */
    pTabSend->finalReplyTout = finalReplyTout;
    
    /* Garder l'adresse lettre de replique finale */
    pTabSend->finalReplyLetter = finalReplyLetter;
    
    /* Envoyer la lettre */
    if (mboxSend (mboxId, replyMboxTab[myIndice], (char *) pText,
		  pText->dataSize + sizeof (LETTER_HDR)) != OK) {
	/* Liberer l'identificateur d'envoi */
	pTabSend->status = FREE;
	return (ERROR);
    }
    
    /* Garder l'identificateur d'envoi, si necessaire */
    if (block != BLOCK_ON_FINAL_REPLY)
	*pSendId = sendId;
    
    /* Si pas bloquant, retourner */
    if (block == NO_BLOCK)
	return (pTabSend->status);
    
    /* Boucle d'attente */
    FOREVER {
	/* Distribuer les repliques qui sont deja disponibles */
	gcomDispatch (replyMboxTab[myIndice]);
	
	/* Verifier les timeouts */
	status = gcomVerifTout (sendId, &timeout);
	
	/* Verifier si bloque en attente de replique finale */
	if (block == BLOCK_ON_FINAL_REPLY) {
	    /* Retourner si l'attente est finie */
	    if (status != WAITING_FINAL_REPLY && 
		status != WAITING_INTERMED_REPLY)
		return (status);
	} else if (status != WAITING_INTERMED_REPLY) {
	    
	    /* Sinon, la tache est bloquee en attente 
	       de la replique intermediaire */
	    return (status);
	}
	
	/* Attendre */
	if (mboxPause (replyMboxTab[myIndice], timeout) == ERROR) {
	    return (ERROR);
	}
    } /* forever */
}


/******************************************************************************
* 
*  gcomReplyStatus - Status d'une replique
*
*  Description:
*  Cette fonction permet de verifier l'etat d'une replique. Les etats
*  possibles sont: attendant replique intermediaire, attendant la replique
*  finale, timeout replique intermediaire, timeout replique finale, 
*  replique finale OK.
* 
*  Retourne : etat de la replique ou ERROR
*/

int 
gcomReplyStatus(int sendId)
{
    int timeout;                 /* Temps qui reste jusqu'au timeout */
    
    /* Verifier l'id du send */
    if (sendId < 0 || sendId > MAX_SEND) {
	errnoSet (S_gcomLib_ERR_SEND_ID);
	return (ERROR);
    }
    
    /* Distribuer les repliques qui sont deja disponibles */
    gcomDispatch (replyMboxTab[MY_TASK_DEV_IDX]);
    
    /* Verifier les timeouts et retourner */
    return (gcomVerifTout (sendId, &timeout));
}

/******************************************************************************
*
*  gcomReplyWait -  Attente d'une lettre de replique
*
*  Description:
*  Cette fonction permet d'attendre l'arrivee soit d'une replique 
*  intermediaire, soit d'une replique finale. La tache se suspend jusqu'a
*  ce que cette replique arrive ou jusqu'a ce que la periode de timeout 
*  s'ecoule.
*
*  Retourne: etat de la replique ou ERROR
*/

int 
gcomReplyWait(int sendId, int replyLetterType)
{
    int status;                    /* Etat de la replique */
    int timeout = NO_WAIT;		/* Temps qui reste jusqu'au timeout */
    int indice;

    indice = MY_TASK_DEV_IDX;
    
    /* Verifier l'id du send */
    if (sendId < 0 || sendId > MAX_SEND) {
	errnoSet (S_gcomLib_ERR_SEND_ID);
	return (ERROR);
    }
    
    /* Verifier le type de la replique attendue */
    if (replyLetterType != INTERMED_REPLY &&
	replyLetterType != FINAL_REPLY) {
	errnoSet (S_gcomLib_REPLY_LETTER_TYPE);
	return (ERROR);
    }
    
    /* Boucle d'attente */
    FOREVER {
	/* Distribuer les repliques qui sont deja disponibles */
	gcomDispatch (replyMboxTab[indice]);
	
	/* Verifier les timeouts */
	status = gcomVerifTout (sendId, &timeout);
	
	/* Verifier si attend la replique finale */
	if (replyLetterType == FINAL_REPLY) {
	    /* Verifier s'il faut retourner */
	    if (status != WAITING_INTERMED_REPLY &&
		status != WAITING_FINAL_REPLY)
		return (status);
	} else if (status != WAITING_INTERMED_REPLY) {
	    /* Sinon, attend la replique intermediaire */
	    return (status);
	}
	/* Attendre */
	if (mboxPause (replyMboxTab[indice], timeout) == ERROR) {
	    return (ERROR);
	}
    } /* forever */
}

/******************************************************************************
*
*  gcomSendIdFree -  Liberer un identificateur d'envoi
*
*  Description:
*  Cette fonction permet de liberer un identificateur d'envoi obtenu apres
*  un envoi non-bloquant. 
*  Remarque: avant de liberer l'identicateur d'envoi, il faut etre absolument
*  sur que la tache ne recevra ni l'acquittement ni la replique associes.
*
*  Retourne: OK ou ERROR
*/

STATUS 
gcomSendIdFree(int sendId)
{
    int taskIndice;

    /* Verifier l'id du send */
    if (sendId < 0 || sendId > MAX_SEND) {
	errnoSet (S_gcomLib_ERR_SEND_ID);
	return (ERROR);
    }
    
    taskIndice = MY_TASK_DEV_IDX;

    /* Liberer l'identificateur d'envoi */
    sendTab[taskIndice][sendId].status = FREE;
    return (OK);
}

/******************************************************************************
*
*  gcomSendIdList - Obtention de la liste de sendIds pendants
*
*  Description:
*  Permet l'obtention d'une liste avec les sendIds des envois encore pendants.
*  La taille max de cette liste doit etre fournie par l'utilisateur.
*
*     int *pList;             Liste ou` mettre les sendIds 
*     int maxList;            Taille max de la liste 
*
*  Retourne: le nombre de sendIds mis dans la liste
*/

int 
gcomSendIdList(int *pList, int maxList)
{
    SEND *pTabSend;           /* Pointeur vers tableau de sends de la tache */
    int sendId;               /* Identificateur du send */
    int cList;                /* Compteur de la liste */
    
    /* Calculer le pointeur vers le debut du tableau de sends */
    pTabSend = &sendTab[MY_TASK_DEV_IDX][0];
    
    /* Remplir la liste avec les sendIds non libres */
    for (sendId = 0, cList = 0; sendId < MAX_SEND && cList < maxList; 
	 sendId++) {
	/* Verifier si envoi pendant */
	if (pTabSend->status != FREE) {
	    /* Remplir la liste */
	    *pList = sendId;
	    
	    /* Incrementer le pointeur de la liste */
	    pList++;
	    
	    /* Incrementer le compteur de la liste */
	    cList++;
	}
	
	/* Incrementer le pointeur du tableau */
	pTabSend = (SEND *) pTabSend + 1;
    } /* for */

    /* Retourner le nombre de sendIds pendants */
    return (cList);
}

/******************************************************************************
*
*  gcomReplyLetterBySendId  -  Obtenir lettre replique a partir sendId
*
*  Description:
*  Cette fonction permet l'obtention des lettres de replique (intermediaire
*  ou finale) qui sont associees a un sendId fourni comme argument.
*
*     int sendId;                            Identificateur du send 
*     LETTER_ID *pIntermedReplyLetter;       Ptr vers lettre intermed 
*     LETTER_ID *pFinalReplyLetter;          Ptr vers lettre finale 
*
*  Retourne : Neant
*/

void 
gcomReplyLetterBySendId (int sendId, LETTER_ID *pIntermedReplyLetter, 
			 LETTER_ID *pFinalReplyLetter)
{
    SEND *pTabSend;           /* Pointeur vers tableau de sends de la tache */
    
    /* Calculer le pointeur vers position sendId dans tableau de sends */
    pTabSend = &sendTab[MY_TASK_DEV_IDX][sendId];
    
    /* Copier l'id de la lettre de replique intermediaire */
    if (pIntermedReplyLetter != (LETTER_ID *) NULL)
	*pIntermedReplyLetter = pTabSend->intermedReplyLetter;
    
    /* Copier l'id de la lettre de replique finale */
    if (pFinalReplyLetter != (LETTER_ID *) NULL) 
	*pFinalReplyLetter = pTabSend->finalReplyLetter;
}


/******************************************************************************
*
*  gcomLetterRcv - Reception d'une lettre
*
*  Description:
*  Cette fonction permet de prendre une lettre du mailbox de reception.
*
*     LETTER_ID letter;           Entree: lettre de reception 
*     MBOX_ID *pOrigMboxId;       Sortie: mbox originataire lettre 
*     int *pSendId;               Sortie: identif. de send du message 
*     int timeout;                Timeout d'attente du message 
*
*  Retourne: 
*  TRUE (si lettre), FALSE (si timeout) ou ERROR
*/  

BOOL  
gcomLetterRcv (LETTER_ID letter, MBOX_ID *pOrigMboxId, int *pSendId, 
	       int timeout)
{
    LETTER_HDR_ID pText;          /* Pointeur vers texte de la lettre */
    int rcvStatus;                /* Etat de la reception */
    
    /* Verifier l'initialisation de la lettre */
    if (letter->flagInit != GCOM_FLAG_INIT) {
	errnoSet (S_gcomLib_NOT_A_LETTER);
	return (ERROR);
    }
    
    /* Obtenir le pointeur vers le texte de la lettre */
    pText = letter->pHdr;
    
    /* Attendre l'arrivee d'un message sur le mailbox */
    if ((rcvStatus = mboxRcv (rcvMboxTab[MY_TASK_DEV_IDX], pOrigMboxId,
			      (char *) pText, letter->size, timeout)) <= 0)
	return (rcvStatus);
    
    /* Garder l'id du send */
    *pSendId = pText->sendId;
    return (TRUE);
}

/*****************************************************************************
*
*  gcomLetterReply - Envoi d'une replique 
*
*  Description:
*  Cette fonction permet a une tache d'envoier une lettre de replique
*  (finale ou intermediaire) a une lettre recue.
*
*    MBOX_ID mboxId;              Mbox ou` mettre la lettre de replique 
*    int sendId;                  Id de send de l'originateur 
*    int replyLetterType;         Type de la lettre de replique a envoyer 
*    LETTER_ID letterReply;       Lettre de replique 
*
*  Retourne:
*  OK ou ERROR
*/

STATUS 
gcomLetterReply(MBOX_ID mboxId, int sendId, int replyLetterType,
		LETTER_ID letterReply)
{
    LETTER_HDR_ID pText;           /* Pointeur vers texte de la lettre */
    
    /* Verifier le type de lettre de replique a envoyer */
    if (replyLetterType != INTERMED_REPLY &&
	replyLetterType != FINAL_REPLY) {
	errnoSet (S_gcomLib_REPLY_LETTER_TYPE);
	return (ERROR);
    }
    
    /* Verifier l'initialisation de la lettre */
    if (letterReply->flagInit != GCOM_FLAG_INIT) {
	errnoSet (S_gcomLib_NOT_A_LETTER);
	return (ERROR);
    }
    
    /* Obtenir le pointeur vers le texte de la lettre */
    pText = letterReply->pHdr;
    
    /* Remplir l'en-tete de la lettre de replique */
    pText->sendId = sendId;
    pText->type = replyLetterType;
    
    /* Envoyer la lettre */
    return (mboxSend (mboxId, rcvMboxTab[MY_TASK_DEV_IDX], (char *) pText,
		      pText->dataSize + sizeof (LETTER_HDR)));
}


/****************************************************************************
*
*  gcomDispatch  -  Distribue les lettres de replique
*
*  Description:
*  Cette fonction distribue les lettres de replique, jusqu'a ce qu'il 
*  n'y ait plus rien dans le mailbox de replique. 
*
*  Retourne: Neant.
*/

static void
gcomDispatch (MBOX_ID replyMbox)
{
    MBOX_ID fromId;            /* Mbox qui a origine le message */
    int nbytes;                /* Nombre de bytes dans mbox ou dans message */
    LETTER_HDR hdr;            /* En-tete de la lettre recue */
    LETTER_ID letter;          /* Lettre recue */
    SEND *pSend;               /* Pointeur vers structure du send */
    int sendId;                /* Identificateur de send d'une replique */
    int indice = MY_TASK_DEV_IDX;
    
    /* Boucler tant qu'il y aura des repliques */
    while (mboxIoctl (replyMbox, FIO_NBYTES, (char *) &nbytes) == OK &&
	   nbytes != 0) {
        LOGDBG(("comLib:gcomDispatch: %d bytes in mbox %d\n",
		nbytes, replyMbox));

	/* Epier le contenu du mailbox */
	if (mboxSpy (replyMbox, &fromId, &nbytes, (char *) &hdr, 
		     sizeof (LETTER_HDR)) == sizeof (LETTER_HDR) &&
	    (sendId = hdr.sendId) >= 0 && sendId < MAX_SEND) {
	    /* Calculer le pointeur vers la structure donnees send */
	    pSend = &sendTab [indice][sendId];
	    
	    /* Verifier le type de replique */
	    switch (hdr.type) {
	      /* Replique intermediaire */
	      case INTERMED_REPLY:
		
		/* Obtenir la lettre associee */
		letter = pSend->intermedReplyLetter;
		
		/* Si c'est le cas, recevoir cette replique */
		if (pSend->status == WAITING_INTERMED_REPLY &&
		    letter->flagInit == GCOM_FLAG_INIT &&
		    mboxRcv (replyMbox, &fromId, (char *) letter->pHdr,
			     letter->size, 1) > 0) {
		    /* Indiquer l'attente de la replique finale */
		    pSend->status = WAITING_FINAL_REPLY;
		    continue;
		}
		break;
		
		/* Reply final */
	      case FINAL_REPLY:
		
		/* Obtenir la lettre associee */
		letter = pSend->finalReplyLetter;
		
		/* Si c'est le cas, recevoir cette replique */
		if ((pSend->status == WAITING_INTERMED_REPLY ||
		     pSend->status == WAITING_FINAL_REPLY) &&
		    letter->flagInit == GCOM_FLAG_INIT &&
		    mboxRcv (replyMbox, &fromId, (char *) letter->pHdr, 
			     letter->size, 1) > 0) {
		    /* Indiquer que la replique a ete recue et continuer */
		    pSend->status = FINAL_REPLY_OK;
		    continue;
		}
	    } /* case */
	}
      
	/* Jeter la lettre et continuer */
	logMsg("Problem while receiving a reply\n");
	(void) mboxSkip (replyMbox);
    } /* while */
}

/******************************************************************************
*
*  gcomVerifTout  -  Verifier si timeout
*
*  Description:
*  Cette fonction verifie si la periode de timeout d'ack ou reply
*  s'est ecoulee. Si oui, indiquer timeout.
*
*  Retourne : etat de la replique
*/

static int 
gcomVerifTout (int sendId, int *pTimeout)
{
    SEND *pSend;                   /* Ptr vers l'id de send */
    int status;                    /* Status du send */
    int totalTimeout;              /* Periode totale de timeout */
    int toutStatus;                /* Status du send si timeout */
    
    /* Calculer le pointeur vers position dans le tableau de sends */
    pSend = &sendTab[MY_TASK_DEV_IDX][sendId];
    
    /* Verifier si la tache attend une replique */
    switch (status = pSend->status) {
      case FINAL_REPLY_OK:
	
	/* Liberer l'idSend */
	pSend->status = FREE;
	return (FINAL_REPLY_OK);
	
      case WAITING_INTERMED_REPLY:
	
	/* Obtenir temps total de timeout */
	totalTimeout = pSend->intermedReplyTout;
	
	/* Status si le timeout a lieu */
	toutStatus = INTERMED_REPLY_TIMEOUT;
	break;
	
      case WAITING_FINAL_REPLY:
	
	/* Obtenir la valeur de la periode totale de timeout */
	totalTimeout = pSend->finalReplyTout;
	
	/* Status si le timeout a lieu */
	toutStatus = FINAL_REPLY_TIMEOUT;
	break;
	
      default:
	/* Retourner l'etat du send */
	return (status);
    }
    
    /* Verifier si on attend indefiniement */
    if (totalTimeout == 0)
	*pTimeout = 0;
    
    /* Sinon, verifier si timeout */
    else if ((*pTimeout = totalTimeout - (int) (tickGet() - pSend->time)) <= 0)
	return (pSend->status = toutStatus);
    
    /* Retourner l'etat */
    return (status);
}

/******************************************************************************
*
*  gcomVerifStatus  -  Verifier si timeout SANS LIBERATION DU SENDID
*
*  Description:
*  Cette fonction verifie si la periode de timeout d'ack ou reply
*  s'est ecoulee. Si oui, indiquer timeout.
*
*  Retourne : etat de la replique
*/

static int 
gcomVerifStatus(int sendId)
{
    SEND *pSend;                   /* Ptr vers l'id de send */
    int status;                    /* Status du send */
    int totalTimeout;              /* Periode totale de timeout */
    int toutStatus;                /* Status du send si timeout */
    
    /* Calculer le pointeur vers position dans le tableau de sends */
    pSend = &sendTab[MY_TASK_DEV_IDX][sendId];
    
    /* Verifier si la tache attend une replique */
    switch (status = pSend->status) {
      case FINAL_REPLY_OK:
	return (FINAL_REPLY_OK);
	
      case WAITING_INTERMED_REPLY:
	
	/* Obtenir temps total de timeout */
	totalTimeout = pSend->intermedReplyTout;
	
	/* Status si le timeout a lieu */
	toutStatus = INTERMED_REPLY_TIMEOUT;
	break;
	
      case WAITING_FINAL_REPLY:
	
	/* Obtenir la valeur de la periode totale de timeout */
	totalTimeout = pSend->finalReplyTout;
	
	/* Status si le timeout a lieu */
	toutStatus = FINAL_REPLY_TIMEOUT;
	break;
	
      default:
	/* Retourner l'etat du send */
	return (status);
    } /* switch */
    
  /* verifier si timeout */
    if (totalTimeout != 0) {
       if ((totalTimeout - (int) (tickGet() - pSend->time)) <= 0)
	    return (pSend->status = toutStatus);
    }
    
    /* Retourner l'etat */
    return (status);
}








