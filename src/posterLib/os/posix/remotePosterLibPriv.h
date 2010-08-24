/*
 * Copyright (c) 2000, 2004 CNRS/LAAS
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

/* Structure of remote posters */
typedef  struct {
    void *vxPosterId;		/* Id of the remote poster (hist. vxWorks) */
    const char *hostname;	/* host of the poster */
    pthread_key_t key;		/* key to thread-specific client ID */
    unsigned int dataSize;	/* Size of the poster (for local cache) */
    void *dataCache;		/* local data cache
				   (for posterTake/Give and Addr) */
    int pid;			/* process Id of the poster owner */
    POSTER_OP op;		/* type of access declared to posterTake */
    H2_ENDIANNESS endianness;
} *REMOTE_POSTER_ID, REMOTE_POSTER_STR;

/* Structure to cache per host client keys to thread-specific data */
typedef struct HOST_CLIENT_KEY {
	const char *hostname;
	pthread_key_t key;
	struct HOST_CLIENT_KEY *next;
} HOST_CLIENT_KEY;

extern int clientKeyFind(const char *, pthread_key_t *);
extern CLIENT *clientCreate(pthread_key_t, const char *);
extern void clientRemove(pthread_key_t);
#endif
