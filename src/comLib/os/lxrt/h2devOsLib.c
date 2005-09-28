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
/*
  TODO: how to destroy h2devMutex...
*/

#include "pocolibs-config.h"
__RCSID("$LAAS$");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <signal.h>
#include <limits.h>
#include <pthread.h>

#include <rtai_lxrt.h>

#include "portLib.h"
#include "errnoLib.h"
#include "h2semLib.h"
#include "mboxLib.h"
#include "posterLib.h"
#include "h2errorLib.h"
#include "h2devLib.h"
#include "smMemLib.h"
#include "smObjLib.h"

/* #define COMLIB_DEBUG_H2DEVLIB */

#ifdef COMLIB_DEBUG_H2DEVLIB
#include "taskLib.h"
# define LOGDBG(x)	logMsg x
#else
# define LOGDBG(x)
#endif

/* Paths pour le lancement de posterServ */
#ifndef POSTERSERV_PATH
#define POSTERSERV_PATH "/usr/local/robots/bin/posterServ"
#endif

/**
 ** Variables globales
 **/

H2_DEV_STR *h2Devs = NULL;

static int shmid = -1;
static char h2devFileName[MAXPATHLEN];
static SEM_ID h2devMutex = NULL;
static int posterServPid = -1;		/* pid du serveur de posters */
static char const posterServPath[] = POSTER_SERV_PATH;

/* Prototypes fonctions locales */
static void get_h2devMutex(void);

/*----------------------------------------------------------------------*/

/**
 ** Determine la clef IPC correspondant a un device h2
 **
 ** type  : type de device
 ** dev   : numero du device
 ** create: TRUE si la cle doit etre cree, 
 **         FALSE si elle doit deja exister
 **/
long
h2devGetKey(int type, int dev, BOOL create, int *pFd)
{
    char *home;
    key_t key;
    struct utsname uts;
    int fd;
    int amode;
    
    /*
     * Recherche du fichier de cles 
     */
    /* Essaye d'abord H2DEV_DIR */
    home = getenv("H2DEV_DIR");
    if (home != NULL) {
	/* Verifie l'existence et le droit d'ecriture */
	if (create) {
	    amode = R_OK|W_OK|X_OK;
	} else {
	    amode = R_OK|X_OK;
	}
	if (access(home, amode) < 0) {
	    home = NULL;
	}
    }
	
    if (home == NULL) {
	/* Sinon, essaye HOME */
	home = getenv("HOME");
    }
    if (home == NULL) {
	errnoSet(S_h2devLib_BAD_HOME_DIR);
	return ERROR;
    }
    if (uname(&uts) == -1) {
	errnoSet(errno);
	return ERROR;
    }
    /* Test sur la longueur de la chaine */
    if (strlen(home)+strlen(uts.nodename)+strlen(H2_DEV_NAME)+3 > MAXPATHLEN) {
	errnoSet(S_h2devLib_BAD_HOME_DIR);
	return ERROR;
    }
    sprintf(h2devFileName, "%s/%s-%s", home, H2_DEV_NAME, uts.nodename);
    
    if (create) {
	/* Creation du fichier */
	fd = open(h2devFileName, O_WRONLY | O_CREAT | O_EXCL, PORTLIB_MODE);
	if (fd < 0) {
	    errnoSet(errno);
	    return ERROR;
	}
    } else {
	/* teste l'existence */
	fd = open(h2devFileName, O_RDONLY, 0); 
	if (fd == -1) {
	    errnoSet(S_h2devLib_NOT_INITIALIZED);
	    return ERROR;
	}
    }
    /* Retourne eventuellement le file descriptor */
    if (pFd != NULL) {
	*pFd = fd;
    } else {
	close(fd);
    }
    /* Calcul de l'id de la clef en fonction du type et du numero de device */
    key = ftok(h2devFileName, dev*H2DEV_MAX_TYPES + type);
    if (key == -1) {
	errnoSet(errno);
	close(fd);
	return ERROR;
    }
    return key;
}

/*----------------------------------------------------------------------*/

/**
 ** Initialisation
 **/
