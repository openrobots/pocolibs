/*
 * Copyright (c) 2004 
 *      Autonomous Systems Lab, Swiss Federal Institute of Technology.
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

#include <portLib.h>

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <linux/kdev_t.h>

#include <rtai_nam2num.h>
#define KEEP_STATIC_INLINE /* ... */
#include <rtai_shm.h>

#include <smObjLib.h>
#include <errnoLib.h>
#include <h2devLib.h>

#include "h2device.h"

#ifdef COMLIB_DEBUG_H2DEVLIB
# define LOGDBG(x)	logMsg x
#else
# define LOGDBG(x)
#endif

/* Global h2devices -
 * 
 * This is mmapped from kernel space, so don't write into it!
 * Ultimately I think this should be mapped read-only, but for now we
 * just use the simple rtai_shm scheme...
 */
H2_DEV_STR *h2Devs = NULL;

/* h2dev file descriptor */
int h2devfd = -1;

/* local functions */
static inline STATUS	doioctl(unsigned int r, struct h2devop *op);


/*
 * --- h2devGetKey ------------------------------------------------------
 *
 * Compute a unique key for type and dev. XXX This is a copy/paste of
 * the same function in kernel space. We should probably share the
 * definition to avoid getting out of sync at some point...
 * See h2devOsLib.c for additional comments.
 */

long
h2devGetKey(int type, int dev, BOOL dummy, int *dummy2)
{
   long key;

#if H2DEV_MAX_TYPES*(H2_DEV_MAX+1) > 0xfff
# error "Please revisit this function"
#endif

   key = nam2num("h2devs") & 0xfffff000;
   key |= (dev*H2DEV_MAX_TYPES + type) & 0xfff;

   LOGDBG(("comLib:h2devGetKey: key for type %d and dev %d is %d\n",
	   type, dev, key));

   return key;
}


/*
 * --- h2devInit --------------------------------------------------------
 *
 * Open device and do kernel initialization.
 */

STATUS
h2devInit(int smMemSize)
{
   STATUS s;
   struct h2devop op;   

   if (h2devfd >= 0) {
      errnoSet(EEXIST);
      return ERROR;
   }

   /* XXX we should probably use H2DEV_DIR env var (as in posix version,
    * but with a different purpose) to supersede the hardcoded /dev path */

   h2devfd = open("/dev/" H2DEV_DEVICE_NAME, O_RDWR);
   if (h2devfd < 0) {
      errnoSet(S_h2devLib_NOT_INITIALIZED);
      return ERROR;
   }

   LOGDBG(("comLib:h2devInit: opened /dev/" H2DEV_DEVICE_NAME "\n"));

   /* initialize on kernel side */
   op.arg[0]._long = smMemSize;
   s = doioctl(H2DEV_IOC_DEVINIT, &op);

   LOGDBG(("comLib:h2devInit: done with kernel init\n"));

   return s;
}


/*
 * --- h2devAttach ------------------------------------------------------
 *
 * Open device, attach on kernel's side and map shared memory.
 */

STATUS
h2devAttach(void)
{
   struct h2devop op;   
   long key;
   int i;

   if (h2devfd >= 0) return OK;

   key = h2devGetKey(H2_DEV_TYPE_H2DEV, 0, FALSE/*don't create*/, NULL);
   if (key == ERROR) return ERROR;

   h2devfd = open("/dev/" H2DEV_DEVICE_NAME, O_RDWR);
   if (h2devfd < 0) {
      errnoSet(S_h2devLib_NOT_INITIALIZED);
      return ERROR;
   }

   LOGDBG(("comLib:h2devAttach: opened /dev/" H2DEV_DEVICE_NAME "\n"));

   if (doioctl(H2DEV_IOC_DEVATTACH, &op) != OK) {
      close(h2devfd);
      h2devfd = -1;
      return ERROR;
   }

   LOGDBG(("comLib:h2devAttach: attached to h2 devices\n"));

   h2Devs = rtai_malloc(key, sizeof(H2_DEV_STR)*H2_DEV_MAX);
   if (!h2Devs) {
      close(h2devfd);
      h2devfd = -1;

      errnoSet(S_smObjLib_SHMGET_ERROR);
      return ERROR;
   }

   LOGDBG(("comLib:h2devAttach: mapped h2 devices at %p\n", h2Devs));

   LOGDBG(("Id   Type   UID Name\n"));
   LOGDBG(("------------------------------------------------\n"));
   for (i = 0; i < H2_DEV_MAX; i++) {
      if (H2DEV_TYPE(i) != H2_DEV_TYPE_NONE) {
	 LOGDBG(("%2d %2d %5d %s\n", i, H2DEV_TYPE(i), H2DEV_UID(i),
		 H2DEV_NAME(i)?H2DEV_NAME(i):"?"));
      }
   } /* for */
   LOGDBG(("------------------------------------------------\n"));

   return OK;
}


