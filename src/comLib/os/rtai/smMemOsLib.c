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

#include <portLib.h>

#ifdef __KERNEL__
# include <linux/module.h>
# include <rtai_sched.h>
#else
# include <stdio.h>
# include <fcntl.h>
#endif /* __KERNEL__ */

#include <rtai_shm.h>

#include <errnoLib.h>
#include <smMemLib.h>
#include <smObjLib.h>
#include <h2devLib.h>

#ifdef COMLIB_DEBUG_SMMEMLIB
# define LOGDBG(x)	logMsg x
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
#ifdef __KERNEL__
    addr = rtai_kmalloc(key, smMemSize + 2*sizeof(SM_MALLOC_CHUNK));
#else
    addr = rtai_malloc(key, smMemSize + 2*sizeof(SM_MALLOC_CHUNK));
#endif
    if (!addr) {
       h2devFree(dev);
       errnoSet(S_smObjLib_SHMGET_ERROR);
       return ERROR;
    }
    H2DEV_MEM_SHM_ID(dev) = key;
    H2DEV_MEM_SIZE(dev) = smMemSize;
    
    /* Mark bloc as free */
    header = (SM_MALLOC_CHUNK *)addr + 1;
    header->length = smMemSize;
    header->next = NULL;
    header->prev = NULL;
    header->signature = SIGNATURE;
    
    smMemFreeList = header;

    LOGDBG(("comLib:smMemInit: shm created, %d bytes\n", smMemSize));
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
    
    if (smMemFreeList != NULL) return OK;

    dev = h2devFind(SM_MEM_NAME, H2_DEV_TYPE_MEM);
    LOGDBG(("comLib:smMemAttach: shm device is %d, key %d, %d bytes\n",
	    dev, H2DEV_MEM_SHM_ID(dev), H2DEV_MEM_SIZE(dev)));
    if (dev < 0) return ERROR;

#ifdef __KERNEL__
    /* nothing to do */
#else
    {
       SM_MALLOC_CHUNK *addr;

       addr = rtai_malloc(H2DEV_MEM_SHM_ID(dev),
			  H2DEV_MEM_SIZE(dev) + 2*sizeof(SM_MALLOC_CHUNK));
       LOGDBG(("comLib:smMemAttach: attached to shm at %p\n", addr));
       smMemFreeList = addr + 1;
    }
#endif
    if (smMemFreeList == NULL) {
       errnoSet(S_smObjLib_SHMGET_ERROR);
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

    if (smMemFreeList == NULL) return OK;

    dev = h2devFind(SM_MEM_NAME, H2_DEV_TYPE_MEM);
    if (dev < 0) return ERROR;

    /* Detach memory */
#ifdef __KERNEL__
    rtai_kfree(H2DEV_MEM_SHM_ID(dev));
#else
    rtai_free(H2DEV_MEM_SHM_ID(dev), smMemBase());
#endif
    smMemFreeList = NULL;
        
    /* Free h2 device */
    h2devFree(dev);
    
    return OK;
}
