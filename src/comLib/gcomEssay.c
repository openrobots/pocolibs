/*
 * Copyright (c) 2003 CNRS/LAAS
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

/* DESCRIPTION :
   Permet le test des routines du gestionnnaire de communications.
   En particulier, il est utile pour la verification de l'etat des mailboxes du
   systeme et, eventuelement, pour leur deletion, tests, etc.
   Attention: programme tres BIDON, codifie rapidement pour faire des tests.
   Ne pas le prendre comme exemple!
*/

#include "pocolibs-config.h"
__RCSID("$LAAS$");

#ifdef VXWORKS
#include <vxWorks.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errnoLib.h>

#include "portLib.h"
#include "gcomLib.h"

#ifndef VXWORKS
#define h2scanf scanf
#endif

/* Taille max des lettres de test */
#define  MAX_LETTER_SIZE           20000

/* Caractere des lettres d'envoi */
#define  SEND_CHAR                '1'

/* Caracteres des lettres de replique */
#define  INTERMED_REPLY_CHAR      '2'
#define  FINAL_REPLY_CHAR         '3'

/* Type des donnees du send */
#define  SEND_DATA_TYPE            4

/* Type des donnees des repliques */
#define  INTERMED_REPLY_DATA_TYPE  5
#define  FINAL_REPLY_DATA_TYPE     6

static int envLettre(void);
static int repLettre(void);
static int waitReply(void);
static int statusReply(void);
static int pauseMbox(void);
static void bilanPrint(int bilan);
static void gcomObjFree(char *bufMes, LETTER_ID letter1, LETTER_ID letter2, LETTER_ID letter3);
static STATUS verifMes(char *bufMes, int nbytes, int tstChar);

/******************************************************************************
*
*   gcomEssay   -  Routine d'essai de gcom
*
*   Description : 
*   Execute des fonctions d'essai des routines de gcom.
*
*   Retourne : OK ou ERROR
*/

STATUS
gcomEssay(void)
{
    int nFunc;
    int rcvSize, replySize;
    int bilan;
    char procName[20];
    char *menu = "\n\
FONCTIONS DISPONIBLES:\
\n\n\
1 - ENVOYER UNE LETTRE;\
\n\
2 - RECEVOIR ET REPLIQUER UNE LETTRE;\
\n\
3 - ATTENDRE UNE REPLIQUE (INTERMEDIAIRE OU FINALE);\
\n\
4 - VERIFIER L'ETAT D'UNE (DES) REPLIQUE(S);\
\n\
5 - IMPRIMER L'ETAT DE TOUS LES MAILBOXES;\
\n\
6 - ATTENDRE L'ARRIVEE DE MESSAGE SUR LES MAILBOXES;\
\n\
7 - FIN;\
\n\n\
TAPEZ LE NUMERO DE LA FONCTION DESIREE : ";

      /* Demander le nom a donner a ce processus */
  do printf("\nDEFINISSEZ LE NOM DE CE PROCESSUS: ");
  while (h2scanf("%s", procName) != 1);
     
  /* Demander la taille du mbox de reception */
  do printf("FOURNISSEZ LA TAILLE DU MBOX DE RECEPTION: ");
  while (h2scanf("%d", &rcvSize) != 1 || rcvSize < 0);

  /* Demander la taille du mailbox de repliques */
  do printf("FOURNISSEZ LA TAILLE DU MBOX DE REPLIQUES: ");
  while (h2scanf("%d", &replySize) != 1 || replySize < 0);
  
  /* Initialiser les mailboxes du processus */
  if (gcomInit(procName, rcvSize, replySize) != OK)
    {
      printf("Erreur pendant initialisation. Bye!\n");
      return(ERROR);
    }

  /* Boucler indefiniement */
  FOREVER
    {
      /* Initialiser le registre d'erreur */
      errnoSet(0);

      /* Printer le menu des commandes disponibles / demander le choix */
      do printf(menu);
      while (h2scanf("%d", &nFunc) != 1);

      /* Sauter une ligne */
      printf("\n");

      /* Executer la commande demandee */
      switch (nFunc)
	{
	  /* Envoyer une lettre */
	case 1:                    
	  bilan = envLettre();
	  break;

	  /* Recevoir et repliquer a une lettre */
	case 2:
	  bilan = repLettre();
	  break;

	  /* Attendre une replique */
	case 3:
	  bilan = waitReply();
	  break;

	case 4:
	  bilan = statusReply();
	  break;

	case 5:
	  bilan = OK;
	  gcomMboxShow();
	  break;

	case 6:
	  bilan = pauseMbox();
	  break;

	case 7:
	  bilan = gcomEnd();
	  printf("BYE!\n\n");
	  return(bilan);

	  /* Fonction inconnue */
	default:
	  bilan = ERROR;
	  printf("FONCTION INCONNUE!!!\7\n");
	}
      
      /* Printer le bilan de la commande */
      printf("BILAN = %d, ERRNO = %d\n", bilan, errnoGet());
    }
}


