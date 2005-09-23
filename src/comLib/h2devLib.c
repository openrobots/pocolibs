/*
 * Copyright (c) 2004-2005
 *      Autonomous Systems Lab, Swiss Federal Institute of Technology.
 * Copyright (c) 1990, 2003-2005 CNRS/LAAS
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

#include "portLib.h"

#if defined(__RTAI__) && defined(__KERNEL__)
# include <linux/kernel.h>
# include <linux/sched.h>
#else
# include <sys/types.h>
# include <stdio.h>
# include <string.h>
# include <unistd.h>
#endif

#include <fnmatch.h>

#include "errnoLib.h"
#include "h2semLib.h"
#include "mboxLib.h"
#include "h2errorLib.h"
#include "h2devLib.h"
const H2_ERROR h2devLibH2errMsgs[] = H2_DEV_LIB_H2_ERR_MSGS;

#include "smMemLib.h"
#include "smObjLib.h"

#if defined(__RTAI__) && defined(__KERNEL__)
# define getuid()      0
#endif

#ifdef COMLIB_DEBUG_H2DEVLIB
# define LOGDBG(x)	logMsg x
#else
# define LOGDBG(x)
#endif

/* Local fucntions prototypes */
static int h2devFindAux(const char *name, H2_DEV_TYPE type);

/*----------------------------------------------------------------------*/

/**
 ** Record errors messages
 **/
int
h2devRecordH2ErrMsgs()
{
    return h2recordErrMsgs("h2devRecordH2ErrMsg", "h2devLib", M_h2devLib, 
			   sizeof(h2devLibH2errMsgs)/sizeof(H2_ERROR), 
			   h2devLibH2errMsgs);
}

/**
 ** Allocation d'un device h2
 **/
int
h2devAlloc(char *name, H2_DEV_TYPE type)
{
    int i;

    if (h2devAttach() == ERROR) {
	return ERROR;
    }
    h2semTake(0, WAIT_FOREVER);

    /* Verifie que le nom n'existe pas */
    if (type != H2_DEV_TYPE_SEM && h2devFindAux(name, type) != ERROR) {
	h2semGive(0);
	errnoSet(S_h2devLib_DUPLICATE_DEVICE_NAME);
	return ERROR;
    }
    /* Recherche un device libre */
    for (i = 0; i < H2_DEV_MAX; i++) {
	if (h2Devs[i].type == H2_DEV_TYPE_NONE) {
	    /* Trouve' */
	    strncpy(h2Devs[i].name, name, H2_DEV_MAX_NAME);
	    h2Devs[i].type = type;
	    h2Devs[i].uid = getuid();
	    h2semGive(0);
	    LOGDBG(("comLib:h2devAlloc: created device %d\n", i));
	    return i;
	}
    } /* for */

    /* Pas de device libre */
    h2semGive(0);
    errnoSet(S_h2devLib_FULL);
    return ERROR;
}

/*----------------------------------------------------------------------*/

/**
 ** Liberation d'un device h2
 **/
STATUS
h2devFree(int dev)
{
    uid_t uid = getuid();

    if (h2devAttach() == ERROR) {
	return ERROR;
    }
    if (uid != H2DEV_UID(dev) && uid != H2DEV_UID(0)) {
	errnoSet(S_h2devLib_NOT_OWNER);
	return ERROR;
    }
    h2semTake(0, WAIT_FOREVER);
    h2Devs[dev].type = H2_DEV_TYPE_NONE;
    h2semGive(0);
    return OK;
}


/*----------------------------------------------------------------------*/

/**
 ** Clean h2 devices matching glob pattern `name'.
 **/

STATUS
h2devClean(const char *name)
{
   int i, match = 0;
   unsigned char *pool;

   if (h2devAttach() == ERROR) {
      return ERROR;
   }
   /* Look for devices */
   for (i = 0; i < H2_DEV_MAX; i++) {
      if (H2DEV_TYPE(i) != H2_DEV_TYPE_NONE && 
	  fnmatch(name, H2DEV_NAME(i), 0) == 0) {
	 logMsg("Freeing %s\n", H2DEV_NAME(i));
	 match++;
	 switch (H2DEV_TYPE(i)) {
	      case H2_DEV_TYPE_MBOX:
		 mboxDelete(i);
		 break;
	      case H2_DEV_TYPE_POSTER:
		pool = smObjGlobalToLocal(H2DEV_POSTER_POOL(i));
		if (pool != NULL) 
		    smMemFree(pool);
		h2semDelete(H2DEV_POSTER_SEM_ID(i));
		h2devFree(i);
		break;
	      case H2_DEV_TYPE_TASK:
		h2semDelete(H2DEV_TASK_SEM_ID(i));
		h2devFree(i);
		break;
	      case H2_DEV_TYPE_SEM:
	      case H2_DEV_TYPE_NONE:
		break;
	      default:
		/* error */
		logMsg("comLib: unknown device type %d\n", H2DEV_TYPE(i));
		return ERROR;
		break;
	    } /* switch */
	} 
    } /* for */

    if (match == 0) {
	logMsg("No matching device\n");
	return ERROR;
    }
    return OK;
}


/*----------------------------------------------------------------------*/

/**
 ** Recherche d'un device h2
 **/

static int 
h2devFindAux(const char *name, H2_DEV_TYPE type)
{
    int i;

    for (i = 0; i < H2_DEV_MAX; i++) {
	if ((type == h2Devs[i].type) 
	    && (strcmp(name, h2Devs[i].name) == 0)) {
	    return(i);
	}
    } /* for */
    return ERROR;
}

int
h2devFind(char *name, H2_DEV_TYPE type)
{
    int i;

    if (name == NULL) {
	errnoSet(S_h2devLib_BAD_PARAMETERS);
	return ERROR;
    }
    if (h2devAttach() == ERROR) {
	return ERROR;
    }
    h2semTake(0, H2DEV_TIMEOUT);
    i = h2devFindAux(name, type);
    h2semGive(0);

    if (i >= 0) {
	return i;
    } else {
	errnoSet(S_h2devLib_NOT_FOUND);
	return ERROR;
    }
}

/*----------------------------------------------------------------------*/

/**
 ** Retourne le semId des semaphores
 **/
int
h2devGetSemId(void)
{
    if (h2devAttach() == ERROR) {
	return ERROR;
    }
    if (h2Devs[0].type != H2_DEV_TYPE_SEM) {
	errnoSet(S_h2devLib_BAD_DEVICE_TYPE);
	return ERROR;
    }
    return h2Devs[0].data.sem.semId;
}
