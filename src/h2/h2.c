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

#include <getopt.h>
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
	    "Usage: %s init [-p][SM_MEM_SIZE]\n"
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
	for (i = 0; i < sizeof(rep) && isspace((int)rep[i]); i++) ;
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
static int 
h2init(int smMemSize, int posterServFlag)
{
    printf("Initializing %s devices: ", PACKAGE_NAME);
    fflush(stdout);
    /* Creation du tableau des devices h2 */
    if (h2devInit(smMemSize, posterServFlag) == ERROR) {
	/* Essai recuperation erreur */
	switch (errnoGet()) {
	  case EEXIST:
	    printf("\n%s devices already exist on this machine.\n"
		"Do you want to delete and recreate them (y/n) ? ",
	        PACKAGE_NAME);
	    if (getyesno(0)) { 
		/* Try to remove the devices first */
		h2devEnd();
		if (h2devInit(smMemSize, posterServFlag) == ERROR) {
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

/*------------------------------------------x----------------------------*/

int
h2end(void)
{
    printf("Removing %s devices: ", PACKAGE_NAME);
    fflush(stdout);
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
   if (h2devClean(name) == ERROR) {
      h2perror("h2devClean");
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
    int status = ERROR;
    int posterServFlag = 0;
    int c;

    progname = argv[0];
    
    while ((c = getopt(argc, argv, "p")) != -1) {
	    switch (c) {
	    case 'p':
		    posterServFlag++;
		    break;
	    default: 
		    usage();
	    }
    }
    argc -= optind;
    argv += optind;

    /* Initialisation du systeme temps-reel, sans horloge */
    osInit(0);
    
    switch (argc) {
      case 1: 
	if (strcmp(argv[0], "init") == 0) {
	    /* Initialisation des devices h2 */
		status = h2init(SM_MEM_SIZE, posterServFlag);
	} else if (strcmp(argv[0], "end") == 0) {
	    /* Destruction des devices h2 */
	    status =  h2end();
	} else if (strcmp(argv[0], "info") == 0) {
	    /* Affichage des devices h2 */
	    status = h2info();
	} else if (strcmp(argv[0], "listModules") == 0) {
	    h2listModules();
	    status = OK;
	} else {
	    usage();
	}
	break;
      case 2:
	if (strcmp(argv[0], "init") == 0) {
	    status = h2init(atoi(argv[1]), posterServFlag);
	} else if (strcmp(argv[0], "printErrno") == 0) {
	    h2printErrno (strtol(argv[1],(char **)NULL, 0));
	    status = OK;
	} else if (strcmp(argv[0], "clean") == 0) {
	    status = cleanDevs(argv[1]);
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
 
