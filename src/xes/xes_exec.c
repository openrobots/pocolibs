/**
 ** Copyright (c) 1994 LAAS/CNRS 
 **
 ** Matthieu Herrb - Thu Aug  4 1994
 **
 ** $Source$
 ** $Revision$
 ** $Date$
 ** 
 **/
#ident "$Id$"

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
    
