/*
 * Copyright (c) 2019 CNRS/LAAS
 *
 * Permission to use, copy, modify, and/or distribute this software for any
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
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "pocolibs-config.h"

#include <sys/types.h>
#include "portLib.h"
#include "semLib.h"
#include "taskLib.h"
#include "gcomLib.h"
#include "h2devLib.h"
#include "h2evnLib.h"
#include "smMemLib.h"
#include "h2initGlob.h"
#include "csLib.h"


int
pocoregress_init(void)
{
	int i, devs[200];
	char name[32];

	logMsg("h2devMax test started\n");

	for (i = 0; i < 200; i++) {
		snprintf(name, sizeof(name), "th2devMax%d", i);
		devs[i] = h2devAlloc(name, H2_DEV_TYPE_MBOX);
		if (devs[i] == ERROR) {
			/* check if the limit was reached */
			if (i < h2devSize() - 2) {
				logMsg("error allocating mbox %d: %lx\n",
				    i, errnoGet());
				return 2;
			} else {
				logMsg("OK error at %d %d\n", i, h2devSize());
				return 0;
			}
		}
	}
	logMsg("hit %d\n", i);
	return 2;
}
