/*
 * Copyright (c) 2004 
 *      Autonomous Systems Lab, Swiss Federal Institute of Technology.
 * Copyright (c) 1990, 2003-2004 CNRS/LAAS
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
/*  LABORATOIRE D'AUTOMATIQUE ET D'ANALYSE DE SYSTEMES - LAAS / CNRS        */
/*  PROJET HILARE II - ROUTINES DE MANIPULATION DE RING BUFFERS (h2rngLib.c)*/
/****************************************************************************/

/* DESCRIPTION :
   Bibliotheque des routines de manipulation de ring buffers, 
   version HILARE II  (inter-processeurs). OBS.: MAINTENIR TOUJOURS
   L'ALIGNEMENT PAR LONG-WORDS!
*/

#include "pocolibs-config.h"
__RCSID("$LAAS$");

#ifdef VXWORKS
#include <vxWorks.h>
#else
#include <portLib.h>
#endif

#if defined(__RTAI__) && defined(__KERNEL__)
# include <linux/kernel.h>
# include <linux/sched.h>
# include <asm/uaccess.h>
#else
# include <sys/types.h>
# include <stdlib.h>
# include <string.h>
#endif

#include <taskLib.h>
#include <errnoLib.h>
#include <smMemLib.h>
#include <h2rngLib.h>

#define COMLIB_DEBUG_H2RNGLIB

#ifdef COMLIB_DEBUG_H2RNGLIB
# define LOGDBG(x)	logMsg x
#else
# define LOGDBG(x)
#endif


/*****************************************************************************
*
*   H2RNG_WR1  -  Ecrire buffer sur ring, sans retour au debut
*
*   Description :
*   Ecrit directement un buffer sur le ring, sans le couper en deux morceaux.
*
*   Retourne :
*   Neant
*/

#define H2RNG_WR1(pTo, buf, nbytes)	\
  (					\
   memcpy (pTo, buf, nbytes),		\
   pTo = pTo + nbytes                   \
   )                                                   


/*****************************************************************************
*
*   H2RNG_WR2 -  Ecrire buffer sur ring, en deux morceaux
*
*   Description :
*   Ecrit le contenu du buffer su le ring, en le coupant si necessaire
*   en deux morceux (retour au debut du ring).
*
*   Retourne : Neant
*/

#define H2RNG_WR2(pTo, buf, nbytes, ntop, n, pDeb)	\
  (                                                     \
   (ntop == -1) ?                                       \
   (                                                    \
      H2RNG_WR1(pTo, buf, nbytes)			\
    )                                                   \
   :                                                    \
   (                                                    \
    (ntop <= nbytes) ?                                  \
    (                                                   \
     memcpy (pTo, buf, ntop),				\
     n = nbytes - ntop,                                 \
     memcpy (pDeb, buf + ntop, n),			\
     ntop = -1,                                         \
     pTo = pDeb + n                                     \
     )                                                  \
    :                                                   \
    (                                                   \
     memcpy (pTo, buf, nbytes),				\
     ntop = ntop - nbytes,                              \
     pTo = pTo + nbytes                                 \
     )                                                  \
    )                                                   \
   )


/****************************************************************************
*
*   BLK_WR1  -  Ecrire un block sur le ring (1 seul morceau)
*
*   Description :
*   Ecrit un block sur le ring, sans retour au debut du ring (1 seul morceau).
*   Sont ecrits : nombre de bytes du message, id du block, le message 
*   de l'utilisateur et le caractere de fin.
*
*   Retourne : Neant
*/

#define BLK_WR1(pTo, idBlk, buf, nbytes)		\
  (							\
   H2RNG_WR1(pTo, (char *) &nbytes, sizeof(nbytes)),	\
   H2RNG_WR1(pTo, (char *) &idBlk, sizeof(idBlk)),	\
   H2RNG_WR1(pTo, buf, nbytes),				\
   *pTo = H2RNG_CAR_END					\
   )



/*****************************************************************************
*
*   BLK_WR2  -  Ecrire un block sur le ring (2 morceaux)
*
*   Description :
*   Ecrit un block sur le ring, si necessaire en le coupant en deux morceux.
*   Sont ecrits : nombre de bytes du message, id du block, le message
*   de l'utilisateur et le caractere de fin.
*
*   Retourne : Neant
*/

#define BLK_WR2(pTo, idBlk, buf, nbytes, ntop, pDeb)			\
  (									\
   H2RNG_WR2(pTo, (char *)&nbytes, sizeof(nbytes), ntop, n, pDeb),	\
   H2RNG_WR2(pTo, (char *)&idBlk, sizeof(idBlk), ntop, n, pDeb),	\
   H2RNG_WR2(pTo, buf, nbytes, ntop, n, pDeb),				\
   *pTo = H2RNG_CAR_END							\
   )



/*****************************************************************************
*
*   H2RNG_RD1  -  Lire un buffer sur le ring (1 morceau) 
*
*   Description :
*   Lit un nombre donne de bytes sur le ring (sans se preoccuper avec
*   le retour au debut du ring)
*
*   Retourne : Neant
*/

