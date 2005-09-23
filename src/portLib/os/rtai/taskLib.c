/*
 * Copyright (c) 2004 
 *      Autonomous Systems Lab, Swiss Federal Institute of Technology.
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

#include <linux/slab.h>
#include <rtai_sched.h>
#include <rtai_schedcore.h>

#include "portLib.h"
static const H2_ERROR const portLibH2errMsgs[]  = PORT_LIB_H2_ERR_MSGS;

#include "errnoLib.h"
#include "sysLib.h"
#include "wdLib.h"
#define TASKLIB_C
#include "taskLib.h"
#include "taskHookLib.h"

#ifdef PORTLIB_DEBUG_TASKLIB
# define LOGDBG(x)	logMsg x
#else
# define LOGDBG(x)
#endif

extern int	sysClkTickDuration(void);

/*
 * Struture to pass parameters to a VxWorks-like task main routine
 */
typedef struct taskParams {
    int arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10;
} TASK_PARAMS;


/*
 * A VxWorks compatible task descriptor
 */
struct OS_TCB {
   RT_TASK rtid;		/* task id - must be the first element */

   int magic;			/* add some robustness */
#define PORTLIB_TASK_MAGIC	0x3e0f0e32

   char *name;			/* pointer to task name */
   int options;			/* task options bits */
   int priority;		/* vxWorks compatible prio */
   FUNCPTR entry;		/* entry point */
   int errorStatus;		/* error code */

   unsigned long userData;	/* user data */
   TASK_PARAMS params;		/* parameters */

   struct OS_TCB *next;		/* next tcb in list */
};

/* global list of tasks */
static OS_TCB *taskList = NULL;
static int numTask = 0;

/*
 * Task hooks globals 
 */
typedef struct TASK_HOOK_LIST {
    FUNCPTR hook;
    struct TASK_HOOK_LIST *next;
} TASK_HOOK_LIST;

static TASK_HOOK_LIST *createHooks = NULL;
static TASK_HOOK_LIST *deleteHooks = NULL;
/* static TASK_HOOK_LIST *switchHooks = NULL; */

static STATUS	executeHooks(TASK_HOOK_LIST *list, OS_TCB *tcb);
static void	taskCleanUp(void *tcb, int dummy);

/*----------------------------------------------------------------------*/

/*
 * Record errors messages
 */
int
portRecordH2ErrMsgs()
{
    return h2recordErrMsgs("portRecordH2ErrMsg", "portLib", M_portLib, 
			   sizeof(portLibH2errMsgs)/sizeof(H2_ERROR), 
			   portLibH2errMsgs);
}

/*----------------------------------------------------------------------*/

/*
 * Two functions to map a VxWorks-like  priority to an RTAI priority
 */
static int
priorityVxToRTAI(int priority)
{
   return RT_SCHED_HIGHEST_PRIORITY +
      ((RT_SCHED_LOWEST_PRIORITY - RT_SCHED_HIGHEST_PRIORITY) / 255) *
      priority;
}

/*----------------------------------------------------------------------*/

static int 
priorityRTAIToVx(int priority)
{
   return 255 * (priority - RT_SCHED_HIGHEST_PRIORITY) /
      (RT_SCHED_LOWEST_PRIORITY - RT_SCHED_HIGHEST_PRIORITY);
}

/*----------------------------------------------------------------------*/

/*
 * TaskLib init function
 */
STATUS
taskLibInit(void)
{
   return OK;
}

/*----------------------------------------------------------------------*/

/*
 * Thread clean-up function
 */
static void 
taskCleanUp(void *tcb, int dummy)
{
   LOGDBG(("portLib:taskCleanUp: calling delete hooks "
	   "for task 0x%p ('%s')\n", tcb, ((OS_TCB *)tcb)->name));

   executeHooks(deleteHooks, tcb);
}

/*----------------------------------------------------------------------*/

