/*
 * Copyright (c) 2010,2024 CNRS/LAAS
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
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include <portLib.h>
#include <taskLib.h>


static int
suspend_test(void)
{
   logMsg("trying to suspend ourselves\n");
   taskSuspend(0);
   logMsg("not suspended...\n");
   return 0;
}

int
pocoregress_init(void)
{
	pid_t pid, result;
	int status;

	pid = fork();
	switch (pid) {
	case -1:
		warn("fork");
		return 2;
	case 0:
		return suspend_test();
	default:
		do {
			result = waitpid(pid, &status, 0);
		} while (result == -1 && errno == EINTR);
		if (result == pid && 
		    WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT) 
			return 0;
		if (result == -1) {
			warn("waitpid");
			return 2;
		}
		warnx("process didn't suspend\n");
		return 2;
	}
}
