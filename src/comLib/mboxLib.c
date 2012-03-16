/*
 * Copyright (c) 1990, 2003-2005,2012 CNRS/LAAS
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
#include "taskLib.h"
#include "errnoLib.h"
#include "h2devLib.h"
#include "h2semLib.h"
#include "h2rngLib.h"
#include "h2errorLib.h"
#include "mboxLib.h"
#include "smObjLib.h"

static const H2_ERROR mboxLibH2errMsgs[] = MBOX_LIB_H2_ERR_MSGS;
static const H2_ERROR h2rngLibH2errMsgs[] = H2_RNG_LIB_H2_ERR_MSGS;

#ifdef COMLIB_DEBUG_MBOXLIB
# define LOGDBG(x)     logMsg x
#else
# define LOGDBG(x)
#endif

/*----------------------------------------------------------------------*/

/**
 ** Mailboxes initialisations
 **
 ** Description :
 ** This procedure must be called during initialization
 ** in each task using mailboxes
 **/
STATUS
mboxInit(char *procName)		/* unused parameter procName */
{
    long tid = taskIdSelf();
    const char *tName;
    int dev;

    /* record error msgs */
    h2recordErrMsgs("mboxInit", "h2rngLib", M_h2rngLib, 			
		    sizeof(h2rngLibH2errMsgs)/sizeof(H2_ERROR), 
		    h2rngLibH2errMsgs);
    h2recordErrMsgs("mboxInit", "mboxLib", M_mboxLib, 			
		    sizeof(mboxLibH2errMsgs)/sizeof(H2_ERROR), 
		    mboxLibH2errMsgs);

    if (tid == 0) return ERROR;
    tName = taskName(tid);

    /* Look for the h2 device associated with current task */
    dev = h2devFind((char *)tName, H2_DEV_TYPE_TASK);

    /* If not found, create one */
    if (dev == ERROR) {
 
        /* reset error */
        errnoSet(0);

	/* alloc dev */
	dev = h2devAlloc((char *)tName, H2_DEV_TYPE_TASK);
	if (dev == ERROR) {
	    return(ERROR);
	}
	H2DEV_TASK_TID(dev) = tid;

	/* Create a synchronization semaphore for this task */
	H2DEV_TASK_SEM_ID(dev) = h2semAlloc(H2SEM_SYNC);
	if (H2DEV_TASK_SEM_ID(dev) == ERROR) {
	    return ERROR;
	}
	/* Store the device index in the user data of this task */
	taskSetUserData(0, dev);
	LOGDBG(("comLib:mboxInitSelf: initialized for task %lx\n", tid));

    } else {
	/* Existing device found, check consistency */
	if (H2DEV_TASK_TID(dev) != tid || taskGetUserData(0) != dev) {
	    errnoSet(S_mboxLib_NAME_IN_USE);
	    return(ERROR);
	}
	LOGDBG(("comLib:mboxInitSelf: task %lx already initialized\n", tid));
    }
    return(OK);
}


/*----------------------------------------------------------------------*/

STATUS
mboxEnd(long taskId)
{
    int i; 
    long dev;

    if (taskId == 0) {
	taskId = taskIdSelf();
    }
    /* Device for the given task */
    dev = taskGetUserData(taskId);

    /* Free all mailboxes attached to this task */
    for (i = 0; i < H2_DEV_MAX; i++) {
	if (H2DEV_TYPE(i) == H2_DEV_TYPE_MBOX
	    && H2DEV_MBOX_TASK_ID(i) == dev) {
	    mboxDelete(i);
	}
    }
    /* Free the global synchronisation semaphore of the task */
    h2semDelete(H2DEV_TASK_SEM_ID(dev));
    /* Free the device for this task */
    h2devFree(dev);
    return(OK);
}

/*----------------------------------------------------------------------*/

/**
 **  mboxCreate  -  Create a mailbox device
 **
 **  Description:
 **  Create a mailbox with given name and size
 **
 **  Returns: OK or ERROR
 **/

