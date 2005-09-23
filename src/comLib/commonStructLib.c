/*
 * Copyright (c) 1990, 2003-2005 CNRS/LAAS
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
/*   LABORATOIRE D'AUTOMATIQUE ET D'ANALYSE DE SYSTEMES - LAAS / CNRS        */
/*   PROJET HILARE II - BIBLIO DE MANIPULATION DE STRUCTURES COMMUNES        */
/*****************************************************************************/

/* VERSION ACTUELLE / HISTORIQUE DES MODIFICATIONS :
   version 1.1; mai90; gb, rfc, cl, gv; 1ere version;
*/

/* DESCRIPTION :
   Bibliotheque de manipulation de structures de donnees communes, utiles
   pour le partage de donnees entre des taches d'une meme CPU.
*/
#include "pocolibs-config.h"
__RCSID("$LAAS$");

#include "portLib.h"

#if defined(__RTAI__) && defined(__KERNEL__)
# include <linux/slab.h>
#else
# include <stdlib.h>
# include <string.h>
#endif

#include "errnoLib.h"

#include "commonStructLib.h"
static const H2_ERROR const commonStructLibH2errMsgs[] = COMMON_STRUCT_LIB_H2_ERR_MSGS;

#if defined(__RTAI__) && defined(__KERNEL__)
# define malloc(x)	kmalloc(x, GFP_KERNEL)
# define free(x)	kfree(x)
#endif

/* ROUTINES DISPONIBLES A L'UTILISATEUR : */
 
/* CREER UNE STRUCTURE DE DONNEES COMMUNE :
   Cette fonction permet a la tache appelante de creer une structure
   commune de donnees.

   STATUS commonStructCreate (len, pStructAdrs)
      int len;             * Taille de la structure commune allouee *
      void **pStructAdrs;  * Sortie: ou` mettre adresse de base structure *

   Retourne : OK ou ERROR

   Exemple:
   ...
   typedef struct {
       float x, y, theta;
   } POSITION;           * Def. de type structure stockage position robot *
   ...
   POSITION *pPosition;  * Ptr vers structure commune de position *
  ...
  commonStructCreate (sizeof (POSITION), &pPosition);
  ...
*/

/* PRENDRE L'ACCES A UNE STRUCTURE DE DONNEES COMMUNE :
   Cette routine permet de prendre le controle sur une structure de donnees
   commune, allouee auparavant au moyen de commonStructCreate. Apres avoir
   pris le controle, la tache appelante peut executer de cycles indivisibles
   de lecture/ecriture sur cette zone memoire (l'acces des autres taches etant
   exclus).

   STATUS commonStructTake (pCommonStruct)
      void *pCommonStruct;   * Adresse base structure *
                             * (retournee par commonStructCreate) *

   Retourne : OK ou ERROR

   Exemple: 
   ...
   typedef struct {
       float x, y, theta;
   } POSITION;           * Def. de type structure stockage position robot *
   ...
   POSITION *pPosition;  * Ptr vers structure commune de position *
  ...
  commonStructCreate (sizeof (POSITION), &pPosition);
  ...
  commonStructTake (pPosition);
  ...
  pPosition->x = 2.0;
  theta = pPosition->theta;
  ...
  commonStructGive (pPosition);    * Voir routine suivante *
  ...
*/

/* LIBERER L'ACCES A UNE STRUCTURE DE DONNEES COMMUNE :
   Cette routine permet de liberer l'acces a une structure de donnees commune.

   STATUS commonStructGive (pCommonStruct)
      void *pCommonStruct;   * Adresse base structure *
	                     * (retournee par commonStructCreate) *
   
   Retourne : OK ou ERROR

   Exemple: voir l'exemple precedent
*/

/* COPIER LE CONTENU D'UNE STRUCTURE DE DONNEES COMMUNE :
   Cette fonction permet de copier le contenu d'une structure de donnees
   commune sur un tampon fourni par l'utilisateur.

   STATUS commonStructCopy (pCommonStruct, toFromFlag, pBuf)
      void *pCommonStruct;   * Adresse base structure *
      int toFromFlag;        * Flag d'indication de direction de copie *
                             * TO ou FROM *
      void *pBuf;            * Ou` mettre les donnees copiees *

   Retourne: OK ou ERROR

   Remarque: Il n'est pas necessaire de faire commonStructTake avant!!!!
*/

/* SUPPRIMER UNE STRUCTURE DE DONNEES COMMUNE :
   Cette fonction permet de liberer la structure de donnees commune.

   VOID commonStructDelete (pCommonStruct)
      void *pCommonStruct;   * Adresse base structure *
                             * (retournee par commonStructCreate) *
*/


/******************************************************************************
*
*   commonStructCreate - Creer une structure de donnees commune
*
*   Description :
*   Cette fonction permet a la tache appelante de creer une structure
*   commune de donnees.
*
*   Retourne : OK ou ERROR
*/

