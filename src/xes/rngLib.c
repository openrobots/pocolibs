/*
 * Copyright (c) 2003 CNRS/LAAS
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef __svr4__
#include <macros.h>
#else
#  ifndef min
#    define min(a,b) ((a)<(b)?(a):(b))
#  endif
#endif

#include "rngLib.h"

/*****************************************************************************
*
*   rngCreate  -  Creation d'un ring buffer
*
*   Description :
*   Cree et initialise un ring buffer de taille fournie par l'utilisateur.
* 
*   Retourne : identificateur du ring buffer ou NULL
*/

RNG_ID rngCreate (nbytes)
     int nbytes;             /* Nombre de bytes */

{
  RNG_ID rngId;             /* Pointeur vers tete */

  /* Taille du ring buffer */
  nbytes = nbytes + 1;

  /* Allouer memoire pour l'en-tete et pour le buffer */
  if ((rngId = (RNG_ID) malloc ((u_int) (nbytes + sizeof (RNG_HDR)))) == NULL)
    return (NULL);

  /* Initialiser l' en-tete */
  rngId->pRd = 0;
  rngId->pWr = 0;
  rngId->size = nbytes;

  /* Retourner le pointeur vers le ring buffer */
  return (rngId);
}



/*****************************************************************************
*
*    rngFlush  -  Nettoyer un ring buffer
*
*    Description :
*    Vide le ring buffer. Les donnees existentes sont perdues.
*
*    Retourne : Neant
*/

void rngFlush (rngId)
     RNG_ID rngId;       /* Identificateur du ring buffer */

{
  /* Reseter les pointeurs */
  rngId->pRd = 0;
  rngId->pWr = 0;
}



/*****************************************************************************
*
*    rngWhereIs  -  Positions actuelles des pointeurs d'ecriture/lecture
*
*    Description :
*    Donne les valeurs des pointeurs du ring buffer
*
*    Retourne : Neant
*/

void rngWhereIs (rngId, ppRd, ppWr)
     RNG_ID rngId;       /* Identificateur du ring buffer */
     int *ppRd;          /* Ou` mettre le pointeur de lecture */
     int *ppWr;          /* Ou` mettre le pointeur d'ecriture */

{
  *ppRd = rngId->pRd;
  *ppWr = rngId->pWr;
}



/*****************************************************************************
*
*    rngBufGet  -  Prendre des caracteres a partir d'un ring buffer
*
*    Description :
*    Copie les caracteres du ring buffer sur le buffer de l'utilisateur.
*    Copie autant de bytes qu'il y a sur le ring buffer, jusqu'au nombre
*    max. defini par l'utilisateur. Les bytes lus sont enleves du ring buffer.
*
*    Retourne : nombre de bytes effectivement lus.
*/

int rngBufGet (rngId, buf, maxbytes)
     RNG_ID rngId;         /* Identificateur du ring buffer */
     char *buf;            /* Pointeur vers buffer utilisateur */
     int maxbytes;         /* Nombre max. de bytes a lire */