STATUS
mboxCreate(char *name, int size, MBOX_ID *pMboxId)
{
    H2_MBOX_STR *mbox;
    H2RNG_ID rngId;
    MBOX_ID dev;

    /* Allocate a h2 device */
    dev = h2devAlloc(name, H2_DEV_TYPE_MBOX);
    if (dev == ERROR) {
        return(ERROR);
    }
    mbox = H2DEV_MBOX_STR(dev);
    /* Create a global mutex sempaphore */
    if ((mbox->semExcl = h2semAlloc(H2SEM_EXCL)) == ERROR) {
	return ERROR;
    }
    /* Create a global synchronization semaphore */
    if ((mbox->semSigRd = h2semAlloc(H2SEM_SYNC)) == ERROR) {
	return ERROR;
    }
    LOGDBG(("comLib:mboxCreate: semaphores created\n"));

    /* Allocate a ring buffer */
    rngId = h2rngCreate(H2RNG_TYPE_BLOCK, size);
    if (rngId == NULL) {
	return ERROR;
    }
    /* Store the global identifier of this ring buffer */
    mbox->rngId = (H2RNG_ID)smObjLocalToGlobal(rngId);

    /* Other informations */
    mbox->size = size;
    mbox->taskId = taskGetUserData(0);

    /* That's it */
    *pMboxId = dev;
    LOGDBG(("comLib:mboxCreate: new mbox %d of size %d\n", dev, size));
    return OK;
}

/*----------------------------------------------------------------------*/

STATUS
mboxDelete(MBOX_ID mboxId)
{
    uid_t uid = getuid();

    if (uid != H2DEV_UID(mboxId) && uid != H2DEV_UID(0)) {
	errnoSet(S_mboxLib_NOT_OWNER);
	return ERROR;
    }
    /* free the ring buffer */
    h2rngDelete (smObjGlobalToLocal(H2DEV_MBOX_RNG_ID(mboxId)));

    /* Free the synchronization semaphore */
    h2semDelete (H2DEV_MBOX_SEM_ID(mboxId));

    /* Free the mutex semaphore */
    h2semDelete (H2DEV_MBOX_SEM_EXCL_ID(mboxId));

    /* Free the h2 device */
    h2devFree(mboxId);

    return OK;
}

/*----------------------------------------------------------------------*/

/**
 **   mboxFind  -  Lookup a mailbox from its name
 **
 **   Description:
 **   This functions finds a mailbox device using the mailbox name
 **
 **   Returns:
 **   OK or ERROR
 **/
STATUS
mboxFind(char *name, MBOX_ID *pMboxId)
{
    MBOX_ID mbox;

    mbox = h2devFind(name, H2_DEV_TYPE_MBOX);
    if (mbox == ERROR) {
	return(ERROR);
    }
    *pMboxId = mbox;
    return OK;
}

/*----------------------------------------------------------------------*/

void
mboxShow(void)
{
    int i;
    int nMess, bytes, size;

    if (h2devAttach() == ERROR) {
	return;
    }
    logMsg("\n");
    logMsg("Name                              Id     Size NMes    Bytes\n");
    logMsg("-------------------------------- --- -------- ---- --------\n");
    for (i = 0; i < H2_DEV_MAX; i++) {
	if (H2DEV_TYPE(i) == H2_DEV_TYPE_MBOX) {
	    mboxIoctl(i, FIO_SIZE, &size);
	    mboxIoctl(i, FIO_NMSGS, &nMess);
	    mboxIoctl(i, FIO_NBYTES, &bytes);
	    logMsg("%-32s %3d %8d %4d %8d\n", H2DEV_NAME(i), i,
		   size, nMess, bytes);
	}
    } /* for */
    logMsg("\n");
}

/*----------------------------------------------------------------------*/

/**
 **   mboxIoctl  -  Ask for information about a mailbox
 **
 **   Description:
 **   similar to the Unix ioctl() function. Gets various informations
 **   about a mailbox device
 **
 **   Returns:
 **   OK or ERROR
 **/
