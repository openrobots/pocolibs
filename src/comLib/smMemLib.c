/*
 * Copyright (c) 1999, 2003-2004 CNRS/LAAS
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

/***
 *** Gestion de memoire partagee compatible VxMP
 ***
 ***/
#include "pocolibs-config.h"
__RCSID("$LAAS$");

#include <portLib.h>

#if defined(__RTAI__) && defined(__KERNEL__)
# include <linux/kernel.h>
# include <linux/sched.h>
#else
# include <stdio.h>
# include <string.h>
# include <assert.h>
#endif

#include <errnoLib.h>
#include <smMemLib.h>
#include <smObjLib.h>
#include <h2devLib.h>

#define COMLIB_DEBUG_SMMEMLIB

/* handle MALLOC_TRACE */
#ifdef MALLOC_TRACE
# define LOGDBG(x)    malloc_trace x
#endif

#if defined(COMLIB_DEBUG_SMMEMLIB) && !defined(LOGDBG)
# define LOGDBG(x)     logMsg x
#else
# define LOGDBG(x)
#endif

#if defined(__RTAI__) && defined(__KERNEL__)
# ifdef NDEBUG
#  define assert(x)
# else
#  define assert(x) if (!(x)) logMsg("assert failed " #x)
# endif
#endif

/**
 ** Global variables
 **/
/* smMemFreeList is a local address */
SM_MALLOC_CHUNK *smMemFreeList = NULL; /* free chunks list */
#ifdef MALLOC_TRACE
FILE *malloc_trace_file = NULL;
#endif

/* Minimum allocation unit */
#define MALLOC_MIN_CHUNK 16

/* Magic number to mark allocated chunks */
#define MALLOC_MAGIC ((SM_MALLOC_CHUNK *)0x5f5f5f5f)

/* round to upper multiple of m */
#define ROUNDUP(n,m) ((((n)+((m)-1))/(m))*(m))

/* free list traversal until cond */
#define EVERY_FREE(cond) \
    for (c = smMemFreeList; c != NULL; \
         c = smObjGlobalToLocal(c->next)) {\
        if (cond) { \
	    break; \
	} \
    }

/* Size including header size */
#define REAL_SIZE(s) ((s)+sizeof(SM_MALLOC_CHUNK))


/**
 ** Insert a bloc in the sorted free list
 **    if list == NULL create a new list
 **
 **/
static void
insert_after(SM_MALLOC_CHUNK **list, SM_MALLOC_CHUNK *nc)
{
    SM_MALLOC_CHUNK *c, *pc, *tmp;

    if (*list == NULL) {
	nc->prev = NULL;
	nc->next = NULL;
	nc->signature = SIGNATURE;
	*list = nc;
	return;
    }
    if (nc < *list) {
	/* insert at the beginning */
	nc->next = smObjLocalToGlobal(*list);
	nc->prev = NULL;
	nc->signature = SIGNATURE;
	(*list)->prev = smObjLocalToGlobal(nc);
	*list = nc;
	return;
    }
    pc = NULL;
    for (c = *list; c != NULL; 
	 c = smObjGlobalToLocal(c->next)) {
	if (nc < c) {
	    nc->next = smObjLocalToGlobal(c);
	    nc->prev = c->prev;
	    nc->signature = SIGNATURE;
	    tmp = smObjGlobalToLocal(c->prev);
	    tmp->next = smObjLocalToGlobal(nc);
	    c->prev = smObjLocalToGlobal(nc);
	    return;
	}
	pc = c;
    }
    pc->next = smObjLocalToGlobal(nc);
    nc->prev = smObjLocalToGlobal(pc);
    nc->next = NULL;

} /* insert_after */

/**
 ** remove a chunk from the free list 
 **/
static void
remove_chunk(SM_MALLOC_CHUNK **list, SM_MALLOC_CHUNK *oc)
{
    SM_MALLOC_CHUNK *tmp;

    assert(oc->signature == SIGNATURE);

    if (oc == *list) {
	*list = smObjGlobalToLocal(oc->next);
	if (*list != NULL) {
	    (*list)->prev = NULL;
	}
	return;
    }
    tmp = smObjGlobalToLocal(oc->prev);
    tmp->next = oc->next;
    if (oc->next != NULL) {
	tmp = smObjGlobalToLocal(oc->next);
	tmp->prev = oc->prev;
    }
}

/*----------------------------------------------------------------------*/

#ifdef MALLOC_TRACE
static void
malloc_trace(char *fmt, ...)
{
    va_list ap;
    char *name;
    FILE *out;

    name = getenv("MALLOC_TRACE");
    if (name != NULL) {
      out = fopen(name, "a");
      setvbuf(out, NULL, _IONBF, 0);
      if (out != NULL) {
	va_start(ap, fmt);
	vfprintf(out, fmt, ap);
	fclose(out);
	va_end(ap);
      }
    }
} /* malloc_trace */
#endif
   
/*----------------------------------------------------------------------*/

/**
 ** malloc function 
 **/
