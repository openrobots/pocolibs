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

/***
 *** Gestion de memoire partagee compatible VxMP
 ***
 ***/
#include "config.h"
__RCSID("$LAAS$");

#include <assert.h>

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <portLib.h>
#include <errnoLib.h>

#include <smMemLib.h>
#include <smObjLib.h>
#include <h2devLib.h>


#define SIGNATURE 0xdeadbeef
#define MDEBUG

/**
 ** Allocation unit header
 **/
typedef struct SM_MALLOC_CHUNK {
    size_t length;		/* usable size */
    struct SM_MALLOC_CHUNK *next;
    struct SM_MALLOC_CHUNK *prev;
    unsigned long signature;	/* for alignement */
} SM_MALLOC_CHUNK;

/**
 ** Static variables
 **/
/* free_list est exprime en adresse locale */
static SM_MALLOC_CHUNK *free_list = NULL; /* free chunks list */
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
    for (c = free_list; c != NULL; \
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
	    remove_chunk(&free_list, c);
	    c->next = MALLOC_MAGIC;
	    return((void *)(c+1));
	}
    } else {
	return NULL;
    }

} /* malloc */
   
/*----------------------------------------------------------------------*/

/*
 * Cree le segment de memoire partagee
 */
STATUS 
smMemInit(int smMemSize)
{
    key_t key;
    int dev;
    void *addr;
    SM_MALLOC_CHUNK *header;
    
    /* allocation d'un device h2 */
    dev = h2devAlloc(SM_MEM_NAME, H2_DEV_TYPE_MEM);
    if (dev == ERROR) {
        return ERROR;
    }
    /* Clef associee aux SHM */
    key = h2devGetKey(H2_DEV_TYPE_MEM, dev, FALSE, NULL);
    if (key == ERROR) {
        return ERROR;
    }    
    /* Creation du segment de memoire partage'e */
    do {
        H2DEV_MEM_SHM_ID(dev) = shmget(key, 
				       smMemSize + 2*sizeof(SM_MALLOC_CHUNK), 
				       (IPC_CREAT | IPC_EXCL | PORTLIB_MODE));
        if (H2DEV_MEM_SHM_ID(dev) < 0 && errno != EINTR) {
            h2devFree(dev);
            errnoSet(S_smObjLib_SHMGET_ERROR);
            return ERROR;
        }
    } while (H2DEV_MEM_SHM_ID(dev) < 0);
    /* Attach du SHM */
    do {
        addr = shmat(H2DEV_MEM_SHM_ID(dev), NULL, 0);
        if (addr == (void *)-1 && errno != EINTR) {
            shmctl(H2DEV_MEM_SHM_ID(dev), IPC_RMID, NULL);
            h2devFree(dev);
            errnoSet(S_smObjLib_SHMAT_ERROR);
            return ERROR;
	}
    } while (addr == (void *)-1);
    
    /* Memorise le bloc comme libre */
    header = (SM_MALLOC_CHUNK *)addr + 1;
    header->length = smMemSize;
    header->next = NULL;
    header->prev = NULL;
    header->signature = SIGNATURE;
    
    free_list = header;
    
    return OK;
} 

/*----------------------------------------------------------------------*/

/*
 * Attache le segment de memoire partagee
 */
STATUS
smMemAttach(void)
{
    int dev;
    void *addr;
    
    if (free_list != NULL) {
	return OK;
    }
    dev = h2devFind(SM_MEM_NAME, H2_DEV_TYPE_MEM);
    if (dev < 0) {
	return ERROR;
    }

    /* Attach du SHM */
    do {
        addr = shmat(H2DEV_MEM_SHM_ID(dev), NULL, 0);
        if (addr == (void *)-1 && errno != EINTR) {
            errnoSet(S_smObjLib_SHMAT_ERROR);
            shmctl(H2DEV_MEM_SHM_ID(dev), IPC_RMID, NULL);
            h2devFree(dev);
            return ERROR;
	}
    } while (addr == (void *)-1);
    free_list = (SM_MALLOC_CHUNK *)addr + 1;

    return OK;
}

/*----------------------------------------------------------------------*/

