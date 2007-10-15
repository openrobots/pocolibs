/*
 * Copyright (c) 2004 CNRS/LAAS
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

#include <stdio.h>

#include "h2devLib.h"
#include "h2timerLib.h"

extern int pocoregress_init(void);

int
main(int argc, char *argv[])
{
	int status;
	
	osInit(100);
	if (h2devInit(1<<10, FALSE) == ERROR) {
		char buf[1024];
		h2getErrMsg(errnoGet(), buf, sizeof(buf));
		printf("cannot create h2 devices: %s\n", buf);
		exit(2);
	}
	
	if (h2timerInit() == ERROR) {
		printf("cannot not initialize h2 timers\n");
		h2devEnd();
		exit(2);
	}
	
	printf("--- pocolibs regression test -------------------\n");
	status = pocoregress_init();
	
	h2timerEnd();
	h2devEnd();
	osExit();
	return status;
}
