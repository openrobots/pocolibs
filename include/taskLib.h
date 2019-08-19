/*
 * Copyright (c) 1999, 2003-2011 CNRS/LAAS
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

#ifndef _TASKLIB_H
#define _TASKLIB_H

#include "semLib.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Flags for tasks
 */
#define VX_FP_TASK		0x0008 /* f-point coprocessor support */

/*
 * Opaque task descriptor
 */
typedef struct OS_TCB OS_TCB, WIND_TCB;

/*
 * Function prototypes
 */
extern STATUS taskLibInit(void);
#ifdef TASKLIB_C
extern long taskSpawn(char *, int, int, int, FUNCPTR, int, int, int,
		       int, int, int, int, int, int, int);
#else
/* Awful hack */
extern long taskSpawn(char *name, int priority, int options,
    int stackSize, FUNCPTR entryPt, ... ) __attribute__((deprecated("use taskSpawn2() instead")));
#endif
extern long taskSpawn2(const char *name, int priority, int options, int stackSize,
    void *(*start_routine)(void *), void *arg);
extern STATUS taskDelete(long);
extern const char *taskName(long);
extern STATUS taskSuspend(long);
extern STATUS taskResume(long);
extern STATUS taskPrioritySet(long, int);
extern STATUS taskPriorityGet(long, int *);
extern STATUS taskLock(void);
extern STATUS taskUnlock(void);
extern STATUS taskDelay(int);
extern long taskIdSelf(void);
extern OS_TCB *taskTcb(long);
extern STATUS taskSetUserData(long, unsigned long);
extern unsigned long taskGetUserData(long);
extern STATUS taskOptionsGet(long, int *);
extern STATUS taskOptionsSet(long, int, int);
extern long taskNameToId(const char *);
extern long taskFromThread(const char *);

#ifdef __cplusplus
}
#endif

#endif
