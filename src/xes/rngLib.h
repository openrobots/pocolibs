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
#ifndef _RNGLIB_H
#define _RNGLIB_H

/* En-tete d'un ring buffer */
typedef struct {
    int pRd;                             /* Pointeur de lecture */
    int pWr;                             /* Pointeur d'ecriture */
    size_t size;                            /* Taille du ring buffer */
} RNG_HDR;

typedef RNG_HDR *RNG_ID;

#ifdef __STDC__
int rngIsEmpty (RNG_ID rngId);
RNG_ID rngCreate (size_t nbytes);
int rngBufPut (RNG_ID rngId, char *buf, size_t nbytes);
int rngBufSkip (RNG_ID rngId, int nbytes);
int rngBufGet (RNG_ID rngId, char *buf, size_t maxbytes);
int rngBufSpy (RNG_ID rngId, char *buf, size_t maxbytes);
int rngNBytes (RNG_ID rngId);
void rngDelete (RNG_ID rngId);
void rngFlush (RNG_ID rngId);
void rngMoveAhead (RNG_ID rngId, int pWr);
void rngWhereIs (RNG_ID rngId, int *ppRd, int *ppWr);
#endif

#endif