#define H2RNG_RD1(pFrom, buf, nbytes)			\
  (                                                     \
   memcpy (buf, pFrom, nbytes),				\
   pFrom = pFrom + nbytes                               \
   )                                                                     



/*****************************************************************************
*   H2RNG_RD2  -  Lire un buffer sur le ring (2 morceaux)
*
*   Description :
*   Lit un nombre donne de bytes sur le ring et les met sur un buffer
*   fourni par l'utilisateur. Si necessaire, l'ecriture sur ce buffer
*   est effectuee en 2 morceaux.
*
*   Retourne : Neant
*/

#define H2RNG_RD2(pFrom, buf, nbytes, ntop, n, pDeb)	\
  (                                                     \
   (ntop == -1) ?                                       \
   (                                                    \
    H2RNG_RD1(pFrom, buf, nbytes)			\
    )                                                   \
   :                                                    \
   (                                                    \
    (ntop <= nbytes) ?                                  \
    (                                                   \
     memcpy (buf, pFrom, ntop),				\
     n = nbytes - ntop,                                 \
     memcpy (buf + ntop, pDeb, n),			\
     ntop = -1,                                         \
     pFrom = pDeb + n                                   \
     )                                                  \
    :                                                   \
    (                                                   \
     memcpy (buf, pFrom, nbytes),			\
     ntop = ntop - nbytes,                              \
     pFrom = pFrom + nbytes                             \
     )                                                  \
    )                                                   \
   )


/*****************************************************************************
*
*   h2rngCreate  -  Creation d'un ring buffer inter-processeurs
*
*   Description :
*   Cree et initialise un ring buffer de taille fournie par l'utilisateur.
* 
*   Retourne : identificateur du ring buffer ou NULL
*/

H2RNG_ID 
h2rngCreate(int type,			/* Type du ring buffer */
	    int nbytes)			/* Nombre de bytes */
{
    H2RNG_ID rngId;             /* Pointeur vers tete */
    int flgInit;                /* Flag d'initialisation */
    
    /* Verifier si le nombre de bytes est positif */
    if (nbytes <= 0) {
	errnoSet (S_h2rngLib_ILLEGAL_NBYTES);
	return ((H2RNG_ID) NULL);
    }

    /* Verifier le type demande de ring buffer */
    switch (type) {
      case H2RNG_TYPE_BYTE:           /* Ring buffer type byte */
	
	/* Taille du ring buffer */
	nbytes = nbytes + 1;
	
	/* Indiquer l'initialisation */
	flgInit = H2RNG_INIT_BYTE;
	break;
	
      case H2RNG_TYPE_BLOCK:         /* Ring buffer type "block" */
	
	/* Taille du ring buffer */
	nbytes = nbytes + 12 - (nbytes & 3);
	
	/* Indiquer l'initialisation */
	flgInit = H2RNG_INIT_BLOCK;
	break;
	
      default:                       /* Types inconnus */

	/* Indiquer l'erreur */
	errnoSet (S_h2rngLib_ILLEGAL_TYPE);
	return ((H2RNG_ID) NULL);
    } /* switch */
    
    /* Allouer memoire pour l'en-tete et pour le buffer */
    if ((rngId = (H2RNG_ID) 
	 smMemMalloc ((u_int) (nbytes + sizeof (H2RNG_HDR)))) == NULL) {
	return ((H2RNG_ID) NULL);
    }
    
    /* Initialiser l' en-tete */
    rngId->pRd = 0;
    rngId->pWr = 0;
    rngId->size = nbytes;
    rngId->flgInit = flgInit;
    
    /* Retourner le pointeur vers le ring buffer */
    return (rngId);
}



/*****************************************************************************
*
*    h2rngFlush  -  Nettoyer un ring buffer
*
*    Description :
*    Vide le ring buffer. Les donnees existentes sont perdues.
*
*    Retourne : OK ou ERROR
*/

STATUS 
h2rngFlush(H2RNG_ID rngId)		/* Identificateur du ring buffer */
{
    /* Retourner, si ring buffer non-initialise */
    if (rngId == NULL || (rngId->flgInit != H2RNG_INIT_BYTE &&
	rngId->flgInit != H2RNG_INIT_BLOCK)) {
	errnoSet (S_h2rngLib_NOT_A_RING);
	return (ERROR);
    }
    
    /* Reseter les pointeurs */
    rngId->pRd = 0;
    rngId->pWr = 0;
    return (OK);
}


/*****************************************************************************
*
*    h2rngBufGet  -  Prendre des caracteres a partir d'un ring buffer
*
*    Description :
*    Copie les caracteres du ring buffer sur le buffer de l'utilisateur.
*    Copie autant de bytes qu'il y a sur le ring buffer, jusqu'au nombre
*    max. defini par l'utilisateur. Les bytes lus sont enleves du ring buffer.
*
*    Retourne : nombre de bytes effectivement lus.
*/

