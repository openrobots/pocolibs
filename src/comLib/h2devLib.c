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

#include "portLib.h"
#include "errnoLib.h"
#include "h2semLib.h"
#include "mboxLib.h"
#include "posterLib.h"
#include "h2errorLib.h"
#include "h2devLib.h"
#include "smMemLib.h"
#include "smObjLib.h"

/* Paths pour le lancement de posterServ */
#ifndef EXEC_PREFIX
#define EXEC_PREFIX "/usr/local/robots"
#endif

/**
 ** Variables globales
 **/

H2_DEV_STR *h2Devs = NULL;

static int shmid = -1;
static char h2devFileName[MAXPATHLEN];
static pthread_mutex_t h2devMutex = PTHREAD_MUTEX_INITIALIZER;
static int posterServPid = -1;		/* pid du serveur de posters */
static char posterServPath[PATH_MAX];

/* Prototypes fonctions locales */
static int h2devFindAux(const char *name, H2_DEV_TYPE type);

/*----------------------------------------------------------------------*/

/**
 ** Determine la clef IPC correspondant a un device h2
 **
 ** type  : type de device
 ** dev   : numero du device
 ** create: TRUE si la cle doit etre cree, 
 **         FALSE si elle doit deja exister
 **/
key_t
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
    char buf[16];
    int savedError;
    
    key = h2devGetKey(H2_DEV_TYPE_H2DEV, 0, TRUE, &fd);
    if (key == ERROR) {
	return ERROR;
    }
    shmid = shmget(key, sizeof(H2_DEV_STR)*H2_DEV_MAX,
		   IPC_CREAT | IPC_EXCL | PORTLIB_MODE);
    if (shmid == -1) {
	errnoSet(S_smObjLib_SHMGET_ERROR);
	close(fd);
	return(ERROR);
    }
    pthread_mutex_lock(&h2devMutex);
    h2Devs = shmat(shmid, NULL, 0);
    if (h2Devs == NULL) {
	errnoSet(S_smObjLib_SHMAT_ERROR);
	pthread_mutex_unlock(&h2devMutex);
	close(fd);
	return(ERROR);
    }
    /* Creation semaphores */
    h2Devs[0].type = H2_DEV_TYPE_SEM;
    h2Devs[0].uid = getuid();
    strcpy(h2Devs[0].name, "h2semLib");
    if (h2semInit(0, &(H2DEV_SEM_SEM_ID(0))) == ERROR) {
	pthread_mutex_unlock(&h2devMutex);
	close(fd);
	return ERROR;
    }
    /* Allocation a la main du premier semaphore */
    h2semCreate0(H2DEV_SEM_SEM_ID(0), SEM_EMPTY);

    /* Initialisation */
    for (i = 1; i < H2_DEV_MAX; i++) {
	/* marque tous les devices libres */
	h2Devs[i].type = H2_DEV_TYPE_NONE;
    }
    h2semGive(0);
    pthread_mutex_unlock(&h2devMutex);

    /* Creation de la memoire partagee */
    if (smMemInit(smMemSize) == ERROR) {
	/* Memorise le code d'erreur car h2devEnd() l'ecrase */
	savedError = errnoGet();
	/* Detruit tout ce qui a deja ete cree */
	h2devEnd();
	errnoSet(savedError);		/* restore l'erreur de smMemInit */
        return ERROR;
    }
    /* Demarrage du serveur de posters */
    if (getenv("POSTER_HOST") == NULL) {
	snprintf(posterServPath, PATH_MAX, "%s/bin/posterServ", 
		 EXEC_PREFIX);
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

    pthread_mutex_lock(&h2devMutex);
    if (h2Devs != NULL) {
	pthread_mutex_unlock(&h2devMutex);
	return OK;
    }
    key = h2devGetKey(H2_DEV_TYPE_H2DEV, 0, FALSE, &fd);
    if (key == -1) {
	pthread_mutex_unlock(&h2devMutex);
	return ERROR;
    }
    shmid = shmget(key, sizeof(H2_DEV_STR)*H2_DEV_MAX, PORTLIB_MODE);
    if (shmid == -1) {
	errnoSet(errno);
	pthread_mutex_unlock(&h2devMutex);
	close(fd);
	return ERROR;
    }
    h2Devs = shmat(shmid, NULL, 0);
    if (h2Devs == NULL) {
	errnoSet(errno);
	pthread_mutex_unlock(&h2devMutex);
	close(fd);
	return ERROR;
    }
    /* Lecture pid du serveur de posters */
    n = read(fd, buf, sizeof(buf));
    if (n < 0) {
	errnoSet(errno);
	pthread_mutex_unlock(&h2devMutex);
	close(fd);
	return ERROR;
    }
    sscanf(buf, "%d", &posterServPid);
    close(fd);

    pthread_mutex_unlock(&h2devMutex);
    return OK;
}