static void *
internal_malloc(size_t size)
{
    SM_MALLOC_CHUNK *c, *nc;

#ifdef MALLOC_ZERO_RETURNS_NULL
    if (size == 0) {
	return(NULL);
    }
#endif

    /* allocate at least MALLOC_MIN_CHUNK bytes and multiple of 
       sizeof (double) */
    size = ROUNDUP(size, sizeof(double));
    if (size < MALLOC_MIN_CHUNK) {
	size = MALLOC_MIN_CHUNK;
    }
    /* look for a free chunk of size > size */
    EVERY_FREE(c->length >= size);

    if (c != NULL) {
	/* found a chunk */
	assert(c->signature == SIGNATURE);
	if (c->length > size + REAL_SIZE(MALLOC_MIN_CHUNK)) {
	    /* split it */
	    nc = (SM_MALLOC_CHUNK *)((char *)c + c->length - size);
	    nc->length = size;
	    c->length -= REAL_SIZE(size);

	    nc->next = MALLOC_MAGIC;
	    nc->signature = SIGNATURE;
	    return((void *)(nc+1));
	} else {
	    
	    /* supress it from free list */
	    remove_chunk(&smMemFreeList, c);
	    c->next = MALLOC_MAGIC;
	    return((void *)(c+1));
	}
    } else {
	return NULL;
    }

} /* malloc */
    
/*----------------------------------------------------------------------*/

/*
 * Retourne l'adresse de base de la memoire partagee 
 * dans l'espace d'adressage local 
 */
unsigned long
smMemBase(void)
{
    if (smMemFreeList == NULL) {
	if (smMemAttach() == ERROR) {
	    return 0;
	}
    }
    return (unsigned long)(smMemFreeList - 1);
}

/*----------------------------------------------------------------------*/

/**
 ** Allocation d'une zone dans le segment de memoire partagee
 **/
void *
smMemMalloc(unsigned int nBytes)
{
    void *result;
    
    if (smMemFreeList == NULL) {
	if (smMemAttach() == ERROR) {
	    return NULL;
	}
    }
    result = internal_malloc(nBytes);

    LOGDBG(("comLib:smMemLib: alloc %u -> 0x%lx\n", 
	    nBytes, (unsigned long)result));
    return result;
}

/*----------------------------------------------------------------------*/

/**
 ** Allocation et mise a zero
 **/
void *
smMemCalloc(int elemNum, int elemSize)
{
    void *data;
    
    data = smMemMalloc(elemNum*elemSize);
    if (data == NULL) {
	return NULL;
    }
    memset(data, 0, elemNum*elemSize);
    return data;
}

/*----------------------------------------------------------------------*/

/**
 ** Changement de taille d'une zone 
 **/
void *
smMemRealloc(void *pBlock, unsigned newSize)
{
    void *newBlock;

    newBlock = smMemMalloc(newSize);
    if (newBlock == NULL) {
	return NULL;
    }
    if (pBlock != NULL) {
	memcpy(newBlock, pBlock, newSize);
	smMemFree(pBlock);
    }
    return newBlock;
}

/*----------------------------------------------------------------------*/

/**
 ** Liberation d'une zone 
 **/
STATUS 
smMemFree(void *ptr)
{
    SM_MALLOC_CHUNK *oc, *c;

    LOGDBG(("comLib:smMemLib: free 0x%x\n", (unsigned)ptr));

    if (ptr == NULL) {
       LOGDBG(("comLib:smMemLib: free(NULL)\n"));
       return ERROR;
    }
    /* get a pointer to the header */
    oc = (SM_MALLOC_CHUNK *)ptr - 1;
    /* test for allocated bloc */
    if (oc->next != MALLOC_MAGIC) {
	/* what to do ? */
       LOGDBG(("comLib:smMemLib: free(something not returned by malloc)\n"));
       return ERROR;
    }
    /* insert free chunk in the free list */
    insert_after(&smMemFreeList, oc);
    /* test if can merge with preceding chunk */
    c = smObjGlobalToLocal(oc->prev);
    if (c != NULL && 
	oc == (SM_MALLOC_CHUNK *)((char *)c + REAL_SIZE(c->length))) {
	/* merge */
	c->length += REAL_SIZE(oc->length);
	remove_chunk(&smMemFreeList, oc);
	oc = c;
   }
    /* test if can merge with following chunk */
    c = smObjGlobalToLocal(oc->next);
    if (c == (SM_MALLOC_CHUNK *)((char *)oc + REAL_SIZE(oc->length))) {
	/* merge (=> oc->next != NULL) */
	oc->length += REAL_SIZE(c->length);
	remove_chunk(&smMemFreeList, c);
    }
    return OK;
}

/*----------------------------------------------------------------------*/

/**
 ** Affichage de statistiques sur l'allocateur 
 **/
void
smMemShow(BOOL option)
{
    unsigned long bytes = 0, blocks = 0, maxb = 0;
    SM_MALLOC_CHUNK *c;

    if (option) {
	logMsg("\nFREE LIST:\n");
	logMsg(" num   addr        size\n");
	logMsg(" --- ---------- ----------\n");
    }
    /* Parcours de la liste des blocs libres */
    for (c = smMemFreeList; c != NULL; c = smObjGlobalToLocal(c->next)) {
	assert(c->signature == SIGNATURE);
	blocks++;
	bytes += c->length;
	if (c->length > maxb) {
	    maxb = c->length;
	}
	if (option) {
	    logMsg("%4ld 0x%08lx %10lu\n", blocks, (unsigned long)c, 
		   (unsigned long)c->length);
	}
    }
    if (option) {
	logMsg("\nSUMMARY:\n");
    }
    logMsg(" status    bytes    blocks   ave block  max block\n");
    logMsg(" ------ ---------- -------- ---------- ----------\n");
    logMsg("current\n");
    logMsg("   free %10lu %9lu %10lu %10lu\n", bytes, blocks, 
	   bytes/blocks, maxb);
#ifdef notyet
    logMsg("  alloc %10d %9d %10d %10s\n", allocBytes, allocBlocks, 
	   allocBytes/allocBlocks, "-");
#endif
}


