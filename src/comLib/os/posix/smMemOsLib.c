/*
 * Copyright (c) 1999, 2003-2004,2016,2023-2024 CNRS/LAAS
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

extern void *smMemBaseAddr;
extern SM_MALLOC_CHUNK *smMemFreeList;
  
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

    /* remember base address */
    smMemBaseAddr = addr;

    /* Memorise le bloc comme libre */
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
    void *addr;
    
    if (smMemBaseAddr != NULL) {
	return OK;
    }
    dev = h2devFind(SM_MEM_NAME, H2_DEV_TYPE_MEM);
    if (dev == ERROR) {
	return ERROR;
    }

    /* Attach du SHM */
    do {
        addr = shmat(H2DEV_MEM_SHM_ID(dev), NULL, 0);
        if (addr == (void *)-1 && errno != EINTR) {
            errnoSet(S_smObjLib_SHMAT_ERROR);
            return ERROR;
	}
    } while (addr == (void *)-1);
    smMemBaseAddr = addr;
    smMemFreeList = (SM_MALLOC_CHUNK *)addr + 1;

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
    if (smMemBaseAddr == NULL) {
	return ERROR;
    }
    dev = h2devFind(SM_MEM_NAME, H2_DEV_TYPE_MEM);
    if (dev == ERROR) {
	return ERROR;
    }
    /* Detach le shared memory segment */
    shmdt((char *)smMemBaseAddr);
    smMemBaseAddr = NULL;
    smMemFreeList = NULL;
    /* Libere le shared memory segment */
    shmctl(H2DEV_MEM_SHM_ID(dev), IPC_RMID, NULL);
    
    /* Libere le device h2 */
    h2devFree(dev);
    
    return OK;
}
