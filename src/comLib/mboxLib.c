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

#include "config.h"
__RCSID("$LAAS$");

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "portLib.h"
#include "taskLib.h"
#include "errnoLib.h"
#include "h2devLib.h"
#include "h2semLib.h"
#include "h2evnLib.h"
#include "h2rngLib.h"
#include "mboxLib.h"
#include "smObjLib.h"
#include "xes.h"

/** 
 ** Variables globales
 **/

/*----------------------------------------------------------------------*/

/**
 ** Initialisation des mailboxes
 **
 **  Description :
 **  Au debut de chaque tache qui emploi les mailboxes, on doit appeler
 **  cette routine. 
 **/
STATUS 
mboxInit(char *procName)
{
    int tid = taskIdSelf();
    OS_TCB *tcb = taskTcb(tid);
    int dev;
    
    if (tcb == NULL) {
	return ERROR;
    }
    /* Recherche le device associe' au processus courant */
    dev = h2devFind(tcb->name, H2_DEV_TYPE_TASK);

    /* Si pas trouve', alloue un device */
    if (dev == ERROR) {
	dev = h2devAlloc(tcb->name, H2_DEV_TYPE_TASK);
	if (dev == ERROR) {
	    return(ERROR);
	}
	H2DEV_TASK_TID(dev) = tid;
	
	/* Cree le semaphore de synchro */
	H2DEV_TASK_SEM_ID(dev) = h2semAlloc(H2SEM_SYNC);	
	if (H2DEV_TASK_SEM_ID(dev) == ERROR) {
	    return ERROR;
	}
	/* Memorise le device h2 */
	taskSetUserData(0, dev);
	
    } else {
	/* verifie le process id et le taskId */
	if (H2DEV_TASK_TID(dev) != tid || taskGetUserData(0) != dev) {
	    errnoSet(S_mboxLib_NAME_IN_USE);
	    return(ERROR);
	}
    }
    return(OK);
}

/*----------------------------------------------------------------------*/

/**
 **  mboxCreate  -  Creation d'un device mbox
 **
 **  Description :
 **  Cree un mailbox de nom et taille donnes.
 **
 **  Retourne : OK ou ERROR
 **/

STATUS
mboxCreate(char *name, int size, MBOX_ID *pMboxId)
{
    H2_MBOX_STR *mbox;
    H2RNG_ID rngId;
    MBOX_ID dev;

    /* Alloue un device h2 */
    dev = h2devAlloc(name, H2_DEV_TYPE_MBOX);
    if (dev == ERROR) {
        return(ERROR);
    }
    mbox = H2DEV_MBOX_STR(dev);
    /* Allouer un semaphore d'exclusion */
    if ((mbox->semExcl = h2semAlloc(H2SEM_EXCL)) == ERROR) {
	return ERROR;
    }
    /* Allouer un semaphore de synchro de lecture */
    if ((mbox->semSigRd = h2semAlloc(H2SEM_SYNC)) == ERROR) {
	return ERROR;
    }
    
    /* Creer un ring buffer */
    rngId = h2rngCreate(H2RNG_TYPE_BLOCK, size);
    if (rngId == NULL) {
	return ERROR;
    }
    /* Memoriser l'identificateur global du ring buffer */
    mbox->rngId = (H2RNG_ID)smObjLocalToGlobal(rngId);

    /* Autres informations */
    mbox->size = size;
    mbox->taskId = taskGetUserData(0);

    /* C'est tout */
    *pMboxId = dev;
    return OK;
}

/*----------------------------------------------------------------------*/

/**
 **   mboxFind  -  Chercher un mailbox par son nom
 ** 
 **   Description :
 **   Cette fonction permet de trouver l'identificateur d'un mailbox,
 **   a partir de son nom. 
 **
 **   Retourne :
 **   OK ou ERROR
 **/