STATUS
mboxIoctl(MBOX_ID mboxId, int codeFunc, void *pArg)
{
    H2RNG_ID rid;                /* ring buffer */
    int n;                       /* number of messages or bytes */

    /* Get the local address of the ring buffer */
    rid = (H2RNG_ID)smObjGlobalToLocal(H2DEV_MBOX_RNG_ID(mboxId));

    /* Execute the requested function */
    switch (codeFunc) {
      case FIO_NMSGS:                   /* Number of messages in
					   the mailbox */
	n = h2rngNBlocks (rid);
	break;

      case FIO_GETNAME:                 /* Name of the mailbox */

	(void) strcpy ((char *) pArg, H2DEV_NAME(mboxId));
	return (OK);

      case FIO_NBYTES:                  /* Number of bytes in the mailbox */

	n = h2rngNBytes (rid);
	break;

      case FIO_FLUSH:                   /* Clear the mailbox */

	h2rngFlush (rid);
	return (OK);

      case FIO_SIZE:                    /* Size of the mailbox */

	n = H2DEV_MBOX_STR(mboxId)->size;
	break;

      default:                          /* Unknown request */

	errnoSet (S_mboxLib_BAD_IOCTL_CODE);
	return (ERROR);
    } /* switch */

    /* Check for errors */
    if (n == ERROR)
	return (ERROR);

    /* Store the result */
    *((int *) pArg) = n;
    return (OK);

}

/*----------------------------------------------------------------------*/

int
mboxRcv(MBOX_ID mboxId, MBOX_ID *pFromId, char *buf, int maxbytes,
	int timeout)
{
    int nr;                       /* number of read bytes */
    int takeStat;                 /* status of semTake() */
    H2RNG_ID rid;

    /* Compute local address of ring buffer */
    rid = (H2RNG_ID)smObjGlobalToLocal(H2DEV_MBOX_RNG_ID(mboxId));

    /* Flush the synchronisation semaphore */
    h2semFlush(H2DEV_MBOX_SEM_ID(mboxId));

    /* Wait for a message */
    while (1) {

	/* Check if a message is available */
	if ((nr = h2rngNBytes (rid)) > 0) {
	    /* Read it */
	    nr = h2rngBlockGet (rid, (int *) pFromId, buf, maxbytes);
	    LOGDBG(("comLib:mboxRcv: read %d bytes from mbox %d in mbox %d\n",
		    nr, *(int *)pFromId, mboxId));
	    return (nr);
	}

	/* return if ERROR */
	if (nr < 0)
	    return (ERROR);

	/* otherwise, wait */
	if ((takeStat = h2semTake (H2DEV_MBOX_SEM_ID(mboxId), timeout))
	    != TRUE)
	    return (takeStat);
    }
}

/*----------------------------------------------------------------------*/

/**
 **   mboxPause  - Wait for a message in a mailbox
 **
 **   Description:
 **   suspends the execution of the current task until a message becomes
 **   available in the selected mailbox
 **
 **   Returns: TRUE, FALSE or ERROR
 **/
BOOL
mboxPause(MBOX_ID mboxId, int timeout)
{
    int nMbox;                  /* mailbox index */
    int nMes;                   /* number of messages in a mailbox */
    int takeStatus;             /* status of semTake() */
    long myTaskId;              /* my task identifier */
    H2SEM_ID semId;

    /* Check if we're waiting on all mailboxes */
    if (mboxId == ALL_MBOX) {
	myTaskId = taskGetUserData(0);
	/* get and flush my task synchronization semaphore */
	semId = H2DEV_TASK_SEM_ID(myTaskId);
	h2semFlush(semId);

	/* Wait for a message */
	while (1) {
	    /* Check for messages in one of mailboxes attached to this task */
	    for (nMbox = 0; nMbox < H2_DEV_MAX; nMbox++) {
		if (H2DEV_TYPE(nMbox) == H2_DEV_TYPE_MBOX
		    && H2DEV_MBOX_TASK_ID(nMbox) == myTaskId) {
		    if (mboxIoctl (nMbox, FIO_NMSGS, (char *) &nMes)
			== OK && nMes != 0) {
			return (TRUE);
		    }
		}
	    } /* for */

	    /* no message, wait for the synchronization semaphore */
	    LOGDBG(("comLib:mboxPause: waiting on sem #%d\n", semId));
	    if ((takeStatus = h2semTake(semId, timeout)) != TRUE) {
		return (takeStatus);
	    }
	} /* while */
    }


    /* Flush the synchonization semaphore */
    h2semFlush(H2DEV_MBOX_SEM_ID(mboxId));

    /* Wait for a message */
    while (1) {
	/* Check if a message is present in the mailbox */
	if (mboxIoctl (mboxId, FIO_NMSGS, (char *) &nMes) == OK && nMes != 0)
	    return (TRUE);

	/* otherwise, wait */
	if ((takeStatus = h2semTake (H2DEV_MBOX_SEM_ID(mboxId), timeout))
	    != TRUE) {
	    return (takeStatus);
	}
    } /* while */
}


