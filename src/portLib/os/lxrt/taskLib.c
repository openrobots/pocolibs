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
#include "pocolibs-config.h"
__RCSID("$LAAS$");

#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/mman.h>

#include <rtai_lxrt.h>

#include "portLib.h"
#include "errnoLib.h"
#include "sysLib.h"
#define TASKLIB_C
#include "taskLib.h"
#include "taskHookLib.h"

/* #define PORTLIB_DEBUG_TASKLIB */
#ifdef PORTLIB_DEBUG_TASKLIB
# define LOGDBG(x)	logMsg x
#else
# define LOGDBG(x)
#endif

extern int main(int, char **);

static pthread_key_t taskControlBlock;
static OS_TCB *taskList = NULL;
static int numTask = 0;
static int rr_min_priority;
static int rr_max_priority;

#ifdef __OpenBSD__
#define PTHREAD_MIN_PRIORITY -1
#define PTHREAD_MAX_PRIORITY -1
#endif

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
  RT_TASK *LXRT_task;
    char *name;				/* pointer to task name */
    int options;			/* task options bits */
    int policy;				/* scheduling policy */
    int priority;			/*  */
    FUNCPTR entry;			/* entry point */
    int errorStatus;			/* error code */
    pthread_t tid;			/* thread id */
    pid_t pid;				/* process id */
    unsigned long userData;		/* user data */
    TASK_PARAMS params;			/* parameters */
    struct OS_TCB *next;		/* next tcb in list */
    unsigned int magic;			/* magic */
};

#define TASK_MAGIC  0x5441534b

/*
 * Task hooks globals 
 */
typedef struct TASK_HOOK_LIST {
    FUNCPTR hook;
    struct TASK_HOOK_LIST *next;
} TASK_HOOK_LIST;

TASK_HOOK_LIST *createHooks = NULL;
TASK_HOOK_LIST *deleteHooks = NULL;

static STATUS executeHooks(TASK_HOOK_LIST *list, OS_TCB *tcb);
/*----------------------------------------------------------------------*/

/*
 * Two functions to map a VxWorks-like  priority to a Posix priority
 */
static int
priorityVxToPosix(int priority)
{
	if (rr_max_priority != -1) {
		return rr_min_priority +
                    (255 - priority)*(rr_max_priority- rr_min_priority)/255;
	} else {
		return -1;
	}
}

/*----------------------------------------------------------------------*/

static int 
priorityPosixToVx(int priority)
{
	if (rr_min_priority != rr_max_priority)
		return (rr_max_priority - priority)*255
			/(rr_max_priority - rr_min_priority);
	else 
		return 0;	/* arbitrary value */
}

/*----------------------------------------------------------------------*/

/*
 * TaskLib init function
 */
