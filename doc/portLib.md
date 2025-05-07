portLib
=======

PortLib (POsix Real-Time LIBrary) is a library that helps porting
applications written for the VxWorks real-time system to a
Unix-compatible system providing the Posix.4 real-time extensions.


Types
-----

_portLib_ defines the following types:

#### STATUS

    OK
    ERROR

#### Function pointers

	typedef void (*TASKHOOKFUNC)(OS_TCB *);
	typedef void (*WDROUTINEFUNC)(long);
	
Macros
------

    WAIT_FOREVER
	NO_WAIT

	MIN
	MAX

Error Codes
-----------

	S_portLib_NO_MEMORY
	S_portLib_NO_SUCH_TASK
	S_portLib_NOT_IN_A_TASK
	S_portLib_INVALID_TASKID
	S_portLib_NOT_IMPLEMENTED

Initialisation
--------------

Any Unix process that wants to use portLib functions should call the
[osInit](/osInit) function first. Not that this function is also
called by the [comLib](comLib) Initialisation function
([h2initGlob](h2initGlob)). Hence a task using
`h2initGlob()` to initialize comLib and portLib doesn't need to
explicitely call `osInit()`.

### osInit

    #include <portLib.h>
    STATUS osInit(int clkRate)

`osInit()` initializes the real-time OS library. It should be called one
before any call to other functions of the library. The _clkRate_ parameter
specifies the clock rate used in this process. A value of 0 disables the
clock. The maximum value on most systems is the HZ constant, which is
generally equal to 100.

`osInit()` returns `OK` or `ERROR` in case of an error. Then it sets
the errno value of the task.


Tasks
-----

PortLib uses Posix threads to implement VxWorks tasks. All
fonctionalities of VxWorks tasks are not present. Especially, portLib
uses the default Linux scheduler in the Posix threads implementation,
which doesn't share VxWorks real-time capabilities.

### taskSpawn2

	#include <taskLib.h>

    long taskSpawn2(const char *name, int priority, int options, int stackSize,
    void *(*start_routine)(void *), void *arg);

`taskSpawn2()` is similar to `taskSpawn` except that the calling
convention of the function run in the new task follows the
`pthread_create` prototype, which is more portable and permits to pass
arbitrary types to the new task. This should be used instead of
`taskSpawn()` in new code.

### taskDelete

	#include <taskLib.h>

	STATUS taskDelete(long taskId)

The `taskDelete()` function deletes (terminates) the specified task.

### taskSuspend

    #include <taskLib.h>

	STATUS taskSuspend(long taskId)

The `taskSuspend()` function is supposed to suspend the execution of
the given task.  In the implementation of pocoLibs on POSIX systems,
it is implemented by a call to `abort(2)` in order to save a core image
of the process.  This is done this way because in traditional pocoLibs
applications, `taskSuspend()` is used to stop a failed task for
debugging purposes.

### taskDelay

	#include <taskLib.h>

	STATUS taskDelay(int ticks)

The `taskDelay()` function suspends the execution of the current task for
the specified number of ticks.

### taskIdSelf

	#include <taskLib.h>

	long taskIdSelf(void)

The `taskIdSelf()` function returns the identifier of the current task. In
most of the above mentionned functions, if _tid_ is zero, it will design
the current task (ie `taskIdSelf()`.)


### taskSetUserData

	#include <taskLib.h>

	STATUS taskSetUserData(long taskId, unsigned long data)

### taskGetUserData

    #include <taskLib.h>

	unsigned long taskGetUserData(long taskId)

### taskCreateHookAdd

    #include <taskLookLib.h>
    STATUS taskCreateHookAdd(FUNCPTR createHook);

### taskCreateHookDelete

    #include <taskLookLib.h>
    STATUS taskCreateHookDelete(FUNCPTR createHook);

### taskSwitchHookAdd

    #include <taskLookLib.h>
    STATUS taskSwitchHookAdd(FUNCPTR switchHook);

### taskSwitchHookDelete

    #include <taskLookLib.h>
    STATUS taskSwitchHookDelete(FUNCPTR switchHook);

### taskDeleteHookAdd

    #include <taskLookLib.h>
    STATUS taskDeleteHookAdd(FUNCPTR deleteHook);

### taskDeleteHookDelete

    #include <taskLookLib.h>
    STATUS taskDeleteHookDelete(FUNCPTR deleteHook);



System Clock
------------

The sysLib library groups all functions that are system dependant and
provided by board support packages (BSP) under VxWorks. In the case of
pocoLibs, only clock handling functions are provided by sysLib.

### tickAnnounce

    #include <tickLib.h>
	void tickAnnounce(int)

`tickAnnounce()` is a function provided to be used as the default
handler for the system clock. This function handles the expiration of
the system watch dogs for pocoLibs and handles the tick counter.  The
default pocoLibs initialization routine, `h2initGlob()` registers this
function as handler of the system clock.

### tickGet

    #include <tickLib.h>
	unsigned long tickGet(void)

`ticGet()` returns the current value of the tick counter.

### tickSet

	#include <tickLib.h>
	void tickSet(unsigned long ticks)

`tickSet()` sets the value of the tick counter to the value of ticks.


### sysClkRateSet

	#include <sysLib.h>
	STATUS sysClkRateSet(int ticksPerSecond)

The rate of the system clock can be set with the `sysClkRateSet()` func‐
tion, to the given ticksPerSecond value. The initial rate of the system
clock is defined by the call to `h2initGlob()`.

`sysClkRateSet()` return `OK` or `ERROR` if an error occured.

