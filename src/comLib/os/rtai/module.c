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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#ifdef CONFIG_DEVFS_FS
# include <linux/devfs_fs_kernel.h>
#endif

#include <portLib.h>
#include <taskLib.h>
#include <h2devLib.h>
#include <h2timerLib.h>
#include <smMemLib.h>

#include "h2device.h"

MODULE_AUTHOR ("LAAS/CNRS");
MODULE_DESCRIPTION ("comLib - real-time communication layer");
MODULE_LICENSE ("BSD");

int h2dev_major = H2DEV_DEVICE_MAJOR;	/* defaults to dynamic allocation */
MODULE_PARM(h2dev_major, "i");
MODULE_PARM_DESC(h2dev_major, "h2dev major number");

static struct file_operations fops;
#ifdef CONFIG_DEVFS_FS
static devfs_handle_t de;
#endif

#include "h2initGlob.h"

/**
 ** comLib initialization for an RTAI module
 **/

STATUS
h2initGlob(int ticksPerSec)
{
   /* There is nothing to do here. comLib should already be present in
    * the kernel and configured properly. */

   /* Maybe we should however report an error if ticksPerSec is not the
    * same as the current one? */

   return OK;
}


/**
 ** comLib initialization
 **/

int
comLib_init(void)
{
   int status;

   /* initialize timers */
   if (h2timerInit() == ERROR) {
      logMsg("comLib: could not initialize h2 timers\n");
      return -EIO;
   }
   logMsg("comLib: timers initialized\n");

   /* register /dev/h2dev device */
   memset(&fops, 0, sizeof(fops));
   SET_MODULE_OWNER(&fops);
   fops.open = h2devopen;
   fops.release = h2devrelease;
   fops.ioctl = h2devioctl;

#ifdef CONFIG_DEVFS_FS
   de = devfs_register(NULL, H2DEV_DEVICE_NAME,
		       h2dev_major?0:DEVFS_FL_AUTO_DEVNUM, h2dev_major, 0,
		       S_IFCHR|S_IRUGO|S_IWUGO, &fops, NULL);
   if (!de) {
      logMsg("comLib: cannot register device `" H2DEV_DEVICE_NAME "\n");
      return -EIO;
   }
#else
   status = register_chrdev(h2dev_major, H2DEV_DEVICE_NAME, &fops);
   if (status < 0) {
      logMsg("comLib: cannot register device `" H2DEV_DEVICE_NAME
	     "' with major `%d'\n", h2dev_major);
      return status;
   }
   if (h2dev_major == 0) h2dev_major = status; /* dynamic allocation */
   logMsg("comLib: registered device `" H2DEV_DEVICE_NAME
	  "' with major %d\n", h2dev_major);
#endif /* CONFIG_DEVFS_FS */

   logMsg("comLib: successfully loaded\n");
   return 0;
}

void
comLib_exit(void)
{
   int status;

   /* destroy all remaining devices */
   if (h2timerEnd() == ERROR) {
      logMsg("comLib: could not delete h2 timers\n");
   }

   if (h2devEnd() == ERROR) {
      logMsg("Error: could not delete h2 devices\n");
   }

#ifdef CONFIG_DEVFS_FS
   if (de) devfs_unregister(de);
#else
   /* unregister /dev/h2dev device */
   status = unregister_chrdev(h2dev_major, H2DEV_DEVICE_NAME);
   if (status < 0) {
      logMsg("comLib: cannot unregister device\n");
   }
#endif /* CONFIG_DEVFS_FS */

   logMsg("comLib: unloaded\n");
}

module_init(comLib_init);
module_exit(comLib_exit);