STATUS
h2devInit(int smMemSize)
{
    key_t key;
    int i;
    int fd; 
#if 0
    char buf[16];
#endif
    int savedError;

    LOGDBG(("comLib:h2devInit: begin initialisation\n"));

    key = h2devGetKey(H2_DEV_TYPE_H2DEV, 0, TRUE, &fd);
    if (key == ERROR) {
      fprintf(stderr, "h2devGetKey error\n");
      LOGDBG(("comLib:h2devInit: get key error\n"));
      return ERROR;
    }
    LOGDBG(("comLib:h2devInit: get key\n"));

    shmid = shmget(key, sizeof(H2_DEV_STR)*H2_DEV_MAX,
		   IPC_CREAT | IPC_EXCL | PORTLIB_MODE);
    if (shmid == -1) {
	errnoSet(S_smObjLib_SHMGET_ERROR);
	close(fd);
	LOGDBG(("comLib:h2devInit: get shmid error\n"));
	return(ERROR);
    }
    LOGDBG(("comLib:h2devInit: get shmid\n"));


    h2devMutex = semMCreate(0);
    if (h2devMutex == NULL)
      {
	logMsg("h2devInit error\n");
	LOGDBG(("comLib:h2devInit: h2devMutex create error\n"));
	return ERROR;
      }
    semTake(h2devMutex,WAIT_FOREVER);
    LOGDBG(("comLib:h2devInit: get h2devMutex\n"));

    
    h2Devs = shmat(shmid, NULL, 0);
    if (h2Devs == NULL) {
	errnoSet(S_smObjLib_SHMAT_ERROR);
	semGive(h2devMutex);
	close(fd);
	LOGDBG(("comLib:h2devInit: shmat error\n"));
	return(ERROR);
    }
    LOGDBG(("comLib:h2devInit: shmat ok\n"));

    /* initialize memory ??? */

    /* Creation semaphores */
    h2Devs[0].type = H2_DEV_TYPE_SEM;
    h2Devs[0].uid = getuid();
    strcpy(h2Devs[0].name, "h2semLib");
    if (h2semInit(0, &(H2DEV_SEM_SEM_ID(0)[0])) == ERROR) {
	semGive(h2devMutex);
	close(fd);
	LOGDBG(("comLib:h2devInit: h2semInit error\n"));
	return ERROR;
    }
    LOGDBG(("comLib:h2devInit: h2semInit ok\n"));

    /* Allocation a la main du premier semaphore */
    h2semCreate0(0, SEM_EMPTY);
    LOGDBG(("comLib:h2devInit: h2semCreate0 ok\n"));


    /* Initialisation */
    for (i = 1; i < H2_DEV_MAX; i++) {
	/* marque tous les devices libres */
	h2Devs[i].type = H2_DEV_TYPE_NONE;
    }
    h2semGive(0);
    semGive(h2devMutex);
    LOGDBG(("comLib:h2devInit: h2semGive0 and h2devMutex free\n"));


    /* Creation de la memoire partagee */
    if (smMemInit(smMemSize) == ERROR) {
	/* Memorise le code d'erreur car h2devEnd() l'ecrase */
	savedError = errnoGet();
	/* Detruit tout ce qui a deja ete cree */
	h2devEnd();
	errnoSet(savedError);		/* restore l'erreur de smMemInit */
	LOGDBG(("comLib:h2devInit: smMemInit error\n"));
        return ERROR;
    }
    LOGDBG(("comLib:h2devInit: smMemInit ok\n"));

    /* Demarrage du serveur de posters */
#if 0
    if (getenv("POSTER_HOST") == NULL) {
	posterServPid = fork();
	if (posterServPid == 0) {
	    /* Processus fils */
	    if (execl(posterServPath, "posterServ", (char *)NULL) == -1) {
		fprintf(stderr, "h2devInit: couldn't exec posterServ\n");
		exit(-1);
	    }
	} else {
	    /* Ecrit le pid dans le lock */
	    i = snprintf(buf, sizeof(buf), "%d\n", posterServPid);
	    if (write(fd, buf, i) < 0) {
		errnoSet(errno);
		close(fd);
		return ERROR;
	    }
	    close(fd);
	}
    } else {
	/* Ecrit -1 dans le lock */
	i = snprintf(buf, sizeof(buf), "-1\n");
	if (write(fd, buf, i) < 0) {
	    errnoSet(errno);
	    close(fd);
	    return ERROR;
	}
	close(fd);
    }
#endif
    LOGDBG(("comLib:h2devInit: initilisation ok\n"));
    return OK;
}
    
/*----------------------------------------------------------------------*/

/*
 * Retrouve le pointeur sur la structure partagee H2_DEV
 */