/*****************************************************************************
*
*   envLettre - Envoi d'une lettre
*
*   Description:
*   Teste l'envoi d'une lettre. Les tests concernes sont: ecriture d'une
*   lettre, envoi d'une lettre et lecture de replique (si bloq). 
*
*   Retourne: bilan de l'envoi
*/

static int 
envLettre(void)
{
    int size;                          /* Taille d'une lettre a envoyer */
    char destName [20];                /* Nom du processus destinataire */
    char yesCar1, yesCar2, yesCar3;    /* Caracteres de confirmation */
    int sendType;                      /* Type du send */
    int intermedTout, finalTout;       /* Timeouts */
    MBOX_ID mboxId;                    /* Id. du mailbox de destination */
    LETTER_ID letter1;                 /* Lettre d'envoi */
    LETTER_ID letter2;                 /* Lettre de replique intermediaire */
    LETTER_ID letter3;                 /* Lettre de replique finale */
    char *bufMes;                      /* Buffer de messages */
    int bilan;                         /* Bilan de l'envoi */
    int sendId;                        /* Identif. d'envoi */
    int dataType;                      /* Type des donnees recues */
    
    /* Demander la taille de la lettre */
    do printf("TAILLE DE LA LETTRE A ENVOYER :");
    while (h2scanf("%d", &size) != 1 || size < 0 || size > MAX_LETTER_SIZE);
    
    /* Demander le nom du processus de destination */
    do printf("NOM DU PROCESSUS DE DESTINATION: ");
    while (h2scanf("%s", destName) != 1);
    
    /* Demander si une replique intermediaire est desiree */
    do printf("ATTEND REPLIQUE INTERMEDIAIRE ? (y/n) : ");
    while (h2scanf("%c", &yesCar1) != 1 || (yesCar1 != 'y' && yesCar1 != 'n'));
    
    /* Bloquer en attente de la replique intermediaire */
    if (yesCar1 == 'y')
    {
	do printf("BLOQUER JUSQU'A L'ARRIVEE DE LA REPLIQUE INTERMEDIAIRE ? (y/n) : ");
	while (h2scanf("%c", &yesCar2) != 1 ||
	       (yesCar2 != 'y' && yesCar2 != 'n'));
    }
    else yesCar2 = 'n';
    
    /* Verifier s'il faut bloquer en attente de la replique finale */
    printf("BLOQUER JUSQU'A L'ARRIVE DE LA REPLIQUE FINALE ? (y/n) : ");
    while (h2scanf("%c", &yesCar3) != 1 || (yesCar3 != 'y' && yesCar3 != 'n'));
    
    /* Obtenir le type d'envoi */
    if (yesCar3 == 'y')
	sendType = BLOCK_ON_FINAL_REPLY;
    else if (yesCar2 == 'y')
	sendType = BLOCK_ON_INTERMED_REPLY;
    else 
	sendType = NO_BLOCK;
    
    /* Demander le timeout pour l'attente de la replique intermediaire */
    if (yesCar1 == 'y') {
	do printf("TIMEOUT POUR L'ATTENTE DE LA REPLIQUE INTERMEDIAIRE :");
	while (h2scanf("%d", &intermedTout) != 1 || intermedTout < 0);
    } else {
	intermedTout = 0;
    }
    
    /* Demander le timeout pour la replique finale */
    do printf("TIMEOUT POUR L'ATTENTE DE LA REPLIQUE FINALE :");
    while (h2scanf("%d", &finalTout) != 1 || finalTout < 0);
    
    /* Chercher l'identificateur du mailbox de destination */
    if (gcomMboxFind(destName, &mboxId) != OK) {
	printf("Erreur pendant find de %s\n", destName);
	return(ERROR);
    }
    
    /* Allouer de la memoire */
    if ((bufMes = malloc(MAX_LETTER_SIZE)) == NULL) {
	printf("Erreur pendant malloc!\n");
	return(ERROR);
    }
    
    /* Prendre la lettre d'envoi */
    if (gcomLetterAlloc(size, &letter1) != OK) {
	printf("Erreur pendant l'allocation lettre envoi!\n");
	gcomObjFree(bufMes, (LETTER_ID) 0, (LETTER_ID) 0, (LETTER_ID) 0);
	return(ERROR); 
    }
    
    /* Prendre la lettre de replique intermediaire */
    if (yesCar1 == 'y') {
	if (gcomLetterAlloc(size, &letter2) != OK) {
	    printf("Erreur d'alloc. lettre replique intermediaire!\n");
	    gcomObjFree(bufMes, letter1, (LETTER_ID) 0, (LETTER_ID) 0); 
	    return(ERROR);
	}
    } else {
	letter2 = (LETTER_ID) NULL;
    }

    /* Allouer une lettre de replique finale */
    if (gcomLetterAlloc(size, &letter3) != OK) {
	printf("Erreur pendant l'allocation lettre replique finale!\n");
	gcomObjFree(bufMes, letter1, letter2, (LETTER_ID) 0);
	return(ERROR);
    }
    
    /* Remplir le buffer de messages avec les caracteres d'envoi */
    memset(bufMes, SEND_CHAR, MAX_LETTER_SIZE);
    
    /* Remplir la lettre d'envoi */
    if (gcomLetterWrite(letter1, SEND_DATA_TYPE, bufMes, size, 0) != OK) {
	printf("Erreur pendant l'ecriture de la lettre!\n");
	gcomObjFree(bufMes, letter1, letter2, letter3);
	return(ERROR);
    }
	  
    /* Envoyer le message vers le destinataire */
    bilan = gcomLetterSend(mboxId, letter1, letter2, letter3, sendType, 
			    &sendId, intermedTout * NTICKS_PER_SEC, 
			    finalTout * NTICKS_PER_SEC);
    
    /* Verifier si erreur pendant l'envoi */
    if (bilan < 0) {
	printf("Erreur pendant l'envoi!\n");
	gcomObjFree(bufMes, letter1, letter2, letter3);
	return(ERROR);
    }

    /* Si pas bloquant, retourner tout de suite */
    if (sendType == NO_BLOCK) {
	printf("L'IDENTIFICATEUR DE L'ENVOI EST : %d\n", sendId);
	gcomObjFree(bufMes, letter1, (LETTER_ID) 0, (LETTER_ID) 0);
	return(OK);
    }
    
    /* Sinon, printer le bilan de l'envoi */
    bilanPrint(bilan);
    
    /* Retourner ERROR, si libre ou timeout */
    if (bilan == INTERMED_REPLY_TIMEOUT || bilan == FINAL_REPLY_TIMEOUT ||
	bilan == FREE) {
	printf("\7");
	gcomObjFree(bufMes, letter1, letter2, letter3);
	return(ERROR);
    }
    
    /* Verifier si la replique intermediaire a ete recue */
    if (sendType == BLOCK_ON_INTERMED_REPLY && bilan == WAITING_FINAL_REPLY) {
	/* Lire la lettre de replique intermediaire recue */
	dataType = gcomLetterType(letter2);
	
	/* Lire la lettre recue */
	if (gcomLetterRead(letter2, bufMes, MAX_LETTER_SIZE, 0)
	    != size || verifMes(bufMes, size, INTERMED_REPLY_CHAR) != OK ||
	    dataType != INTERMED_REPLY_DATA_TYPE) {
	    printf("Mauvais contenu replique intermediaire!\n");
	    gcomObjFree(bufMes, letter1, letter2, letter3);
	    return(ERROR);
	}

	/* Indiquer que la lettre de replique a ete bien recue */
	printf("CONTENU DE LA REPLIQUE INTERMEDIAIRE OK!\n");
	
	/* Signaler le numero de l'identicateur d'envoi */
	printf("L'IDENTIFICATEUR DE L'ENVOI EST : %d\n", sendId);
	gcomObjFree(bufMes, letter1, letter2, (LETTER_ID) 0);
	return(OK);
    }

    /* Verifier si le reply final a ete recu */
    if (bilan == FINAL_REPLY_OK) {
	/* Obtenir le type des donnees */
	dataType = gcomLetterType(letter3);
	
	/* Lire la lettre recue */
	if (gcomLetterRead(letter3, bufMes, MAX_LETTER_SIZE, 0)
	    != size || verifMes(bufMes, size, FINAL_REPLY_CHAR) != OK ||
	    dataType != FINAL_REPLY_DATA_TYPE) {
	    printf("Mauvais contenu replique finale!\n");
	    gcomObjFree(bufMes, letter1, letter2, letter3);
	    return(ERROR);
	}
	
	/* Signaler que la replique finale a ete bien recue */
	printf("CONTENU DE LA REPLIQUE FINALE OK!\n");
    }
    
    /* Liberer les objets alloues */
    gcomObjFree(bufMes, letter1, letter2, letter3);
    return(OK);
}