/* 
 * A Helper function to convert form VxWorks task function prototype to 
 * rtai task function prototype 
 */
void
taskStarter(int data)
{
    OS_TCB *tcb = (OS_TCB *)data;
    TASK_PARAMS *p = (TASK_PARAMS *)&(tcb->params);
    static int result;
    
    /* Execute create hooks */
    executeHooks(createHooks, tcb);

    result = (*(tcb->entry))(p->arg1, p->arg2, p->arg3, p->arg4, p->arg5, 
			     p->arg6, p->arg7, p->arg8, p->arg9, p->arg10);

    LOGDBG(("portLib:taskStarter: task 0x%p ('%s') "
	    "terminated normally\n", tcb, tcb->name));
}

/*----------------------------------------------------------------------*/

/*
 * Create a new task 
 */
long
taskSpawn(char *name, int priority, int options, int stackSize,
	  FUNCPTR entryPt, int arg1, int arg2, int arg3, int arg4, int arg5,
	  int arg6, int arg7, int arg8, int arg9, int arg10)
{
   int status;
   OS_TCB *tcb;
   char bufName[12];

   /*
    * Allocate a TCB 
    */
   tcb = (OS_TCB *)kmalloc(sizeof(OS_TCB), GFP_KERNEL);
   if (tcb == NULL) {
      errnoSet(S_portLib_NO_MEMORY);
      return ERROR;
   }

   if (name == NULL) {
      sprintf(bufName, "t%d", ++numTask);
      name = bufName;
   }
   tcb->name = kmalloc(strlen(name)+1, GFP_KERNEL);
   if (tcb->name == NULL) {
      kfree(tcb);
      errnoSet(S_portLib_NO_MEMORY);
      return ERROR;
   }

   if (tcb->name) strcpy(tcb->name, name);

   tcb->options = options;
   tcb->priority = priority;
   tcb->entry = entryPt;
   tcb->errorStatus = 0;
   tcb->userData = 0; /* XXX */
   tcb->magic = PORTLIB_TASK_MAGIC;
    
   tcb->params.arg1 = arg1;
   tcb->params.arg2 = arg2;
   tcb->params.arg3 = arg3;
   tcb->params.arg4 = arg4;
   tcb->params.arg5 = arg5;
   tcb->params.arg6 = arg6;
   tcb->params.arg7 = arg7;
   tcb->params.arg8 = arg8;
   tcb->params.arg9 = arg9;
   tcb->params.arg10 = arg10;

   /*
    * Create task
    */

   status = rt_task_init(&tcb->rtid, taskStarter, (int/*damn...*/)tcb,
			 stackSize, priorityVxToRTAI(priority),
			 options & VX_FP_TASK, NULL);
   if (status != 0) {
      kfree(tcb->name);
      kfree(tcb);
      errnoSet(status);
      return ERROR;
   }

   if (set_exit_handler(&tcb->rtid, taskCleanUp, tcb, 0) == 0) {
      kfree(tcb->name);
      kfree(tcb);
      errnoSet(S_portLib_NO_MEMORY);
      return ERROR;
   }

   /*
    * add new task in task list
    */
   tcb->next = taskList;
   taskList = tcb;

   LOGDBG(("portLib:taskSpawn: spawning task 0x%p ('%s')\n",
	   tcb, tcb->name));

   /* wake up the task */
   rt_task_resume(&tcb->rtid);

   return (long)tcb;
}

/*----------------------------------------------------------------------*/

/*
 * Delete a task 
 */