STATUS
taskLibInit(void)
{
    OS_TCB *tcb;
    struct sched_param param;
    int policy;
    char name[12];
    static struct sched_param mysched;
    
    /* Determine the min and max allowable priorities */
#ifdef PTHREAD_MIN_PRIORITY
    rr_min_priority = PTHREAD_MIN_PRIORITY;
#else
    rr_min_priority = sched_get_priority_min (SCHED_FIFO);
#endif
#ifdef PTHREAD_MAX_PRIORITY
    rr_max_priority = PTHREAD_MAX_PRIORITY;
#else
    rr_max_priority = sched_get_priority_max (SCHED_FIFO);
#endif

    /* Create a thread specific key to access TCB */
    pthread_key_create(&taskControlBlock, NULL);
    
    /* allocate a TCB for main thread */
    tcb = (OS_TCB *)malloc(sizeof(OS_TCB));
    if (tcb == NULL) {
	return ERROR;
    }
    snprintf(name, sizeof(name), "t%d", getpid());
    tcb->name = strdup(name);
#ifdef HAVE_PTHREAD_ATTR_SETSCHEDPOLICY
    pthread_getschedparam(pthread_self(), &policy, &param);
#else
    policy = 0;
#endif
    tcb->policy = policy;
    tcb->priority = priorityPosixToVx(param.sched_priority);
    tcb->options = 0;
    tcb->entry = main;
    tcb->errorStatus = errno;
    /* Process and thread ids */
    tcb->pid = getpid();
    tcb->tid = pthread_self();
    /* The tcb list is initially empty */
    tcb->next = NULL;
    /* Mark the TCB */
    tcb->magic = TASK_MAGIC;

    /* Register the TCB pointer in the thread-specific key */
#ifdef DEBUG
    fprintf(stderr, "registering task specific value %lu %lu\n",
	(unsigned long)tcb, (unsigned long)pthread_self());
#endif
    if (pthread_setspecific(taskControlBlock, (void *)tcb) != 0) {
	    fprintf(stderr, "pthread_setspecific failed\n");
	return ERROR;
    }

    /* LXRT stuff */
    rt_allow_nonroot_hrt();

    mysched.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
    if (sched_setscheduler( 0, SCHED_FIFO, &mysched ) == -1 )
      {
	printf("ERROR IN SETTING THE POSIX SCHEDULER\n");
	return ERROR;
      }  

    tcb->LXRT_task = rt_task_init_schmod(nam2num(tcb->name),
					 RT_SCHED_LOWEST_PRIORITY,
					 0, 0, SCHED_FIFO, 1);
    if(tcb->LXRT_task == NULL)
      {
	fprintf(stderr, "rt_task_init_schmod, taskLibInit failed, name=%s\n", tcb->name);
	return ERROR;
      }

    rt_set_oneshot_mode();
    start_rt_timer(0);		/* in one shot mode timer 
				   don't use periode */


    /* Initialize the global task List */
    taskList = tcb;

    LOGDBG(("taskLib:taskLibInit: initialized\n"));

    return OK;
}

/*----------------------------------------------------------------------*/

/*
 * Thread clean-up function
 */
void 
taskCleanUp(void *data)
{
    OS_TCB *tcb = (OS_TCB *)data;
    OS_TCB *t;

    executeHooks(deleteHooks, tcb);

    /* Remove from task List */
    if (tcb == taskList) 
      {
	taskList = taskList->next;
      } 
    else 
      {
	for (t = taskList; t->next != NULL; t = t->next) 
	  {
	    if (t->next == tcb) 
	      {
		t->next = tcb->next;
		break;
	      }
	  }/* for */
      }
    free(tcb);

}

/*----------------------------------------------------------------------*/

/* 
 * A Helper function to convert form VxWorks task function prototype to 
 *  pthreads function prototype 
 */
