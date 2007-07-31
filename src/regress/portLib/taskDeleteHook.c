/*
 * Copyright (c) 2007 CNRS/LAAS
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
#ident "$Id$"

#include <stdio.h>

#include <portLib.h>
#include <taskLib.h>
#include <taskHookLib.h>

int 
myDeleteHook(OS_TCB *tcb)
{
	printf("Calling delete hook for task %s\n", taskName((long)tcb));
	return 0;
}


int
suicideTask(void)
{
	printf("tSuicide: deleting self\n");
	taskDelete(0);
	return 0;
}

int 
returnTask(void)
{
	printf("tReturn: returning at the end of the task\n");
	return 0;
}

int
victimTask(void)
{
	SEM_ID sem;

	sem = semBCreate(SEM_Q_FIFO, SEM_EMPTY);

	printf("tVictim: blocking task, waiting for external murder\n");
	semTake(sem, WAIT_FOREVER);
	return 0;
}


int
pocoregress_init()
{
	long tid;

	taskDeleteHookAdd(myDeleteHook);
	tid = taskSpawn("tVictim", 10, VX_FP_TASK, 10000, victimTask, NULL);
	taskDelay(100);
	printf("killing tVictim\n");
	taskDelete(tid);
	taskDelay(100);
	i();
	taskSpawn("tSuicide", 10, VX_FP_TASK, 10000, suicideTask, NULL);
	taskDelay(100);
	i();
	taskSpawn("tReturn", 10, VX_FP_TASK, 10000, returnTask, NULL);
	taskDelay(100);
	i();
	return 0;
}

