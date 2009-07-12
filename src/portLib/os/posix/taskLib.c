/*
 * Copyright (c) 1999, 2003-2004,2009 CNRS/LAAS
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

#include <sys/types.h>
#ifdef __linux__
#include <execinfo.h>
#endif
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#ifdef OSAPI_ART
# include <linux/art_task.h>
#endif

#include "portLib.h"
static const H2_ERROR portLibH2errMsgs[]  = PORT_LIB_H2_ERR_MSGS;


#include "errnoLib.h"
#include "sysLib.h"
#include "wdLib.h"
#define TASKLIB_C
#include "taskLib.h"
#include "taskHookLib.h"

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
    pthread_mutex_t *starter;		/* guard tcb during task startup */
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
 * Record errors messages
 */
int
portRecordH2ErrMsgs(void)
{
    return h2recordErrMsgs("portRecordH2ErrMsg", "portLib", M_portLib, 
			   sizeof(portLibH2errMsgs)/sizeof(H2_ERROR), 
			   portLibH2errMsgs);
}
/*----------------------------------------------------------------------*/

/*
 * Two functions to map a VxWorks-like  priority to a Posix priority
 */
#if defined(HAVE_PTHREAD_ATTR_SETSCHEDPOLICY) || defined(OSAPI_ART)
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
#endif

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
    
    /* Determine the min and max allowable priorities */
#ifdef OSAPI_ART
    rr_min_priority = ART_PRIO_MIN;
    rr_max_priority = ART_PRIO_MAX;
#else

# ifdef PTHREAD_MIN_PRIORITY
    rr_min_priority = PTHREAD_MIN_PRIORITY;
# else
    rr_min_priority = sched_get_priority_min (SCHED_RR);
# endif
# ifdef PTHREAD_MAX_PRIORITY
    rr_max_priority = PTHREAD_MAX_PRIORITY;
# else
    rr_max_priority = sched_get_priority_max (SCHED_RR);
# endif

#endif

    /* Create a thread specific key to access TCB */
    pthread_key_create(&taskControlBlock, NULL);
    
    /* allocate a TCB for main thread */
    tcb = (OS_TCB *)malloc(sizeof(OS_TCB));
    if (tcb == NULL) {
	return ERROR;
    }
    snprintf(name, sizeof(name), "tUnix%d", (int)getpid());
    tcb->name = strdup(name);
#if defined(HAVE_PTHREAD_ATTR_SETSCHEDPOLICY) && !defined(OSAPI_ART)
    pthread_getschedparam(pthread_self(), &policy, &param);
#else
    policy = 0;
    param.sched_priority = rr_min_priority;
#endif

    tcb->policy = policy;
    tcb->priority = priorityPosixToVx(param.sched_priority);
    tcb->options = 0;
    tcb->entry = NULL;			/* implicit main() */
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
	errnoSet(errno);
	return ERROR;
    }

    /* Initialize the global task List */
    taskList = tcb;

#ifdef OSAPI_ART
    {
      int s;

      /* use art_yield() to figure out if we are already a real-time task. If
       * not, switch to real-time scheduling, with sysClkRate period and lowest
       * priority. */
      s = art_yield();
      if (s) {
	s = art_enter(ART_PRIO_MIN, ART_TASK_PERIODIC, 1000000/sysClkRateGet());
	if (s) {
	  perror("art_enter");
	  return ERROR;
	}
      }
    }
#endif
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

#ifdef DEBUG
    printf("taskCleanUp \"%s\" %p\n", tcb->name, tcb);