int 
h2rngBufGet(H2RNG_ID rngId,		/* Identificateur du ring buffer */
	    char *buf,			/* Pointeur vers buffer utilisateur */
	    int maxbytes)		/* Nombre max. de bytes a lire */
{
    int n1, n2, nbytes;                          
    int pWr, pRd;                 
    
    /* Retourner, si ring buffer non-initialise */
    if (rngId == NULL || rngId->flgInit != H2RNG_INIT_BYTE) {
	errnoSet (S_h2rngLib_NOT_A_BYTE_RING);
	return (ERROR);
    }

    /* Valeurs congelees des pointeurs d'ecriture et de lecture */
    pWr = rngId->pWr;
    pRd = rngId->pRd;
    
    /* Verifier si on doit faire la copie en deux morceaux */
    if ((n1 = pWr - pRd) < 0) {
	/* Nombre de bytes a copier = min (maxBytes, nombre pos. occupees) */
	nbytes = min (maxbytes, n1 + rngId->size);
	
	/* Taille du premier morceau occupe */
	n1 = rngId->size - pRd;
	
	/* Verifier si plus grand que nombre de bytes a lire */
	if (n1 > nbytes) {
	    /* Faire une seule copie */
	    memcpy (buf, (char *) (rngId+1) + pRd, nbytes);
	    
	    /* Actualiser pointeur de lecture et retourner */
	    rngId->pRd = pRd + nbytes;
	    return (nbytes);
	}
	
	/* Copier tout le premier morceau */
	memcpy (buf, (char *) (rngId+1) + pRd, n1);
	
	/* Taille du deuxieme morceau */
	n2 = nbytes - n1;
	
	/* Copier le deuxieme morceau et retourner */
	memcpy (buf+n1, (char *) (rngId+1), n2);
	
	/* Actualiser le pointeur de lecture et retourner */
	rngId->pRd = n2;
	return (n1 + n2);
    }
    
    /* Copier tout, d'une seule fois */
    memcpy (buf, (char *) (rngId+1) + pRd, n1 = min (maxbytes, n1));
    
    /* Actualiser le pointeur de lecture et retourner */
    rngId->pRd = pRd + n1;
    return (n1);
}



/******************************************************************************
*
*   h2rngBufPut  -  Ecrire des bytes sur le ring buffer
*
*   Description :
*   Copie le contenu du buffer de l'utilisateur sur le ring buffer.
*   Le nombe specifie de bytes va etre mis dans le ring buffer, s'il y a
*   de la place.
*
*   Retourne :
*   Nombre de bytes effectivement copies.
*/

int 
h2rngBufPut(H2RNG_ID rngId, /* Ring buffer ou mettre les bytes */    
	    char *buf, 	    /* Buffer a copier */                    
	    int nbytes)	    /* Nombre de bytes a essayer de copier */
{
    int n1, n2;                          
    int pWr, pRd;                 
    
    /* Retourner, si ring buffer non-initialise */
    if (rngId == NULL || rngId->flgInit != H2RNG_INIT_BYTE) {
	errnoSet (S_h2rngLib_NOT_A_BYTE_RING);
	return (ERROR);
    }
    
    /* Verifier si le nombre de bytes est positif */
    if (nbytes <= 0) {
	errnoSet (S_h2rngLib_ILLEGAL_NBYTES);
	return (ERROR);
    }
    
    /* Valeurs congelees des pointeurs d'ecriture et de lecture */
    pWr = rngId->pWr;
    pRd = rngId->pRd;
    
    /* Verifier l'etat des pointeurs */
    if ((n1 = pRd - pWr) <= 0) {
	/* Nombre de bytes a ecrire = min (pos.libres, nbytes) */
	nbytes = min (n1 + rngId->size - 1, nbytes);
	
	/* Taille du premier morceau libre */
	n1 = rngId->size - pWr;
	
	/* Verifier si plus grand que le nombre de bytes a ecrire */
	if (n1 > nbytes) {
	    /* Faire une seule copie */
	    memcpy ((char *) (rngId+1) + pWr, buf, nbytes);
	    
	    /* Actualiser pointeur d'ecriture et retourner */
	    rngId->pWr = pWr + nbytes;
	    return (nbytes);
	}
	
	/* Copier tout le premier morceau */
	memcpy ((char *) (rngId+1) + pWr, buf, n1);
	
	/* Taille du deuxieme morceau */
	n2 = nbytes - n1;
	
	/* Copier le deuxieme morceau et retourner */
	memcpy ((char *) (rngId+1), buf+n1, n2);
	
	/* Actualiser le pointeur d'ecriture et retourner */
	rngId->pWr = n2;
	return (nbytes);
    } 

    /* Copier tout, d'une seule fois */
    memcpy ((char *) (rngId+1) + pWr, buf, nbytes = min (nbytes, n1 - 1));
    
    /* Actualiser le pointeur d'ecriture et retourner */
    rngId->pWr = pWr + nbytes;
    return (nbytes);
}



/*****************************************************************************
*
*   h2rngBlockPut  -  Ecrire un block de caracteres sur un ring buffer
*
*   Description :
*   Copie le contenu du buffer de l'utilisateur sur le ring buffer, sous
*   le format de "block". Le overhead associe est de 9 ou 10 bytes, dependant
*   de la taille du message. Le en-tete du message consiste du nombre de
*   bytes du message, suivi d'un identificateur de block (fourni par 
*   l'utilisateur).
*
*   Retourne :
*   Taille du buffer de l'utilisateur, 0 s'il n'y a pas de place, ERROR si
*   pas un ring buffer type block
*/
 
