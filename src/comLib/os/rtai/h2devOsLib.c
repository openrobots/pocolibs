/*
 * Copyright (c) 1990, 2003-2004 CNRS/LAAS
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
#include "pocolibs-config.h"
__RCSID("$LAAS$");

#include <linux/module.h>

#include <rtai_sched.h>
#include <rtai_shm.h>

#include "portLib.h"
#include "errnoLib.h"
#include "h2semLib.h"
#include "mboxLib.h"
#include "posterLib.h"
#include "h2errorLib.h"
#include "h2devLib.h"
#include "smMemLib.h"
#include "smObjLib.h"

#define COMLIB_DEBUG_H2DEVLIB

#ifdef COMLIB_DEBUG_H2DEVLIB
# define LOGDBG(x)	logMsg x
#else
# define LOGDBG(x)
#endif

/**
 ** Variables globales
 **/

H2_DEV_STR *h2Devs = NULL;
static SEM_ID h2devMutex;



/*----------------------------------------------------------------------*/

/**
 ** Compute a unique key for type and dev.
 **
 ** type  : device type
 ** dev   : device number
 **/
long
h2devGetKey(int type, int dev, BOOL dummy, int *dummy2)
{
   key_t key;

   /* There is no easy way to deal with multiple users with independant
    * h2devs in kernel space. Thus we have only one key per device and
    * type.
    * To compute the key, we keep only the most significative bits of
    * nam2num and or them with type and dev. The 0xfff mask which is used
    * must be consistent with the actual values of H2DEV_MAX_TYPES and 
    * H2_DEV_MAX. */

#if H2DEV_MAX_TYPES*(H2_DEV_MAX+1) > 0xfff
# error "Please revisit this function"
#endif

   key = nam2num("h2devs") & 0xfffff000;
   key |= (dev*H2DEV_MAX_TYPES + type) & 0xfff;

   return key;
}

/*----------------------------------------------------------------------*/

/**
 ** Initialisation
 **/
STATUS
h2devInit(int smMemSize)
{
    key_t key;
    int i;
    
    key = h2devGetKey(H2_DEV_TYPE_H2DEV, 0, TRUE, NULL);

    h2Devs = rtai_kmalloc(key, sizeof(H2_DEV_STR)*H2_DEV_MAX);
    if (!h2Devs) {
       LOGDBG(("comLib:h2devInit: cannot allocate memory\n"));
       errnoSet(S_smObjLib_SHMGET_ERROR);
       return ERROR;
    }

    h2devMutex = semMCreate(0);
    if (!h2devMutex) {
       LOGDBG(("comLib:h2devInit: cannot create mutex\n"));
       rtai_kfree(key);
       h2Devs = NULL;
       return ERROR;
    }

    if (semTake(h2devMutex, WAIT_FOREVER) != OK) {
       rtai_kfree(key);
       h2Devs = NULL;
       return ERROR;
    }

    /* Create semaphores array */
    h2Devs[0].type = H2_DEV_TYPE_SEM;
    h2Devs[0].uid = 0;
    strcpy(h2Devs[0].name, "h2semLib");
    if (h2semInit(0, &(H2DEV_SEM_SEM_ID(0))) == ERROR) {
       semGive(h2devMutex);
       return ERROR;
    }
    /* Create first semaphore by hand */
    h2semCreate0(H2DEV_SEM_SEM_ID(0), SEM_EMPTY);

    /* Initialize all devices to an empty state */
    for (i = 1; i < H2_DEV_MAX; i++) {
       h2Devs[i].type = H2_DEV_TYPE_NONE;
    }
    h2semGive(0);
    semGive(h2devMutex);

    /* Create memory pool */
    if (smMemInit(smMemSize) == ERROR) {
       int savedError;
       /* Remember error code since h2devEnd() overwrites it */
       savedError = errnoGet();
       /* Free everything */
       h2devEnd();
       errnoSet(savedError);
       return ERROR;
    }

    return OK;
}
    
/*----------------------------------------------------------------------*/

/*
 * This is useless in kernel space - but of course it's here for compat.
 */
STATUS 
h2devAttach(void)
{
   return h2Devs?OK:ERROR;
}


/*----------------------------------------------------------------------*/

/**
 ** Destroy h2 devices 
 **/
STATUS
h2devEnd(void)
{
    int i, rv = OK;

    /* Destroy devices according to their types */
    for (i = 0; i < H2_DEV_MAX; i++) {
       switch (H2DEV_TYPE(i)) {
	  case H2_DEV_TYPE_MBOX:
	     mboxDelete(i);
	     break;
	  case H2_DEV_TYPE_POSTER: {
#if 0
	     POSTER_ID p;
	     if (posterFind(H2DEV_NAME(i), &p) == OK) {
		posterDelete(p);
	     }
#endif
	     break;
	  }
	  case H2_DEV_TYPE_TASK:
	     break;
	  case H2_DEV_TYPE_NONE:
	     break;

	  case H2_DEV_TYPE_SEM:
	  case H2_DEV_TYPE_MEM:
	     /* Nothing to do right now - see below */
	     break;

	  default:
	     /* error */
	     break;
       } /* switch */
    } /* for */

    /* Free shared memory */
    smMemEnd();

    /* Free semaphores - do this in the very end since sem#0 is needed
     * almost everywhere... */
    h2semEnd();

    /* Free h2 devices */
    rtai_kfree(h2devGetKey(H2_DEV_TYPE_H2DEV, 0, TRUE, NULL));

    /* Destroy global mutex */
    semDelete(h2devMutex);

    /* mark global data as invalid */
    h2Devs = NULL;
    h2devMutex = NULL;
    return rv;
}

/*----------------------------------------------------------------------*/

/**
 ** Display a summary of h2 devices
 **/
static char *h2devTypeName[] = {
    "NONE",
    "H2DEV",
    "SEM",
    "MBOX",
    "POSTER",
    "TASK",
    "MEM",
};

STATUS
h2devShow(void)
{
    int i;
    
    logMsg("Id   Type   UID Name\n");
    logMsg("------------------------------------------------\n");
    for (i = 0; i < H2_DEV_MAX; i++) {
       semTake(h2devMutex, WAIT_FOREVER);
	if (H2DEV_TYPE(i) != H2_DEV_TYPE_NONE) {
	    logMsg("%2d %6s %5d %s\n", i,
		   h2devTypeName[H2DEV_TYPE(i)],  H2DEV_UID(i), 
		   H2DEV_NAME(i));
	}
	semGive(h2devMutex);
    } /* for */
    logMsg("------------------------------------------------\n");
    return OK;
}
