/*
 * Copyright (c) 1992, 2005, 2011,2014 CNRS/LAAS
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

/* ----------------------- INCLUSIONS ------------------------------- */

#define VERBOSE 0

#include "pocolibs-config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "portLib.h"
#include "errnoLib.h"
#include "h2errorLib.h"

/* ---------------------------- STRUCTURES  ----------------------------- */
/* Description of the errors of one module or lib */
typedef struct H2_MODULE_ERRORS {
  char *name;  /* name of the module */
  int id;         /* id of the module (must be unique) */
  int nbErrors;   /* number of errors for this module */
  const H2_ERROR *errorsArray;  /* array of the errors */
} H2_MODULE_ERRORS;

typedef struct H2_MOD_ERRORS_LIST {
  H2_MODULE_ERRORS modErrors;
  struct H2_MOD_ERRORS_LIST *next;
} H2_MOD_ERRORS_LIST;

/* ------------------------- GLOBAL VARIABLES  -------------------------- */
/* the list of the recorded errors */
static H2_MOD_ERRORS_LIST *modErrorsList;

/* ----------------- PROTOTYPES OF INTERNAL FUNCTIONS -------------------- */

static const H2_ERROR * findErrorId(const H2_MODULE_ERRORS *modErrors, short error);
static const H2_MODULE_ERRORS * findSourceId(short source);
static H2_MOD_ERRORS_LIST * allocModErrorsElt(const char *moduleName, int moduleId,
					      int nbErrors,
					      const H2_ERROR errors[]);

/* -----------------------------------------------------------------
 *
 * h2recordErrMsgs - Record a new *H2_MODULE_ERRORS
 *
 *
 * returns : 1 if recorded, or else 0
 */
int
h2recordErrMsgs(const char *bywho,
		const char *moduleName, short moduleId,
		int nbErrors, const H2_ERROR errors[])
{
  H2_MOD_ERRORS_LIST *last=modErrorsList;
  H2_MOD_ERRORS_LIST *prev;
  H2_MOD_ERRORS_LIST *smaller=NULL, *tmp;
  int sameId, sameName;

  /* create list */
  if (!modErrorsList) {
    if (!(modErrorsList=allocModErrorsElt(moduleName, moduleId,
					  nbErrors, errors))) {
      printf ("h2recordErrMsgs by %-20s error: cannot alloc errors\n",
	      bywho?bywho:"?");
      return 0;
    }
#if VERBOSE > 1
    printf("h2recordErrMsgs by %-20s: module id  %5d  M_%-16s\n",
	   bywho?bywho:"?", moduleId, moduleName);
#endif
    return 1;
  }

  /* look for last elt */
  prev = last;
  do {
    sameId = last->modErrors.id == moduleId;
    sameName = strcmp(last->modErrors.name, moduleName) ? 0 : 1;

    /* id already recorded */
    if (sameId) {
      if (sameName) {
#if VERBOSE > 0
	printf ("h2recordErrMsgs by %-20s info: M_%s (%d) already recorded\n",
		bywho?bywho:"?", last->modErrors.name, last->modErrors.id);
#endif
      }
      else {
	printf ("h2recordErrMsgs by %-20s error: %d already recorded for M_%s\n",
		bywho?bywho:"?", last->modErrors.id, last->modErrors.name);
      }
      return 1;
    }

    /* name already recorded */
    else if (sameName) {
      printf ("h2recordErrMsgs by %-20s warning: M_%s already recorded with id %d\n",
	      bywho?bywho:"?", last->modErrors.name, last->modErrors.id);
    }

    /* test if number smaller */
    if (last->modErrors.id < moduleId) {
      smaller = last;
    }
    prev = last;
    last = last->next;
  }while (last);

  /* record new elt */
  if (!(tmp=allocModErrorsElt(moduleName, moduleId, nbErrors, errors)))
    return 0;

  /* insert in list */
  if (!smaller) {
    tmp->next = modErrorsList;
    modErrorsList = tmp;
  }
  else {
    tmp->next = smaller->next;
    smaller->next = tmp;
  }

#if VERBOSE > 1
  printf("h2recordErrMsgs by %-20s: module id  %5d  M_%-16s\n",
	 bywho?bywho:"?", moduleId, moduleName);
#endif

  return 1;
}

/* -----------------------------------------------------------------
 *
 * allocModErrorsElt
 *
 */
static H2_MOD_ERRORS_LIST *
allocModErrorsElt(const char *moduleName, int moduleId,
		  int nbErrors, const H2_ERROR errors[])
{
  H2_MOD_ERRORS_LIST *elt;

  if (!(elt=malloc(sizeof(H2_MOD_ERRORS_LIST)))) {
    printf ("h2recordErrMsgs: cannot alloc errors\n");
    return NULL;
  }
  elt->modErrors.name = strdup(moduleName);
  elt->modErrors.id = moduleId;
  elt->modErrors.nbErrors = nbErrors;
  elt->modErrors.errorsArray = errors;
  elt->next = NULL;
  return elt;
}

/* --------------------------------------------------------------------
 *
 *  h2printErrno -
 *
 *  Description: print message corresponding to the error number
 *
 *  Returns: nothing
 */

void
h2printErrno(int numErr)
{
  char string[256];
  logMsg(h2getErrMsg(numErr, string, 256));
  logMsg("\n");
}

/* --------------------------------------------------------------------
 *
 *  h2perror - like perror for the errors recorded with h2addErrno
 *
 *  Returns: nothing
 */