STATUS
taskDelete(long tid)
{
   OS_TCB *tcb = taskTcb(tid);
   OS_TCB *t;
   int status;

   if (!tcb) {
      LOGDBG(("portLib:taskDelete: no such task 0x%lx\n", tid));
      return ERROR;
   }

   LOGDBG(("portLib:taskDelete: deleting 0x%lx ('%s')\n",
	   tid, tcb->name));

   /* delete hooks are run */
   status = rt_task_delete(&tcb->rtid);
   if (status != 0) {
      LOGDBG(("portLib:taskDelete: no such task 0x%lx\n", tid));
   }

#if 0
   /* Remove from task List.
    * Locking required?? Also, this is O(n). Wouldn't it be worth using
    * doubly chained lists? */
   if (tcb == taskList) {
      taskList = taskList->next;
   } else {
      for (t = taskList; t->next != NULL; t = t->next) {
	 if (t->next == tcb) {
	    t->next = tcb->next;
	    break;
	 }
      }/* for */
   }
   if (tcb->name) kfree(tcb->name);
   kfree(tcb);
#endif
    
   LOGDBG(("portLib:taskDelete: deleted 0x%lx\n", tid));
   return OK;
}

/*----------------------------------------------------------------------*/

/* 
 * Return the name of a task
 */   
const char * 
taskName(long tid)
{
   OS_TCB *tcb = taskTcb(tid);

   if (!tcb) return NULL;
   return tcb->name;
}

/*----------------------------------------------------------------------*/

/* 
 * Suspend a task
 */   
STATUS 
taskSuspend(long tid)
{
   OS_TCB *tcb = taskTcb(tid);

   LOGDBG(("portLib:taskSuspend: suspending 0x%lx ('%s')\n",
	   tid, tcb->name));

   if (rt_task_suspend(&tcb->rtid) != 0)
      return ERROR;

   return OK;
}

/*----------------------------------------------------------------------*/
 
/*
 * Resume a suspended task 
 */
STATUS 
taskResume(long tid)
{
   OS_TCB *tcb = taskTcb(tid);

   if (rt_task_resume(&tcb->rtid) != 0)
      return ERROR;

   LOGDBG(("portLib:taskResume: resuming 0x%lx ('%s')\n",
	   tid, tcb->name));
   return OK;
}

/*----------------------------------------------------------------------*/

/*
 * define a new priority for a task
 */
STATUS 
taskPrioritySet(long tid, int newPriority)
{
   OS_TCB *tcb = taskTcb(tid);

   tcb->priority = priorityVxToRTAI(newPriority);
   rt_change_prio(&tcb->rtid, tcb->priority);
   return OK;
}

/*----------------------------------------------------------------------*/

/*
 * Get the current priority of a task
 */
STATUS
taskPriorityGet(long tid, int *pPriority)
{
   OS_TCB *tcb = taskTcb(tid);

   *pPriority = priorityRTAIToVx(tcb->priority);;
   return OK;
}

/*----------------------------------------------------------------------*/

/*
 * Stop the scheduler
 */
STATUS
taskLock(void)
{
   rt_sched_lock();
   return OK;
}

/*----------------------------------------------------------------------*/

/*
 * Restart the scheduler 
 */
STATUS
taskUnlock(void)
{
   rt_sched_unlock();
   return OK;
}

/*----------------------------------------------------------------------*/

/*
 * Sleep for a number of ticks, or just yield cpu if ticks==0.
 */
STATUS
taskDelay(int ticks)
{
   OS_TCB *tcb = taskTcb(0);

   if (!tcb) {
      LOGDBG(("portLib:taskDelay: called outside task context\n"));
      errnoSet(S_portLib_NOT_IN_A_TASK);
      return ERROR;
   }
   LOGDBG(("portLib:taskDelay: delaying 0x%p ('%s') for %d ticks\n",
	   tcb, tcb->name, ticks));

   switch(ticks) {
      case 0:
	 rt_task_yield();
	 break;

      default:
	 rt_sleep(ticks * sysClkTickDuration());
	 break;
   }

   LOGDBG(("portLib:taskDelay: delayed 0x%p ('%s')\n",
	   tcb, tcb->name));
   return OK;
}

/*----------------------------------------------------------------------*/
    
/*
 * Return id of current task
 */
