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
#ifndef H_H2DEV_DEVICE
#define H_H2DEV_DEVICE

#include <portLib.h>
#include <semLib.h>
#include <h2devLib.h>
#include <h2semLib.h>
#include <mboxLib.h>
#include <posterLib.h>

/* /dev entry */
#define H2DEV_DEVICE_MAJOR	0	/* defaults to dynamic allocation */
#define H2DEV_DEVICE_NAME	"h2dev" 

/* ioctl argument */
struct h2devop {
   int err;			/* error code */
   union {
      BOOL _bool;
      long _long;
      void *_buffer;
      char _string[H2_DEV_MAX_NAME];
      H2SEM_ID _semid;
      MBOX_ID _mboxid;
      POSTER_ID _posterid;
   } arg[4];			/* four args at most */
};

/* ioctl requests */
#include <linux/ioctl.h>

#define H2DEV_IOC_MAGIC		0xD3

/* used to create numbers */
#define H2_IOARGS(in, out)	((((in)&0xf)<<4) | ((out)&0xf))
#define H2_IOR(nr, out)		\
   _IOC(_IOC_READ, H2DEV_IOC_MAGIC, (nr), H2_IOARGS(0, out))
#define H2_IOWR(nr, in, out)	\
   _IOC(_IOC_READ|_IOC_WRITE, H2DEV_IOC_MAGIC, (nr), H2_IOARGS(in, out))

/* used to decode args number */
#define H2_IOARGS_IN(nr)	((_IOC_SIZE(nr)>>4)&0xf)
#define H2_IOARGS_OUT(nr)	(_IOC_SIZE(nr)&0xf)

/* h2devLib */
#define H2DEV_IOC_DEVINIT	H2_IOWR(  0, 1, 0)
#define H2DEV_IOC_DEVEND	H2_IOR(   1,    0)
#define H2DEV_IOC_DEVATTACH	H2_IOR(   2,    0)
#define H2DEV_IOC_DEVALLOC	H2_IOWR(  3, 2, 1)
#define H2DEV_IOC_DEVFREE	H2_IOWR(  4, 1, 0)
#define H2DEV_IOC_DEVCLEAN	H2_IOWR(  5, 1, 0)
#define H2DEV_IOC_DEVFIND	H2_IOWR(  6, 2, 1)
#define H2DEV_IOC_DEVSHOW	H2_IOR(   7,    0)

/* semLib */
#define H2DEV_IOC_SEMALLOC	H2_IOWR( 10, 1, 1)
#define H2DEV_IOC_SEMDELETE	H2_IOWR( 11, 1, 0)
#define H2DEV_IOC_SEMTAKE	H2_IOWR( 12, 2, 1)
#define H2DEV_IOC_SEMGIVE	H2_IOWR( 13, 1, 0)
#define H2DEV_IOC_SEMFLUSH	H2_IOWR( 14, 1, 0)
#define H2DEV_IOC_SEMSHOW	H2_IOWR( 15, 1, 0)

/* mboxLib */
#define H2DEV_IOC_MBOXSEND	H2_IOWR( 20, 4, 0)

/* highest ioctl number */
#define H2DEV_IOC_MAXNR		20

#ifdef __KERNEL__

#include <asm/semaphore.h>

#define H2DEV_IOCTASKPRI	200	/* this should be low */
#define H2DEV_IOCTASKSTACKSZ	20480	/* XXX to be adjusted */

/* device private data */
struct h2device {
   struct semaphore sem;	/* mutex semaphore */
   long ioTid;			/* RT-task performing operations */
   SEM_ID sync;			/* synchronization with RT-task */
   int done;

   int request;			/* ioctl request # */
   struct h2devop op;		/* operation params */
   struct h2devop or;		/* operation results */

   void *userbuffer;		/* moving user data into kernel space */
   size_t usersize;
};

int	h2devopen(struct inode *i, struct file *f);
int	h2devrelease(struct inode *i, struct file *f);
int	h2devioctl(struct inode *i, struct file *f, unsigned int request,
		unsigned long arg);
STATUS	h2devinstallPosterHook(STATUS (*hook)(struct h2device *h));

#endif /* __KERNEL__ */

#endif /* H_H2DEV_DEVICE */
