/* $LAAS$ */
/*
 * Copyright (c) 1999, 2003-2004 CNRS/LAAS
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
#define PORTLIB_UNPRIVILEGED	0x8000 /* INTERNAL: comLib/rtai only */

/*
 * Opaque task descriptor
 */
typedef struct OS_TCB OS_TCB, WIND_TCB;

/*
 * Function prototypes
 */
extern STATUS taskLibInit(void);
#ifdef TASKLIB_C
extern long taskSpawn ( char *name, int priority, int options, int stackSize, 
		       FUNCPTR entryPt, int arg1, int arg2, int arg3, 
		       int arg4, int arg5, int arg6, int arg7, int arg8, 
		       int arg9, int arg10  );
#else
extern long taskSpawn ( char *name, int priority, int options, 
		       int stackSize, FUNCPTR entryPt, ... );
#endif
extern STATUS taskDelete ( long tid );
extern const char *taskName ( long tid );
extern STATUS taskSuspend ( long tid );
extern STATUS taskResume ( long tid );
extern STATUS taskPrioritySet ( long tid, int newPriority );
extern STATUS taskPriorityGet ( long tid, int *pPriority );
extern STATUS taskLock ( void );
extern STATUS taskUnlock ( void );
extern STATUS taskDelay ( int ticks );
extern long taskIdSelf ( void );
extern OS_TCB *taskTcb ( long tid );
extern STATUS taskSetUserData ( long tid, unsigned long data );
extern unsigned long taskGetUserData ( long tid );
extern STATUS taskOptionsGet ( long tid, int *pOptions );
extern STATUS taskOptionsSet ( long tid, int mask, int newOptions );
extern long taskNameToId(char *name);

#ifdef __cplusplus
};
#endif

#endif