/*----------------------------------------------------------------------*/

/**
 ** Allocation d'un device h2
 **/
int
h2devAlloc(char *name, H2_DEV_TYPE type)
{
    int i;

    if (h2devAttach() == ERROR) {
	return ERROR;
    }
    h2semTake(0, WAIT_FOREVER);

    /* Verifie que le nom n'existe pas */
    if (type != H2_DEV_TYPE_SEM && h2devFindAux(name, type) != ERROR) {
	h2semGive(0);
	errnoSet(S_h2devLib_DUPLICATE_DEVICE_NAME);
	return ERROR;
    }
    /* Recherche un device libre */
    for (i = 0; i < H2_DEV_MAX; i++) {
	if (h2Devs[i].type == H2_DEV_TYPE_NONE) {
	    /* Trouve' */
	    strncpy(h2Devs[i].name, name, H2_DEV_MAX_NAME);
	    h2Devs[i].type = type;
	    h2Devs[i].uid = getuid();
	    h2semGive(0);
	    return i;
	}
    } /* for */

    /* Pas de device libre */
    h2semGive(0);
    errnoSet(S_h2devLib_FULL);
    return ERROR;
}

/*----------------------------------------------------------------------*/

/**
 ** Liberation d'un device h2
 **/
STATUS
h2devFree(int dev)
{
    uid_t uid = getuid();

    if (h2devAttach() == ERROR) {
	return ERROR;
    }
    if (uid != H2DEV_UID(dev) && uid != H2DEV_UID(0)) {
	errnoSet(S_h2devLib_NOT_OWNER);
	return ERROR;
    }
    h2semTake(0, WAIT_FOREVER);
    h2Devs[dev].type = H2_DEV_TYPE_NONE;
    h2semGive(0);
    return OK;
}

/*----------------------------------------------------------------------*/

/**
 ** Recherche d'un device h2
 **/

static int 
h2devFindAux(const char *name, H2_DEV_TYPE type)
{
    int i;

    for (i = 0; i < H2_DEV_MAX; i++) {
	if ((type == h2Devs[i].type) 
	    && (strcmp(name, h2Devs[i].name) == 0)) {
	    return(i);
	}
    } /* for */
    return ERROR;
}

int
h2devFind(char *name, H2_DEV_TYPE type)
{
    int i;

    if (name == NULL) {
	errnoSet(S_h2devLib_BAD_PARAMETERS);
	return ERROR;
    }
    if (h2devAttach() == ERROR) {
	return ERROR;
    }
    h2semTake(0, H2DEV_TIMEOUT);
    i = h2devFindAux(name, type);
    h2semGive(0);

    if (i >= 0) {
	return i;
    } else {
	errnoSet(S_h2devLib_NOT_FOUND);
	return ERROR;
    }
}

/*----------------------------------------------------------------------*/

/**
 ** Destruction des h2 devices 
 **/
STATUS
h2devEnd(void)
{
    int i, rv = OK;
#ifndef __DARWIN__
    POSTER_ID p;
#endif

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
#ifndef __DARWIN__
	    if (posterFind(H2DEV_NAME(i), &p) == OK) {
		posterDelete(p);
	    }
#else
	    /* On Darwin strict libraries dependencies are required 
	     * So posterLib functions cannot be called here 
	     * Thus only free local resources */
	    H2DEV_TYPE(i) = H2_DEV_TYPE_NONE;
#endif
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
 ** Retourne le semId des semaphores
 **/
int
h2devGetSemId(void)
{
    if (h2devAttach() == ERROR) {
	return ERROR;
    }
    if (h2Devs[0].type != H2_DEV_TYPE_SEM) {
	errnoSet(S_h2devLib_BAD_DEVICE_TYPE);
	return ERROR;
    }
    return h2Devs[0].data.sem.semId;
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
	pthread_mutex_lock(&h2devMutex);
	if (H2DEV_TYPE(i) != H2_DEV_TYPE_NONE) {
	    printf("%2d %6s %5d %s\n", i,
		   h2devTypeName[H2DEV_TYPE(i)],  H2DEV_UID(i), 
		   H2DEV_NAME(i));
	}
	pthread_mutex_unlock(&h2devMutex);
    } /* for */
    printf("------------------------------------------------\n");
    return OK;
}