STATUS
mboxFind(char *name, MBOX_ID *pMboxId)
{
    MBOX_ID mbox;

    mbox = h2devFind(name, H2_DEV_TYPE_MBOX);
    if (mbox == ERROR) {
	return(ERROR);
    }
    *pMboxId = mbox;
    return OK;
}

/*----------------------------------------------------------------------*/

/**
 **  mboxSend  -  Envoi d'un message a un mailbox
 **
 **  Description :
 **  Envoi un message vers un device mbox. Prend le semaphore d'exclusion
 **  du device, verifie s'il est ouvert. En fonction du numero de la carte
 **  destinataire, calcule l'adresse du ring buffer du device. Ecrit
 **  le message et signalise le destinataire que un message a ete pose sur son
 **  mailbox.
 **
 **  Retourne : OK ou ERROR
 **/

STATUS
mboxSend(MBOX_ID toId, MBOX_ID fromId, char *buf, int nbytes)
{
    H2RNG_ID rngId;			/* ring buffer du device */
    H2SEM_ID semTask;
    int result;
    
    /* Prendre le semaphore d'exclusion mutuelle du device */
    if (h2semTake (H2DEV_MBOX_SEM_EXCL_ID(toId), WAIT_FOREVER) != TRUE) {
	return (ERROR);
    }
    /* Obtenir l'adresse locale du ring buffer */
    rngId = (H2RNG_ID)smObjGlobalToLocal(H2DEV_MBOX_RNG_ID(toId));
    
    /* Ecrire le block du message (avec mon identif) */
    if ((result = h2rngBlockPut (rngId, (int) fromId, buf, nbytes)) 
	!= nbytes) {
	if (result == 0) {
	    errnoSet (S_mboxLib_MBOX_FULL);
	}
	h2semGive (H2DEV_MBOX_SEM_EXCL_ID(toId));
	return (ERROR);
    } 
    /* Signaler au device destinataire qu'il a un message pret a lire */
    if (h2semGive (h2semGive(H2DEV_MBOX_SEM_ID(toId))) == ERROR) {
	printf("erreur give semSigRd\n");
    }
    /* Signaler l'evenement a la tache creatrice du mailbox */
    semTask = H2DEV_TASK_SEM_ID(H2DEV_MBOX_TASK_ID(toId));
    if (h2semGive(semTask) == ERROR) {
	printf("erreur give semTask\n");
    }
    /* Lacher le semaphore d'exclusion */
    h2semGive(H2DEV_MBOX_SEM_EXCL_ID(toId));
    
    /* OK, c'est bon ! */
    return (OK);

}

/*----------------------------------------------------------------------*/

int
mboxRcv(MBOX_ID mboxId, MBOX_ID *pFromId, char *buf, int maxbytes, 
	int timeout)
{
    int nr;                       /* Nombre de bytes lus */
    int takeStat;                 /* Status prise semaphore */
    H2RNG_ID rid;

    /* Passer l'id du ring buffer en local */
    rid = (H2RNG_ID)smObjGlobalToLocal(H2DEV_MBOX_RNG_ID(mboxId));

    /* Effacer le semaphore de synchro */
    h2semFlush(H2DEV_MBOX_SEM_ID(mboxId));
    
    /* Attendre l'arrivee d'un message */
    while (1) {
	
	/* Verifier s'il y a un message */
	if ((nr = h2rngNBytes (rid)) > 0) {
	    /* Lire le message */
	    nr = h2rngBlockGet (rid, (int *) pFromId, buf, maxbytes);
	    return (nr);
	}
	
	/* Retourner, si ERROR */
	if (nr < 0)
	    return (ERROR);
	
	/* Sinon, attendre */
	if ((takeStat = h2semTake (H2DEV_MBOX_SEM_ID(mboxId), timeout)) 
	    != TRUE)
	    return (takeStat);
    }
}

/*----------------------------------------------------------------------*/