/******************************************************************************
*
*   repLettre  -  Recevoir et repliquer une lettre
*
*   Description:
*   Par moyen de cette fonction, une lettre est recue et une replique
*   envoyee vers l'originateur de la lettre.
*
*   Retourne: bilan d'execution;
*/

static int 
repLettre(void)
{
    int timeout;              /* Timeout d'attente */
    int dataType;             /* Type donnees recues */
    char *bufMes;             /* Buffer de messages */
    MBOX_ID origMbox;         /* Mbox processus originateur message */
    int sendId;               /* Identificateur du send */
    LETTER_ID letter1;        /* Lettre de reception */
    LETTER_ID letter2;        /* Lettre de replique */
    char origName [20];       /* Nom du processus originateur du message */
    int nbytes;               /* Taille du message recu */
    int bilan;                /* Bilan d'une action */
    char yesCar;              /* Caractere de confirmation */
    
    /* Demander le timeout d'attente de message */
    do printf("TIMEOUT D'ATTENTE : ");
    while (h2scanf("%d", &timeout) != 1 || timeout < 0);
    
    /* Allouer le buffer de messages */
    if ((bufMes = malloc(MAX_LETTER_SIZE)) == NULL) {
	printf("Erreur de malloc!\n");
	return(ERROR);
    }
    
    /* Allouer la lettre de reception */
    if (gcomLetterAlloc(MAX_LETTER_SIZE, &letter1) != OK) {
	printf("Erreur pendant allocation lettre de reception\n");
	gcomObjFree(bufMes, (LETTER_ID) 0, (LETTER_ID) 0, (LETTER_ID) 0);
	return(ERROR);
    }
    
    /* Attendre l'arrivee de la lettre */
    bilan = gcomLetterRcv(letter1, &origMbox, &sendId, 
			   timeout * NTICKS_PER_SEC);
    
    /* Verifier si lettre recue */
    if (bilan != TRUE) {
	/* Verifier si timeout */
	if (bilan == FALSE)
	    printf("TIMEOUT\n\7");
	else
	    printf("Erreur de reception!");
	
	/* Liberer les objets alloues */
	gcomObjFree(bufMes, letter1, (LETTER_ID) 0, (LETTER_ID) 0);
	return(ERROR);
    }

    /* Obtenir le type des donnees */
    dataType = gcomLetterType(letter1);
    
    /* Lire la lettre recue */
    if ((nbytes = gcomLetterRead(letter1, bufMes, MAX_LETTER_SIZE, 0)) < 0) {
	printf("Erreur pendant lecture lettre recue\n");
	gcomObjFree(bufMes, letter1, (LETTER_ID) 0, (LETTER_ID) 0);
	return(ERROR);
    }
    
    /* Imprimer le nombre de bytes recus */
    printf("UNE LETTRE DE %d BYTES A ETE BIEN RECUE\n", nbytes);
    
    /* Verifier le contenu de la lettre recue */
    if (verifMes(bufMes, nbytes, SEND_CHAR) != OK ||
	dataType != SEND_DATA_TYPE) {
	printf("Erreur contenu lettre recue\n");
	gcomObjFree(bufMes, letter1, (LETTER_ID) 0, (LETTER_ID) 0);
	return(ERROR);
    } else {
	printf("LE CONTENU DE LA LETTRE EST OK!\n");
    }
    
    /* Obtenir le nom de l'originateur du message */
    if (gcomMboxName(origMbox, origName) != OK) {
	printf("Erreur obtention du nom de l'originateur lettre\n");
	gcomObjFree(bufMes, letter1, (LETTER_ID) 0, (LETTER_ID) 0);
	return(ERROR);
    }
    
    /* Imprimer le nom du processus originateur du message */
    printf("ORIGINATEUR DU MESSAGE: %s\n", origName);
    
    /* Allouer une lettre pour le reply */
    if (gcomLetterAlloc(nbytes, &letter2) != OK) {
	printf("Erreur allocation lettre de replique!\n");
	gcomObjFree(bufMes, letter1, (LETTER_ID) 0, (LETTER_ID) 0);
	return(ERROR);
    }
    
    /* Demander s'il faut envoyer une replique intermediaire */
    do printf("ENVOYER UNE REPLIQUE INTERMEDIAIRE ? (y/n) : ");
    while (h2scanf("%c", &yesCar) != 1 || (yesCar != 'y' && yesCar != 'n'));
    
    /* Verifier s'il faut former une lettre replique intermediaire */
    if (yesCar == 'y') {
	/* Former le message a envoyer */
	memset(bufMes, INTERMED_REPLY_CHAR, nbytes);
	
	/* Ecrire la lettre de replique */
	if (gcomLetterWrite(letter2, INTERMED_REPLY_DATA_TYPE, 
			     bufMes, nbytes, 0) != OK) {
	    printf("Erreur ecriture lettre de replique intermed.!\n");
	    gcomObjFree(bufMes, letter1, letter2,(LETTER_ID) 0);
	    return(ERROR);
	}
	
      /* Envoyer la replique intermediaire */
	if (gcomLetterReply (origMbox, sendId, INTERMED_REPLY, letter2) 
	    != OK) {
	    printf("Erreur pendant l'envoi de la replique intermed.\n");
	    gcomObjFree(bufMes, letter1, letter2, (LETTER_ID) 0);
	    return(ERROR);
	}
	
	/* Indiquer que la replique intermediaire a ete bien envoyee */
	printf("REPLIQUE INTERMEDIAIRE BIEN ENVOYEE!\n");
    }
    
    /* Former la replique finale a envoyer */
    memset(bufMes, FINAL_REPLY_CHAR, nbytes);
    
    /* Ecrire la lettre de replique finale */
    if (gcomLetterWrite(letter2, FINAL_REPLY_DATA_TYPE, 
			 bufMes, nbytes, 0) != OK) {
	printf("Erreur ecriture lettre de replique finale!\n");
	gcomObjFree(bufMes, letter1, letter2, (LETTER_ID) 0);
	return(ERROR);
    }
    
    /* Envoyer la replique finale */
    if (gcomLetterReply(origMbox, sendId, FINAL_REPLY, letter2) 
	!= OK) {
	printf("Erreur pendant l'envoi de la replique finale\n");
	gcomObjFree(bufMes, letter1, letter2, (LETTER_ID) 0);
	return(ERROR);
    }
    
    /* Indiquer que la replique finale a ete bien envoyee */
    printf("REPLIQUE FINALE BIEN ENVOYEE!\n");

    /* Liberer les objets alloues et retourner */
    gcomObjFree(bufMes, letter1, letter2, (LETTER_ID) 0);
    return(OK);
}