int 
h2rngBlockPut(H2RNG_ID rngId,       /* Ring buffer ou mettre les bytes */    
	      int idBlk,            /* Identificateur du block */            
	      char *buf,            /* Buffer a copier */                    
	      int nbytes)           /* Nombre de bytes a essayer de copier */
{
    int ntop, nt, n;
    int pWr, pRd, size;                 
    char *pTo;
    char *pDeb;
    
    /* Retourner, si ring buffer non-initialise */
    if (rngId == NULL || rngId->flgInit != H2RNG_INIT_BLOCK) {
	errnoSet (S_h2rngLib_NOT_A_BLOCK_RING);
	return (ERROR);
    }
    
    /* Verifier si le nombre de bytes est positif */
    if (nbytes <= 0) {
	errnoSet (S_h2rngLib_ILLEGAL_NBYTES);
	return (ERROR);
    }
    
    /* Valeurs congelees des pointeurs d'ecriture et de lecture */
    pWr = rngId->pWr;
    pRd = rngId->pRd;

    /* Calculer la taille totale du block a ecrire */
    nt = nbytes + sizeof(nbytes) + sizeof(idBlk) + 4 - (nbytes & 3);
  
    /* Obtenir le ptr vers 1ere position libre du ring */
    pTo = (char *) (rngId+1) + pWr;
    
    /* Verifier l'etat des pointeurs */
    if (pRd <= pWr) {
	/* Obtenir la taille du ring */
	size = rngId->size;
	
	/* Nombre de bytes libres jusqu'au top */
	ntop = size - pWr;
	
	/* Verifier si plus grand que le nombre de bytes a ecrire */
	if (ntop > nt) {
	    /* Ecrire le block */
	    BLK_WR1(pTo, idBlk, buf, nbytes);
	    
	    /* Actualiser pointeur d'ecriture et retourner */
	    rngId->pWr = pWr + nt;
	    return (nbytes);
	}
	
	/* Sinon, verifier si on peut ecrire le block */
	if (nt > ntop + pRd - 2)
	    return (0);
	
	/* Calculer le pointeur vers le debut du ring */
	pDeb = (char *) (rngId+1);
	
	/* Ecrire en 2 parties */
	BLK_WR2(pTo, idBlk, buf, nbytes, ntop, pDeb);
	
	/* Actualiser le pointeur d'ecriture et retourner */
	rngId->pWr = nt + pWr - size;
	return (nbytes);
    }
    
    /* Calculer le nombre de bytes libres */
    ntop = pRd - pWr - 2;
    
    /* Retourner 0, s'il n'y a pas de place */
    if (nt > ntop)
	return (0);
    
    /* Copier tout le block, d'une seule fois */
    BLK_WR1(pTo, idBlk, buf, nbytes);

    /* Actualiser le pointeur d'ecriture et retourner */
    rngId->pWr = pWr + nt;
    return (nbytes);
}



/*****************************************************************************
*
*    h2rngBlockGet  -  Prendre un block de caracteres dans un ring buffer
*
*    Description :
*    Lit un block de caracteres dans le ring buffer et le met dans le buffer
*    de l'utilisateur. Recupere aussi l'id du block.
*
*    Retourne : nombre de bytes effectivement lus ou ERROR.
*/