void *
taskStarter(void *data)
{
    OS_TCB *tcb = (OS_TCB *)data;
    TASK_PARAMS *p = (TASK_PARAMS *)&(tcb->params);
    static int result;
    
    tcb->pid = getpid();

    /* Register the TCB pointer in a thread-specific key */
    if (pthread_setspecific(taskControlBlock, (void *)tcb) != 0) {
	return NULL;
    }
    /* Register a cleanup function */
    pthread_cleanup_push(taskCleanUp, data);

    /* Execute create hooks */
    executeHooks(createHooks, tcb);

    /* LXRT sutff */
    tcb->LXRT_task = rt_task_init_schmod(nam2num(tcb->name),
					 RT_SCHED_LOWEST_PRIORITY,
					 0, 0, SCHED_FIFO, 1);

    if(tcb->LXRT_task == NULL)
      {
	fprintf(stderr, "rt_task_init_schmod, taskStarter failed\n");
	return NULL;
      }

    rt_set_usp_flags_mask(FORCE_SOFT);

    mlockall(MCL_CURRENT | MCL_FUTURE);
    rt_make_hard_real_time();

    rt_set_oneshot_mode();
    start_rt_timer(0);		/* in one shot mode timer 
				   don't use periode */

    LOGDBG(("taskStarter, tcb->LXRT_task = %#x \n", tcb->LXRT_task));
    LOGDBG(("taskStarter, name=%s\n", tcb->name));

    /* everything in this function must be RT */
    result = (*(tcb->entry))(p->arg1, p->arg2, p->arg3, p->arg4, p->arg5, 
				p->arg6, p->arg7, p->arg8, p->arg9, p->arg10);

    LOGDBG(("portLib:taskLib:taskStarter: result of %s = %d\n",
	    tcb->name, result));

    /* LXRT stuff */

    /* 
       If I am in the LXRT world, it's mean that I have finished my
       job so I must go back in the linux world to die properly.

       If I am in the Linux world it's because someone call
       taskDelete() on me. So I must be compliant with taskDelete()
       and let it kill me.

       exemple: 
       - when sysClkThread() finish it is Linux world.
       - when a task end itself it is in LXRT world.

       If a task end itself it must be in LXRT world.
    */

    if (rt_is_hard_real_time(tcb->LXRT_task))
      {
	/* LXRT world */
	LOGDBG(("portLib:taskLib:taskStarter: no more RT task \"%s\" (ended in RT)\n", tcb->name, result));
      }
    else
      {
	/* Linux world */
	LOGDBG(("portLib:taskLib:taskStarter: no more RT task \"%s\" (ended in Linux)\n", tcb->name, result));
      }

    rt_task_delete(tcb->LXRT_task);

    /* Execute cleanup functions */
    pthread_cleanup_pop(1);

    /* Terminate thread */
    pthread_exit(&result);
    return NULL;
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
    pthread_attr_t thread_attr;
    pthread_t thread_id;
    struct sched_param thread_param;
#ifndef NO_SCHEDPARAM
    int policy;
#endif /* NO_SCHEDPARAM */
    int status;
    OS_TCB *tcb;
    char bufName[12];

    /*
     * Allocate a TCB 
     */
    tcb = (OS_TCB *)malloc(sizeof(OS_TCB));
    if (tcb == NULL) {
	return ERROR;
    }
    tcb->magic = TASK_MAGIC;
    if (name != NULL) {
	tcb->name = strdup(name);
    } else {
	sprintf(bufName, "t%d", ++numTask);
	tcb->name = strdup(bufName);
    }
    tcb->options = options;
    tcb->entry = entryPt;
    tcb->errorStatus = 0;
    tcb->pid = getpid();
    tcb->userData = 0; /* XXX */
    
    /* 
     * Initialize thread attributes 
     */
    status = pthread_attr_init (&thread_attr);
    if (status != 0) {
	errnoSet(status);
	return ERROR;
    }

#if 0
    /* 
       the pthreads are not in detached state so pthread_join() and
       then taskJoin() works
    */
    status = pthread_attr_setdetachstate(&thread_attr, 
					 PTHREAD_CREATE_DETACHED);
    if (status != 0) {
	errnoSet(status);
	return ERROR;
    }
#endif

#ifdef notyet
#ifdef _POSIX_THREAD_ATTR_STACKSIZE
    /*
     * If supported, determine the default stack size and report
     * it, and then select a stack size for the new thread.
     *
     * Note that the standard does not specify the default stack
     * size, and the default value in an attributes object need
     * not be the size that will actually be used.  Solaris 2.5
     * uses a value of 0 to indicate the default.
     */
    status = pthread_attr_setstacksize(&thread_attr, stackSize);
    if (status != 0) {
	errnoSet(status);
	return ERROR;
    }
#endif    
#endif
#ifdef HAVE_PTHREAD_ATTR_SETSCHEDPOLICY
    if (rr_min_priority > 0 && rr_min_priority > 0) {
	    /* Set priority of new thread */
	    thread_param.sched_priority = priorityVxToPosix(priority);
#ifndef __linux__
	    /* XXX Voir message de commit de la version 1.17 */
	    status = pthread_attr_setschedpolicy(&thread_attr, SCHED_FIFO);
	    switch (status) {
	    case 0:
		    /* set policy is ok, set priority */
		    status = pthread_attr_setschedparam(&thread_attr, 
							&thread_param);
		    if (status != 0) {
			    errnoSet(status);
			    return ERROR;
		    }
		    break;
		    
#ifdef ENOTSUP
# if ENOTSUP != EOPNOTSUPP
	    case ENOTSUP:
# endif
#endif /* ENOTSUP */
	    case EOPNOTSUPP:
		    /* fprintf(stderr, 
		       "warning: cannot set RR scheduling policy\n"); */
		    break;
		    
	    default :
		    errnoSet(status);
		    return ERROR;
	    } /* switch */
#endif /* __linux__ */
    }
#endif /* HAVE_PTHREAD_ATTR_SETSCHEDPOLICY */

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

    status = pthread_create(&thread_id, &thread_attr, taskStarter, tcb);
    if (status != 0) {
	errnoSet(status);
	return ERROR;
    }

    sleep(5);

#ifdef HAVE_PTHREAD_ATTR_SETSCHEDPOLICY
    /* Read back the scheduler parameters of the created thread */
    pthread_getschedparam(thread_id, &policy, &thread_param);
    tcb->policy = policy;
#else
    tcb->policy = 0;
#endif /* HAVE_PTHREAD_ATTR_SETSCHEDPOLICY */
    tcb->priority = priorityPosixToVx(thread_param.sched_priority);

    /* Register thread id in TCB */
    tcb->tid = thread_id;


    /* LXRT stuff */
    /* we must wait for the task to be started... */
    /* nothing... the LXRT_task is created in taskStarter */

    /*
     * add new task in task list
     */
    tcb->next = taskList;
    taskList = tcb;

    return (long)tcb;
}

