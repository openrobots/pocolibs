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

#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <asm/uaccess.h>

#include <taskLib.h>
#include <errnoLib.h>

#include "h2device.h"

#ifdef COMLIB_DEBUG_H2DEVLIB
# define LOGDBG(x)	logMsg x
# define LOGKDBG(x)	logMsg x

static const char *	h2ioctlname(int n);
#else
# define LOGDBG(x)
# define LOGKDBG(x)
#endif

/* local functions */
static int copy_to_buffer(struct h2device *h, void *user, size_t len);
static int tIoctlTask(struct h2device *h);


/*
 * --- h2devopen --------------------------------------------------------
 *
 * Performs initialization for the current process
 */

int
h2devopen(struct inode *i, struct file *f)
{
   int e;
   char tName[16];
   struct h2device *h;

   LOGKDBG(("h2device: opening device\n"));

   /* setup private data */
   h = kmalloc(sizeof(*h), GFP_KERNEL);
   if (!h) return -ENOMEM;
   f->private_data = h;

   h->userbuffer = NULL;
   h->usersize = 0;
   sema_init(&h->sem, 1/*full*/);
   h->sync = semBCreate(0, SEM_EMPTY);
   if (!h->sync) { e = ENOMEM; goto err; }

   /* spawn the task that will handle ioctls on behalf of the process
    * performing the open(). This is an unprivileged task. */
   snprintf(tName, sizeof(tName), "tIoctl%d", current->pid);
   h->ioTid = taskSpawn(tName, H2DEV_IOCTASKPRI,
			VX_FP_TASK|PORTLIB_UNPRIVILEGED,
			H2DEV_IOCTASKSTACKSZ, tIoctlTask, h);

   LOGKDBG(("h2device: device successfully opened\n"));
   return 0;

  err:
   if (h->sync) semDelete(h->sync);
   kfree(h);
   return -e;
}

int
h2devrelease(struct inode *i, struct file *f)
{
   struct h2device *h = f->private_data;

   if (!h) return 0;

   LOGKDBG(("h2device: releasing device\n"));

   taskDelete(h->ioTid);
   if (h->sync) semDelete(h->sync);
   if (h->userbuffer) kfree(h->userbuffer);
   kfree(h);

   LOGKDBG(("h2device: device released\n"));
   return 0;
}

int
h2devioctl(struct inode *i, struct file *f,
	   unsigned int request, unsigned long arg)
{
   struct h2device *h = f->private_data;
   size_t size;
   int e;

   if (_IOC_TYPE(request) != H2DEV_IOC_MAGIC) return -ENOTTY;
   if (_IOC_NR(request) > H2DEV_IOC_MAXNR) return -ENOTTY;

   LOGKDBG(("h2device: handling %s\n", h2ioctlname(request)));

   /* lock other threads of this process */
   if (down_interruptible(&h->sem)) return -ERESTARTSYS;

   /* copy arguments into kernel space */
   if (_IOC_DIR(request) & _IOC_WRITE) {
      size = (char *)&h->op.arg[H2_IOARGS_IN(request)] - (char *)&h->op;

      LOGKDBG(("h2device: copying %d bytes from user\n", size));

      if (copy_from_user(&h->op, (void *)arg, size)) {
	 up(&h->sem);
	 return -EFAULT;
      }

      /* a special case: we need to copy a user buffer */
      if (request == H2DEV_IOC_MBOXSEND) {
	 e = copy_to_buffer(h, h->op.arg[2]._buffer, h->op.arg[3]._long);
	 if (e) {
	    up(&h->sem);
	    return -e;
	 }
      }
   }

   h->or.err = 0;
   h->request = request;
   h->done = 0;

   switch(h->request) {
      /* Some operations are not real-time, so we do them here: they
       * involve allocating shared memory, which is not possible in hard
       * real-time anyway. For the real-time operations, see the default
       * case below. */
      case H2DEV_IOC_DEVINIT:
	 h->or.err = h2devInit(h->op.arg[0]._long);
	 break;

      case H2DEV_IOC_DEVEND:
	 h->or.err = h2devEnd();
	 break;

      default:
	 /* let the async task do the job in hard real-time */
	 h->user = current;
	 set_current_state(TASK_INTERRUPTIBLE);

	 semGive(h->sync);
	 schedule();

	 LOGKDBG(("h2device: async task done\n"));
	 if (!h->done) {
	    up(&h->sem);
	    LOGKDBG(("h2device: interrupted by signal\n"));
	    return -ERESTARTSYS;
	 }
	 break;
   }

   /* copy results to user space */
   if (_IOC_DIR(request) & _IOC_READ) {
      size = (char *)&h->or.arg[H2_IOARGS_OUT(request)] - (char *)&h->or;

      LOGKDBG(("h2device: copying %d bytes to user\n", size));

      if (copy_to_user((void *)arg, &h->or, size)) {
	 up(&h->sem);
	 return -EFAULT;
      }
   }

   up(&h->sem);

   LOGKDBG(("h2device: %s handled\n", h2ioctlname(request)));
   return h->or.err?-EIO:0;
}


