/*
 * Copyright (c) 1990, 2003 CNRS/LAAS
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
/*   LABORATOIRE D'AUTOMATIQUE ET D'ANALYSE DE SYSTEMES  -  LAAS / CNRS      */
/*   PROJET HILARE II  -  PROCESSUS DE TEST DES MAILBOXES  (mboxEssay.c)     */
/*****************************************************************************/

/* VERSION ACTUELLE / HISTORIQUE DES MODIFICATIONS :
   version 1.1, aout89, Ferraz de Camargo, 1ere version;
*/

/* DESCRIPTION :
   Permet la realisation de tests/manipulations avec les mailboxes du sytemes.
   En particulier, il est utile pour la verification de l'etat des mailboxes du
   systeme et, eventuelement, pour leur deletion, tests, etc.
*/
#include "config.h"
__RCSID("$LAAS$");

#ifdef VXWORKS
#include <vxWorks.h>
#include <memLib.h>
#else
#include "portLib.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <errnoLib.h>
#include <string.h>

#ifdef VXWORKS
#include "h2sysLib.h"
#endif

#include "mboxLib.h"
#include "xes.h"


/******************************************************************************
*
*   mboxEssay   -  Routine d'essai des mailboxes
*
*   Description : 
*   Execute des fonctions d'essai des mailboxes
*
*   Retourne : OK ou ERROR
*/

STATUS 
mboxEssay (void)
     
