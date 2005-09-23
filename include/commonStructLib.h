/* $LAAS$ */
/*
 * Copyright (c) 1991, 2003-2004 CNRS/LAAS
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
#ifndef  COMMON_STRUCT_H
#define  COMMON_STRUCT_H

/* DESCRIPTION :
   Bibliotheque de manipulation de structures de donnees communes, utiles
   pour le partage de donnees entre des taches d'une meme CPU.
*/

#include "semLib.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -- STRUCTURES ------------------------------------------------- */

/* Flag d'indication d'initialisation de la structure */
#define  COMMON_STRUCT_INIT_FLAG                  0x11223344

/* Flag d'indication de la direction de copie */
#define  TO                                       0
#define  FROM                                     1

/* Definition de type de l'en-tete des structures communes de donnees */
typedef struct {
  int initFlag;		/* initialization flag */
  int nBytes;		/* number of bytes in the structure */
  SEM_ID semId;		/* mutex semaphore for the structure */
  int unused;		/* for alignment */
} COMMON_STRUCT_HDR, *COMMON_STRUCT_ID;

/* -- ERRORS CODES ----------------------------------------------- */

#include "h2errorLib.h"
#define   M_commonStructLib    506

/* Codes d'erreur */
#define S_commonStructLib_ISNT_COMMON_STRUCT H2_ENCODE_ERR(M_commonStructLib, 0)

#define COMMON_STRUCT_LIB_H2_ERR_MSGS { \
 {"ISNT_COMMON_STRUCT", H2_DECODE_ERR(S_commonStructLib_ISNT_COMMON_STRUCT)},  \
}

extern const H2_ERROR commonStructLibH2errMsgs[]; /* = COMMON_STRUCT_LIB_H2_ERR_MSGS */

/* -- PROTOTYPES ------------------------------------------------- */

extern STATUS commonStructCopy ( void *pCommonStruct, int toFromFlag, 
    void *pBuf );
extern STATUS commonStructCreate ( int len, void **pStructAdrs );
extern STATUS commonStructDelete ( void *pCommonStruct );
extern STATUS commonStructGive ( void *pCommonStruct );
extern STATUS commonStructTake ( void *pCommonStruct );

#ifdef __cplusplus
};
#endif

#endif /* COMMON_STRUCT_H */
