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
 *** Shared memory allocation functions
 ***
 ***/
#include "pocolibs-config.h"
__RCSID("$LAAS$");

#include <linux/module.h>

#include <rtai_sched.h>
#include <rtai_shm.h>

#include <portLib.h>
#include <errnoLib.h>

#include <smMemLib.h>
#include <smObjLib.h>
#include <h2devLib.h>


#define COMLIB_DEBUG_SMMEMLIB

#ifdef COMLIB_DEBUG_SMMEMLIB
# define LOGDBG(x)	printk x
#else
# define LOGDBG(x)
#endif

extern SM_MALLOC_CHUNK *smMemFreeList;

/*----------------------------------------------------------------------*/

/*
 * Create a shared memory segment
 */
STATUS 
smMemInit(int smMemSize)
{
    key_t key;
    int dev;
    void *addr;
    SM_MALLOC_CHUNK *header;
    
    /* create an h2 device */
    dev = h2devAlloc(SM_MEM_NAME, H2_DEV_TYPE_MEM);
    if (dev == ERROR) {
        return ERROR;
    }
    /* Get a unique key */
    key = h2devGetKey(H2_DEV_TYPE_MEM, dev, FALSE, NULL);
    if (key == ERROR) {
        return ERROR;
    }    
    /* Allocate memory */
    addr = rtai_kmalloc(key, smMemSize + 2*sizeof(SM_MALLOC_CHUNK));
    if (!addr) {
       h2devFree(dev);
       errnoSet(S_smObjLib_SHMGET_ERROR);
       return ERROR;
    }
    H2DEV_MEM_SHM_ID(dev) = key;
    
    /* Mark bloc as free */
    header = (SM_MALLOC_CHUNK *)addr + 1;
    header->length = smMemSize;
    header->next = NULL;
    header->prev = NULL;
    header->signature = SIGNATURE;
    
    smMemFreeList = header;
    
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
    
    if (smMemFreeList != NULL) {
	return OK;
    }
    dev = h2devFind(SM_MEM_NAME, H2_DEV_TYPE_MEM);
    if (dev < 0) {
	return ERROR;
    }

    return OK;
}

/*----------------------------------------------------------------------*/

/*
 * Free a shared memory segment
 */
STATUS
smMemEnd(void)
{
    int dev;

    if (smMemFreeList == NULL) {
	return ERROR;
    }
    dev = h2devFind(SM_MEM_NAME, H2_DEV_TYPE_MEM);
    if (dev < 0) {
	return ERROR;
    }
    /* Detach memory */
    smMemFreeList = NULL;
    rtai_kfree(H2DEV_MEM_SHM_ID(dev));
    
    /* Free h2 device */
    h2devFree(dev);
    
    return OK;
}
