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

#include "pocolibs-config.h"
__RCSID("$LAAS$");
#include <stdio.h>
#include <stdlib.h>

#ifdef VXWORKS
#include <vxWorks.h>
#else
#include <portLib.h>
#endif

#include "posterLib.h"
#include <errnoLib.h>
#include "h2errorLib.h"

int
main(int argc, char **argv)
{
    char *nom;
    int i;
    POSTER_ID id;
    
    if (argc == 1) {
	nom = "test";
    } else {
	nom = argv[1];
    }

    if (posterCreate(nom, sizeof(int), &id) != OK) {
	printf("Error posterCreate %x\n", errnoGet());
	h2printErrno(errnoGet());
	exit(-1);
    }
    
    while (1) {

	scanf("%d", &i);
	/* 	
	posterWrite(id, 0, &i, sizeof(int));
	*/

	if (posterTake(id, POSTER_WRITE) == ERROR) {
		fprintf(stderr, "Error posterTake: ");
		h2printErrno(errnoGet());
	}
	*(int *)posterAddr(id) = i;
	if (posterGive(id) == ERROR) {
		fprintf(stderr, "Error posterGive: ");
		h2printErrno(errnoGet());
	}

	if (i == -1) {
	    printf("Bye\n");
	    posterDelete(id);
	    exit(0);
	}
    }
    return 0;
}
