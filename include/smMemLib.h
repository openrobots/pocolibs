/*
 * Copyright (c) 1999, 2003 CNRS/LAAS
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

#ifndef _SMMEMLIB_H
#define _SMMEMLIB_H

#ifdef __cplusplus
extern "C" {
#endif

/* Nom du device h2 associe */
#define SM_MEM_NAME "smMem"

/* Taille par defaut de la shared memory */
#ifndef __sun
#define SM_MEM_SIZE 0x400000	/* 4Mo */
#else
#define SM_MEM_SIZE 0x100000		/* 1Mo seulement sous Solaris */
#endif

/* Allocation unit header */
typedef struct SM_MALLOC_CHUNK {
    unsigned long length;      /* usable size */
    struct SM_MALLOC_CHUNK *next;
    struct SM_MALLOC_CHUNK *prev;
    unsigned long signature;   /* for alignement */
#define SIGNATURE 0xdeadbeef
} SM_MALLOC_CHUNK;

STATUS smMemInit(int smMemSize);
STATUS smMemAttach(void);
STATUS smMemEnd(void);
unsigned long smMemBase(void);
void *smMemMalloc(size_t nBytes);
void *smMemCalloc(size_t elemNum, size_t elemSize);
void *smMemRealloc(void *pBlock, size_t newSize);
STATUS smMemFree(void *ptr); 
void smMemShow(int option);
#ifdef __cplusplus
};
#endif

#endif