/******************************************************************************
*
*   waitReply  -  Attendre une replique
*
*   Description:
*   Cette fonction permet d'attendre une replique, etant donne
*   son identificateur d'envoi.
*
*   Retourne: bilan de la commande
*/

static int 
waitReply(void)

{
    int sendId;                  /* Identificateur d'envoi */
    int bilan;                   /* Bilan de la commande */
    int dataType;                /* Type de donnee replique */
    LETTER_ID letter1, letter2;  /* Lettre de replique */
    char *bufMes;                /* Buffer de messages */
    int nbytes;                  /* Nombre de bytes de la replique */
    int waitType;                /* Type d'attente desiree */
    char yesCar;                 /* Caractere de confirmation */
    
    /* Demander l'identificateur de send */
    do printf("IDENTIFICATEUR D'ENVOI : ");
    while (h2scanf("%d", &sendId) != 1 || sendId < 0 || sendId >= MAX_SEND);
    
    /* Demander si une replique intermediaire est desiree */
    printf("ATTENDRE SEULEMENT LA REPLIQUE INTERMEDIAIRE ? (y/n) : ");
    while (h2scanf("%c", &yesCar) != 1 || (yesCar != 'y' && yesCar != 'n'));
    
    /* Determiner le type d'attente */
    if (yesCar == 'y')
	waitType = INTERMED_REPLY;
    else
	waitType = FINAL_REPLY;
    
    /* Attendre la replique */
    if ((bilan = gcomReplyWait(sendId, waitType)) < 0) {
	printf("Erreur pendant reception replique!\n");
	return(ERROR);
    }
    
    /* Imprimer le bilan de la replique */
    bilanPrint(bilan);
    
    /* Retourner ERROR, si libre ou timeout */
    if (bilan == INTERMED_REPLY_TIMEOUT || bilan == FINAL_REPLY_TIMEOUT ||
	bilan == FREE) {
	printf("\7");
	return(ERROR);
    }
    
    /* Obtenir l'adresse des lettres de replique */
    gcomReplyLetterBySendId(sendId, &letter1, &letter2);
    
    /* Allouer de la memoire */
    if ((bufMes = malloc(MAX_LETTER_SIZE)) == NULL) {
	printf("Erreur pendant malloc!\n");
	return(ERROR);
    }
    
    /* Verifier le contenu de la replique intermediaire */
    if (bilan == WAITING_FINAL_REPLY && letter1 != (LETTER_ID) 0) {
	/* Obtenir le type des donnees */
	dataType = gcomLetterType(letter1);
	
	/* Lire la lettre recue */
	nbytes = gcomLetterRead(letter1, bufMes, MAX_LETTER_SIZE, 0);
	
	/* Verifier si erreur pendant la lecture de la replique */
	if (nbytes < 0) {
	    printf("Erreur pendant lecture de la replique intermed.\n");
	    gcomObjFree(bufMes, letter1, letter2, (LETTER_ID) 0);
	    return(ERROR);
	}
	
	/* Imprimer le nombre de bytes recus */
	printf ("UNE REPLIQUE INTERMEDIAIRE DE %d BYTES A ETE RECUE.\n",
		       nbytes);
	
	/* Verifier le contenu de la replique */
	if (verifMes(bufMes, nbytes, INTERMED_REPLY_CHAR) != OK ||
	    dataType != INTERMED_REPLY_DATA_TYPE) {
	    printf("Mauvais contenu de la replique intermediaire!\n");
	    gcomObjFree(bufMes, letter1, letter2, (LETTER_ID) 0);
	    return(ERROR);
	}
	
	/* Signaler que la replique a ete bien recue */
	printf("CONTENU DE LA REPLIQUE INTERMEDIAIRE OK!\n");
    }

    /* Verifier le contenu de la replique finale */
    if (bilan == FINAL_REPLY_OK) {
	/* Obtenir le type des donnees */
	dataType = gcomLetterType(letter2);
	
	/* Lire la lettre recue */
	nbytes = gcomLetterRead(letter2, bufMes, MAX_LETTER_SIZE, 0);
	
	/* Verifier si erreur pendant la lecture de la replique */
	if (nbytes < 0) {
	    printf("Erreur pendant lecture de la replique finale\n");
	    gcomObjFree(bufMes, letter1, letter2, (LETTER_ID) 0);
	    return(ERROR);
	}
	
	/* Imprimer le nombre de bytes recus */
	printf("UNE REPLIQUE FINALE DE %d BYTES A ETE RECUE.\n",
		       nbytes);
	
	/* Verifier le contenu de la replique */
	if (verifMes(bufMes, nbytes, FINAL_REPLY_CHAR) != OK ||
	    dataType != FINAL_REPLY_DATA_TYPE) {
	    printf("Mauvais contenu de la replique finale!\n");
	    gcomObjFree(bufMes, letter1, letter2, (LETTER_ID) 0);
	    return(ERROR);
	}
	
	/* Indiquer que la replique finale est OK */
	printf("CONTENU DE LA REPLIQUE FINALE OK!\n");
    }
    return(OK);
}
  

