/* $LAAS$ */
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

/*-------------- INDIQUER L'INCLUSION DE CE FICHIER ------------------------*/
 
#ifndef  H2_ERROR_LIB_H
#define  H2_ERROR_LIB_H

/****************************************************************************/
/*   LABORATOIRE D'AUTOMATIQUE ET D'ANALYSE DE SYSTEMES - LAAS / CNRS       */
/*   PROJET HILARE II - TRAITEMENT DES ERREURS                              */
/*   FICHIER D'EN-TETE "h2errorLib.h"                                       */
/****************************************************************************/
 
/* VERSION ACTUELLE / HISTORIQUE DES MODIFICATIONS :
   version 1.1; Dec92; sf;
*/

#include <errnoLib.h>

#ifdef __cplusplus
extern "C" {
#endif


/* 
 * Macros pour decoder un code d'erreur "a la VxWorks"
 */
#define H2_SOURCE_ERR(numErr)      (numErr >= 0 ? (numErr)>>16 : numErr/100*100)
#define H2_NUMBER_ERR(numErr)      (numErr >= 0 ? ((numErr)<<16)>>16 : - numErr + numErr/100*100)

/* 
 * Macros pour tester l'origine d'une erreur:
 *
 *   NUMERO SOURCE:         SOURCE:
 *         n < 0              comLib UNIX  (old version)
 *         n = 0              system
 *     n/100 = 5              comLib
 *     n/100 = 6              hardLib
 *     n/100 > 6              modules
 *     n/100 = 10             modules MARTHA
 *
 */
#define H2_SYS_ERR_FLAG(numErr)                 /* Erreur systeme */ \
 (numErr > 0 && ((numErr)>>16) == 0 ? TRUE : FALSE)  
#define H2_VX_ERR_FLAG(numErr)                  /* Erreur vxworks */ \
 (numErr > 0 && ((numErr)>>16) > 0 && ((numErr)>>16)/100 < 1 ? TRUE : FALSE)
#define H2_COM_VX_ERR_FLAG(numErr)              /* Erreur comLib sur VxW */\
 (numErr >= 0 && ((numErr)>>16)/100 == 5 ? TRUE : FALSE)
#define H2_COM_UNIX_ERR_FLAG(numErr)            /* Erreur comLib sur UNIX */\
 (numErr > -700 && numErr <= -100  ? TRUE : FALSE)
#define H2_HARD_ERR_FLAG(numErr)                /* Erreur hardLib */\
 (numErr >= 0 && ((numErr)>>16)/100 == 6 ? TRUE : FALSE)
#define H2_MODULE_ERR_FLAG(numErr)              /* Erreur modules */\
 (numErr >= 0 && ((numErr)>>16)/100 > 6 ? TRUE : FALSE)
#define H2_MARTHA_ERR_FLAG(numErr)              /* Erreur modules martha */\
 (numErr >= 0 && ((numErr)>>16)/100 == 10 ? TRUE : FALSE)

/*
 * Tableau de code d'erreur
 */
typedef struct {
  char *errorName;           /* Nom de l'erreur */
  int errorNum;              /* Numero de l'erreur */
} H2_FAILED_STRUCT, *H2_FAILED_STRUCT_ID;

/*
 * Tableau des sources d'erreurs (ie bibliotheque ou module)
 */
typedef struct {
  char *sourceName;                /* Nom de la source */
  int sourceNum;                   /* Numero de la source */
  H2_FAILED_STRUCT_ID tabErrors;   /* Pointeur sur son tableau des erreurs */
  int nbErrors;                    /* Nombre d'erreurs associees */
} H2_SOURCE_FAILED_STRUCT, *H2_SOURCE_FAILED_STRUCT_ID;
  

/*---------------- PROTOTYPES DES FONCTIONS EXTERNES -----------------------*/

void h2printErrno(int numErr);
void h2perror(char *string);
char const * h2getMsgErrno(int numErr);

/*************************************************************
 * DESCRIPTION de h2printErrno
 *
 * Cette fonction permet de decoder un numero d'erreur et d'afficher le 
 * message correspondant s'il s'agit d'une erreur de module repertorie'e 
 * dans h2errorList.h. S'il s'agit d'une erreur systeme, elle fait appele 
 * aux fonctions specifiques.
 *
 * Pour etre repertorier les erreurs de H2 doivent etre definies selon le 
 * format VxWorks. Exemple:
 *
 * #define   M_locoMsgLib                       (704 << 16)
 * #define   S_locoMsgLib_BAD_GEO_DATA          (M_locoMsgLib | 2)
 *
 * Pour les repertorier effectivement, il faut appeler l'executable perl
 * 'h2addErrno' avec en parametre l'ensemble des fichiers (*.h) codant les 
 * modules (voir exemple dans README).
 */ 

/*--------------------- fin de chargement du fichier ----------------------*/
 
#ifdef __cplusplus
};
#endif

#endif