/**
 **   mboxIoctl  -  Demande de renseignements a un device mbox
 **
 **   Description :
 **   Analogue a la fonction ioctl sous UNIX, cette routine permet la demande
 **   de renseignements sur un device mbox. Voir les premieres pages de
 **   ce fichier !
 **
 **   Retourne :
 **   OK ou ERROR
 **/
STATUS
mboxIoctl(MBOX_ID mboxId, int codeFunc, void *pArg)
{
    H2RNG_ID rid;                /* Pointeur vers ring buffer */
    int n;                       /* Nombre de messages ou de bytes */
    
    /* Obtenir l'adresse du ring buffer */
    rid = (H2RNG_ID)smObjGlobalToLocal(H2DEV_MBOX_RNG_ID(mboxId));

    /* Executer la fonction demandee */
    switch (codeFunc) {
      case FIO_NMSGS:                   /* Nombre de messages dans 
					   le mailbox */
	n = h2rngNBlocks (rid);
	break;
	
      case FIO_GETNAME:                 /* Nom du mailbox */
	
	(void) strcpy ((char *) pArg, H2DEV_NAME(mboxId));
	return (OK);
	
      case FIO_NBYTES:                  /* Nombre de bytes dans mailbox */
	
	/* Obtenir le nombre de bytes dans le ring buffer */
	n = h2rngNBytes (rid);
	break;
	
      case FIO_FLUSH:                   /* Nettoyer le mailbox */
	
	/* Nettoyer le ring buffer */
	h2rngFlush (rid);
	return (OK);
	
      case FIO_SIZE:                    /* Taille d'un mailbox */
      
	/* Obtenir la taille du mailbox */
	n = H2DEV_MBOX_STR(mboxId)->size;
	break;
	
      default:                          /* Fonction inconnue */
	
	errnoSet (S_mboxLib_BAD_IOCTL_CODE);
	return (ERROR);
    } /* switch */
    
    /* Verifier si erreur */
    if (n == ERROR)
	return (ERROR);
    
    /* Garder le resultat */
    *((int *) pArg) = n;
    return (OK);
    
} 

/*----------------------------------------------------------------------*/

/**
 **   mboxPause  -  Attendre l'arrivee d'un message sur un des mailboxes
 **
 **   Description :
 **   Par moyen de cette routine, le processus attend (suspendu) l'arrivee d'un
 **   message sur un de ses mailboxes.
 ** 
 **   Retourne : TRUE, FALSE ou ERROR
 **/
BOOL
mboxPause(MBOX_ID mboxId, int timeout)
{
    int nMbox;                  /* Numero d'un mailbox */
    int nMes;                   /* Nombre de messages dans un mailbox */
    int myTaskId;               /* Mon numero de tache */
    int takeStatus;             /* Etat de la prise de semaphore */
    H2SEM_ID semId;
    
    /* Verifier si on attend sur TOUS les mboxes */
    if (mboxId == ALL_MBOX) {
	myTaskId = taskGetUserData(0);
	/* Obtenir mon semaphore de synchro de tache */
	semId = H2DEV_TASK_SEM_ID(myTaskId);
	
	/* Faire le clear du semaphore de synchr. */
	h2semFlush(semId);
	
	/* Attendre l'arrivee d'un message */
	while (1) {
	    /* Verifier s'il y a des messages sur mes mailboxes */
	    for (nMbox = 0; nMbox < H2_DEV_MAX; nMbox++) {
		if (H2DEV_TYPE(nMbox) == H2_DEV_TYPE_MBOX 
		    && H2DEV_MBOX_TASK_ID(nMbox) == myTaskId) {
		    if (mboxIoctl (nMbox, FIO_NMSGS, (char *) &nMes)
			== OK && nMes != 0) {
			return (TRUE);
		    }
		}
	    } /* for */
	    /* Sinon, les attendre */
	    if ((takeStatus = h2semTake(semId, timeout)) != TRUE) {
		return (takeStatus);
	    }
	} /* while */
    }
    
      
    /* Clear du semaphore de sync */
    h2semFlush(H2DEV_MBOX_SEM_ID(mboxId));
    
    /* Attendre un message */
    while (1) {
	/* Verifier s'il y a un message sur le mailbox */
	if (mboxIoctl (mboxId, FIO_NMSGS, (char *) &nMes) == OK && nMes != 0)
	    return (TRUE);
	
	/* Sinon, attendre */
	if ((takeStatus = h2semTake (H2DEV_MBOX_SEM_ID(mboxId), timeout)) 
	    != TRUE) {
	    return (takeStatus);
	}
    } /* while */
}