STATUS 
commonStructCreate(int len,		/* Taille de la structure commune 
					   allouee */ 
		   void **pStructAdrs)	/* Ou` mettre adresse de base 
					   de la structure */ 
{
    char *pStrPool;	/* memory pool address */
    int poolSize;	/* memory pool size */
    SEM_ID semId;	/* mutex semaphore */

    /* record errors msg */
    h2recordErrMsgs("commonStructCreate", "commonStructLib", M_commonStructLib, 
		    sizeof(commonStructLibH2errMsgs)/sizeof(H2_ERROR), 
		    commonStructLibH2errMsgs);

    /* Obtenir la taille du pool de memoire */
    poolSize = sizeof (COMMON_STRUCT_HDR) + len;

    /* Allouer le pool de memoire */
    if ((pStrPool = malloc(poolSize)) == NULL)
	return ERROR;
    
    /* Create a mutex semaphore */
    if ((semId = semMCreate(0)) == NULL) {
	free(pStrPool);
	return ERROR;
    }
	
    /* Nettoyer la structure de donnees */
    memset(pStrPool, 0, poolSize);
    
    /* Stocker l'id du semaphore */
    ((COMMON_STRUCT_ID) pStrPool)->semId = semId;
    
    /* Garder le nombre de bytes de la structure allouee */
    ((COMMON_STRUCT_ID) pStrPool)->nBytes = len;
    
    /* Indiquer que la structure a ete cree (initialisee) */
    ((COMMON_STRUCT_ID) pStrPool)->initFlag = COMMON_STRUCT_INIT_FLAG;
    
    /* Stocker l'adresse de la premiere position utile structure */
    *pStructAdrs = pStrPool + sizeof (COMMON_STRUCT_HDR);
    return (OK);
}


/******************************************************************************
*
*  commonStructTake - Prendre l'acces a une structure de donnees commune
*
*  Description :
*  Cette routine permet de prendre le controle sur une structure de donnees
*  commune, allouee auparavant au moyen de commonStructCreate. Apres avoir
*  pris le controle, la tache appelante peut executer de cycles indivisibles
*  de lecture/ecriture sur cette zone memoire (l'acces des autres taches etant
*  exclus).
*
*  Retourne : OK ou ERROR
*/

STATUS 
commonStructTake(void *pCommonStruct)	/* Adresse de base de la structure */
{
    COMMON_STRUCT_ID strId;       /* Id de la structure */
    
    /* Obtenir l'adresse de base de la structure */
    strId = (COMMON_STRUCT_ID)pCommonStruct - 1;
    
    /* Verifier si la structure existe */
    if (strId->initFlag != COMMON_STRUCT_INIT_FLAG) {
	errnoSet (S_commonStructLib_ISNT_COMMON_STRUCT);
	return (ERROR);
    }
  
    /* Wait for mutex */
    semTake(strId->semId, WAIT_FOREVER);
    return (OK);
}



/******************************************************************************
*
*  commonStructGive - Liberer l'acces a une structure de donnees commune
*
*  Description :
*  Cette routine permet de liberer l'acces a une structure de donnees commune.
*
*  Retourne : OK ou ERROR
*/

STATUS 
commonStructGive(void *pCommonStruct)	/* Adresse de base de la structure */
{
    COMMON_STRUCT_ID strId;       /* Id de la structure */
    
    /* Obtenir l'adresse de base de la structure */
    strId = (COMMON_STRUCT_ID)pCommonStruct - 1;
    
    /* Verifier si la structure existe */
    if (strId->initFlag != COMMON_STRUCT_INIT_FLAG) {
	errnoSet(S_commonStructLib_ISNT_COMMON_STRUCT);
	return ERROR;
    }
    
    /* Signal mutex */
    semGive(strId->semId);
    return (OK);
}


/******************************************************************************
*
*  commonStructCopy - Copier le contenu d'une structure de donnees commune
*
*  Description:
*  Cette fonction permet de copier le contenu d'une structure de donnees
*  commune sur un tampon fourni par l'utilisateur.
*
*  Retourne: OK ou ERROR
*/

STATUS 
commonStructCopy(void *pCommonStruct, /* Adresse base structure */        
		 int toFromFlag,      /* Flag de direction de copie */    
		 void *pBuf)	      /* Ou` mettre les donnees copiees */
{
    COMMON_STRUCT_ID strId;       /* Id de la structure */
    
    /* Obtenir l'adresse de base de la structure */
    strId = (COMMON_STRUCT_ID)pCommonStruct - 1;
    
    /* Verifier si la structure existe */
    if (strId->initFlag != COMMON_STRUCT_INIT_FLAG) {
	errnoSet(S_commonStructLib_ISNT_COMMON_STRUCT);
	return ERROR;
    }
    
    /* Wait for mutex */
    semTake(strId->semId, WAIT_FOREVER);
    
    /* Effectuer le transfert des donnees (en fonction de la direction) */
    if (toFromFlag == TO)
	memcpy (pBuf, pCommonStruct, strId->nBytes);
    else memcpy (pCommonStruct, pBuf, strId->nBytes);
    
    /* Signal mutex */
    semGive(strId->semId);
    return OK;
}



/******************************************************************************
*
*  commonStructDelete - Supprimer une structure de donnees commune
*
*  Description :
*  Cette fonction permet de liberer une structure de donnees commune.
*
*  Retourne : OK ou ERROR
*/

STATUS 
commonStructDelete(void *pCommonStruct) /* Adresse de base de la structure */
{
    COMMON_STRUCT_ID strId;       /* Id de la structure */
    
    /* Obtenir l'adresse de base de la structure */
    strId = (COMMON_STRUCT_ID)pCommonStruct - 1;
    
    /* Verifier si la structure existe */
    if (strId->initFlag != COMMON_STRUCT_INIT_FLAG) {
	errnoSet (S_commonStructLib_ISNT_COMMON_STRUCT);
	return (ERROR);
    }
    
    /* Indiquer que la structure commune de donnees a ete supprimee */
    strId->initFlag = FALSE;
    
    /* Destroy mutex semaphore */
    semDelete(strId->semId);
    
    /* Liberer le pool de memoire et retourner */
    free((char *) strId);
    return (OK);
}