/*----------------------------------------------------------------------*/

/*
 * Delete a task 
 * Actually a task kill by another isn't reallu killed... the task
 * killed must be kill-aware with rt_is_hard_real_time().
 */
STATUS
taskDelete(long tid)
{
    OS_TCB *tcb = (OS_TCB *)tid;
    int status;
    static int result = 0;

    LOGDBG(("portLib:taskLib:taskDelete: begin\n"));

    /* kill myself */
    if (tid == 0) 
      {
	tid = taskIdSelf();
	tcb = (OS_TCB *)tid;

	LOGDBG(("portLib:taskLib:taskDelete: tid == 0, task \"%s\"\n",
		tcb->name));

	/* from LXRT world or Linux world ? */
	if (rt_is_hard_real_time(tcb->LXRT_task))
	  {
	    /* LXRT world */
	    /* rt_make_soft_real_time(); */
	    LOGDBG(("portLib:taskLib:taskDelete: RT task \"%s\" ended in RT\n", tcb->name));
	  }
	else
	  {
	    /* Linux world */
	    /* it should not happened... 
	       because a task started by taskSpawn() is in RT and
	       should not use lxrt function
	       rt_make_soft_real_time()... */
	    LOGDBG(("portLib:taskLib:taskDelete: RT task \"%s\" ended in Linux\n", tcb->name));
	  }

	rt_task_delete(tcb->LXRT_task);
	/* check for error.. */

	LOGDBG(("portLib:taskLib:taskDelete: posix task \"%s\" ended\n",
		tcb->name, result));
	
	/* Execute cleanup functions */
	/* pthread_cleanup_pop(1); */
	
	/* Terminate thread */
	pthread_exit(&result);
      }	/* tid == 0 */

    LOGDBG(("portLib:taskLib:taskDelete: ---beginning, task \"%s\"\n",
	    tcb->name));

    if (tcb->magic != TASK_MAGIC)
      {
	fprintf(stderr, "taskDelete invalid taskId\n");
	return ERROR;
      }

    logMsg("trying to delete a task... no implemented...\n");
    return ERROR;

    LOGDBG(("portLib:taskLib:taskDelete: waiting for LXRT task \"%s\" to die!\n", tcb->name));

    /* come back in linux world "tcb->name"! */
    LOGDBG(("portLib:taskLib:taskDelete: send FORCE_SOFT to task \"%s\"\n",
	    tcb->name));
    rt_set_usp_flags(rt_get_adr(nam2num(tcb->name)), FORCE_SOFT);
    LOGDBG(("portLib:taskLib:taskDelete: FORCE_SOFT task \"%s\" done!\n",
	    tcb->name));

    /* wait for the RT part of "tcb->name" to die */
    /* actually I just wait a few... */
    rt_sleep(nano2count(90000000));
    rt_sleep(nano2count(90000000));
    rt_sleep(nano2count(90000000));
    rt_sleep(nano2count(90000000));

    LOGDBG(("portLib:taskLib:taskDelete: waiting for pthread \"%s\" to die!\n",
	    tcb->name));
    status = pthread_cancel(tcb->tid);
    if ( status != 0) 
      {
	errnoSet(status);
	return ERROR;
      }
    LOGDBG(("portLib:taskLib:taskDelete: pthread \"%s\" dead!\n",
	    tcb->name));

    LOGDBG(("portLib:taskLib:taskDelete: ---end\n"));
    return OK;
}

