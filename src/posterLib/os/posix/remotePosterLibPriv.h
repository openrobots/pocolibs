/* $LAAS$
/*
 * Copyright (c) 2000, 2003 CNRS/LAAS
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
#ifndef _REMOTEPOSTERLIBPRIV_H
#define _REMOTEPOSTERLIBPRIV_H

#include "posters.h"

/* Redefinition du type POSTER_ID */
typedef  struct {
    void *vxPosterId;		/* Id VxWorks du Poster */
    CLIENT *client;		/* Struct RPC client du poster */
    unsigned int dataSize;	/* Taille du poster */
    void *dataCache;		/* Cache local des donne'es 
				   (for posterTake/Give and Addr) */
    int pid;			/* PID du createur */
    POSTER_OP op;		/* derniere operation posterTake */
    H2_ENDIANNESS endianness;
} *REMOTE_POSTER_ID, REMOTE_POSTER_STR;

#endif