#endif
    executeHooks(deleteHooks, tcb);

    /* Remove from task List */
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
    if (tcb->starter != NULL) {
	    pthread_mutex_unlock(tcb->starter);
	    pthread_mutex_destroy(tcb->starter);
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
    
    pthread_mutex_lock(tcb->starter); 	/* released in taskCleanUp() */
    tcb->pid = getpid();

    /* Register the TCB pointer in a thread-specific key */
    if (pthread_setspecific(taskControlBlock, (void *)tcb) != 0) {
	return NULL;
    }
    /* Register a cleanup function */
    pthread_cleanup_push(taskCleanUp, data);

#ifdef OSAPI_ART
    {
      int s;

      /* switch to real-time scheduling, with sysClkRate period */
      s = art_enter(priorityVxToPosix(tcb->priority),
		    ART_TASK_PERIODIC, 1000000/sysClkRateGet());
      if (s) {
	perror("art_enter");
	return ERROR;
      }
    }
#endif

    /* Execute create hooks */
    executeHooks(createHooks, tcb);

#ifdef DEBUG
    printf("taskStart \"%s\" %p\n", tcb->name, tcb);
#endif

    result = (*(tcb->entry))(p->arg1, p->arg2, p->arg3, p->arg4, p->arg5, 
				p->arg6, p->arg7, p->arg8, p->arg9, p->arg10);

    /* Execute cleanup functions */
    pthread_cleanup_pop(1);

#ifdef OSAPI_ART
    /* leave real-time scheduling */
    art_exit();
#endif

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
#ifdef HAVE_PTHREAD_ATTR_SETSCHEDPOLICY
    int policy;
#endif /* HAVE_PTHREAD_ATTR_SETSCHEDPOLICY */
    int status;
    OS_TCB *tcb;
    char bufName[12];

    /*
     * Allocate a TCB 
     */
    tcb = (OS_TCB *)malloc(sizeof(OS_TCB));
    if (tcb == NULL) {
	errnoSet(S_portLib_NO_MEMORY);
	return ERROR;
    }
    tcb->starter = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if (tcb->starter == NULL) {
	    errnoSet(S_portLib_NO_MEMORY);
	    free(tcb);
	    return ERROR;
    }
    /* Startup mutex - guaranties the the tcb isn't destroyed early */
    pthread_mutex_init(tcb->starter, NULL);
    pthread_mutex_lock(tcb->starter);
    tcb->magic = TASK_MAGIC;
    if (name != NULL) {
	tcb->name = strdup(name);
    } else {
	snprintf(bufName, sizeof(bufName), "t%d", ++numTask);
	tcb->name = strdup(bufName);
    }
#ifdef DEBUG
    printf("taskSpawn \"%s\" %p\n", tcb->name, tcb);
#endif
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
    status = pthread_attr_setdetachstate(&thread_attr, 
					 PTHREAD_CREATE_DETACHED);
    if (status != 0) {
	errnoSet(status);
	pthread_mutex_unlock(tcb->starter);
	return ERROR;
    }
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
	pthread_mutex_unlock(tcb->starter);
	return ERROR;
    }
#endif    
#endif
#if defined(HAVE_PTHREAD_ATTR_SETSCHEDPOLICY) && !defined(OSAPI_ART)
    if (rr_min_priority > 0 && rr_min_priority > 0) {
	    /* Set priority of new thread */
	    thread_param.sched_priority = priorityVxToPosix(priority);
#ifndef __linux__
	    /* XXX Voir message de commit de la version 1.17 */
	    status = pthread_attr_setschedpolicy(&thread_attr, SCHED_RR);
	    switch (status) {
	    case 0:
		    /* set policy is ok, set priority */
		    status = pthread_attr_setschedparam(&thread_attr, 
							&thread_param);
		    if (status != 0) {
			    errnoSet(status);
			    pthread_mutex_unlock(tcb->starter);
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
		    pthread_mutex_unlock(tcb->starter);
		    return ERROR;
	    } /* switch */
#endif /* __linux__ */
    }
#endif /* HAVE_PTHREAD_ATTR_SETSCHEDPOLICY && !OSAPI_ART */

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
	pthread_mutex_unlock(tcb->starter);
	return ERROR;
    }
#ifdef OSAPI_ART
    tcb->policy = 0;
    thread_param.sched_priority = priorityVxToPosix(priority);
#else

# ifdef HAVE_PTHREAD_ATTR_SETSCHEDPOLICY
    /* Read back the scheduler parameters of the created thread */
    pthread_getschedparam(thread_id, &policy, &thread_param);
    tcb->policy = policy;
# else
    tcb->policy = 0;
    thread_param.sched_priority = rr_min_priority;
# endif /* HAVE_PTHREAD_ATTR_SETSCHEDPOLICY */

#endif /* OSAPI_ART */
    tcb->priority = priorityPosixToVx(thread_param.sched_priority);

    /* Register thread id in TCB */
    tcb->tid = thread_id;

    /*
     * add new task in task list
     */
    tcb->next = taskList;
    taskList = tcb;
    pthread_mutex_unlock(tcb->starter);
    return (long)tcb;
}

/*----------------------------------------------------------------------*/