int 
h2rngBlockGet(H2RNG_ID rngId, /* Identificateur du ring buffer */    
	      int *pidBlk,    /* Ou` mettre l'id du block */         
	      char *buf,      /* Pointeur vers buffer utilisateur */ 
	      int maxbytes)   /* Nombre max. de bytes a lire */      
{
    int nbytes, ntop, nt, no, n;
    int pWr, pRd, size;                 
    char *pFrom;
    char *pDeb;
    
    /* Retourner, si ring buffer non-initialise */
    if (rngId == NULL || rngId->flgInit != H2RNG_INIT_BLOCK) {
	errnoSet (S_h2rngLib_NOT_A_BLOCK_RING);
	return (ERROR);
    }
    
    /* Valeurs congelees des pointeurs d'ecriture et de lecture */
    pWr = rngId->pWr;
    pRd = rngId->pRd;
    
    /* Retourner, s'il n'y a pas de message */
    if (pWr == pRd)
	return (0);
    
    /* Calculer le pointeur vers 1ere position a lire */
    pFrom = (char *) (rngId+1) + pRd;
    
    /* Verifier l'etat des pointeurs */
    if (pRd > pWr) {
	/* Obtenir la taille du ring */
	size = rngId->size;
	
	/* Nombre de bytes occupes jusqu'au top */
	ntop = size - pRd;
	
	/* Nombre total de bytes occupes */
	no = ntop + pWr;
	
	/* Calculer le pointeur vers le debut du ring */
	pDeb = (char *) (rngId+1);
	
	/* Verifier si le nombre de bytes occupes est insuffisant */
	if (no < sizeof(nbytes)) {
	    errnoSet (S_h2rngLib_SMALL_BLOCK);
	    (void) h2rngFlush (rngId);
	    return (ERROR);
	}
	
	/* Lire le nombre de bytes du message ("net") */
	H2RNG_RD2(pFrom,
		  (char *) &nbytes, sizeof(nbytes), ntop, n, pDeb);

	/* Verifier si plus grand que le buffer de l'utilisateur */
	if (nbytes > maxbytes) {
	    errnoSet (S_h2rngLib_SMALL_BUF);
	    (void) h2rngFlush (rngId);
	    return (ERROR);
	}
	
	/* Calculer la taille totale du message */
	nt = nbytes + sizeof(nbytes) + sizeof(*pidBlk) + 4 - (nbytes & 3);
	
	/* Verifier si compatible avec le nombre de bytes occupes */
	if (nt > no) {
	    errnoSet (S_h2rngLib_BIG_BLOCK);
	    (void) h2rngFlush (rngId);
	    return (ERROR);
	}

	/* Lire l'id du block */
	H2RNG_RD2(pFrom,
		  (char *) pidBlk, sizeof(*pidBlk), ntop, n, pDeb);
      
	/* Lire le message */
	H2RNG_RD2(pFrom, buf, nbytes, ntop, n, pDeb);

	/* Verifier le caractere de synchronisation */
	if (*pFrom != H2RNG_CAR_END) {
	    errnoSet (S_h2rngLib_ERR_SYNC);
	    (void) h2rngFlush (rngId);
	    return (ERROR);
	}
	
	/* Actualiser le pointeur de lecture */
	if ((pRd = pRd + nt) >= size)
	    rngId->pRd = pRd - size;
	else rngId->pRd = pRd;
	
	/* Retourner le nombre de bytes */
	return (nbytes);
    }
  
    /* Calculer le nombre de bytes occupes */
    no = pWr - pRd;

    /* Verifier si nombre de bytes occupes est trop petit */
    if (no < sizeof(nbytes)) {
	errnoSet (S_h2rngLib_SMALL_BLOCK);
	(void) h2rngFlush (rngId);
	return (ERROR);
    }
    
    /* Lire le nombre de bytes du message */
    H2RNG_RD1(pFrom, (char *) &nbytes, sizeof(nbytes));
    
    /* Verifier si le buffer utilisateur comporte message */
    if (nbytes > maxbytes) {
	errnoSet (S_h2rngLib_SMALL_BUF);
	(void) h2rngFlush (rngId);
	return (ERROR);
    }
  
    /* Calculer la taille totale du block */
    nt = nbytes + sizeof(nbytes) + sizeof(*pidBlk) + 4 - (nbytes & 3);
  
    /* Verifier si compatible avec le nombre de bytes occupes */
    if (nt > no) {
	errnoSet (S_h2rngLib_BIG_BLOCK);
	(void) h2rngFlush (rngId);
	return (ERROR);
    }
    
    /* Lire l'id du block */
    H2RNG_RD1(pFrom, (char *) pidBlk, sizeof(*pidBlk));
    
    /* Lire le message lui-meme */
    H2RNG_RD1(pFrom, buf, nbytes);

    /* Verifier le caractere de fin de message */
    if (*pFrom != H2RNG_CAR_END) {
	errnoSet (S_h2rngLib_ERR_SYNC);
	(void) h2rngFlush (rngId);
	return (ERROR);
    }
    
    /* Actualiser le pointeur de lecture et retourner */
    rngId->pRd = pRd + nt;
    return (nbytes);
}
  

/*****************************************************************************
*
*    h2rngBlockSpy  -  Spionner le contenu du ring buffer
*
*    Description :
*    Spionne le contenu du 1er block dans le ring buffer.
*    Obtient le nombre de bytes du message, l'id du block et
*    un e'chantillon des donnees du block
*
*    Retourne : nombre de bytes effectivement lus ou ERROR.
*/

