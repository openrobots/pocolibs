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

/*
 * Message logging facilities
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#include <rtai.h>

#define LOGLIB_C
#include "portLib.h"
#include "taskLib.h"
#include "logLib.h"

/* log queue */
struct LogQ {
   long tid;
   const char *fmt;
   int arg1, arg2, arg3, arg4, arg5, arg6;
};

static spinlock_t logQSync = SPIN_LOCK_UNLOCKED;
static struct LogQ *logQ;
static volatile int first, last, lost;
static int max;

/* XXX for some obscure reason, I cannot get wait queues working on PPC
 * (PCore 3750 board)... So we do it by hand :| */
struct task_list {
   struct task_struct *task;
   struct task_list *next;
};
static struct task_list *towakeup = NULL;

/* proc entry */
static struct file_operations logLibOperations;
static struct proc_dir_entry *logLib;

static ssize_t		logRead(struct file *file, char *buf,
				size_t count, loff_t *ppos);


/*
 * --- logInit ----------------------------------------------------------
 *
 * Initialize logging library. Parameter `fd' is ignored.
 */

STATUS
logInit(int fd, int maxMsgs)
{
   /* setup queue */
   logQ = vmalloc(maxMsgs * sizeof(*logQ));
   if (!logQ) return ERROR;

   first = last = lost = 0;
   max = maxMsgs;

   /* create proc entry */
   memset(&logLibOperations, 0, sizeof(logLibOperations));
   logLibOperations.read = logRead;

   logLib = create_proc_entry("portLib/logLib", 0, NULL);
   if (logLib) logLib->proc_fops = &logLibOperations;

   return OK;
}


/*
 * --- logInit ----------------------------------------------------------
 *
 * Cleanup logging library.
 */

void
logEnd()
{
   remove_proc_entry("portLib/logLib", NULL);
   vfree(logQ);
}


/*
 * --- logMsg -----------------------------------------------------------
 *
 * Log an error message
 */

int
logMsg(const char *fmt,
       int arg1, int arg2, int arg3, int arg4, int arg5, int arg6)
{
   unsigned long flags;
   
   flags = rt_spin_lock_irqsave(&logQSync);

   logQ[last].tid = taskIdSelf();
   logQ[last].fmt = fmt;
   logQ[last].arg1 = arg1;
   logQ[last].arg2 = arg2;
   logQ[last].arg3 = arg3;
   logQ[last].arg4 = arg4;
   logQ[last].arg5 = arg5;
   logQ[last].arg6 = arg6;

   last++; last %= max;
   if (last == first) {
      /* we overwrote an old message */
      lost++;
      first++; first %= max;
   }

   for (;towakeup; towakeup = towakeup->next) wake_up_process(towakeup->task);
   rt_spin_unlock_irqrestore(flags, &logQSync);

   return 1/*xxx should be the number of bytes, but we don't have it*/;
}


/*
 * --- logRead ----------------------------------------------------------
 *
 * Formats and display log information.  Note that if count is too small,
 * we will only read partial messages...
 */

static ssize_t
logRead(struct file *file, char *buf, size_t count, loff_t *ppos)
{
   unsigned long flags;
   const char *name;
   struct LogQ c;
   int n, len, err;
   char *page, *log;

   flags = rt_spin_lock_irqsave(&logQSync);

   /* if there are no messages, wait and sleep */
   if (first == last) {
      struct task_list me = { current, towakeup };
      struct task_list *p;

      /* add ourself to wake up list */
      towakeup = &me;
      set_current_state(TASK_INTERRUPTIBLE);

      rt_spin_unlock_irqrestore(flags, &logQSync);
      schedule();
      flags = rt_spin_lock_irqsave(&logQSync);

      /* remove ourself if we're still here */
      if (towakeup) {
	 if (towakeup == &me)
	    towakeup = towakeup->next;
	 else for(p = towakeup; p->next; p = p->next)
	    if (p->next == &me) {
	       p->next = p->next->next;
	       break;
	    }
      }

      /* if we've been awoken by a signal, return */
      if (first == last) {
	 rt_spin_unlock_irqrestore(flags, &logQSync);
	 return -ERESTARTSYS;
      }
   }

   /* copy next message locally */
   err = lost;
   c = logQ[first];

   lost = 0;
   first++; first %= max;

   rt_spin_unlock_irqrestore(flags, &logQSync);

   /* get some buffer */
   n = 0;
   log = page = (char *)__get_free_page(GFP_KERNEL);
   if (!page) return -ENOMEM;
   if (count > PAGE_SIZE) count = PAGE_SIZE;

   /* warn for lost messages */
   if (err) {
      len = snprintf(log, count,
		       "logTask: LOST %d MESSAGE%s\n", err, err>1?"S":"");
      n += len;
      log += len;
      count -= len;
   }

   /* format task name */
   if (c.tid) {
      name = taskName(c.tid);
      if (name)
	 len = snprintf(log, count, "%s: ", name);
      else
	 len = snprintf(log, count, "%ld: ", c.tid);
      n += len;
      log += len;
      count -= len;
   }

   /* format message itself */
   if (c.fmt) {
      len = snprintf(log, count, c.fmt,
		     c.arg1, c.arg2, c.arg3, c.arg4, c.arg5, c.arg6);
      n += len;
      log += len;
      count -= len;
   }

   if (copy_to_user(buf, page, n)) n = -EFAULT;
   free_page((unsigned long)page);
   return n;
}