/******************************************************************************
*
*   statusReply  -  Etat actuel de la replique
*
*   Description:
*   Par moyen de cette fonction, on obtient l'etat de la replique.
*
*   Retourne: bilan de la commande
*/

static int 
statusReply(void)
{
    int sendId;                  /* Identificateur d'envoi */
    int bilan;                   /* Bilan de la commande */
    int dataType;                /* Type de donnee replique */
    LETTER_ID letter1, letter2;  /* Lettres de replique */
    char *bufMes;                /* Buffer de messages */
    int nbytes;                  /* Nombre de bytes de la replique */
    
    /* Demander l'identificateur de send */
    do printf("IDENTIFICATEUR D'ENVOI : ");
    while (h2scanf("%d", &sendId) != 1 || sendId < 0 || sendId >= MAX_SEND);
    
    /* Attendre la replique */
    if ((bilan = gcomReplyStatus(sendId)) < 0) {
	printf("Erreur pendant obtention status replique!\n");
	return(ERROR);
    }
    
    /* Imprimer le bilan de la replique */
    bilanPrint(bilan);
    
    /* Retourner ERROR, si libre ou timeout */
    if (bilan == INTERMED_REPLY_TIMEOUT || bilan == FINAL_REPLY_TIMEOUT ||
	bilan == FREE) {
	printf("\7");
	return(ERROR);
    }
    
    /* Obtenir l'adresse des lettres de replique */
    gcomReplyLetterBySendId(sendId, &letter1, &letter2);
    
    /* Allouer de la memoire */
    if ((bufMes = malloc(MAX_LETTER_SIZE)) == NULL) {
	printf("Erreur pendant malloc!\n");
	return(ERROR);
    }
    
    /* Verifier le contenu de la replique intermediaire */
    if (bilan == WAITING_FINAL_REPLY && letter1 != (LETTER_ID) 0) {
      /* Obtenir le type des donnees */
	dataType = gcomLetterType(letter1);
      
	/* Lire la lettre recue */
	nbytes = gcomLetterRead(letter1, bufMes, MAX_LETTER_SIZE, 0);
	
	/* Verifier si erreur pendant la lecture de la replique */
	if (nbytes < 0) {
	    printf("Erreur pendant lecture de la replique intermed.\n");
	    gcomObjFree(bufMes, letter1, letter2, (LETTER_ID) 0);
	    return(ERROR);
	}
	
	/* Imprimer le nombre de bytes recus */
	printf("UNE REPLIQUE INTERMEDIAIRE DE %d BYTES A ETE RECUE.\n",
		       nbytes);
	
	/* Verifier le contenu de la replique */
	if (verifMes(bufMes, nbytes, INTERMED_REPLY_CHAR) != OK ||
	    dataType != INTERMED_REPLY_DATA_TYPE) {
	    printf("Mauvais contenu de la replique intermediaire!\n");
	    gcomObjFree(bufMes, letter1, letter2, (LETTER_ID) 0);
	    return(ERROR);
	}
	
	/* Signaler que la replique a ete bien recue */
	printf ("CONTENU DE LA REPLIQUE INTERMEDIAIRE OK!\n");
	gcomObjFree(bufMes, letter1, (LETTER_ID) 0, (LETTER_ID) 0);
	return(OK);
    }

    /* Verifier le contenu de la replique finale */
    if (bilan == FINAL_REPLY_OK) {
	/* Obtenir le type des donnees */
	dataType = gcomLetterType(letter2);
	
	/* Lire la lettre recue */
	nbytes = gcomLetterRead(letter2, bufMes, MAX_LETTER_SIZE, 0);
	
	/* Verifier si erreur pendant la lecture de la replique */
	if (nbytes < 0) {
	    printf("Erreur pendant lecture de la replique finale\n");
	    gcomObjFree(bufMes, letter1, letter2, (LETTER_ID) 0);
	    return(ERROR);
	}

	/* Imprimer le nombre de bytes recus */
	printf("UNE REPLIQUE FINALE DE %d BYTES A ETE RECUE.\n",
		     nbytes);
	
      /* Verifier le contenu de la replique */
	if (verifMes(bufMes, nbytes, FINAL_REPLY_CHAR) != OK ||
	    dataType != FINAL_REPLY_DATA_TYPE) {
	    printf("Mauvais contenu de la replique finale!\n");
	    gcomObjFree(bufMes, letter1, letter2, (LETTER_ID) 0);
	    return(ERROR);
	}

	/* Indiquer que la replique finale est OK */
	printf("CONTENU DE LA REPLIQUE FINALE OK!\n");
	gcomObjFree(bufMes, letter1, letter2, (LETTER_ID) 0);
    }
    return(OK);
}
  

