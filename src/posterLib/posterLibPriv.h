/* $LAAS$ */
/*
 * Copyright (c) 1990, 2003 CNRS/LAAS
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
#ifndef _POSTERLIBPRIV_H
#define _POSTERLIBPRIV_H

#include "h2endianness.h"

typedef struct POSTER_FUNCS {
    STATUS (* init)(void);
    STATUS (* create)(char *, int, POSTER_ID *);
    STATUS (* memCreate)(char *, int, void *, int, POSTER_ID *);
    STATUS (* delete)(POSTER_ID);
    STATUS (* find)(char *, POSTER_ID *);
    int (* write)(POSTER_ID, int, void *, int);
    int (* read)(POSTER_ID, int, void *, int);
    STATUS (* take)(POSTER_ID, POSTER_OP);
    STATUS (* give)(POSTER_ID);
    void *(* addr)(POSTER_ID);
    STATUS (* ioctl)(POSTER_ID, int, void *);
    STATUS (* show)(void);
    STATUS (* setEndianness)(POSTER_ID, H2_ENDIANNESS);
    STATUS (* getEndianness)(POSTER_ID, H2_ENDIANNESS *);
} POSTER_FUNCS;


/* 
 * Pointers on real functions
 */
extern const POSTER_FUNCS posterLocalFuncs, posterRemoteFuncs;

typedef enum {
  POSTER_ACCESS_LOCAL,
  POSTER_ACCESS_REMOTE
} POSTER_ACCESS_TYPE;


typedef struct POSTER_STR {
    char name[H2_DEV_MAX_NAME];		/* name of the poster */
    POSTER_ACCESS_TYPE type;		/* type local ou remote */
    H2_ENDIANNESS endianness;           /* data (ie, writer) endianness */
    long posterId;			/* id specifique */
    const POSTER_FUNCS *funcs;		/* pointeurs vers les fonctions */
    struct POSTER_STR *next;		/* pointer to next element */
} POSTER_STR;

#endif