{
  int nFunc;                        /* Fonction a executer */
  int size;                         /* Taille du mailbox a creer */
  int tout;                         /* Timeout d'attente d'evenement */
  int nbytes;                       /* Nombre de bytes recus */
  int nt;                           /* Nombre total de bytes d'un message */
  int bilan;                        /* Bilan d'execution d'une commande */
  char yesCar;                      /* Caractere yes/no */
  char name1[20], name2[20];        /* Nom des mailboxes sous test */
  char *bufMes;                     /* Ptr vers buffer de messages */
  MBOX_ID mboxId1, mboxId2;         /* Identificateurs de mailbox */

  char *menu = "\n\
FONCTIONS DISPONIBLES:\
\n\n\
1 - CREER UN MAILBOX;\
\n\
2 - ENVOYER UN MESSAGE A UN MAILBOX;\
\n\
3 - EPIER UN MAILBOX;\
\n\
4 - LIRE UN MESSAGE D'UN MAILBOX;\
\n\
5 - SAUTER UN MESSAGE;\
\n\
6 - DELETER UN MAILBOX;\
\n\
7 - ATTENDRE UN MESSAGE;\
\n\
8 - IMPRIMER L'ETAT DE TOUS LES MAILBOXES;\
\n\
9 - FIN;\
\n\n\
TAPEZ LE NUMERO DE LA FONCTION DESIREE : ";

  xes_init(NULL);

  /* Demander le nom a donner a ce processus */
  do printf ("\nDEFINISSEZ LE NOM DE CE PROCESSUS: ");
  while (h2scanf ("%s", name1) != 1);

  xes_set_title("mboxEssay - %s", name1);

  /* Initialiser les routines de mailbox et allouer de la memoire*/
  if (mboxInit (name1) != OK ||
      (bufMes = malloc (1000)) == NULL)
    {
      (void) printf ("Erreur d'initialisation! Bye!\n");
      return (ERROR);
    }

  /* Boucler infiniement */
  FOREVER
    {
      /* Initialiser le bilan */
      bilan = ERROR;
      errnoSet (0);

      /* Printer le menu des commandes disponibles / demander le choix */
      do printf (menu);
      while (h2scanf ("%d", &nFunc) != 1);

      /* Executer la commande demandee */
      switch (nFunc)
	{
	/* Creation d'un mailbox */  
	case 1:                    

	  /* Demander le nom du mailbox a creer */
	  do printf ("\nNOM DU MAILBOX A CREER: ");
	  while (h2scanf ("%s", name1) != 1);

	  /* Demander la taille du mailbox a creer */
	  do printf ("TAILLE DU MAILBOX: ");
	  while (h2scanf ("%d", &size) != 1);

	  /* Creer effectivement le mailbox */
	  bilan = mboxCreate (name1, size, &mboxId1);
	  break;

	/* Envoyer un message vers un mailbox */
	case 2:

	  /* Demander le nom du mailbox de destination */
	  do printf ("\nNOM DU MAILBOX DE DESTINATION: ");
	  while (h2scanf ("%s", name1) != 1);

	  /* Demander le nom du mailbox d'origine */
	  do printf ("NOM DU MAILBOX D'ORIGINE: ");
	  while (h2scanf ("%s", name2) != 1);
	  
	  /* Demander le message a envoyer */
	  do printf ("MESSAGE: ");
	  while (h2scanf ("%s", bufMes) != 1);

	  /* Chercher l'identificateur du mailbox de destination */
	  if (mboxFind (name1, &mboxId1) != OK)
	    {
	      (void) printf ("Erreur pendant find de %s\n", name1);
	      break;
	    }

	  /* Chercher l'identificateur du mailbox d'origine */
	  if (mboxFind (name2, &mboxId2) != OK)
	    {
	      (void) printf ("Erreur pendant find de %s\n", name2);
	      break;
	    }

	  /* Envoyer le message vers le destinataire */
	  if ((bilan = mboxSend (mboxId1, mboxId2, bufMes, strlen (bufMes)))
	      != OK)
	    (void) printf ("Erreur pendant l'envoi du message!\n");
	  break;

	/* Epier un mailbox */
	case 3:

	  /* Demander le nom du mailbox a epier */
	  do printf ("\nNOM DU MAILBOX A EPIER: ");
	  while (h2scanf ("%s", name1) != 1);

	  /* Demander le nombre de bytes a echantillonner */
	  do printf ("NOMBRE DE BYTES A ECHANTILLONNER: ");
	  while (h2scanf ("%d", &nbytes) != 1);

	  /* Chercher l'identificateur du mailbox a epier */
	  if (mboxFind (name1, &mboxId1) != OK)
	    {
	      (void) printf ("Erreur pendant find de %s\n", name1);
	      break;
	    }

	  /* Epier le message */
	  if ((nbytes = mboxSpy (mboxId1, &mboxId2, &nt, bufMes, nbytes))
	      == ERROR)
	    {
	      (void) printf ("Erreur pendant espionnage de %s\n", name1);
	      break;
	    }
	  
	  /* S'il n'y a pas de message */
	  if (nbytes == 0)
	    {
	      (void) printf ("IL N'Y A PAS DE MESSAGE SUR LE MAILBOX!\n");
	      continue;
	    }

	  /* Mettre le caractere de fin de chaine */
	  bufMes[nbytes] = '\0';

	  /* Determiner le nom de l'originateur du message */
	  if (mboxIoctl (mboxId2, FIO_GETNAME, name2) != OK)
	    {
	      (void) printf ("Erreur durant identif. nom de l'originateur!\n");
	      break;
	    }

	  /* Imprimer le message recu et le nom de l'originateur */
	  (void) printf ("NOMBRE TOTAL DE BYTES DU MESSAGE: %d\n", nt);
	  (void) printf ("ORIGINATEUR DU MESSAGE: %s\n", name2);
	  (void) printf ("NOMBRE DE BYTES DE L'ECHANTILLON: %d\n", nbytes);
	  (void) printf ("ECHANTILLON DU MESSAGE: %s\n", bufMes);
	  continue;

	/* Lire un mailbox */
	case 4:

	  /* Demander le nom du mailbox a lire */
	  do printf ("\nNOM DU MAILBOX A LIRE: ");
	  while (h2scanf ("%s", name1) != 1);

	  /* Demander le timeout d'attente de message */
          do printf ("TIMEOUT: ");
	  while (h2scanf ("%d", &tout) != 1);

	  /* Chercher l'identificateur du mailbox a lire */
	  if (mboxFind (name1, &mboxId1) != OK)
	    {
	      (void) printf ("Erreur pendant find de %s\n", name1);
	      break;
	    }

	  /* Lire le message */
	  if ((nbytes = mboxRcv (mboxId1, &mboxId2, bufMes, 1000, 
				 tout == WAIT_FOREVER ? tout : 
				 tout * NTICKS_PER_SEC))
	      == ERROR)
	    {
	      (void) printf ("Erreur pendant lecture de %s\n", name1);
	      break;
	    }
	  
	  /* Verifier si timeout */
	  if (nbytes == 0)
	    {
	      (void) printf ("TIMEOUT!\7\n");
	      continue;
	    }

	  /* Mettre le caractere de fin de chaine */
	  bufMes[nbytes] = '\0';

	  /* Determiner le nom de l'originateur du message */
	  if (mboxIoctl (mboxId2, FIO_GETNAME, name2) != OK)
	    {
	      (void) printf ("Erreur durant identif. nom de l'originateur!\n");
	      break;
	    }

	  /* Imprimer le message recu et le nom de l'originateur */
	  (void) printf ("ORIGINATEUR DU MESSAGE: %s\n", name2);
	  (void) printf ("MESSAGE: %s\n", bufMes);
	  continue;

	/* Sauter un message */
	case 5:

	  /* Demander le nom du mailbox */
	  do printf ("\nNOM DU MAILBOX A MANIPULER: ");
	  while (h2scanf ("%s", name1) != 1);
	  
	  /* Chercher l'identificateur du mailbox par son nom */
	  if (mboxFind (name1, &mboxId1) != OK)
	    {
	      (void) printf ("Erreur pendant find de %s\n", name1);
	      break;
	    }

	  /* Demander de sauter un message */
	  bilan = mboxSkip (mboxId1);
	  break;

	/* Deletion d'un mailbox */
	case 6:

	  /* Demander le nom du mailbox a deleter */
	  do printf ("\nNOM DU MAILBOX A DELETER: ");
	  while (h2scanf ("%s", name1) != OK);

	  /* Chercher l'identificateur du mailbox par son nom */
	  if (mboxFind (name1, &mboxId1) != OK)
	    {
	      (void) printf ("Erreur pendant find de %s\n", name1);
	      break;
	    }

	  /* Deleter le mailbox */
	  bilan = mboxDelete (mboxId1);
	  break;

	/* Se suspendre, en attente de message sur un des mailboxes */
	case 7:

	  /* Demander le temps max. d'attente */
	  do printf ("\nTEMPS D'ATTENTE: ");
	  while (h2scanf ("%d", &tout) != 1);

	  /* Demander sur type d'attente */
	  do printf ("ATTENDRE SUR TOUS LES MBOX? (y/n) : ");
	  while (h2scanf ("%c", &yesCar) != 1 || 
		 (yesCar != 'y' && yesCar != 'n'));
	  
	  /* Verifier si on attend un message sur tous les mboxes */
	  if (yesCar == 'y')
	    {
	      /* Se suspendre sur tous les mboxes */
		if (tout != WAIT_FOREVER) 
		    tout = tout*NTICKS_PER_SEC;
	    
	      if ((bilan = mboxPause (ALL_MBOX, tout)) == FALSE)
		{
		  (void) printf ("TIMEOUT!\7\n");
		  continue;
		}
	      break;
	    }

	  /* Demander le nom du mailbox */
	  do printf ("NOM DU MAILBOX OU` ATTENDRE: ");
	  while (h2scanf ("%s", name1) != 1);

	  /* Chercher l'identificateur du mailbox par son nom */
	  if (mboxFind (name1, &mboxId1) != OK)
	    {
	      (void) printf ("Erreur pendant find de %s\n", name1);
	      break;
	    }
	  
	  /* Suspendre sur mailbox */
	  if ((bilan = mboxPause (mboxId1, tout * NTICKS_PER_SEC)) == FALSE)
	    {
	      (void) printf ("TIMEOUT!\7\n");
	      continue;
	    } 
	  break;

        /* Imprimer l'etat de tous les mailboxes du systeme */
	case 8:
	  mboxShow ();
	  continue;

	/* Fin du processus de test */
	case 9:
	  bilan = mboxEnd (0);
	  free (bufMes);
	  (void) printf ("BILAN = %d, ERRNO = %d\n", bilan, errnoGet ());
	  (void) printf ("CIAO!\n");
	  return (OK);

	default:
	  (void) printf ("FONCTION INCONNUE!!!\7\n");
	}
      
      /* Printer le bilan de la commande */
      (void) printf ("BILAN = %d, ERRNO = %d\n", bilan, errnoGet ());
    }
}