/*----------------------------------------------------------------------*/

STATUS
mboxDelete(MBOX_ID mboxId)
{
    uid_t uid = getuid();

    if (uid != H2DEV_UID(mboxId) && uid != H2DEV_UID(0)) {
	errnoSet(S_mboxLib_NOT_OWNER);
	return ERROR;
    }
    /* Deleter le ring buffer */
    h2rngDelete (smObjGlobalToLocal(H2DEV_MBOX_RNG_ID(mboxId)));
    
    /* Liberer le semaphore de synchronisation */
    h2semDelete (H2DEV_MBOX_SEM_ID(mboxId));

    /* Liberer le semaphore d'exclusion */
    h2semDelete (H2DEV_MBOX_SEM_EXCL_ID(mboxId));
    
    /* Liberer le device */
    h2devFree(mboxId);
    
    return OK;

}

/*----------------------------------------------------------------------*/

int
mboxSpy(MBOX_ID mboxId, MBOX_ID *pFromId, int *pNbytes, 
	char *buf, int maxbytes)
{
    /* Epier le message */
    return h2rngBlockSpy (smObjGlobalToLocal(H2DEV_MBOX_RNG_ID(mboxId)),
			   (int *)pFromId, pNbytes, buf, maxbytes);
}

/*----------------------------------------------------------------------*/

STATUS
mboxSkip(MBOX_ID mboxId)
{
    /* Demander de sauter un message */
    return h2rngBlockSkip (smObjGlobalToLocal(H2DEV_MBOX_RNG_ID(mboxId)));
    
}

/*----------------------------------------------------------------------*/

STATUS
mboxEnd(int taskId)
{
    int i, dev;
    
    if (taskId == 0) {
	taskId = taskIdSelf();
    }
    /* Device associe a` la tache */
    dev = taskGetUserData(taskId);

    /* Destruction de toutes les mbox associees a ce process */
    for (i = 0; i < H2_DEV_MAX; i++) {
	if (H2DEV_TYPE(i) == H2_DEV_TYPE_MBOX
	    && H2DEV_MBOX_TASK_ID(i) == dev) {
	    mboxDelete(i);
	}
    }
    /* Destruction du semaphore associe a la tache */
    h2semDelete(H2DEV_TASK_SEM_ID(dev));
    /* Destruction du device associe a la tache */
    h2devFree(dev);
    return(OK);
}

/*----------------------------------------------------------------------*/

void
mboxShow(void)
{
    int i;
    int nMess, bytes, size;
    
    if (h2devAttach() == ERROR) {
	return;
    }
    putchar('\n');
    printf("Name                              Id     Size NMes    Bytes\n"
	   "-------------------------------- --- -------- ---- --------\n");
    for (i = 0; i < H2_DEV_MAX; i++) {
	if (H2DEV_TYPE(i) == H2_DEV_TYPE_MBOX) {
	    mboxIoctl(i, FIO_SIZE, &size);
	    mboxIoctl(i, FIO_NMSGS, &nMess);
	    mboxIoctl(i, FIO_NBYTES, &bytes);
	    printf("%-32s %3d %8d %4d %8d\n", H2DEV_NAME(i), i,
		   size, nMess, bytes);
	}
    } /* for */
    putchar('\n');
}

/*----------------------------------------------------------------------*/

