/*
 * Copyright (C) 1994 LAAS/CNRS 
 *
 * Matthieu Herrb - Wed Apr 27 1994
 *
 * $Source$
 * $Revision$
 * $Date$
 * 
 */
#ifndef _RNGLIB_H
#define _RNGLIB_H

/* En-tete d'un ring buffer */
typedef struct {
    int pRd;                             /* Pointeur de lecture */
    int pWr;                             /* Pointeur d'ecriture */
    int size;                            /* Taille du ring buffer */
} RNG_HDR;

typedef RNG_HDR *RNG_ID;

#ifdef __STDC__
int rngIsEmpty (RNG_ID rngId);
RNG_ID rngCreate (int nbytes);
int rngBufPut (RNG_ID rngId, char *buf, int nbytes);
int rngBufSkip (RNG_ID rngId, int nbytes);
int rngBufGet (RNG_ID rngId, char *buf, int maxbytes);
int rngBufSpy (RNG_ID rngId, char *buf, int maxbytes);
int rngNBytes (RNG_ID rngId);
void rngDelete (RNG_ID rngId);
void rngFlush (RNG_ID rngId);
void rngMoveAhead (RNG_ID rngId, int pWr);
void rngWhereIs (RNG_ID rngId, int *ppRd, int *ppWr);
#endif

#endif