/*----------------------------------------------------------------------*/

int
mboxSpy(MBOX_ID mboxId, MBOX_ID *pFromId, int *pNbytes,
	char *buf, int maxbytes)
{
    /* Spy the ring buffer */
    return h2rngBlockSpy (smObjGlobalToLocal(H2DEV_MBOX_RNG_ID(mboxId)),
			   (int *)pFromId, pNbytes, buf, maxbytes);
}

/*----------------------------------------------------------------------*/

STATUS
mboxSkip(MBOX_ID mboxId)
{
    /* Skip a message in the ring buffer */
    return h2rngBlockSkip (smObjGlobalToLocal(H2DEV_MBOX_RNG_ID(mboxId)));

}


/*----------------------------------------------------------------------*/

/**
 **  mboxSend  -  Send a message to a mailbox
 **
 **  Description:
 **  Sends a message to a mailbox device. Takes the mutex semaphore
 **  for the device, check that it exists, computes the local address
 **  of the ring buffer for this device, writes the messages in the
 **  ring buffer and frees the synchronization semaphore to signal
 **  the mailbox owner that a message was written.
 **
 **  Returns: OK or ERROR
 **/

STATUS
mboxSend(MBOX_ID toId, MBOX_ID fromId, char *buf, int nbytes)
{
    H2RNG_ID rngId;			/* ring buffer of the device */
    H2SEM_ID semTask;
    int result;
    char msg[64];

    if (H2DEV_TYPE(toId) != H2_DEV_TYPE_MBOX) {
      errnoSet(S_mboxLib_MBOX_CLOSED);
      return ERROR;
    }

    /* take the mutex semaphore of the device */
    if (h2semTake (H2DEV_MBOX_SEM_EXCL_ID(toId), WAIT_FOREVER) != TRUE) {
	return (ERROR);
    }
    /* Get the local address of the ring buffer */
    rngId = (H2RNG_ID)smObjGlobalToLocal(H2DEV_MBOX_RNG_ID(toId));

    /* Write a block corresponding to the message */
    if ((result = h2rngBlockPut (rngId, (int) fromId, buf, nbytes))
	!= nbytes) {
        LOGDBG(("comLib:mboxSend: wrote %d bytes in mbox %d\n", result, toId));
	if (result == 0) {
	    errnoSet (S_mboxLib_MBOX_FULL);
	}
	h2semGive (H2DEV_MBOX_SEM_EXCL_ID(toId));
	return (ERROR);
    }
    /* Signal the mailbox that there's a message to read */
    LOGDBG(("comLib:mboxSend: signaling sem #%d\n", H2DEV_MBOX_SEM_ID(toId)));
    if (h2semGive(H2DEV_MBOX_SEM_ID(toId)) == ERROR) {
	logMsg("erreur give semSigRd\n");
        return ERROR;
    }
    /* Signal the event to the task owning the mailbox */
    semTask = H2DEV_TASK_SEM_ID(H2DEV_MBOX_TASK_ID(toId));
    if (h2semSet(semTask, 1) == ERROR) {
      logMsg("comLib:mboxSend:h2semSet: %s", h2getErrMsg(errnoGet(), msg, 64));
	return ERROR;
    }
    /* Free the mutex */
    h2semGive(H2DEV_MBOX_SEM_EXCL_ID(toId));

    /* OK, done */
    LOGDBG(("comLib:mboxSend: wrote %d bytes in mbox %d\n", result, toId));
    return (OK);

}