int 
h2rngBlockSpy(H2RNG_ID rngId, /* Identificateur du ring buffer */ 
	      int *pidBlk,    /* Ou` mettre l'id du block */ 
	      int *pnbytes,   /* Ou` mettre le nombre de bytes message 
				 ds block */ 
	      char *buf,      /* Pointeur vers buffer utilisateur */ 
	      int maxbytes)   /* Echantillon de bytes a lire */ 
{
    int nbytes, ntop, nt, no, n;
    int pWr, pRd, size;                 
    char *pFrom;
    char *pDeb;
    
    /* Retourner, si ring buffer non-initialise */
    if (rngId == NULL || rngId->flgInit != H2RNG_INIT_BLOCK) {
	errnoSet (S_h2rngLib_NOT_A_BLOCK_RING);
	return (ERROR);
    }
    
    /* Valeurs congelees des pointeurs d'ecriture et de lecture */
    pWr = rngId->pWr;
    pRd = rngId->pRd;
    
    /* Retourner, s'il n'y a pas de message */
    if (pWr == pRd)
	return (0);
    
    /* Calculer le pointeur vers 1ere position a lire */
    pFrom = (char *) (rngId+1) + pRd;
    
    /* Verifier l'etat des pointeurs */
    if (pRd > pWr) {
	/* Obtenir la taille du ring */
	size = rngId->size;
	
	/* Nombre de bytes occupes jusqu'au top */
	ntop = size - pRd;
	
	/* Nombre total de bytes occupes */
	no = ntop + pWr;
	
	/* Calculer le pointeur vers le debut du ring */
	pDeb = (char *) (rngId+1);

	/* Verifier si le nombre de bytes occupes est insuffisant */
	if (no < sizeof(nbytes)) {
	    errnoSet (S_h2rngLib_SMALL_BLOCK);
	    (void) h2rngFlush (rngId);
	    return (ERROR);
	}
	
	/* Lire le nombre de bytes du message ("net") */
	H2RNG_RD2(pFrom,
		  (char *) &nbytes, sizeof(nbytes), ntop, n, pDeb);

	/* Calculer la taille totale du message */
	nt = nbytes + sizeof(nbytes) + sizeof(*pidBlk) + 4 - (nbytes & 3);

	/* Verifier si compatible avec le nombre de bytes occupes */
	if (nt > no) {
	    errnoSet (S_h2rngLib_BIG_BLOCK);
	    (void) h2rngFlush (rngId);
	    return (ERROR);
	}
	
	/* Lire l'id du block */
	H2RNG_RD2(pFrom,
		  (char *) pidBlk, sizeof(*pidBlk), ntop, n, pDeb);
	
	/* Garder le nombre de bytes du message */
	*pnbytes = nbytes;
	
	/* Definir le nombre de bytes a` lire */
	nbytes = min (nbytes, maxbytes);
	
	/* Lire le message */
	H2RNG_RD2(pFrom, buf, nbytes, ntop, n, pDeb);
	
	/* Retourner le nombre de bytes */
	return (nbytes);
    }
  
    /* Calculer le nombre de bytes occupes */
    no = pWr - pRd;

    /* Verifier si nombre de bytes occupes est trop petit */
    if (no < sizeof(nbytes)) {
	errnoSet (S_h2rngLib_SMALL_BLOCK);
	(void) h2rngFlush (rngId);
	return (ERROR);
    }
    
    /* Lire le nombre de bytes du message */
    H2RNG_RD1(pFrom, (char *) &nbytes, sizeof(nbytes));
    
    /* Calculer la taille totale du block */
    nt = nbytes + sizeof(nbytes) + sizeof(*pidBlk) + 4 - (nbytes & 3);
    
    /* Verifier si compatible avec le nombre de bytes occupes */
    if (nt > no) {
	errnoSet (S_h2rngLib_BIG_BLOCK);
	(void) h2rngFlush (rngId);
	return (ERROR);
    }

    /* Lire l'id du block */
    H2RNG_RD1(pFrom, (char *) pidBlk, sizeof(*pidBlk));
    
    /* Garder le nombre de bytes du message */
    *pnbytes = nbytes;
    
    /* Definir le nombre de bytes a` lire */
    nbytes = min (nbytes, maxbytes);
    
    /* Lire le message lui-meme */
    H2RNG_RD1(pFrom, buf, nbytes);
    
    /* Retourner le nombre de bytes */
    return (nbytes);
}
  

/****************************************************************************
*
*   h2rngIsEmpty  -  Verifier si le ring buffer est vide
*
*   Description :
*   Verifie si le ring buffer est vide. 
*
*   Retourne : TRUE si vide, FALSE si non-vide, ERROR si pas un ring buffer.
*/

BOOL 
h2rngIsEmpty(H2RNG_ID rngId) /* Identificateur du ring buffer */
{
    /* Retourner, si ring buffer non-initialise */
    if (rngId == NULL || (rngId->flgInit != H2RNG_INIT_BYTE &&
	rngId->flgInit != H2RNG_INIT_BLOCK)) {
	errnoSet (S_h2rngLib_NOT_A_RING);
	return (ERROR);
    }
    
    /* Ring buffer est vide si les pointeurs sont egaux */
    return (rngId->pWr == rngId->pRd);
}



/*****************************************************************************
*
*   h2rngIsFull  -  Verifier si le ring buffer est plein (plus d'espace) 
*
*   Description :
*   Verifie si le ring buffer est full.
*
*   Retourne : TRUE si plein, FALSE si non-plein, ERROR si pas un ring buffer.
*/

BOOL 
h2rngIsFull(H2RNG_ID rngId)
{
  int dp;

  if (rngId == NULL) {
      errnoSet(S_h2rngLib_NOT_A_RING);
      return ERROR;
  }
  /* Determine l'ecart entre les pointeurs de lecture et d'ecriture */
  dp = rngId->pRd - rngId->pWr;

  /* Verifier le type du ring buffer */
  switch (rngId->flgInit)
    {
    case H2RNG_INIT_BYTE:           /* Ring buffer type byte */

      return (dp == 1 || dp == 1 - rngId->size);

    case H2RNG_INIT_BLOCK:          /* Ring buffer type "block" */

      if (dp > 0 )
	return (dp < 12);
      return (dp < 12 - rngId->size);

    default:                       /* Type inconnu */

      /* Indiquer l'erreur */
      errnoSet (S_h2rngLib_NOT_A_RING);
      return (ERROR);
    }
}