long
taskIdSelf(void)
{
   RT_TASK *rtid;
   OS_TCB *t;

   rtid = rt_whoami();
   if (!rtid) return 0x0;

   t = (OS_TCB *)rtid;
   if (t->magic != PORTLIB_TASK_MAGIC) return 0x0;

   return (long)t;
}

/*----------------------------------------------------------------------*/

/*
 * Return the TCB of a task
 */
OS_TCB *
taskTcb(long tid)
{
    return (OS_TCB *)(tid == 0 ? taskIdSelf() : tid);
}

/*----------------------------------------------------------------------*/

/*
 * Set some user data associated with the task
 */
STATUS
taskSetUserData(long tid, unsigned long data)
{
   OS_TCB *tcb = taskTcb(tid);

   tcb->userData = data;
   return OK;
}

/*----------------------------------------------------------------------*/

/*
 * Return the user data associated with the task
 */
unsigned long
taskGetUserData(long tid)
{
   OS_TCB *tcb = taskTcb(tid);

   if (tcb == NULL) {
      printk("taskGetUserData: tcb == NULL for tid %lx.\n", tid);
      return 0;
   }

   return tcb->userData;
}

/*----------------------------------------------------------------------*/

/*
 * Change task options
 */
STATUS
taskOptionsSet(long tid, int mask, int newOptions)
{
   OS_TCB *tcb = taskTcb(tid);

   if (!tcb) return ERROR;

   tcb->options = (tcb->options & ~mask) | newOptions;
   return OK;
}

/*----------------------------------------------------------------------*/

/*
 * Examine task options
 */
STATUS
taskOptionsGet(long tid, int *pOptions)
{
   OS_TCB *tcb = taskTcb(tid);

   if (!tcb) return ERROR;

   if (pOptions) *pOptions = tcb->options;
   return OK;
}

/*----------------------------------------------------------------------*/

/*
 * Return the Task Id associated with a name 
 */
long 
taskNameToId(char *name)
{
    OS_TCB *t;
    
    for (t = taskList; t != NULL; t = t->next) {
	if (strcmp(t->name, name) == 0) {
	    break;
	}
    } /* for */
    if (t == NULL) {
	return ERROR;
    } else {
	return (long)t;
    }
}

/*----------------------------------------------------------------------*/

#ifdef not_yet
/*
 * Interactive list of tasks
 */
static char *policyName[] = {
    "OTHER",
    "FIFO",
    "RR",
    "SYS",
    "IA",
};

STATUS
i(void)
{
    OS_TCB *t; 

    printf("  NAME                     TID        SCHED PRI   ERRNO\n"
	   "-------------------- ---------------- ----- --- --------\n");
    for (t = taskList; t != NULL; t = t->next) {
	printf("%-20s %16lx %-5s %3d %8x\n", t->name, (long)t, 
	       policyName[t->policy], t->priority, t->errorStatus);
    }
    return OK;
}
#endif

/*----------------------------------------------------------------------*/

/*
 * Initialize task hook facilities
 */
void
taskHookInit(void)
{
}


/*----------------------------------------------------------------------*/

/*
 * Add a hook to the head of a list 
 */
static STATUS
addHookHead(TASK_HOOK_LIST **list, FUNCPTR hook)
{
    TASK_HOOK_LIST *l;

    if (list == NULL) {
	return ERROR;
    }
    l = (TASK_HOOK_LIST *)kmalloc(sizeof(TASK_HOOK_LIST), GFP_KERNEL);
    if (l == NULL) {
	return ERROR;
    }
    /* Add to the head of the list */
    l->hook = hook;
    l->next = *list;
    *list = l;
    return OK;
}

/*----------------------------------------------------------------------*/

/*
 * Add a hook to the tail of a list 
 */