void
h2perror(const char *inString)
{
  char outString[256];

  if (inString && inString[0])
    logMsg("%s: %s\n", inString, h2getErrMsg(errnoGet(), outString, 256));
  else
    logMsg("%s\n", h2getErrMsg(errnoGet(), outString, 256));
}

/* -----------------------------------------------------------------
 *
 *  h2getMsgErrno - fillin string with error msg
 *
 *  OBSOLET and NON-REENTRANT !!
 *  Please, now use h2getErrMsg().
 *
 *  Returns: string
 */

char * h2getMsgErrno(int fullError)
{
  static char msg[256];

  h2getErrMsg(fullError, msg, 256);
  return msg;
}

/* -----------------------------------------------------------------
 *
 *  h2getErrMsg - fillin string with error msg
 *
 *  Returns: string
 */

char * h2getErrMsg(int fullError, char *string, int maxLength)
{
  short numErr;     /* err code (if not std) */
  short source;     /* lib or module that has emit the err */
  short srcStd;     /* lib or module that has defined the std err (or 0) */
  short errStd;     /* stdErr (or 0) */
  const H2_MODULE_ERRORS *modErrors;
  const H2_MODULE_ERRORS *modStdErrors;
  const H2_ERROR *error;

  /* unknown error */
  if (fullError == 0) {
    snprintf (string, maxLength, "Unknown error");
    return(string);
  }

  /* decode error */
  source = h2decodeError(fullError, &numErr, &srcStd, &errStd);

  /* find out module source */
  if (!(modErrors = findSourceId(source))) {
    if (numErr<0)
      snprintf (string, maxLength, "S_%d_std%d_%d", source, srcStd, errStd);
    else
      snprintf (string, maxLength, "S_%d_%d", source, numErr);
    return(string);
  }

  /* find out standard error */
  if (numErr<0) {

    /* find out std source */
    if (!(modStdErrors = findSourceId(srcStd))) {
      snprintf (string, maxLength, "S_%s_std%d_%d",
		modErrors->name, srcStd, errStd);
      return(string);
    }
    /* find out std err */
    if(!(error = findErrorId(modStdErrors, numErr))) {
      snprintf (string, maxLength, "S_%s_%s_%d",
		modErrors->name, modStdErrors->name, errStd);
      return(string);
    }
    snprintf (string, maxLength, "S_%s_%s_%s",
	      modErrors->name, modStdErrors->name, error->name);
    return(string);
  }

  /* Look for error within given source */
  if (!(error = findErrorId(modErrors, numErr))) {
    snprintf (string, maxLength, "S_%s_%d", modErrors->name, numErr);
    return(string);
  }

  /* Find out */
  snprintf (string, maxLength, "S_%s_%s", modErrors->name, error->name);
  return(string);
}

/* -----------------------------------------------------------------
 *
 * h2decodeError  -
 *     returns M_lib or M_stdLib, the lib that has emit the error
 *     *err is the error code or the still composed std error
 *     *srcStd is the lib or module that has defined the std err (or 0)
 *     *errStd is the emited std err (if any)
 *
 */
short h2decodeError(int fullError,
		    short *err,
		    short *srcStd,
		    short *errStd)
{
  short srcEmit;
  srcEmit = H2_SOURCE_ERR(fullError);
  *err = H2_NUMBER_ERR(fullError);
  if (*err<0) {
    *srcStd = H2_SOURCE_STD_ERR(*err);
    *errStd = H2_NUMBER_STD_ERR(*err);
  }
  else {
    *srcStd = 0;
    *errStd = 0;
  }
  return srcEmit;
}

/* -----------------------------------------------------------------
 *
 * h2listModules
 *
 */
void h2listModules(void)
{
  H2_MOD_ERRORS_LIST *elt=modErrorsList;
  while(elt) {
    printf ("Module id  %5d  M_%-16s  (%2d errors)\n",
	    elt->modErrors.id, elt->modErrors.name, elt->modErrors.nbErrors);
    elt=elt->next;
  }
}

/* -----------------------------------------------------------------
 *
 * h2listErrors
 *
 */
void h2listErrors(void)
{
  H2_MOD_ERRORS_LIST *elt=modErrorsList;
  int i;

  while(elt) {
    printf ("Module id  %5d  M_%-16s  (%2d errors)\n",
	    elt->modErrors.id, elt->modErrors.name, elt->modErrors.nbErrors);
    for (i=0; i<elt->modErrors.nbErrors; i++) {
      printf ("    %2d %s \n",
	      elt->modErrors.errorsArray[i].num,
	      elt->modErrors.errorsArray[i].name);
    }
    elt=elt->next;
  }
}


/*--------------------- FONCTIONS INTERNES ---------------------------------*/


/* --------------------------------------------------------------------
 *
 *  findSourceId - Recherche de l'indice de la source dans le tableau d'erreur
 *
 *  Description:
 *
 *  Returns: Indice de tableau ou -1
 */

static const H2_MODULE_ERRORS * findSourceId(short source)

{
  H2_MOD_ERRORS_LIST *elt=modErrorsList;
  while(elt) {
    if (elt->modErrors.id == source)
      return &elt->modErrors;
    elt = elt->next;
  }
  return NULL;
}

/* --------------------------------------------------------------------
 *
 *  findErrorId - Recherche de l'indice de l'erreur dans le tableau d'erreur
 *
 *  Description:
 *
 *  Returns: Indice de tableau ou -1
 */

static const H2_ERROR * findErrorId(const H2_MODULE_ERRORS *modErrors, short error)
{
  int i;
  for (i=0; i<modErrors->nbErrors; i++) {
    if (modErrors->errorsArray[i].num == error)
      return &(modErrors->errorsArray[i]);
  }
  return NULL;
}