/*****************************************************************************
*
*  pauseMbox  -  Attendre l'arrivee d'un message sur un mailbox
*
*  Description:
*  Attend l'arrivee d'un message sur un mailbox.
*
*  Retourne: bilan de l'action
*/

static int 
pauseMbox(void)
{
    int timeout;                      /* Timeout d'attente */
    char yesCar1, yesCar2;            /* Caracteres de confirmation */
    int mask;                         /* Mask d'attente */
    
    /* Demander le timeout d'attente */
    do printf("TIMEOUT D'ATTENTE : ");
    while (h2scanf("%d", &timeout) != 1 || timeout < 0);
    
    /* Demander si attente sur mbox de reception */
    do printf("ATTENDRE SUR MBOX DE RECEPTION ? (y/n) : ");
    while (h2scanf("%c", &yesCar1) != 1 || (yesCar1 != 'y' && yesCar1 != 'n'));
    
    /* Demander si attente sur mbox de replique */
    do printf("ATTENDRE SUR MBOX DE REPLIQUE ? (y/n) : ");
    while (h2scanf("%c", &yesCar2) != 1 || (yesCar2 != 'y' && yesCar2 != 'n'));
    
    /* Obtenir la mask d'attente */
    mask = ((yesCar1 == 'y') ? RCV_MBOX : 0) | 
 ((yesCar2 == 'y') ? REPLY_MBOX : 0);
    
    /* Attendre */
    mask = gcomMboxPause(timeout * NTICKS_PER_SEC, mask);
    
    /* Interpreter le resultat */
    switch (mask) {
      case 0:
	printf("TIMEOUT!\n\7");
	break;
	
      case RCV_MBOX:
	printf("LETTRE DANS LE MBOX DE RECEPTION!\n");
	break;
	
      case REPLY_MBOX:
	printf("LETTRE DANS LE MBOX DE REPLIQUE!\n");
	break;
	
      case RCV_MBOX | REPLY_MBOX:
	printf("LETTRES DANS LES MBOXES DE RECEPTION ET DE REPLIQUE!\n");
	break;
	
      default:
	printf("Probleme pendant attente!\n");
	break;
    }
    
    return(mask);
}