static STATUS
addHookTail(TASK_HOOK_LIST **list, FUNCPTR hook)
{
    TASK_HOOK_LIST *l, *p;

    if (list == NULL) {
	return ERROR;
    }
    l = (TASK_HOOK_LIST *)kmalloc(sizeof(TASK_HOOK_LIST), GFP_KERNEL);
    if (l == NULL) {
	return ERROR;
    }
    l->hook = hook;
    l->next = NULL;
    /* Test if empty list */
    if (*list == NULL) {
	*list = l;
	return OK;
    }
    /* walk through the list to the last element */
    for (p = *list; p->next != NULL; p = p->next) 
	;
    /* Add it to the tail */
    p->next = l;
    return OK;
}
	
/*----------------------------------------------------------------------*/

/*
 * Remove a hook from a list 
 */
static STATUS
deleteHook(TASK_HOOK_LIST **list, FUNCPTR hook)
{
    TASK_HOOK_LIST *p, *l;

    if (list == NULL || *list == NULL) {
	return ERROR;
    }
    p = NULL;
    for (l = *list; l != NULL; l = l->next) {
	if (l->hook == hook) {
	    break;
	}
	p = l;
    }
    if (l == NULL) {
	/* pas trouve */
	return ERROR;
    }
    if (p == NULL) {
	/* element de tete */
	*list = l->next;
	kfree(l);
	return OK;
    }
    /* Element au milieu */
    p->next = l->next;
    kfree(l);
    return OK;
}

/*----------------------------------------------------------------------*/

/*
 * Execute a list of hooks 
 */
static STATUS
executeHooks(TASK_HOOK_LIST *list, OS_TCB *tcb)
{
    TASK_HOOK_LIST *l;

    for (l = list; l != NULL; l = l->next) {
	l->hook(tcb);
    } /* for */
    return OK;
}
/*----------------------------------------------------------------------*/

/* 
 * Add a routine to be called at every task create
 */
STATUS
taskCreateHookAdd(FUNCPTR createHook)
{
    return addHookTail(&createHooks, createHook);
}

/*----------------------------------------------------------------------*/

/*
 * Delete a previously added task create routine
 */
STATUS 
taskCreateHookDelete(FUNCPTR createHook)
{
    return deleteHook(&createHooks, createHook);
}

/*----------------------------------------------------------------------*/

/* 
 * Add a routine to be called at every task switch
 */
STATUS
taskSwitchHookAdd(FUNCPTR switchHook)
{
    errnoSet(EOPNOTSUPP);
    return  ERROR;
}

/*----------------------------------------------------------------------*/

/*
 * Delete a previously added task switch routine
 */
STATUS 
taskSwitchHookDelete(FUNCPTR switchHook)
{
    errnoSet(EOPNOTSUPP);
    return ERROR;
}

/*----------------------------------------------------------------------*/
/* 
 * Add a routine to be called at every task delete
 */
STATUS
taskDeleteHookAdd(FUNCPTR deleteHook)
{
    addHookHead(&deleteHooks, deleteHook);
    return OK;
}

/*----------------------------------------------------------------------*/

/*
 * Delete a previously added task delete routine
 */
STATUS 
taskDeleteHookDelete(FUNCPTR deleteHook)
{
    deleteHook(&deleteHooks, deleteHook);
    return OK;
}

/*----------------------------------------------------------------------*/

/***
 *** Émulation de errnoLib de VxWorks
 ***/

/* global errno variable for errors that occur outside the context of a
 * portLib task */
static int globalErrno = OK;

int
errnoGet(void)
{
   OS_TCB *tcb = taskTcb(taskIdSelf());
    
   if (tcb != NULL) return tcb->errorStatus;

   return globalErrno;
}

/*----------------------------------------------------------------------*/

STATUS
errnoSet(int errorValue)
{
   OS_TCB *tcb = taskTcb(0);

   LOGDBG(("portLib: errno set to %d\n", errorValue));

   if (tcb != NULL) {
      tcb->errorStatus = errorValue;
   } else {
      globalErrno = errorValue;
   }

   return OK;
}
