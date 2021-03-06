/*
 * Copyright (c) 2004,2012 CNRS/LAAS
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

#include "portLib.h"

#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>

#include "h2devLib.h"
#include "h2timerLib.h"

extern int pocoregress_init(void);

int
main(int argc, char *argv[])
{
	int status;

	osInit(100);
	h2devEnd();
	if (h2devInit(1024*1024, H2_DEV_MAX_DEFAULT, FALSE) == ERROR) {
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

	printf("--- %s regression test -------------------\n",
               basename(argv[0]));
	status = pocoregress_init();

	h2timerEnd();
	h2devEnd();
	osExit();
	return status;
}