/*
 * --- H2DEV_IOC functions ----------------------------------------------
 *
 * Wrappers around ioctl calls.
 */

/* 
 * helper macro: p is prototype, x is ioctl #, b1 is function body before
 * ioctl and b2 function body after ioctl */
#define H2DEV_IOC(p, x, b1, b2)			\
   p {						\
      struct h2devop op;			\
      b1					\
      if (doioctl(x, &op)!=OK) return ERROR;	\
      b2					\
   }

/* helper macro: define function p as noop, returning status s */
#define H2DEV_NOOP(p, s)	p { return s; }


/* --- h2devLib ------------------------------------------------------ */

H2DEV_IOC(STATUS h2devEnd(void), H2DEV_IOC_DEVEND, {
      if (h2Devs) {
	 long key = h2devGetKey(H2_DEV_TYPE_H2DEV, 0, FALSE, NULL);
	 if (key == ERROR) return ERROR;
	 rtai_free(key, h2Devs);
	 h2Devs = NULL;
      }
   }, return OK;)
H2DEV_IOC(
   int h2devAlloc(char *name, H2_DEV_TYPE type), H2DEV_IOC_DEVALLOC, {
      strncpy(op.arg[0]._string, name, sizeof(op.arg[0]._string));
      op.arg[0]._string[sizeof(op.arg[0]._string)-1] = 0;
      op.arg[1]._long = type;
   }, {
      return op.arg[0]._long;
   })
H2DEV_IOC(int h2devFree(int dev), H2DEV_IOC_DEVFREE, {
      op.arg[0]._long = dev;
   }, return OK;)
H2DEV_IOC(STATUS h2devClean(const char *pattern), H2DEV_IOC_DEVCLEAN, {
      strncpy(op.arg[0]._string, pattern, sizeof(op.arg[0]._string));
      op.arg[0]._string[sizeof(op.arg[0]._string)-1] = 0;
   }, return OK;)
H2DEV_IOC(
   int h2devFind(char *name, H2_DEV_TYPE type), H2DEV_IOC_DEVFIND, {
      strncpy(op.arg[0]._string, name, sizeof(op.arg[0]._string));
      op.arg[0]._string[sizeof(op.arg[0]._string)-1] = 0;
      op.arg[1]._long = type;
   }, {
      return op.arg[0]._long;
   })
H2DEV_IOC(STATUS h2devShow(void), H2DEV_IOC_DEVSHOW,,return OK;)


/* --- h2semLib ------------------------------------------------------ */

H2DEV_NOOP(STATUS h2semInit(int num, int *pSemId),	OK)
H2DEV_NOOP(void h2semEnd(void),				)
H2DEV_NOOP(STATUS h2semCreate0(int semId, int value),	ERROR)

H2DEV_IOC(H2SEM_ID h2semAlloc(int type), H2DEV_IOC_SEMALLOC, {
      op.arg[0]._long = type;
   }, return op.arg[0]._semid;)
H2DEV_IOC(STATUS h2semDelete(H2SEM_ID sem), H2DEV_IOC_SEMDELETE, {
      op.arg[0]._semid = sem;
   }, return OK;)
H2DEV_IOC(BOOL h2semTake(H2SEM_ID sem, int timeout), H2DEV_IOC_SEMTAKE, {
      op.arg[0]._semid = sem;
      op.arg[1]._long = timeout;
   }, return op.arg[0]._bool;)
H2DEV_IOC(STATUS h2semGive(H2SEM_ID sem), H2DEV_IOC_SEMGIVE, {
      op.arg[0]._semid = sem;
   }, return OK;)
H2DEV_IOC(STATUS h2semFlush(H2SEM_ID sem), H2DEV_IOC_SEMFLUSH, {
      op.arg[0]._semid = sem;
   }, return OK;)
H2DEV_IOC(STATUS h2semShow(H2SEM_ID sem), H2DEV_IOC_SEMSHOW, {
      op.arg[0]._semid = sem;
   }, return OK;)


/* --- mboxLib ------------------------------------------------------- */

H2DEV_IOC(
   STATUS mboxSend(MBOX_ID toId, MBOX_ID fromId, char *buf, int nbytes ),
   H2DEV_IOC_MBOXSEND, {
      op.arg[0]._mboxid = toId;
      op.arg[1]._mboxid = fromId;
      op.arg[2]._buffer = buf;
      op.arg[3]._long = nbytes;
   }, return OK;)


/*
 * --- doioctl ----------------------------------------------------------
 *
 * Generic function that performs the requested ioctl and handle errors.
 */
 
static inline STATUS
doioctl(unsigned int r, struct h2devop *op)
{
   if (h2devfd < 0) {
      if (h2devAttach() == ERROR)
	 return ERROR;
   }

   if (ioctl(h2devfd, r, op) < 0) {
      errnoSet(op->err);
      return ERROR;
   }

   return OK;
}