### sysClkRateGet

	#include <sysLib.h>
	int sysClkRateGet(void)

The current rate of the system clock is returned by the sysClkRateGet()
function.

### Errors

When a function fails it calls `errnoSet()` with the error code returned
by the underlying operating system.  There is no sysLib specific error
code defined in pocoLibs.

### Bugs

On POSIX based systems, the system clock can be implemented by using
either POSIX defined real-time timers, using `timer_create(3)` or
traditionnal Unix timers based on `setitimer(2)`.  On certain Linux
kernel revisions the setitimer based timers offer better performance
than the POSIX real-time timers. Normally the configure script probes
the behaviour of both kind of timers and selects the best suiting
flavour. The `--disable-posix-timers` configure option can be used to
force use of traditional Unix timers at build time.

Linking pocoLibs application with external libraries that use the
`SIGALRM` signal is known to cause problems to the system clock.


Semaphores
----------

In the case of a POSIX-style operating system,  all these three
semaphore types are local to the Unix process that created them.

### semBCreate

    #include <semLib.h>
	SEM_ID semBCreate(int options, SEM_B_STATE initialState)

`semBCreate()` creates a binary semaphore.

_options_ can be `SEM_Q_FIFO` to specify that tasks waiting on the
semaphore are dequeued in a FIFO order, or `SEM_Q_PRIORITY` to specify
that tasks waiting on the semaphore are dequeued by decreasing
priority.

_initialState_ indicates the initial state of a binary semaphore, either
`SEM_EMPTY` or `SEM_FULL`.

### semCCreate

	#include <semLib.h>
	SEM_ID semCCreate(int options, int initialCount)

`semCCreate()` creates a counting semaphore.
_value_ indicates the initial value of a counting semaphore.

    S_semLib_TOO_MANY_SEM
    S_semLib_ALLOC_ERROR

### semMCreate

    #include <semLib.h>
	SEM_ID semMCreate(int options)

`semMCreate()` Creates a ticket lock which can be used as a mutual
exclusion semaphore.

### semDelete

	#include <semLib.h>
	STATUS semDelete(SEM_ID sem)

`semDelete()` deletes a semaphore from the process and releases any
resources used by the semaphore.  _sem_ is the identifier of the
semaphore to delete.

The behaviour of processes blocked on a semaphore when it is deleted is
not defined.

### semGive

	#include <semLib.h>
	STATUS semGive(SEM_ID sem)

`semGive()` implements the __V__ operation on the given semaphore.

### semTake

	#include <semLib.h>
	STATUS semTake(SEM_ID sem, int timeout)

`semTake()` implements the __P__ operation on the semaphore.
_timeout_ specifies a number of tick in which the operation must
succeed. If the semaphore was not taken with this delay, `ERROR` is
returned and the errno value of the task is set to `S_objLib_TIMEOUT`.


### semFlush

	#include <semLib.h>
	STATUS semFlusuh(SEM_ID sem)

`semFlush()` resets the value of a counting or binary semaphore to 0
or `SEM_EMPTY`, causing the next call to `semTake()` to effectively
block the calling task.

### Return values

`semBCreate()` and `semCCreate()` return the semaphore identifier for the
newly created semaphore or `NULL` in case of an error. In that case an
error code is left in the task's errno value.

`semDelete()`, `semGive()`, `semTake()` and `semFlush()` return `OK`
or `ERROR` in case an error occured. In that case an error code is
left in the task's errno value.

### Errors

The task's errno value can be set to an error code from the operating
system or to:

`S_objLib_OBJ_TIMEOUT`
: A timeout occured on a `semTake()` operation.



Watchdogs
---------

### wdCreate

	#include <wdLib.h>
	WDOG_ID wdCreate(void)

`wdCreate()` creates a new inactive watch dog.

### wdDelete

	#include <wdLib.h>
	STATUS wdDelete(WDOG_ID wdId)

wdDelete() destroys an exising watch dog.

### wdStart

	#include <wdLib.h>
	STATUS wdStart(WDOG_ID wdId, int delay, FUNCPTR pRoutine, long parameter)

`wdStart()` starts the timer associated with a watch dog.  _wdId_ is a
watch dog identifier as returned by `wdCreate()`.  _delay_ is the
number of ticks until the watch dog expires.  _pRoutine_ is a pointer
to a function that will be called when the timer expires.  _parameter_
is an integer value that will be passed to the routine when it will be
called.

### wdCancel

	#include <wdLib.h>
	STATUS wdCancel(WDOG_ID wdId)

`wdCancel()` cancels the active watchDog designated by _wdId_.

### Return values

`wdCreate()` returns the identifier of the newly created watchdor or NULL
if an error occured.

`wdDelete()`,`wdStart()` and `wdCancel()` return `OK` or `ERROR` if
an error ocurred. All functions set the task error code in case of
errors.

### Errors
The error code of the current task can be set to:

`S_memLib_NOT_ENOUGH_MEMORY`
: The allocation of memory for a new watchog failed.

`S_objLib_OBJ_ID_ERROR`
: The wdId passed to a function is not valid.

Error management
----------------

`errnoGet()` and `errnoSet()` are designed to emulate the VxWorks
function of the same name.

### errnoGet

	#include <errnoLib.h>
	int errnoGet(void)

`errnoGet()` returns the pocoLibs error code associated with the current task.

### errnoSet

	#include <errnoLib.h>
    STATUS errnoSet(int errorValue)

`errnoSet()` sets the pocoLibs error code associated with the current
thread to the value passed in errorValue.