{
  int n1, n2, nbytes;                          
  int pWr, pRd;                 

  /* Valeurs congelees des pointeurs d'ecriture et de lecture */
  pWr = rngId->pWr;
  pRd = rngId->pRd;

  /* Verifier si on doit faire la copie en deux morceaux */
  if ((n1 = pWr - pRd) < 0)
    {
      /* Nombre de bytes a copier = min (maxBytes, nombre pos. occupees) */
      nbytes = min (maxbytes, n1 + rngId->size);

      /* Taille du premier morceau occupe */
      n1 = rngId->size - pRd;

      /* Verifier si plus grand que nombre de bytes a lire */
      if (n1 > nbytes)
	{
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
*   rngBufPut  -  Ecrire des bytes sur le ring buffer
*
*   Description :
*   Copie le contenu du buffer de l'utilisateur sur le ring buffer.
*   Le nombe specifie de bytes va etre mis dans le ring buffer, s'il y a
*   de la place.
*
*   Retourne :
*   Nombre de bytes effectivement copies
*/

int rngBufPut (rngId, buf, nbytes)
     RNG_ID rngId;                  /* Ring buffer ou mettre les bytes */
     char *buf;                     /* Buffer a copier */
     int nbytes;                    /* Nombre de bytes a essayer de copier */

{
  int n1, n2;                          
  int pWr, pRd;                 

  /* Valeurs congelees des pointeurs d'ecriture et de lecture */
  pWr = rngId->pWr;
  pRd = rngId->pRd;

  /* Verifier l'etat des pointeurs */
  if ((n1 = pRd - pWr) <= 0)
    {
      /* Nombre de bytes a ecrire = min (pos.libres, nbytes) */
      nbytes = min (n1 + rngId->size - 1, nbytes);
 
      /* Taille du premier morceau libre */
      n1 = rngId->size - pWr;
 
      /* Verifier si plus grand que le nombre de bytes a ecrire */
      if (n1 > nbytes)
        {
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


/******************************************************************************
*
*   rngBufSkip  -  Sauter une portion de message
*
*   Description:
*   Cette fonction permet de sauter une portion d'un message, le nombre de 
*   bytes etant donne par l'utilisateur.
*
*   Retourne :
*/
 
int rngBufSkip (rngId, nbytes)
     RNG_ID rngId;    /* Pointeur vers le ring buffer */
     int nbytes;      /* Nobre de bytes a sauter */
 
{
  int pRd;            /* Valeur du pointeur de lecture */
  int size;           /* Taille du ring buffer */
 
  /* Obtenir la taille du ring buffer */
  size = rngId->size;
 
  /* Verifier le nombre de bytes a sauter */
  if (nbytes >= size)
    return (-1);
 
  /* Obtention du pointeur de lecture */
  pRd = rngId->pRd;
 
  /* Sauter le nombre de bytes demande */
  pRd += nbytes;
 
  /* Actualiser le pointeur de lecture */
  if (pRd >= size)
    rngId->pRd = pRd - size;
  else rngId->pRd = pRd;
  return (0);
}
 
 
/******************************************************************************
*
*   rngBufSpy  -  Epier dans un ring buffer
*
*   Description:
*   Cette fonction permet de epier le contenu d'un ring buffer, sans
*   changer son etat.
* 
*   Retourne : nombre de bytes epies
*/
 
int rngBufSpy (rngId, buf, maxbytes)
     RNG_ID rngId;         /* Identificateur du ring buffer */
     char *buf;            /* Pointeur vers buffer utilisateur */
     int maxbytes;         /* Nombre max. de bytes a epier */
 
{
  int n1, n2, nbytes;                          
  int pWr, pRd;
 
  /* Valeurs congelees des pointeurs d'ecriture et de lecture */
  pWr = rngId->pWr;
  pRd = rngId->pRd;
 
  /* Verifier si on doit faire la copie en deux morceaux */
  if ((n1 = pWr - pRd) < 0)
    {
      /* Nombre de bytes a copier = min (maxBytes, nombre pos. occupees) */
      nbytes = min (maxbytes, n1 + rngId->size);
 
      /* Taille du premier morceau occupe */
      n1 = rngId->size - pRd;
 
      /* Verifier si plus grand que nombre de bytes a lire */
      if (n1 > nbytes)
        {
          /* Faire une seule copie */
          memcpy (buf, (char *) (rngId+1) + pRd, nbytes);
         
          /* retourner */
          return (nbytes);
        }
      
      /* Copier tout le premier morceau */
      memcpy (buf, (char *) (rngId+1) + pRd, n1);
      
      /* Taille du deuxieme morceau */
      n2 = nbytes - n1;
 
      /* Copier le deuxieme morceau et retourner */
      memcpy (buf+n1, (char *) (rngId+1), n2);
 
      /* retourner */
      return (n1 + n2);
    }
 
  /* Copier tout, d'une seule fois */
  memcpy (buf, (char *) (rngId+1) + pRd, n1 = min (maxbytes, n1));
 
  /* retourner */
  return (n1);
}

/******************************************************************************
*
*   rngIsEmpty  -  Verifier si le ring buffer est vide
*
*   Description :
*   Verifie si le ring buffer est vide.
*
*   Retourne : TRUE ou FALSE
*/

int 
rngIsEmpty (rngId)
     RNG_ID rngId;    /* Pointeur vers le ring buffer */

{
  return (rngId->pRd == rngId->pWr);
}


/******************************************************************************
*
*   rngNBytes  -  Lire le nombre de bytes dans le ring buffer
*
*   Description:
*   Lit le nombre de bytes dans le ring buffer.
*
*   Retourne : nombre de bytes dans le ring buffer
*/

int rngNBytes (rngId)
     RNG_ID rngId;    /* Pointeur vers le ring buffer */
     
{
  int dp;

  /* Ecart entre pointeurs */
  dp = rngId->pWr - rngId->pRd;

  /* Calculer le nombre de bytes occupes */
  if (dp >= 0)
      return (dp);
  return (dp + rngId->size);
}


/******************************************************************************
*
*   rngDelete  -  Deleter un ring buffer
*
*   Description :
*   Libere le pool de memoire du ring buffer.
*
*   Retourne : OK ou ERROR
*/

void rngDelete (rngId)
     RNG_ID rngId;    /* Pointeur vers le ring buffer */

{
  /* Liberer le pool de memoire */
  free ((char *) rngId);
}