STATUS 
h2devAttach(void)
{
    key_t key;
    int fd, n;
    char buf[16];

   LOGDBG(("h2devLib:h2devAttach: begin (task \"%s\")\n", 
	   taskName(taskIdSelf())));

    if (h2devMutex == NULL)
      get_h2devMutex();

    LOGDBG(("h2devLib:h2devAttach: semTake h2devMutex=%#x\n",
	    h2devMutex));

    semTake(h2devMutex,WAIT_FOREVER);
    if (h2Devs != NULL) {
	semGive(h2devMutex);
	LOGDBG(("h2devLib:h2devAttach: (h2devMutex free) h2devAttach ok\n"));
	return OK;
    }
    LOGDBG(("h2devLib:h2devAttach: semTake h2devMutex done\n"));


    key = h2devGetKey(H2_DEV_TYPE_H2DEV, 0, FALSE, &fd);
    if (key == -1) {
	semGive(h2devMutex);
	return ERROR;
    }
    LOGDBG(("h2devLib:h2devAttach: h2devGetKey ok\n"));

    shmid = shmget(key, sizeof(H2_DEV_STR)*H2_DEV_MAX, PORTLIB_MODE);
    if (shmid == -1) {
	errnoSet(errno);
	semGive(h2devMutex);
	close(fd);
	LOGDBG(("h2devLib:h2devAttach: shmget error\n"));
	return ERROR;
    }
    LOGDBG(("h2devLib:h2devAttach: shmget ok\n"));

    h2Devs = shmat(shmid, NULL, 0);
    if (h2Devs == NULL) {
	errnoSet(errno);
	semGive(h2devMutex);
	close(fd);
	LOGDBG(("h2devLib:h2devAttach: shmat error\n"));
	return ERROR;
    }
    LOGDBG(("h2devLib:h2devAttach: shmat ok\n"));

    /* Lecture pid du serveur de posters */
    n = read(fd, buf, sizeof(buf));
    if (n < 0) {
	errnoSet(errno);
	semGive(h2devMutex);
	close(fd);
	return ERROR;
    }
    sscanf(buf, "%d", &posterServPid);
    close(fd);

    semGive(h2devMutex);

    LOGDBG(("h2devLib:h2devAttach: ok\n"));

    return OK;
}

/*----------------------------------------------------------------------*/

/**
 ** Destruction des h2 devices 
 ** who can call thsi function ?
 **/
STATUS
h2devEnd(void)
{
    int i, rv = OK;
    POSTER_ID p;

    if (h2devAttach() == ERROR) {
	/* Detruit le fichier de verrou a tout hasard */
	/* XXX C'est pas genial cote securite */
	rv = ERROR;
	goto fail;
    }
    /* Verifie qu'on est le proprietaire des IPC et du fichier */
    if (H2DEV_UID(0) != getuid()) {
	errnoSet(S_h2devLib_NOT_OWNER);
	rv =  ERROR;
	goto fail;
    }
    /* Destruction des devices restants */
    for (i = 0; i < H2_DEV_MAX; i++) {
	switch (H2DEV_TYPE(i)) {
	  case H2_DEV_TYPE_MBOX:
	    mboxDelete(i);
	    break;
	  case H2_DEV_TYPE_POSTER:
	    if (posterFind(H2DEV_NAME(i), &p) == OK) {
		posterDelete(p);
	    }
	    break;
	  case H2_DEV_TYPE_SEM:
	    /* Rien a faire, les semaphores sont liberes plus loin */
	    break;
	  case H2_DEV_TYPE_TASK:
	    break;
	  case H2_DEV_TYPE_NONE:
	    break;
	  default:
	    /* error */
	    break;
	} /* switch */
    } /* for */

    /* Tue le serveur de posters */
    if (posterServPid != -1) {
	kill(posterServPid, SIGTERM);
    }

    /* Libere les semaphores */
    h2semEnd();
    /* Libere la memoire partagee smMem */
    smMemEnd();
    /* Libere la structure memoire partagee des devices h2 */
    if (shmdt((char *)h2Devs) < 0) {
	fprintf(stderr, "h2devEnd: shmdt error %s\n", strerror(errno));
	rv = ERROR;
	goto fail;
    }
    if (shmctl(shmid, IPC_RMID, NULL) < 0) {
	fprintf(stderr, "h2devEnd: shmctl(IPC_RMID) error %s\n", 
		strerror(errno));
	rv = ERROR;
	goto fail;
    }
fail:
    /* detruit le fichier .h2devs */
    unlink(h2devFileName);
    /* marque le pointeur h2Devs comme invalide */
    h2Devs = NULL;
    return rv;
}

/*----------------------------------------------------------------------*/




/**
 ** Affichage info sur les devices h2
 **/
static char *h2devTypeName[] = {
    "NONE",
    "H2DEV",
    "SEM",
    "MBOX",
    "POSTER",
    "TASK",
    "MEM",
};

STATUS
h2devShow(void)
{
    int i;
    
    if (h2devAttach() == ERROR) {
	return ERROR;
    }
    printf("Id   Type   UID Name\n"
	   "------------------------------------------------\n");
    for (i = 0; i < H2_DEV_MAX; i++) {
	semTake(h2devMutex,WAIT_FOREVER);
	if (H2DEV_TYPE(i) != H2_DEV_TYPE_NONE) {
	    printf("%2d %6s %5ld %s\n", i,
		   h2devTypeName[H2DEV_TYPE(i)],  H2DEV_UID(i), 
		   H2DEV_NAME(i));
	}
	semGive(h2devMutex);
    } /* for */
    printf("------------------------------------------------\n");
    return OK;
}

static void get_h2devMutex(void)
{
  LOGDBG(("h2devLib:get_h2devMutex: begin\n"));

  h2devMutex = semMCreate(0);

  LOGDBG(("h2devLib:get_h2devMutex: ok\n"));
}
