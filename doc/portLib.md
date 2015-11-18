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

    typedef int (*FUNCPTR) ();
    typedef int (*INTFUNCPTR)(); 
    typedef void (*VOIDFUNCPTR)();
    typedef float (*FLTFUNCPTR)();
    typedef double (*DBLFUNCPTR)();
    typedef void *(*VOIDPTRFUNCPTR)();

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

_clkRate_ specifies the clock rate (in ticks per second) of the
internal clock that portLib provides to its tasks.


Tasks
-----

PortLib uses Posix threadss to implement VxWorks tasks. All
fonctionalities of VxWorks tasks are not present. Especially, portLib
uses the default Linux scheduler in the Posix threads implementation,
which doesn't share VxWorks real-time capabilities.

### taskSpawn

    #include <taskLib.h>
	
    long taskSpawn(char *name, int priority, int options, int stackSize,
          FUNCPTR entryPt, ...)

### taskSpawn2

	#include <taskLib.h>

    long taskSpawn2(const char *name, int priority, int options, int stackSize,
    void *(*start_routine)(void *), void *arg);

### taskDelete

	#include <taskLib.h>

	STATUS taskDelete(long taskId)

### taskSuspend

    #include <taskLib.h>

	STATUS taskSuspend(long taskId)

### taskDelay

	#include <taskLib.h>

	STATUS taskDelay(int ticks)

### taskIdSelf

	#include <taskLib.h>

	long taskIdSelf(void)

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

### tickGet

    #include <tickLib.h>

	unsigned long tickGet(void)


### sysClkRateSet

	#include <sysLib.h>
	STATUS sysClkRateSet(int ticksPerSecond)

### sysClkRateGet

	#include <sysLib.h>
	int sysClkRateGet(void)


Semaphores
----------


### semBCreate

    #include <semLib.h>
	SEM_ID semBCreate(int options, SEM_B_STATE initialState)

    SEM_Q_FIFO
    SEM_Q_PRIORITY

    SEM_EMPTY
    SEM_FULL

    S_semLib_TOO_MANY_SEM
    S_semLib_ALLOC_ERROR

### semCCreate

	#include <semLib.h>
	SEM_ID semCCreate(int options, int initialCount)

    S_semLib_TOO_MANY_SEM
    S_semLib_ALLOC_ERROR

### semMCreate

    #include <semLib.h>
	SEM_ID semMCreate(int options)

_semMCreate_ Creates a ticket lock.

### semDelete

	#include <semLib.h>
	STATUS semDelete(SEM_ID sem)

    S_semLib_NOT_A_SEM

### semGive

	#include <semLib.h>
	STATUS semGive(SEM_ID sem)

    S_semLib_NOT_A_SEM

### semTake

	#include <semLib.h>
	STATUS semTake(SEM_ID sem, int timeout)

    S_semLib_NOT_A_SEM
    S_semLib_TIMEOUT

### semFlush

	#include <semLib.h>
	STATUS semFlusuh(SEM_ID sem)

    S_semLib_NOT_A_SEM



Watchdogs
---------

### wdCreate

	#include <wdLib.h>
	WDOG_ID wdCreate(void)

	S_wdLib_NOT_ENOUGH_MEMORY

### wdDelete

	#include <wdLib.h>
	STATUS wdDelete(WDOG_ID wdId)

	S_wdLib_ID_ERROR

### wdStart

	#include <wdLib.h>
	STATUS wdStart(WDOG_ID wdId, int delay, FUNCPTR pRoutine, long parameter)

	S_wdLib_ID_ERROR

### wdCancel

	#include <wdLib.h>
	STATUS wdCancel(WDOG_ID wdId)

	S_wdLib_ID_ERROR

Error management
----------------'

### errnoGet

	#include <errnoLib.h>
	int errnoGet(void)

### errnoSet

	#include <errnoLib.h>
    STATUS errnoSet(int errorValue)