long
taskFromThread(char *name)
{
    struct sched_param thread_param;
    OS_TCB *tcb;
    char bufName[12];

    /*
     * Allocate a TCB 
     */
    tcb = (OS_TCB *)malloc(sizeof(OS_TCB));
    if (tcb == NULL) {
	errnoSet(S_portLib_NO_MEMORY);
	return ERROR;
    }
    tcb->magic = TASK_MAGIC;
    if (name != NULL) {
	tcb->name = strdup(name);
    } else {
	snprintf(bufName, sizeof(bufName), "t%d", ++numTask);
	tcb->name = strdup(bufName);
    }
#ifdef DEBUG
    printf("taskFromThread \"%s\" %p\n", tcb->name, tcb);
#endif
    tcb->entry = NULL;
    tcb->errorStatus = 0;
    tcb->pid = getpid();
    tcb->userData = 0; /* XXX */

    tcb->params.arg1 = 0;
    tcb->params.arg2 = 0;
    tcb->params.arg3 = 0;
    tcb->params.arg4 = 0;
    tcb->params.arg5 = 0;
    tcb->params.arg6 = 0;
    tcb->params.arg7 = 0;
    tcb->params.arg8 = 0;
    tcb->params.arg9 = 0;
    tcb->params.arg10 = 0;

#if defined(HAVE_PTHREAD_ATTR_SETSCHEDPOLICY) && !defined(OSAPI_ART)
    /* Read back the scheduler parameters of the current thread */
    pthread_getschedparam(pthread_self(), &policy, &thread_param);
    tcb->policy = policy;
#else
    tcb->policy = 0;
    thread_param.sched_priority = rr_min_priority;
#endif /* HAVE_PTHREAD_ATTR_SETSCHEDPOLICY && !OSAPI_ART */

    tcb->priority = priorityPosixToVx(thread_param.sched_priority);

    /* Register thread id in TCB */
    tcb->tid = pthread_self();

    /* Register the TCB pointer in a thread-specific key */
    if (pthread_setspecific(taskControlBlock, (void *)tcb) != 0) {
	errnoSet(errno);
	return ERROR;
    }
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
 */
STATUS
taskDelete(long tid)
{
    OS_TCB *tcb = (OS_TCB *)tid;
    int status;

    if (tid == 0) {
	static int status = 0;
	pthread_exit(&status);
    }

    if (tcb->magic != TASK_MAGIC) {
	errnoSet(S_portLib_INVALID_TASKID);
	return ERROR;
    }

    status = pthread_cancel(tcb->tid);
    if (status != 0) {
	errnoSet(status);
	return ERROR;
    }    
    /* Removing the task from the tasklist 
       is done in the thread cleanup handler */
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

   if (tcb == NULL)
       return NULL;
   return tcb->name;
}

/*----------------------------------------------------------------------*/

/* 
 * Suspend a task
 *
 */   

#define BT_SIZE 100

STATUS 
taskSuspend(long tid)
{
    OS_TCB *tcb;
#ifdef __linux__
    static void *buffer[BT_SIZE];
    int nptrs;
#endif

    if (tid == 0) {
	tid = taskIdSelf();
    }
#ifdef __linux__
    nptrs = backtrace(buffer, BT_SIZE);
#endif
    tcb = (OS_TCB *)tid;
    if (tcb->magic != TASK_MAGIC) {
	errnoSet(S_portLib_INVALID_TASKID);
	return ERROR;
    }
    fprintf(stderr, "*** suspending task %lx\n", tid);
#ifdef __linux__
    backtrace_symbols_fd(buffer, nptrs, STDERR_FILENO);
#endif
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
    errnoSet(S_portLib_NOT_IMPLEMENTED);
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
	 errnoSet(S_portLib_INVALID_TASKID);
	 return ERROR;
     }

#ifdef OSAPI_ART
     art_exit();
     art_enter(priorityVxToPosix(newPriority),
	       ART_TASK_PERIODIC, 1000000/sysClkRateGet());
#else

# ifdef HAVE_PTHREAD_ATTR_SETSCHEDPOLICY
    my_param.sched_priority = newPriority;
    status = pthread_setschedparam(tcb->tid, SCHED_RR, &my_param);
    if (status != 0) {
	errnoSet(status);
	return ERROR;
    }
# endif

#endif /*OSAPI_ART */
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
#if defined(HAVE_PTHREAD_ATTR_SETSCHEDPOLICY) && !defined(OSAPI_ART)
    struct sched_param my_param;
    int my_policy;
    int status;
    
    if (tcb->magic != TASK_MAGIC) {
	errnoSet(S_portLib_INVALID_TASKID);
	return ERROR;
    }
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
    errnoSet(S_portLib_NOT_IMPLEMENTED);
    return ERROR;
}