/******************************************************************************
*
*   h2rngFreeBytes - Verifier nombre de bytes libres dans ring buffer
*
*   Description :
*   Determine le nombre de bytes libres dans un ring.
*
*   Retourne : nombre de bytes libres dans le ring buffer ou ERROR, si
*   pas un ring buffer.
*/

int 
h2rngFreeBytes(H2RNG_ID rngId)
{
  int dp;       
  int n;

  if (rngId == NULL) {
      errnoSet(S_h2rngLib_NOT_A_RING);
      return ERROR;
  }

  /* Ecart entre pointeurs */
  dp = rngId->pRd - rngId->pWr;

  /* Verifier le type du ring buffer */
  switch (rngId->flgInit)
    {
    case H2RNG_INIT_BYTE:           /* Ring buffer type byte */

      /* Calculer le nombre de bytes libres */
      if (dp > 0)
	return (dp - 1);
      return (dp + rngId->size - 1);

    case H2RNG_INIT_BLOCK:          /* Ring buffer type "block" */

      if (dp > 0)
	n = dp - 11;
      else
	n = dp + rngId->size - 9;
      return (max (n, 0));
      
    default:                       /* Type inconnu */

      /* Indiquer l'erreur */
      errnoSet (S_h2rngLib_NOT_A_RING);
      return (ERROR);
    }
}



/******************************************************************************
*
*   h2rngNBytes  -  Nombre de bytes dans un ring buffer
*
*   Description :
*   Determine le nombre de positions occupees dans un ring buffer type "byte".
*
*   Retourne : nombre de positions occupees dans le ring buffer, ERROR si
*   pas un ring buffer.
*/

int 
h2rngNBytes(H2RNG_ID rngId)
{
  int dp;

  /* Retourner, si ring buffer non-initialise */
  if (rngId == NULL || (rngId->flgInit != H2RNG_INIT_BYTE &&
      rngId->flgInit != H2RNG_INIT_BLOCK))
    {
      errnoSet (S_h2rngLib_NOT_A_RING);
      return (ERROR);
    }

  /* Ecart entre pointeurs */
  dp = rngId->pWr - rngId->pRd;

  /* Calculer le nombre de bytes occupes */
  if (dp >= 0)
    return (dp);
  return (dp + rngId->size);
}



/*****************************************************************************
*
*   h2rngNBlocks - Determine le nombre de blocks dans un ring buffer type block
*
*   Description :
*   Cette procedure donne comme reponse le nombre de blocks dans un ring
*   type block.
*
*   Retourne : nombre de blocks ou ERROR.
*/

int 
h2rngNBlocks(H2RNG_ID rngId)
{
  int nbytes, ntop, nt, no, n, nblocks;
  int pWr, pRd, size;                 
  char *pFrom;
  char *pDeb;

  /* Retourner, si ring buffer non-initialise */
  if (rngId == NULL || rngId->flgInit != H2RNG_INIT_BLOCK)
    {
      errnoSet (S_h2rngLib_NOT_A_BLOCK_RING);
      return (ERROR);
    }

  /* Valeurs congelees des pointeurs d'ecriture et de lecture */
  pWr = rngId->pWr;
  pRd = rngId->pRd;

  /* Obtenir la taille du ring */
  size = rngId->size;
      
  /* Calculer le pointeur vers le debut du ring */
  pDeb = (char *) (rngId+1);

  /* Boucler et compter le nombre total de blocks dans le ring */
  for (nblocks = 0; ; nblocks++)
    {
      /* Calculer le pointeur vers 1ere position a lire */
      pFrom = (char *) (rngId+1) + pRd;

      /* Verifier l'etat des pointeurs */
      if (pRd > pWr)
	{
	  /* Nombre de bytes occupes jusqu'au top */
	  ntop = size - pRd;
      
	  /* Nombre total de bytes occupes */
	  no = ntop + pWr;

	  /* Verifier si le nombre de bytes occupes est insuffisant */
	  if (no < sizeof(nbytes)) {
	    if (no == 0) {            /* Pas de message */
	      return (nblocks);
	    } else {                    /* Erreur */ 
		errnoSet (S_h2rngLib_SMALL_BLOCK);
		(void) h2rngFlush (rngId);
		return (ERROR);
	      }
	  }
	  /* Lire le nombre de bytes du message ("net") */
	  H2RNG_RD2(pFrom,
		    (char *) &nbytes, sizeof(nbytes), ntop, n, pDeb);

	  /* Calculer la taille totale du message */
	  nt = nbytes + sizeof(nbytes) + 8 - (nbytes & 3);

	  /* Verifier si compatible avec le nombre de bytes occupes */
	  if (nt > no)
	    {
	      errnoSet (S_h2rngLib_BIG_BLOCK);
	      (void) h2rngFlush (rngId);
	      return (ERROR);
	    }

	  /* Actualiser le pointeur de lecture et continuer */
	  if ((pRd = pRd + nt) >= size)
	    pRd = pRd - size;
	  continue;
	}
  
      /* Calculer le nombre de bytes occupes */
      no = pWr - pRd;

      /* Verifier si nombre de bytes occupes est trop petit */
      if (no < sizeof(nbytes)) {
	if (no == 0) {             /* Pas de message */
	    return (nblocks);
	} else {
	    errnoSet (S_h2rngLib_SMALL_BLOCK);
	    (void) h2rngFlush (rngId);
	    return (ERROR);
	}
      }
      /* Lire le nombre de bytes du message */
      H2RNG_RD1(pFrom, (char *) &nbytes, sizeof(nbytes));

      /* Calculer la taille totale du block */
      nt = nbytes + sizeof(nbytes) + 8 - (nbytes & 3);

      /* Verifier si compatible avec le nombre de bytes occupes */
      if (nt > no)
	{
	  errnoSet (S_h2rngLib_BIG_BLOCK);
	  (void) h2rngFlush (rngId);
	  return (ERROR);
	}

      /* Actualiser le pointeur de lecture et continuer */
      pRd = pRd + nt;
      continue;
    }
}
  

