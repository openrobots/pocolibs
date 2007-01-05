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
#include <unistd.h>
#include <stdlib.h>

#include <portLib.h>
#include "posterLib.h"
#include "errnoLib.h"
#include "h2errorLib.h"

int 
main(int ac, char **av)
{
    int i;
    POSTER_ID id;
    char *nom;

    if (ac == 1) {
	nom = "test";
    } else {
	nom = av[1];
    }

    if (posterFind(nom, &id) != OK) {
	printf("Error in posterFind\n");
	h2printErrno(errnoGet());
	exit (-1);
    }

    while (1) {
	if (posterRead(id, 0, (void *)&i, sizeof(int)) < 0) {
	    printf("Poster closed %d\n", errnoGet());
	    exit(0);
	}
	printf("Using posterRead: %d\n", i);
	sleep(1);
	if (posterTake(id, POSTER_READ) != OK) {
	    printf("Poster closed\n");
	    exit(0);
	}
	printf("Using posterTake/posterAddr: %d\n", *(int *)posterAddr(id));
	posterGive(id);
	sleep(1);
    }
    return 0;
}

		   