/*----------------------------------------------------------------------*/

/*
 * Restart the scheduler 
 */
STATUS
taskUnlock(void)
{
    errnoSet(S_portLib_NOT_IMPLEMENTED);
    return ERROR;
}

/*----------------------------------------------------------------------*/

/*
 * Sleep for a number of ticks, or just yield cpu if ticks==0.
 *
 * The posix implementation diverges from the VxWorks implementation that
 * wakes up exactly on the next clock tick. 
 * Using nanosleep() on system where it has high resolution, will 
 * wake up the task between to clock ticks.
 *
 * The ART-linux implementation wakes up at the specified clock tick, as under
 * VxWorks.
 */
STATUS
taskDelay(int ticks)
{
#ifdef OSAPI_ART
	unsigned long end = tickGet() + ticks;
	while (tickGet() < end)
	  art_wait();
	return OK;
#else
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
#endif
}

/*----------------------------------------------------------------------*/
    
/*
 * Return the id of current task
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
    if (tcb == NULL)
	    return 0;			/* XXX correct ? */
    if (tcb->magic != TASK_MAGIC) {
	fprintf(stderr, "taskIdSelf: bad task specific data: %lx %lx\n",
		(unsigned long)tcb, (unsigned long)pthread_self());
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
    OS_TCB *tcb = taskTcb(tid);

    if (tcb->magic != TASK_MAGIC) {
	errnoSet(S_portLib_INVALID_TASKID);
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
    OS_TCB *tcb = taskTcb(tid);
    if (tcb != NULL) {
	if (tcb->magic != TASK_MAGIC) {
	    errnoSet(S_portLib_INVALID_TASKID);
	    fprintf(stderr, 
		"taskGetUserData: bad/unregisterd taskId %ld %lx\n",
		tid, (unsigned long)pthread_self());
	    return 0;
	}
        return tcb->userData;
    } else {
        fprintf(stderr, 
		"taskLib: fatal error: taskGetUserData: tcb == NULL.\n");
	abort();
    }
}

/*----------------------------------------------------------------------*/

/*
 * Change task options
 */
STATUS
taskOptionsSet(long tid, int mask, int newOptions)
{
    OS_TCB *tcb = taskTcb(tid);

    if (tcb == NULL) {
	errnoSet(S_portLib_INVALID_TASKID);
	return ERROR;
    }
    if (tcb->magic != TASK_MAGIC) {
	errnoSet(S_portLib_INVALID_TASKID);
	return ERROR;
    }

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
    
   if (tcb == NULL) 
       return ERROR;
   if (tcb->magic != TASK_MAGIC) {
       errnoSet(S_portLib_INVALID_TASKID);
       return ERROR;
   }

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
	if (t->magic != TASK_MAGIC) {
	    errnoSet(S_portLib_INVALID_TASKID);
	    return ERROR;
	}
	if (strcmp(t->name, name) == 0) {
	    break;
	}
    } /* for */
    if (t == NULL) {
	errnoSet(S_portLib_NO_SUCH_TASK);
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
	/* not found */
	return ERROR;
    }
    if (p == NULL) {
	/* head element */
	*list = l->next;
	free(l);
	return OK;
    }
    /* Element in the middle */
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
    errnoSet(S_portLib_NOT_IMPLEMENTED);
    return ERROR;
}

/*----------------------------------------------------------------------*/

/*
 * Delete a previously added task switch routine
 */
STATUS 
taskSwitchHookDelete(FUNCPTR switchHook)
{
    errnoSet(S_portLib_NOT_IMPLEMENTED);
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
 *** Errno manipulation functions
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

    if (tcb != NULL && tcb->magic == TASK_MAGIC) {
	tcb->errorStatus = errorValue;
	return OK;
    } else {
	fprintf(stderr, "errnoSet: no TCB %ld\n", taskIdSelf());
	return ERROR;
    }
}

/*----------------------------------------------------------------------*/
