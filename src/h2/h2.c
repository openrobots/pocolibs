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

#include "pocolibs-config.h"
__RCSID("$LAAS$");

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fnmatch.h>

#include "portLib.h"
#include "errnoLib.h"
#include "h2errorLib.h"
#include "h2devLib.h"
#include "h2initGlob.h"
#include "mboxLib.h"
#include "posterLib.h"
#include "smMemLib.h"
#include "smObjLib.h"

extern void h2listModules(void);

static char *progname;

/*----------------------------------------------------------------------*/

void
usage(void)
{
    fprintf(stderr, 
	    "Usage: %s init [SM_MEM_SIZE]\n"
	    "       %s end\n"
	    "       %s info\n"
	    "       %s listModules\n"
	    "       %s printErrno CODE\n"
	    "       %s clean PATTERN\n", 
	    progname, progname, progname, progname, progname, progname);
    exit(1);
}

/*----------------------------------------------------------------------*/

int 
getyesno(int def)
{
    char rep[10];
    int i;

    while (1) {
	if (fgets(rep, sizeof(rep), stdin) == NULL) {
	    fprintf(stderr, "%s: gets: %s\n", progname, strerror(errno));
	    return(def);
	}
	
	/* skip white space */
	for (i = 0; i < sizeof(rep) && isspace(rep[i]); i++) ;
	if (i == sizeof(rep)) {
	    /* Empty line */
	    return def;
	} else {
	    switch (rep[i]) {
	      case 'y':
	      case 'Y':
	      case 'O':
	      case 'o':
		return 1;
	      case 'n':
	      case 'N':
		return 0;
	      default:
		printf("Please enter 'y' or 'n': ");
	    } /* switch */
	} 
    } /* while */
}
	
/*----------------------------------------------------------------------*/

/*
 * Creation des devices h2
 */
int 
h2init(int smMemSize)
{
    printf("Initializing comLib devices: ");
    fflush(stdout);
    /* Creation du tableau des devices h2 */
    if (h2devInit(smMemSize) == ERROR) {
	/* Essai recuperation erreur */
	switch (errnoGet()) {
	  case EEXIST:
	    printf("\ncomlib devices already exist on this machine.\n"
		   "Do you want to delete and recreate them (y/n) ? ");
	    if (getyesno(0)) { 
		/* Try to remove the devices first */
		h2devEnd();
		if (h2devInit(smMemSize) == ERROR) {
		    printf("Sorry, h2devInit failed again.\n"
			   "Please remove shared memory segments "
			   "and semaphores manually with ipcrm\n");
		    h2perror("h2devInit");
		    return ERROR;
		}
	    } else {
		return ERROR;
	    }
	    break;
	  case S_smObjLib_SHMGET_ERROR:
	    printf("\nThe requested shared memory segment size  (%d) "
		   "is too large. \nPlease specify a smaller size with:\n"
		   "  h2 init <size in bytes>\n", smMemSize);
	    return ERROR;
	  default:
	    /* Other errors */
	    h2perror("\nh2devInit");
	    return ERROR;
	} /* switch */
    } 
    printf("OK\n");
    return OK;
} /* h2init */

/*----------------------------------------------------------------------*/

int
h2end(void)
{
	
    printf("Removing csLib devices: ");
    /* Destruction devices */
    if (h2devEnd() == ERROR) {
	h2perror("\nh2devEnd");
	return ERROR;
    } else {
	printf("OK\n");
    }
    return OK;
}

/*----------------------------------------------------------------------*/

int 
cleanDevs(char *name)
{
    int i, match = 0;
    POSTER_ID p;

    if (h2devAttach() == ERROR) {
	h2perror("cleanDevs");
	return ERROR;
    }
    /* Destruction des devices restants */
    for (i = 0; i < H2_DEV_MAX; i++) {
	if (H2DEV_TYPE(i) != H2_DEV_TYPE_NONE && 
	    fnmatch(name, H2DEV_NAME(i), 0) == 0) {
	    printf("Freeing %s\n", H2DEV_NAME(i));
	    match++;
	    switch (H2DEV_TYPE(i)) {
	      case H2_DEV_TYPE_MBOX:
		mboxDelete(i);
		break;
	      case H2_DEV_TYPE_POSTER:
		if (posterFind(H2DEV_NAME(i), &p) == OK) { 
		    posterDelete(p);
		}
		break;
	      case H2_DEV_TYPE_TASK:
		h2semDelete(H2DEV_TASK_SEM_ID(i));
		h2devFree(i);
		break;
	      case H2_DEV_TYPE_SEM:
	      case H2_DEV_TYPE_NONE:
		break;
	      default:
		/* error */
		fprintf(stderr, "%s: unknown device type %d\n", 
			progname, H2DEV_TYPE(i));
		return ERROR;
		break;
	    } /* switch */
	} 
    } /* for */
    if (match == 0) {
	printf("No matching device\n");
	return ERROR;
    }
    return OK;
}
    
/*----------------------------------------------------------------------*/

int
h2info(void)
{
    if (h2devShow() == ERROR) {
	fprintf(stderr, 
		"%s: comLib devices are not initialized on this machine\n", 
		progname);
	return ERROR;
    }
    return OK;
}

/*----------------------------------------------------------------------*/

int
main(int argc, char *argv[])
{
    int status;

    progname = argv[0];

    /* Initialisation du systeme temps-reel, sans horloge */
    osInit(0);
    
    switch (argc) {
      case 2: 
	if (strcmp(argv[1], "init") == 0) {
	    /* Initialisation des devices h2 */
	    status = h2init(SM_MEM_SIZE);
	} else if (strcmp(argv[1], "end") == 0) {
	    /* Destruction des devices h2 */
	    status =  h2end();
	} else if (strcmp(argv[1], "info") == 0) {
	    /* Affichage des devices h2 */
	    status = h2info();
	} else if (strcmp(argv[1], "listModules") == 0) {
	    h2listModules();
	    status = OK;
	} else {
	    usage();
	}
	break;
      case 3:
	if (strcmp(argv[1], "init") == 0) {
	    status = h2init(atoi(argv[2]));
	} else if (strcmp(argv[1], "printErrno") == 0) {
	    h2printErrno (strtol(argv[2],(char **)NULL, 0));
	    status = OK;
	} else if (strcmp(argv[1], "clean") == 0) {
	    status = cleanDevs(argv[2]);
	} else {
	    usage();
	}
	break;
      default:
	usage();
    }
    if (status == ERROR) {
	exit(3);
    }
    return 0;
} 
 
