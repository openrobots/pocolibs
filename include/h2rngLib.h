/* $LAAS$ */
/*
 * Copyright (c) 1989, 2003 CNRS/LAAS
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

/*-------------- INDIQUER L'INCLUSION DE CE FICHIER -------------------------*/

#ifndef  H2_RNG_LIB_H
#define  H2_RNG_LIB_H


/****************************************************************************/
/*  LABORATOIRE D'AUTOMATIQUE ET D'ANALYSE DE SYSTEMES - LAAS / CNRS        */
/*  PROJET HILARE II - ROUTINES DE MANIPULATION DE RING BUFFERS (h2rngLib.c)*/
/*  FICHIER DE EN-TETE (h2rngLib.h)                                         */
/****************************************************************************/

/* VERSION ACTUELLE / HISTORIQUE DES MODIFICATIONS :
   version 1.1, mars89, Ferraz de Camargo, 1ere version;
*/

/* DESCRIPTION :
   Fichier en tete de la bibliotheque des routines de manipulation
   de ring buffers, version HILARE II  (inter-processeurs)
*/



/* Types des ring buffers */
#define  H2RNG_TYPE_BYTE       0            /* Type byte */
#define  H2RNG_TYPE_BLOCK      1            /* Type block */

/* Flag d'indication d'initialisation */
#define  H2RNG_INIT_BYTE        0x43215678
#define  H2RNG_INIT_BLOCK       0x32145678

/* Caractere d'indication de la fin d'un block */
#define  H2RNG_CAR_END   '$'


/* Identificateur de la bibliotheque */
#define   M_h2rngLib                    (501 << 16)

/* Erreurs */
#define  S_h2rngLib_ILLEGAL_NBYTES        (M_h2rngLib | 0)
#define  S_h2rngLib_ILLEGAL_TYPE          (M_h2rngLib | 1)
#define  S_h2rngLib_NOT_A_RING            (M_h2rngLib | 2)
#define  S_h2rngLib_NOT_A_BYTE_RING       (M_h2rngLib | 3)
#define  S_h2rngLib_NOT_A_BLOCK_RING      (M_h2rngLib | 4)
#define  S_h2rngLib_ERR_SYNC              (M_h2rngLib | 5)
#define  S_h2rngLib_SMALL_BUF             (M_h2rngLib | 6)
#define  S_h2rngLib_SMALL_BLOCK           (M_h2rngLib | 7)
#define  S_h2rngLib_BIG_BLOCK             (M_h2rngLib | 8)


/* Tete d'un ring buffer */
typedef struct {
  int flgInit;      /* Indicateur d'initialisation */
  int pRd;          /* Pointeur de lecture */
  int pWr;          /* Pointeur d'ecriture */
  int size;         /* Taille du ring buffer */
} H2RNG_HDR;

typedef H2RNG_HDR *H2RNG_ID;


/* RING macros */

/****************************************************************************
*
*  H2RNG_ELEM_GET - Prendre un caractere d'un ring type byte
*
*  Description : 
*  Cette macro prend 1 seul caractere du ring type byte. Il faut fournir
*  une variable type temporaire (register int) fromP.
*
*  Retourne : 1, s'il y a un caracter, 0 sinon
*/

#define H2RNG_ELEM_GET(ringId, pCh, fromP)                           \
      (                                                              \
       fromP = (ringId)->pRd,                                        \
       ((ringId)->pWr == fromP) ?                                    \
       0                                                             \
       :                                                             \
       (                                                             \
	*pCh = *((char *) ((ringId) + 1) + fromP),                   \
	(ringId)->pRd = ((++fromP == (ringId)->size) ? 0 : fromP),   \
	1                                                            \
	)                                                            \
       )


/***************************************************************************
*
*   H2RNG_ELEM_PUT - Mettre un caractere dans un ring buffer
*
*   Description : Met un caractere dans un ring buffer type "byte".
*   La variable toP doit etre fournie (register int) toP.
*
*   Retourne : 1 si le caractere a ete mis, 0 sinon (pas de place)
*/

