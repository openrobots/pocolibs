/**
 ** Copyright (c) 1999 LAAS/CNRS 
 **
 ** Matthieu Herrb - Thu May 20 1999
 **
 ** $Source$
 ** $Revision$
 ** $Date$
 ** 
 **/
#ident "$Id$"

#include <portLib.h>
#include <taskLib.h>
#include <stdio.h>

int 
task(int i)
{
    while (1) {
	printf("%d\n", i);
	taskDelay(i);
    }
}

int 
main()
{
    osInit(50);

    taskSpawn("t1", 100, 0, 1000, task, 200);
    taskSpawn("t2", 100, 0, 1000, task, 300); 
    shellMainLoop(stdin, stdout, stderr, "task% ");
    return 0;
}

    
