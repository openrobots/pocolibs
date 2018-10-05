/*
 * Copyright (c) 2018 CNRS/LAAS
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

#include "pocolibs-config.h"

#include <sys/types.h>
#include "portLib.h"
#include "semLib.h"
#include "taskLib.h"
#include "h2devLib.h"
#include "smMemLib.h"
#include "gcomLib.h"

#define N_LETTER_TEST 1024

int
pocoregress_init(void)
{
	LETTER_ID letters[N_LETTER_TEST];
	int i;

	if (gcomInit("gcomAlloc", 256, 256) == ERROR) {
		logMsg("gcomInit failed");
		return 2;
	}
	for (i = 0; i < N_LETTER_TEST; i++) {
		if (gcomLetterAlloc(256, &letters[i]) == ERROR) {
			logMsg("gcomLetterAlloc failed at %d: %x\n", i,
			    errnoGet());
			gcomEnd();
			return 2;
		}
	}
	for (i = 0; i < N_LETTER_TEST; i++) {
		if (gcomLetterDiscard(letters[i]) == ERROR) {
			logMsg("gcomLetterDiscard failed at %d: %x\n", i,
			    errnoGet());
			gcomEnd();
			return 2;
		}
	}
	gcomEnd();
	return 0;
}