/******************************************************************************
*
*   h2rngBlockSkip  -  Sauter un block
*
*   Description :
*   Cette fonction permet de sauter le prochain message sur le ring.
*   Son contenu est alors perdu.
*
*   Retourne : OK ou ERROR 
*/

STATUS 
h2rngBlockSkip(H2RNG_ID rngId)
{
  int nbytes, ntop, nt, no, n;
  int pWr, pRd, size;                 
  char *pFrom;
  char *pDeb;

  /* Retourner, si ring buffer non-initialise */
  if (rngId == NULL || rngId->flgInit != H2RNG_INIT_BLOCK)
    {
      errnoSet (S_h2rngLib_NOT_A_BLOCK_RING);
      return (ERROR);
    }

  /* Valeurs congelees des pointeurs d'ecriture et de lecture */
  pWr = rngId->pWr;
  pRd = rngId->pRd;

  /* Obtenir la taille du ring */
  size = rngId->size;
      
  /* Calculer le pointeur vers le debut du ring */
  pDeb = (char *) (rngId+1);

  /* Calculer le pointeur vers 1ere position a lire */
  pFrom = (char *) (rngId+1) + pRd;

  /* Verifier l'etat des pointeurs */
  if (pRd > pWr)
    {
      /* Nombre de bytes occupes jusqu'au top */
      ntop = size - pRd;
      
      /* Nombre total de bytes occupes */
      no = ntop + pWr;

      /* Verifier si le nombre de bytes occupes est insuffisant */
      if (no < sizeof(nbytes)) {
	if (no == 0) {            /* Pas de message */
	  return (OK);
	} else {                    /* Erreur */ 
	    errnoSet (S_h2rngLib_SMALL_BLOCK);
	    (void) h2rngFlush (rngId);
	    return (ERROR);
	}
      }
      /* Lire le nombre de bytes du message ("net") */
      H2RNG_RD2(pFrom,
		(char *) &nbytes, sizeof(nbytes), ntop, n, pDeb);

      /* Calculer la taille totale du message */
      nt = nbytes + sizeof(nbytes) + 8 - (nbytes & 3);

      /* Verifier si compatible avec le nombre de bytes occupes */
      if (nt > no)
	{
	  errnoSet (S_h2rngLib_BIG_BLOCK);
	  (void) h2rngFlush (rngId);
	  return (ERROR);
	}

      /* Actualiser le pointeur de lecture et retourner */
      if ((pRd = pRd + nt) >= size)
	pRd = pRd - size;
      rngId->pRd = pRd;
      return (OK);
    }
  
  /* Calculer le nombre de bytes occupes */
  no = pWr - pRd;

  /* Verifier si nombre de bytes occupes est trop petit */
  if (no < sizeof(nbytes)) {
    if (no == 0) {              /* Pas de message */
      return (OK);
    } else {
	errnoSet (S_h2rngLib_SMALL_BLOCK);
	(void) h2rngFlush (rngId);
	return (ERROR);
    }
  }
  /* Lire le nombre de bytes du message */
  H2RNG_RD1(pFrom, (char *) &nbytes, sizeof(nbytes));

  /* Calculer la taille totale du block */
  nt = nbytes + sizeof(nbytes) + 8 - (nbytes & 3);

  /* Verifier si compatible avec le nombre de bytes occupes */
  if (nt > no)
    {
      errnoSet (S_h2rngLib_BIG_BLOCK);
      (void) h2rngFlush (rngId);
      return (ERROR);
    }

  /* Actualiser le pointeur de lecture et retourner */
  rngId->pRd = pRd + nt;
  return (OK);
}
  


/******************************************************************************
*
*   h2rngDelete  -  Deleter un ring buffer
*
*   Description :
*   Libere le pool de memoire du ring buffer.
*
*   Retourne : Neant
*/

void 
h2rngDelete(H2RNG_ID rngId)
{
    if (rngId == NULL) {
	return;
    }
    /* Reseter le flag d'initialisation */
    rngId->flgInit = 0;
    
    /* Liberer le pool de memoire */
    smMemFree ((char *) rngId);
}