/*----------------------------------------------------------------------*/

/* 
 * suspends the execution of the calling thread
 * until the thread identified by tid terminates.
 */   
STATUS
taskJoin(long tid, void **val_return)
{
  int status;

  OS_TCB *tcb = (OS_TCB *)tid;

  if (tcb->magic != TASK_MAGIC)
    {
      fprintf(stderr, "taskJoin invalid taskId\n");
      return ERROR;
    }

  LOGDBG(("taskLib:taskJoin\n"));

  status = pthread_join(tcb->tid, val_return);

  if (status != 0)
    {
      errnoSet(status);
      return ERROR;
    }

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
 *
 * XXX should be implemented 
 */   
STATUS 
taskSuspend(long tid)
{
    OS_TCB *tcb;

    if (tid == 0) {
	tid = taskIdSelf();
    }
    tcb = (OS_TCB *)tid;
    if (tcb->magic != TASK_MAGIC) {
	fprintf(stderr, "taskSuspend: bad taskId\n");
	return ERROR;
    }
    printf("*** suspending task %lx\n", tid);
    abort();
    /*NOTREACHED*/
    return ERROR;
}

/*----------------------------------------------------------------------*/
 
/*
 * Resume a suspended task 
 */
STATUS 
taskResume(long tid)
{
    return ERROR;
}

/*----------------------------------------------------------------------*/

/*
 * define a new priority for a task
 */
STATUS 
taskPrioritySet(long tid, int newPriority)
{
	OS_TCB *tcb;
#ifdef HAVE_PTHREAD_ATTR_SETSCHEDPOLICY
    struct sched_param my_param;
    int status;
#endif

    if (tid == 0) 
 	    tid = taskIdSelf();
     
     tcb = (OS_TCB *)tid;
     if (tcb->magic != TASK_MAGIC) {
 	    fprintf(stderr, "taskPrioritySet: bad taskId\n");
 	    return ERROR;
     }
     
#ifdef HAVE_PTHREAD_ATTR_SETSCHEDPOLICY
    my_param.sched_priority = newPriority;
    status = pthread_setschedparam(tcb->tid, SCHED_FIFO, &my_param);
    if (status != 0) {
	errnoSet(status);
	return ERROR;
    }
#endif
    tcb->priority = newPriority;
    return OK;
}

/*----------------------------------------------------------------------*/

/*
 * Get the current priority of a task
 */
STATUS
taskPriorityGet(long tid, int *pPriority)
{
    OS_TCB *tcb = (OS_TCB *)tid;
#ifdef HAVE_PTHREAD_ATTR_SETSCHEDPOLICY
    struct sched_param my_param;
    int my_policy;
    int status;

    status = pthread_getschedparam(tcb->tid, &my_policy, &my_param);
    if (status != 0) {
	errnoSet(status);
	return ERROR;
    }
    if (pPriority != NULL) {
	*pPriority = my_param.sched_priority;
    } 
#else
    *pPriority = tcb->priority;
#endif
    return OK;
}

/*----------------------------------------------------------------------*/

/*
 * Stop the scheduler
 */
STATUS
taskLock(void)
{
    return ERROR;
}

/*----------------------------------------------------------------------*/

/*
 * Restart the scheduler 
 */
STATUS
taskUnlock(void)
{
    return ERROR;
}

/*----------------------------------------------------------------------*/

/*
 * Sleep for a number of ticks, or just yield cpu if ticks==0.
 */
STATUS
taskDelay(int ticks)
{
	struct timespec ts, tr;
	int clkRate = sysClkRateGet();

	if (ticks == 0) {
		return sched_yield() == 0 ? OK : ERROR;
	}
	ts.tv_sec = ticks / clkRate;
	ts.tv_nsec = (ticks % clkRate) * (1000000000/clkRate);
	for (;;) {
		if (nanosleep(&ts, &tr) == 0) 
			return OK;
		if (errno == EINTR) 
			memcpy(&ts, &tr, sizeof ts);
		else 
			return ERROR;
	}
	/*NOTREACHED*/
	return ERROR;
}

/*----------------------------------------------------------------------*/
    
/*
 * Return id of current task
 */
long
taskIdSelf(void)
{
    OS_TCB *tcb;

    if (taskList == NULL) {
	/* TaskLib was not initialized - do it now */
	if (taskLibInit() == ERROR) {
	    return 0;
	}
    }
    tcb = (OS_TCB *)pthread_getspecific(taskControlBlock);
    if (tcb->magic != TASK_MAGIC) {
	fprintf(stderr, "taskIdSelf: bad task specific data: %ld %ld\n",
		(long)tcb, pthread_self());
	abort();
    }
    return (long)tcb;
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
    OS_TCB *tcb = (OS_TCB *)(tid == 0 ? taskIdSelf() : tid);

    if (tcb->magic != TASK_MAGIC) {
	fprintf(stderr, "taskSetUserData: bad taskId\n");
	return ERROR;
    }
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
    OS_TCB *tcb = (OS_TCB *)(tid == 0 ? taskIdSelf() : tid);
    if (tcb != NULL) {
	if (tcb->magic != TASK_MAGIC) {
	    fprintf(stderr, 
		"taskGetUserData: bad/unregisterd taskId %ld %ld\n",
		tid, pthread_self());
	    return 0;
	}
        return tcb->userData;
    } else {
        fprintf(stderr, "taskGetUserData: tcb == NULL.\n");
	abort();
    }
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
	if (t->magic != TASK_MAGIC) {
	    fprintf(stderr, "taskNameToId: bad taskId\n");
	    return ERROR;
	}
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
    l = (TASK_HOOK_LIST *)malloc(sizeof(TASK_HOOK_LIST));
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
    l = (TASK_HOOK_LIST *)malloc(sizeof(TASK_HOOK_LIST));
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
	free(l);
	return OK;
    }
    /* Element au milieu */
    p->next = l->next;
    free(l);
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
    return ERROR;
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

/*----------------------------------------------------------------------*/


int
errnoGet(void)
{
    OS_TCB *tcb = taskTcb(taskIdSelf());
    
    if (tcb != NULL) {
	return tcb->errorStatus;
    }
    return 0;
}


/*----------------------------------------------------------------------*/

STATUS
errnoSet(int errorValue)
{
    OS_TCB *tcb = taskTcb(taskIdSelf());

    if (tcb != NULL) {
	tcb->errorStatus = errorValue;
	return OK;
    } else {
	fprintf(stderr, "errnoSet: no TCB %ld\n", taskIdSelf());
	return ERROR;
    }
}

/*----------------------------------------------------------------------*/
