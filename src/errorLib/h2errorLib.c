/*
 * Copyright (c) 1992, 2003 CNRS/LAAS
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
/****************************************************************************/
/*   LABORATOIRE D'AUTOMATIQUE ET D'ANALYSE DE SYSTEMES - LAAS / CNRS       */
/*   PROJET HILARE II - BIBLIOTHEQUE DES ERREURS                           */
/*   FICHIER D'EN-TETE "h2errorLib.c"                                      */
/****************************************************************************/
 
/* VERSION ACTUELLE / HISTORIQUE DES MODIFICATIONS :
   version 1.1; Dec92; sf; 1ere version;
*/
 
/* DESCRIPTION :
   Fichier d'en-tete de la bibliotheque de fonctions qui permettent 
   l'utilisation de l'agent locamotion par les autres agents du systeme,
   au moyen de messages.
*/

/*-------------------------- INCLUSIONS ------------------------------------*/
 
#include "config.h"
__RCSID("$LAAS$");

#ifdef VXWORKS
#include <vxWorks.h>
#else 
#include "portLib.h"
#endif

#include <stdio.h>
#include <string.h>

#include "errnoLib.h"
#include "h2errorLib.h"

/* Liste des erreurs obtenue par l'executable perl: h2findErrno.pl */
#include "h2errorList.h"


/*------------------- PROTOTYPES DES FONCTIONS INTERNES --------------------*/
 
static int findSourceId(int source);
static int findErrorId(int sourceId, int err);
 
/*------------------------- VARIABLES GLOBALES --------------------------*/
/* message d'erreur */
static char h2msgErrno[1024];

/*--------------------------------------------------------------------------*/

/*****************************************************************************
 *
 *  h2printErrno - 
 *
 *  Description: Affiche le message d'erreur correspondant au numero d'erreur
 *
 *  Retourne: rien
 */
 
void 
h2printErrno(int numErr)
{
  printf(h2getMsgErrno(numErr));
  printf("\n");

}

void 
h2perror(char *string)
{
  if (string && string[0])
    printf("%s: %s\n", string, h2getMsgErrno(errnoGet()));
  else
    printf("%s\n", h2getMsgErrno(errnoGet()));
}

/*****************************************************************************
 *
 *  h2listModules - 
 *
 *  Description: Affiche la liste des modules references.
 *
 *  Retourne: rien
 */
 
void 
h2listModules(void)
{
  int nbSources = (int) ( sizeof(H2_SOURCE_TAB_FAIL)
			 /sizeof(H2_SOURCE_FAILED_STRUCT) );
  int i;
  int sourceNum;
  int prev;

  printf ("\t -- Number attribution --\n"
          "\t 0              system\n"
	  "\t 5??            comLib\n"
	  "\t 6??            hardLib\n"
	  "\t 7?? 8?? 9??    modules H2\n"
	  "\t10??            modules MARTHA\n");

  prev = H2_SOURCE_TAB_FAIL[0].sourceNum;
  for (i=0; i<nbSources; i++) {
    sourceNum = H2_SOURCE_TAB_FAIL[i].sourceNum;
    if( sourceNum > 699 && 
	((sourceNum/10.) - (double)(int)(sourceNum/10) < 0.1000001) &&
	(int)(sourceNum-prev) != 1) {
      printf("%d  %s\n",   
	     H2_SOURCE_TAB_FAIL[i].sourceNum, 
	     H2_SOURCE_TAB_FAIL[i].sourceName);
      prev = sourceNum;
    }
  }
    
}

/*****************************************************************************
 *
 *  h2getMsgErrno - Obtention d'un message d'erreur
 *
 *  Description: Retourne un char* sur la chaine de caractere 
 *               correspondant au numero d'erreur. 
 *
 *  Retourne: rien
 */
 
char const * 
h2getMsgErrno(int numErr)
{
  int source;         /* Numero tache ou  bibliotheque origine de l'erreur */
  int erreur;         /* Numero de l'erreur */
  H2_FAILED_STRUCT_ID tabErr;
  int sourceId, errorId;

  if (numErr == 0) {
    sprintf (h2msgErrno, "OK");
    return(h2msgErrno);
  }

  /* Decode */
  source = H2_SOURCE_ERR(numErr);
  erreur = H2_NUMBER_ERR(numErr);
  
  /* 
   * Erreurs systeme
   */
  /* Erreurs standards (source == 0) */
  if (H2_SYS_ERR_FLAG(numErr)) {
    return(strerror(numErr));
  }

  /* Erreurs vxworks (source < 100) */
#ifndef UNIX
  /* Rem: si on est sur UNIX les erreurs vxworks sont recupere'es 
     par le tableau d'erreur h2 */
  if (H2_VX_ERR_FLAG(numErr)) 
    return(strerror(numErr));
#endif

      
  /* 
   * Autres erreurs (source > 100)
   */
  /* Recherche de la source */
  sourceId = findSourceId(source);
    
  /* Source inconnue */
  if (sourceId == -1) {
    sprintf(h2msgErrno, "source %d error %d (unknown source)", 
	    source, erreur);
    return(h2msgErrno);
  }
 
  /* Recherche de l'indice de l'erreur dans le tableau */
  errorId = findErrorId(sourceId, erreur);
  
  /* Erreur inconnue */
  if (errorId == -1) {
    sprintf (h2msgErrno, "%s: unknown error %d", 
	     H2_SOURCE_TAB_FAIL[sourceId].sourceName, erreur); 
    return(h2msgErrno);
  }
  
  /* On recupere le tableau d'erreurs */
  tabErr = H2_SOURCE_TAB_FAIL[sourceId].tabErrors;
    
  /* Message trouve */
  return(tabErr[errorId].errorName);
}


/*--------------------- FONCTIONS INTERNES ---------------------------------*/


/******************************************************************************
*
*  findSourceId - Recherche de l'indice de la source dans le tableau d'erreur
*
*  Description: 
*
*  Retourne: Indice de tableau ou -1
*/
 
static int findSourceId(int source)

{
  int nbSources = (int) ( sizeof(H2_SOURCE_TAB_FAIL)
			 /sizeof(H2_SOURCE_FAILED_STRUCT) );
  int i;

  /* Cherche de l'indice du tableau  correspondante a la source */
  for(i=0; i<nbSources; i++) 
    if (H2_SOURCE_TAB_FAIL[i].sourceNum == source) 
      return(i);
  
  return(-1);
  
}

/******************************************************************************
*
*  findErrorId - Recherche de l'indice de l'erreur dans le tableau d'erreur
*
*  Description: 
*
*  Retourne: Indice de tableau ou -1
*/
 
static int findErrorId(int sourceId, int error)

{
  int nbErr;
  int i;
  H2_FAILED_STRUCT_ID tabErr;

  /* Pointeur sur tableau d'erreurs de la source */
  tabErr = H2_SOURCE_TAB_FAIL[sourceId].tabErrors;
  nbErr = H2_SOURCE_TAB_FAIL[sourceId].nbErrors;

  /* Cherche l'indice du tableau  correspondant a la source */
  for(i=0; i<nbErr; i++) 
    if (tabErr[i].errorNum == error) 
      return(i);
  
  return(-1);
  
}



