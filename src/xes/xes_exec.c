/*
 * Copyright (c) 1994-2003 CNRS/LAAS
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "xes.h"

void
main(int argc, char **argv)
{
    int fd;
    
    if (argc < 3) {
	fprintf(stderr, "usage: xes_exec machine command [arg] ...\n");
	exit(1);
    }
    if ((fd = xes_init(argv[1])) < 0) {
	perror("xes_init");
	exit(2);
    }
    dup2(fd, 2);
    printf("coucou1");
    
    if (fork() == 0) {
	printf("coucou2");
	execv(argv[2], argv+2);
    }
    exit(0);
}
    