#define H2RNG_ELEM_PUT(ringId, ch, toP)                 \
  (                                                     \
   toP = (ringId)->pWr,                                 \
   (toP == (ringId)->pRd - 1) ?                         \
   0                                                    \
   :                                                    \
   (                                                    \
    (toP == (ringId)->size - 1) ?                       \
    (                                                   \
     ((ringId)->pRd == 0) ?                             \
     0                                                  \
     :                                                  \
     (                                                  \
      *((char *) ((ringId) + 1) + toP) = ch,            \
      (ringId)->pWr = 0,                                \
      1                                                 \
      )                                                 \
     )                                                  \
    :                                                   \
    (                                                   \
     *((char *) ((ringId) + 1) + toP) = ch,             \
     (ringId)->pWr++,                                   \
     1                                                  \
     )                                                  \
    )                                                   \
   )



/*****************************************************************************
*
*   H2RNG_WHERE_IS - Determine la valeur actuelle du pointeur d'ecriture
*
*   Description : Retourne la valeur actuelle du pointeur d'ecriture.
*
*   Retourne : Valeur actuelle du pointeur d'ecriture (register int)
*/

#define H2RNG_WHERE_IS(ringId)                          \
  (                                                     \
   (ringId)->pWr                                        \
   )



/***************************************************************************
*
*   H2RNG_PUT_AHEAD - Mettre caractere dans ring buffer, sans avancer le
*   pointeur d'ecriture.
*
*   Description : Met un caractere dans un ring buffer type "byte".
*   La variable (register int) toP doit etre initialise au prealable
*   par H2RNG_WHERE_IS. H2RNG_PUT_AHEAD n'avance pas le pointeur d'ecriture.
*
*   Retourne : 1 si le caractere a ete mis, 0 sinon (pas de place)
*/

#define H2RNG_PUT_AHEAD(ringId, ch, toP)                \
  (                                                     \
   (toP == (ringId)->pRd - 1) ?                         \
   0                                                    \
   :                                                    \
   (                                                    \
    (toP == (ringId)->size - 1) ?                       \
    (                                                   \
     ((ringId)->pRd == 0) ?                             \
     0                                                  \
     :                                                  \
     (                                                  \
      *((char *) ((ringId) + 1) + toP) = ch,            \
      toP = 0,                                          \
      1                                                 \
      )                                                 \
     )                                                  \
    :                                                   \
    (                                                   \
     *((char *) ((ringId) + 1) + toP) = ch,             \
     toP++,                                             \
     1                                                  \
     )                                                  \
    )                                                   \
   )

/***************************************************************************
*
*   H2RNG_MOVE_AHEAD - Avancer le pointeur d'ecriture.
*
*   Description : Avancer le pointeur d'ecriture a la position donnee par
*   la variable (register int) toP, dont la valeur a ete modifiee par
*   H2RNG_PUT_AHEAD. 
*
*   Retourne : Valeur de toP
*/

#define H2RNG_MOVE_AHEAD(ringId, toP)                   \
  (                                                     \
   (ringId)->pWr = toP                                  \
   )



extern int h2rngBlockGet ( H2RNG_ID rngId, int *pidBlk, char *buf, int maxbytes );
extern int h2rngBlockPut ( H2RNG_ID rngId, int idBlk, char *buf, int nbytes );
extern STATUS h2rngBlockSkip ( H2RNG_ID rngId );
extern int h2rngBlockSpy ( H2RNG_ID rngId, int *pidBlk, int *pnbytes, char *buf, int maxbytes );
extern int h2rngBufGet ( H2RNG_ID rngId, char *buf, int maxbytes );
extern int h2rngBufPut ( H2RNG_ID rngId, char *buf, int nbytes );
extern H2RNG_ID h2rngCreate ( int type, int nbytes );
extern void h2rngDelete ( H2RNG_ID rngId );
extern STATUS h2rngFlush ( H2RNG_ID rngId );
extern int h2rngFreeBytes ( H2RNG_ID rngId );
extern BOOL h2rngIsEmpty ( H2RNG_ID rngId );
extern BOOL h2rngIsFull ( H2RNG_ID rngId );
extern int h2rngNBlocks ( H2RNG_ID rngId );
extern int h2rngNBytes ( H2RNG_ID rngId );


/*------------- fin de chargement du fichier --------------------------------*/
#endif