/*
 * --- copy_to_buffer ---------------------------------------------------
 *
 * Copy user data to kernel buffer.
 */

static int
copy_to_buffer(struct h2device *h, void *user, size_t len)
{
   if (h->usersize < len) {
      if (h->userbuffer) kfree(h->userbuffer);

      h->userbuffer = kmalloc(len, GFP_KERNEL);
      if (!h->userbuffer) {
	 h->usersize = 0;
	 return ENOMEM;
      }
      h->usersize = len;
   }

   if (copy_from_user(h->userbuffer, user, len)) return EFAULT;

   return 0;
}


/*
 * --- tIoctlTask -------------------------------------------------------
 *
 * Handle ioctl requests
 */

static int
tIoctlTask(struct h2device *h)
{
   STATUS s = OK;

   while(1) {
      semTake(h->sync, WAIT_FOREVER);

      LOGDBG(("handling %s\n", h2ioctlname(h->request)));

      switch(h->request) {
#define IOCOP(x, y)	case H2DEV_IOC_ ## x: s = ((y)==ERROR)?ERROR:OK; break

	 /* h2devLib */
	 /* DEVINIT and DEVEND are not handled here */
	 IOCOP(DEVATTACH,	h2devAttach());
	 IOCOP(DEVALLOC,
	       h->or.arg[0]._long = h2devAlloc(h->op.arg[0]._string,
					       h->op.arg[1]._long));
	 IOCOP(DEVFREE,		h2devFree(h->op.arg[0]._long));
	 IOCOP(DEVCLEAN,	h2devClean(h->op.arg[0]._string));
	 IOCOP(DEVFIND,
	       h->or.arg[0]._long = h2devFind(h->op.arg[0]._string,
					      h->op.arg[1]._long));
	 IOCOP(DEVSHOW,		h2devShow());

	 /* h2semLib */
	 IOCOP(SEMALLOC,
	       h->or.arg[0]._semid = h2semAlloc(h->op.arg[0]._long));
	 IOCOP(SEMDELETE,	h2semDelete(h->op.arg[0]._semid));
	 IOCOP(SEMTAKE,
	       h->or.arg[0]._bool = h2semTake(h->op.arg[0]._semid,
					      h->op.arg[1]._long));
	 IOCOP(SEMGIVE,		h2semGive(h->op.arg[0]._semid));
	 IOCOP(SEMFLUSH,	h2semFlush(h->op.arg[0]._semid));
	 IOCOP(SEMSHOW,		h2semShow(h->op.arg[0]._semid));

	 /* mboxLib */
	 IOCOP(MBOXSEND,
	       mboxSend(h->op.arg[0]._mboxid, h->op.arg[1]._mboxid,
			h->userbuffer, h->op.arg[3]._long));

	 default:
	    errnoSet(ENOTTY);
	    s = ERROR;
	    break;
#undef IOCOP
      }
      
      if (s == ERROR) h->or.err = errnoGet();

      /* signal process we're done */
      h->done = 1;
      wake_up_process(h->user);

      LOGDBG(("done %s\n", h2ioctlname(h->request)));
   }
}


#ifdef COMLIB_DEBUG_H2DEVLIB
static const char *
h2ioctlname(int n)
{
   static const struct iocnames {
      int number;
      const char *name;
   } list[] = {
      { H2DEV_IOC_DEVINIT,	"H2DEV_IOC_DEVINIT"	},
      { H2DEV_IOC_DEVEND,	"H2DEV_IOC_DEVEND"	},
      { H2DEV_IOC_DEVATTACH,	"H2DEV_IOC_DEVATTACH"	},
      { H2DEV_IOC_DEVALLOC,	"H2DEV_IOC_DEVALLOC"	},
      { H2DEV_IOC_DEVFREE,	"H2DEV_IOC_DEVFREE"	},
      { H2DEV_IOC_DEVCLEAN,	"H2DEV_IOC_DEVCLEAN"	},
      { H2DEV_IOC_DEVFIND,	"H2DEV_IOC_DEVFIND"	},
      { H2DEV_IOC_DEVSHOW,	"H2DEV_IOC_DEVSHOW"	},

      { H2DEV_IOC_SEMALLOC,	"H2DEV_IOC_SEMALLOC"	},
      { H2DEV_IOC_SEMDELETE,	"H2DEV_IOC_SEMDELETE"	},
      { H2DEV_IOC_SEMTAKE,	"H2DEV_IOC_SEMTAKE"	},
      { H2DEV_IOC_SEMGIVE,	"H2DEV_IOC_SEMGIVE"	},
      { H2DEV_IOC_SEMFLUSH,	"H2DEV_IOC_SEMFLUSH"	},
      { H2DEV_IOC_SEMSHOW,	"H2DEV_IOC_SEMSHOW"	},

      { H2DEV_IOC_MBOXSEND,	"H2DEV_IOC_MBOXSEND"	},

      { -1, NULL }
   };

   struct iocnames *p;
   for(p=list; p->name; p++)
      if (p->number == n) return p->name;

   return "unknown";
}
#endif /* COMLIB_DEBUG_H2DEVLIB */
