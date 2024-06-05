/*
 * Copyright (c) 2004-2005
 *      Autonomous Systems Lab, Swiss Federal Institute of Technology.
 * Copyright (c) 1990, 2003-2005, 2024 CNRS/LAAS
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


#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "portLib.h"

#include <fnmatch.h>

#include "errnoLib.h"
#include "h2semLib.h"
#include "mboxLib.h"
#include "h2errorLib.h"
#include "h2devLib.h"
static const H2_ERROR h2devLibH2errMsgs[] = H2_DEV_LIB_H2_ERR_MSGS;

#include "smMemLib.h"
#include "smObjLib.h"


#ifdef COMLIB_DEBUG_H2DEVLIB
# define LOGDBG(x)	logMsg x
#else
# define LOGDBG(x)
#endif

/* Local fucntions prototypes */
static int h2devAllocAux(const char *name, H2_DEV_TYPE type, int h2devMax);
static int h2devFindAux(const char *name, H2_DEV_TYPE type, int h2devMax);

/*----------------------------------------------------------------------*/

/**
 ** Record errors messages
 **/
int
h2devRecordH2ErrMsgs(void)
{
    return h2recordErrMsgs("h2devRecordH2ErrMsg", "h2devLib", M_h2devLib,
                           sizeof(h2devLibH2errMsgs)/sizeof(H2_ERROR),
                           h2devLibH2errMsgs);
}

/**
 ** Allocation d'un device h2
 **/
static int
h2devAllocAux(const char *name, H2_DEV_TYPE type, int h2devMax)
{
    int i;

    /* Verifie que le nom n'existe pas */
    if (type != H2_DEV_TYPE_SEM && h2devFindAux(name, type, h2devMax) != ERROR) {
        errnoSet(S_h2devLib_DUPLICATE_DEVICE_NAME);
        return ERROR;
    }
    /* Recherche un device libre */
    for (i = 0; i < h2devMax; i++) {
        if (h2Devs[i].type == H2_DEV_TYPE_NONE) {
            /* Trouve' */
            if (snprintf(h2Devs[i].name, H2_DEV_MAX_NAME, "%s", name)
                >= H2_DEV_MAX_NAME) {
                LOGDBG(("comLib:h2devAlloc: device name too long\n"));
                errnoSet(S_h2devLib_BAD_PARAMETERS);
                return ERROR;
            }
            strncpy(h2Devs[i].name, name, H2_DEV_MAX_NAME-1);
            h2Devs[i].type = type;
            h2Devs[i].uid = getuid();
            /* increment previous generation number */
            h2Devs[i].devgen += 1 << (8*sizeof(int) - H2_DEV_GEN_BITS);
            LOGDBG(("comLib:h2devAlloc: created device %d (gen %d)\n", i,
                     H2DEV_GEN(h2Devs[i].devgen)));
            return H2DEV_BY_INDEX(i);
        }
    } /* for */

    /* Pas de device libre */
    errnoSet(S_h2devLib_FULL);
    return ERROR;
}

int
h2devAlloc(const char *name, H2_DEV_TYPE type)
{
    int i, h2devMax;

    if (h2devAttach(&h2devMax) == ERROR) {
        return ERROR;
    }

    h2semTake(0, WAIT_FOREVER);
    i = h2devAllocAux(name, type, h2devMax);
    h2semGive(0);

    return i;
}

int
h2devAllocUnlocked(const char *name, H2_DEV_TYPE type)
{
    int h2devMax;

    if (h2devAttach(&h2devMax) == ERROR) {
        return ERROR;
    }

    return h2devAllocAux(name, type, h2devMax);
}

/*----------------------------------------------------------------------*/

/**
 ** Liberation d'un device h2
 **/
STATUS
h2devFree(int dev)
{
    uid_t uid = getuid();

    if (h2devAttach(NULL) == ERROR) {
        return ERROR;
    }
    if (uid != H2DEV_UID(dev) && uid != H2DEV_UID(0)) {
        errnoSet(S_h2devLib_NOT_OWNER);
        return ERROR;
    }
    h2semTake(0, WAIT_FOREVER);
    H2DEV_TYPE(dev) = H2_DEV_TYPE_NONE;
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
   int i, d, match = 0, h2devMax = 0;
   unsigned char *pool;

   if (h2devAttach(&h2devMax) == ERROR) {
      return ERROR;
   }
   /* Look for devices */
   for (d = 0; d < h2devMax; d++) {
      i = H2DEV_BY_INDEX(d);
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
h2devFindAux(const char *name, H2_DEV_TYPE type, int h2devMax)
{
    int i;

    for (i = 0; i < h2devMax; i++) {
        if ((type == h2Devs[i].type)
            && (strcmp(name, h2Devs[i].name) == 0)) {
          return H2DEV_BY_INDEX(i);
        }
    } /* for */
    return ERROR;
}

int
h2devFind(const char *name, H2_DEV_TYPE type)
{
    int i, h2devMax;

    if (name == NULL) {
        errnoSet(S_h2devLib_BAD_PARAMETERS);
        return ERROR;
    }
    if (h2devAttach(&h2devMax) == ERROR) {
        return ERROR;
    }
    h2semTake(0, H2DEV_TIMEOUT);
    i = h2devFindAux(name, type, h2devMax);
    h2semGive(0);

    if (i != ERROR) {
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
    if (h2devAttach(NULL) == ERROR) {
        return ERROR;
    }
    if (h2Devs[0].type != H2_DEV_TYPE_SEM) {
        errnoSet(S_h2devLib_BAD_DEVICE_TYPE);
        return ERROR;
    }
    return h2Devs[0].data.sem.semId;
}
