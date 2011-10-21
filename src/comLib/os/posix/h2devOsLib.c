/*
 * Copyright (c) 1990, 2003,2008,2009 CNRS/LAAS
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

#ifdef VALGRIND_SUPPORT
#include <valgrind/memcheck.h>
#endif

/* Path of posterServ */
#ifndef POSTER_SERV_PATH
#error "POSTER_SERV_PATH should be set"
#endif

/**
 ** Global variables
 **/

H2_DEV_STR *h2Devs = NULL;

static int shmid = -1;
static char h2devFileName[MAXPATHLEN];
static pthread_mutex_t h2devMutex = PTHREAD_MUTEX_INITIALIZER;
static int posterServPid = -1;		/* pid du serveur de posters */
static char const posterServPath[] = POSTER_SERV_PATH;

/*----------------------------------------------------------------------*/

/**
 ** Find the IPC key corresponding to a h2 device
 **
 ** type  : device type
 ** dev   : device number
 ** create: TRUE if the key should be created
 **         FALSE if the key should already exist
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
     * Look for the key's  pathname
     */
    /* try H2DEV_DIR first */
    home = getenv("H2DEV_DIR");
    if (home != NULL) {
	/* Check for existence and write access */
	if (create) {
	    amode = R_OK|W_OK|X_OK;
	} else {
	    amode = R_OK|X_OK;
	}
	if (access(home, amode) < 0) {
	    errnoSet(S_h2devLib_BAD_HOME_DIR);
	    return ERROR;
	}
    }

    if (home == NULL) {
	/* otherwise try HOME */
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
    /* Check the length of the string */
    if (strlen(home)+strlen(uts.nodename)+strlen(H2_DEV_NAME)+3 > MAXPATHLEN) {
	errnoSet(S_h2devLib_BAD_HOME_DIR);
	return ERROR;
    }
    snprintf(h2devFileName, sizeof(h2devFileName), "%s/%s-%s",
	home, H2_DEV_NAME, uts.nodename);

    if (create) {
	/* Create the file */
	fd = open(h2devFileName, O_WRONLY | O_CREAT | O_EXCL, PORTLIB_MODE);
	if (fd < 0) {
	    errnoSet(errno);
	    return ERROR;
	}
    } else {
	/* Check for existence */
	fd = open(h2devFileName, O_RDONLY, 0);
	if (fd == -1) {
	    errnoSet(S_h2devLib_NOT_INITIALIZED);
	    return ERROR;
	}
    }
    /* Return the file descriptor */
    if (pFd != NULL) {
	*pFd = fd;
    } else {
	close(fd);
    }
    /* compute the key id depending on the device type and number */
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
 ** Initialization
 **/
STATUS
h2devInit(int smMemSize, int posterServFlag)
{
    key_t key;
    int i;
    int fd, pipefd[2];
    char buf[16];
    int savedError;

    h2devRecordH2ErrMsgs();

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
    /* Create semaphores */
    h2Devs[0].type = H2_DEV_TYPE_SEM;
    h2Devs[0].uid = getuid();
    strcpy(h2Devs[0].name, "h2semLib");
    if (h2semInit(0, &(H2DEV_SEM_SEM_ID(0))) == ERROR) {
	pthread_mutex_unlock(&h2devMutex);
	close(fd);
	return ERROR;
    }
    /* Manually allocate the first semaphore */
    h2semCreate0(H2DEV_SEM_SEM_ID(0), SEM_EMPTY);

    /* Initialization */
    for (i = 1; i < H2_DEV_MAX; i++) {
	/* mark all devices as free */
	h2Devs[i].type = H2_DEV_TYPE_NONE;
    }
    h2semGive(0);
    pthread_mutex_unlock(&h2devMutex);

    /* Create shared memory segment */
    if (smMemInit(smMemSize) == ERROR) {
	/* Store the error code, that gets clobbered by h2devEnd() */
	savedError = errnoGet();
	/* Destroy already created devices*/
	h2devEnd();
	errnoSet(savedError);		/* restore smMemInit() status */
        return ERROR;
    }
    /* Start poster server */
    if (getenv("POSTER_HOST") == NULL && posterServFlag == TRUE) {
	if (pipe(pipefd) < 0) {
	    errnoSet(errno);
	    close(fd);
	    return ERROR;
	}
	
	posterServPid = fork();
	if (posterServPid == 0) {
	    close(pipefd[1]);
	    /* wait until pid is written to the file */
	    if (read(pipefd[0], &buf, 3) != 3) {
		printf("h2devInit: pipe read error\n");
		exit(-1);
	    }
	    close(pipefd[0]);
	    /* Processus fils */
	    if (execl(posterServPath, "posterServ", (char *)NULL) == -1) {
		fprintf(stderr, "h2devInit: couldn't exec posterServ\n");
		exit(-1);
	    }
	} else {
	    close(pipefd[0]);
	    /* Store the pid in the lock file */
	    i = snprintf(buf, sizeof(buf), "%d\n", posterServPid);
	    if (write(fd, buf, i) < 0) {
		errnoSet(errno);
		close(fd);
		return ERROR;
	    }
	    close(fd);
	    /* tell child that we're done */
	    if (write(pipefd[1], "OK\n", 3) != 3) {
		errnoSet(errno);
		printf("h2devInit: pipe write error\n");
		return ERROR;
	    }
	    close(pipefd[1]);
	}
    } else {
	/* Store  -1 in the lock file if no posterServ was started */
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
 * Look for the pointer to the shared H2_DEV structure
 */
STATUS
h2devAttach(void)
{
    key_t key;
    int fd, n;
    char buf[16];

    if (h2Devs != NULL) 
        return OK;

    h2devRecordH2ErrMsgs();

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
    /* get the process id of the poster server */
    n = read(fd, buf, sizeof(buf) - 1);
    if (n < 0) {
	errnoSet(errno);
	pthread_mutex_unlock(&h2devMutex);
	close(fd);
	return ERROR;
    }
    buf[n] = 0;
    if (sscanf(buf, "%d", &posterServPid) != 1) {
	errnoSet(EINVAL);
        pthread_mutex_unlock(&h2devMutex);
	close(fd);
	return ERROR;
    }
    close(fd);

    pthread_mutex_unlock(&h2devMutex);


#ifdef VALGRIND_SUPPORT
    VALGRIND_MAKE_READABLE(h2Devs, sizeof(H2_DEV_STR) * H2_DEV_MAX);
#endif

    return OK;
}

/*----------------------------------------------------------------------*/

/**
 ** Destroy h2 devices
 **/
STATUS
h2devEnd(void)
{
    int i, rv = OK;
    POSTER_ID p;

    if (h2devAttach() == ERROR) {
	/* Unlink the lock file, just in case */
	/* XXX Not very clean from security point of vue */
	rv = ERROR;
	goto fail;
    }
    /* Check that we own the IPC and the lock file */
    if (H2DEV_UID(0) != getuid()) {
	errnoSet(S_h2devLib_NOT_OWNER);
	rv =  ERROR;
	goto fail;
    }
    /* Destroy remaining devices */
    for (i = 0; i < H2_DEV_MAX; i++) {
	switch (H2DEV_TYPE(i)) {
	  case H2_DEV_TYPE_MBOX:
	    mboxDelete(i);
	    break;
	  case H2_DEV_TYPE_POSTER:
	    /* Don't call posterLib, to avoid circular lib dependencies */
	    smMemFree(smObjGlobalToLocal(H2DEV_POSTER_POOL(i)));
	    h2semDelete(H2DEV_POSTER_SEM_ID(i));
	    h2devFree(i);
	    break;
	  case H2_DEV_TYPE_SEM:
	    /* Nothing to do, semaphores are cleaned further down */
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

    /* Kill the poster server */
    if (posterServPid != -1) {
	kill(posterServPid, SIGTERM);
    }

    /* Clean  semaphores */
    h2semEnd();
    /* Free shared memory */
    smMemEnd();
    /* Free the shared memory segment holding h2 devices */
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
    /* remove the .h2devs lock file */
    unlink(h2devFileName);
    /* and mark the global pointer as invalid */
    h2Devs = NULL;
    return rv;
}

/*----------------------------------------------------------------------*/




/**
 ** Display information about h2 devices
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
	    printf("%2d %6s %5ld %s\n", i,
		   h2devTypeName[H2DEV_TYPE(i)],  H2DEV_UID(i),
		   H2DEV_NAME(i));
	}
	pthread_mutex_unlock(&h2devMutex);
    } /* for */
    printf("------------------------------------------------\n");
    return OK;
}