/*
 * Libere le segment de memoire partagee 
 */
STATUS
smMemEnd(void)
{
    int dev;

    if (smMemAttach() == ERROR) {
	return ERROR;
    }
    if (free_list == NULL) {
	return ERROR;
    }
    dev = h2devFind(SM_MEM_NAME, H2_DEV_TYPE_MEM);
    if (dev < 0) {
	return ERROR;
    }
    /* Detach le shared memory segment */
    shmdt((char *)(free_list - 1));
    free_list = NULL;
    /* Libere le shared memory segment */
    shmctl(H2DEV_MEM_SHM_ID(dev), IPC_RMID, NULL);
    
    /* Libere le device h2 */
    h2devFree(dev);
    
    return OK;
}
    
/*----------------------------------------------------------------------*/

/*
 * Retourne l'adresse de base de la memoire partagee 
 * dans l'espace d'adressage local 
 */
unsigned long
smMemBase(void)
{
    if (free_list == NULL) {
	if (smMemAttach() == ERROR) {
	    return 0;
	}
    }
    return (unsigned long)(free_list - 1);
}

/*----------------------------------------------------------------------*/

/**
 ** Allocation d'une zone dans le segment de memoire partagee
 **/
void *
smMemMalloc(unsigned int nBytes)
{
    void *result;
    
    if (free_list == NULL) {
	if (smMemAttach() == ERROR) {
	    return NULL;
	}
    }
    result = internal_malloc(nBytes);

#ifdef MALLOC_TRACE
    malloc_trace("alloc %u -> 0x%lx\n", size, (unsigned long)result);
#endif
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

#ifdef MALLOC_TRACE
    malloc_trace("free 0x%x\n", (unsigned)ptr);
#endif

    if (ptr == NULL) {
#ifdef MDEBUG
	fprintf(stderr, "free(NULL)\n");
#endif
	return ERROR;
    }
    /* get a pointer to the header */
    oc = (SM_MALLOC_CHUNK *)ptr - 1;
    /* test for allocated bloc */
    if (oc->next != MALLOC_MAGIC) {
	/* what to do ? */
#ifdef MDEBUG
	fprintf(stderr, "free(something not returned by malloc)\n");
#endif
	return ERROR;
    }
    /* insert free chunk in the free list */
    insert_after(&free_list, oc);
    /* test if can merge with preceding chunk */
    c = smObjGlobalToLocal(oc->prev);
    if (c != NULL && 
	oc == (SM_MALLOC_CHUNK *)((char *)c + REAL_SIZE(c->length))) {
	/* merge */
	c->length += REAL_SIZE(oc->length);
	remove_chunk(&free_list, oc);
	oc = c;
   }
    /* test if can merge with following chunk */
    c = smObjGlobalToLocal(oc->next);
    if (c == (SM_MALLOC_CHUNK *)((char *)oc + REAL_SIZE(oc->length))) {
	/* merge (=> oc->next != NULL) */
	oc->length += REAL_SIZE(c->length);
	remove_chunk(&free_list, c);
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
	printf("\nFREE LIST:\n");
	printf(" num   addr        size\n");
	printf(" --- ---------- ----------\n");
    }
    /* Parcours de la liste des blocs libres */
    for (c = free_list; c != NULL; c = smObjGlobalToLocal(c->next)) {
	assert(c->signature == SIGNATURE);
	blocks++;
	bytes += c->length;
	if (c->length > maxb) {
	    maxb = c->length;
	}
	if (option) {
	    printf("%4ld 0x%08lx %10lu\n", blocks, (unsigned long)c, 
		   (unsigned long)c->length);
	}
    }
    if (option) {
	printf("\nSUMMARY:\n");
    }
    printf(" status    bytes    blocks   ave block  max block\n");
    printf(" ------ ---------- -------- ---------- ----------\n");
    printf("current\n");
    printf("   free %10lu %9lu %10lu %10lu\n", bytes, blocks, 
	   bytes/blocks, maxb);
#ifdef notyet
    printf("  alloc %10d %9d %10d %10s\n", allocBytes, allocBlocks, 
	   allocBytes/allocBlocks, "-");
#endif
}