/*****************************************************************************
*
*   bilanPrint  -  Impression du bilan
*
*   Description:
*   Imprime le bilan d'une replique.
*
*   Retourne: Neant
*/

static void 
bilanPrint(int bilan)
{
    /* Printer le debut */
    printf("BILAN DE LA REPLIQUE : ");
    
    /* Verifier le type de bilan */
    switch (bilan) {
      case FREE:
	printf("ON N'ATTEND PAS DE REPLIQUE!\n");
	break;
      case WAITING_INTERMED_REPLY:
	printf("ATTENTE DE LA REPLIQUE INTERMEDIAIRE\n");
	break;
      case INTERMED_REPLY_TIMEOUT:
	printf("TIMEOUT D'ATTENTE DE LA REPLIQUE INTERMEDIAIRE\n");
	break;
      case WAITING_FINAL_REPLY:
	printf("ATTENTE DE LA REPLIQUE FINALE\n");
	break;
      case FINAL_REPLY_TIMEOUT:
	printf("TIMEOUT D'ATTENTE DE LA REPLIQUE FINALE\n");
	break;
      case FINAL_REPLY_OK:
	printf("LA REPLIQUE FINALE A ETE RECUE\n");
	break;
      default:
	printf("BILAN INCONNU!\n");
    }
}


