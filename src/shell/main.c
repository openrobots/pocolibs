/*
 * Copyright (c) 1990, 2003 CNRS/LAAS
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

/***
 *** Shell interactif a la mode VxWorks
 *** pour comLib/Posix
 ***/

#include "config.h"
__RCSID("$LAAS$");

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#include "portLib.h"
#include "h2initGlob.h"
#include "shellLib.h"
#include "xes.h"

static char *prompt = "h2> ";

static void
exitFunc(void)
{
    printf("Bye.\n");
}

int 
main(int argc, char *argv[])
{
    int freq = 0;
    
    if (argc == 2) {
	freq = atoi(argv[1]);
    }
    if (h2initGlob(freq) == ERROR) {
	exit(-1);
    }

    atexit(exitFunc);

    shellMainLoop(stdin, stdout, stderr, prompt);
    exit(0);
}