/******************************************************************************
*
*   gcomObjFree  -  Liberer les objets alloues
*
*   Description:
*   Libere les objets alloues pendant une seance.
*
*   Retourne: Neant
*/

static void 
gcomObjFree(char *bufMes, LETTER_ID letter1, 
	    LETTER_ID letter2, LETTER_ID letter3)
{
    /* Liberer le buffer de messages */
    if (bufMes != NULL)
	free(bufMes);
    
    /* Liberer 1ere lettre */
    if (letter1 != NULL)
	gcomLetterDiscard(letter1);
    
    /* Liberer 2eme lettre */
    if (letter2 != NULL)
	gcomLetterDiscard(letter2);
    
    /* Liberer 3eme lettre */
    if (letter3 != NULL)
	gcomLetterDiscard(letter3);
}


/****************************************************************************
*
*  verifMes  -  Verification du contenu d'un message
*
*  Description:
*  Verifie le contenu d'un message.
*
*  Retourne : OK ou ERROR
*/

static STATUS 
verifMes(char *bufMes, int nbytes, int tstChar)
{
    int i;                    /* Compteur des caracteres */
    
    /* Verifier si tous les caracteres sont == tstChar */
    for (i = 0; i < nbytes && bufMes [i] == tstChar; i++) {
    }
    
    /* Retourner le bilan du test */
    if (i == nbytes)
	return(OK);
    return(ERROR);
}

/*----------------------------------------------------------------------*/

int 
main(int argc, char *argv[])
{
    if (gcomEssay() != OK) {
	return 3;
    } else {
	return 0;
    }
}
