comLib
======

ComLib is the communication part of Pocolibs. It is mainly composed of
two high-level communication paradigms: a message based client/server
and shared memory with one exclusive writer and several readers
(called posters).

This library was initially developped for the 'Hilare 2' robot
platform at LAAS, hence many parts of its API use the __h2__ prefix to
identify themselve.

The comLib API allows several Unix processes on the same machine to
communicate. posterLib has an extension that make it possible to
semi-transparently access posters on other nodes of the network. See
the _remotePosterLib_ documentation for that.

Initialization
--------------

### h2initGlob

    #include <h2initGlob.h>
	STATUS h2initGlob(int ticksPerSec)

### h2 devices

All comLib objects are derived from a common _h2 device_ structure
which is stored in shared memory. These objects are persistant across
processes, but lost on reboot of the system. The h2devLib library is
used internally to manage these objects.

### Tools

h2devLib also provides a set of command line tools needed to manage
the persistant objects outside of existing processes.

#### h2

    h2 init
	h2 end
	h2 info
	h2 clean <id>


Global Semaphores
-----------------

### h2semAlloc

	#include <h2semLib.h>
    H2SEM_ID h2semAlloc(int type)

### h2semDelete

	#include <h2semLib.h>
	STATUS h2semDelete(H2SEM_ID sem)

### h2semGive

	#include <h2semLib.h>
	STATUS h2semGive(H2SEM_ID sem)
	
### h2semTake

	#include <h2semLib.h>
	STATUS h2semTake(H2SEM_ID sem, int timeout)

### h2semFlush

	#include <h2semLib.h>
	BOOL h2semFlush(H2SEM_ID sem)

Common Structs
--------------

### commonStructCreate

	#include <commonStructLib.h>
    STATUS commonStructCreate(int len, void **pStructAddr)

### commonStructDelete

	#include <commonStructLib.h>
	STATUS commonStructDelete(const void *pCommonStruct)

### commonStructGive

	#include <commonStructLib.h>
	STATUS commonStructGive(const void *pCommonStruct)

### commonStructTake

	#include <commonStructLib.h>
	STATUS commonStructTake(const void *pCommonStruct)

Timers
------

### h2timerAlloc

	#include <h2timerLib.h>
    H2TIMER_ID h2timerAlloc(void);

###h2timerStart

    #include <h2timerLib.h>
    STATUS h2timerStart(H2TIMER_ID timerId, int period, int delay);

### h2timerPause

    #include <h2timerLib.h>
    STATUS h2timerPause(H2TIMER_ID timerId);

#h2timerPauseReset

	#include <h2timerLib.h>
    STATUS h2timerPauseReset(H2TIMER_ID timerId);

### h2timerStop

    #include <h2timerLib.h>
    STATUS h2timerStop(H2TIMER_ID timerId);

### h2timerChangePeriod

    #include <h2timerLib.h>
    STATUS h2timerChangePeriod(H2TIMER_ID timerId, int period);

### h2timerFree

	#include <h2timerLib.h>
    STATUS h2timerFree(H2TIMER_ID timerId);

Mailboxes
---------

### mboxCreate

    #include <moxLib.h>
    STATUS mboxCreate(const char *name, int len, MBOX_ID *pMboxId);

### mboxResize
	#include <moxLib.h>
	STATUS mboxResize(MBOX_ID mboxId, int size);

### mboxDelete

    #include <moxLib.h>
	STATUS mboxDelete(MBOX_ID mboxId);

#### mboxEnd

    #include <moxLib.h>
    STATUS mboxEnd(long taskId);

### mboxFind

    #include <moxLib.h>
	STATUS mboxFind(const char *name, MBOX_ID *pMboxId);

### mbox Init

    #include <moxLib.h>
    STATUS mboxInit(const char *procName);


### mboxIoctl

    #include <moxLib.h>
	STATUS mboxIoctl(MBOX_ID mboxId, int codeFunc, void *pArg);

### mboxPause

    #include <moxLib.h>
    BOOL mboxPause(MBOX_ID mboxId, int timeout);

### mboxRcv

    #include <moxLib.h>
	int mboxRcv(MBOX_ID mboxId, MBOX_ID *pFromId, char *buf,
	            int maxbytes, int timeout);

### mboxSend

	#include <moxLib.h>
	STATUS mboxSend(MBOX_ID toId, MBOX_ID fromId, char *buf, int nbytes);

### mboxSkip

    #include <moxLib.h>
	STATUS mboxSkip(MBOX_ID mboxId);

### mboxSpy

    #include <moxLib.h>
	int mboxSpy(MBOX_ID mboxId, MBOX_ID *pFromId, int *pNbytes,
	             char *buf, int maxbytes);

Client-Server Objects
---------------------

### csClientEnd

	#include <csLib.h>
    STATUS csClientEnd ( CLIENT_ID clientId );

### csClientInit

	#include <csLib.h>
	STATUS csClientInit ( const char *servMboxName, int maxRqstSize, 
  	                     int maxIntermedReplySize, int maxFinalReplySize,
                         CLIENT_ID *pClientId );

### csClientReplyRcv

	#include <csLib.h>
    int csClientReplyRcv ( CLIENT_ID clientId, int rqstId, int block, 
                           char *intermedReplyDataAdrs,
						   int intermedReplyDataSize, 
                           FUNCPTR intermedReplyDecodFunc,
						   char *finalReplyDataAdrs, 
                           int finalReplyDataSize,
						   FUNCPTR finalReplyDecodFunc );

### csClientRqstIdFree

	#include <csLib.h>
    int csClientRqstIdFree ( CLIENT_ID clientId, int rqstId );

### csClientRqstSend

	#include <csLib.h>
    STATUS csClientRqstSend ( CLIENT_ID clientId, int rqstType, 
                              char *rqstDataAdrs, int rqstDataSize,
							  FUNCPTR codFunc, BOOL intermedFlag, 
                              int intermedReplyTout, int finalReplyTout,
                              int *pRqstId );
### csMoxEnd

	#include <csLib.h>
    STATUS csMboxEnd ( void );

### csMboxInit

	#include <csLib.h>
    STATUS csMboxInit ( const char *mboxBaseName, int rcvMboxSize, 
                        int replyMboxSize );

### csMboxUpdate

	#include <csLib.h>
    STATUS csMboxUpdate( int rcvMboxSize, int replyMboxSize );

### csMboxStatus

	#include <csLib.h>
    int csMboxStatus ( int mask );

### csMboxWait

	#include <csLib.h>
    int csMboxWait ( int timeout, int mboxMask );

### csServEnd

	#include <csLib.h>
    STATUS csServEnd ( SERV_ID servId );

### csServFuncInstall

	#include <csLib.h>
    STATUS csServFuncInstall ( SERV_ID servId, int rqstType, 
                               FUNCPTR rqstFunc );

### csServInit

	#include <csLib.h>
    STATUS csServInit ( int maxRqstDataSize, int maxReplyDataSize, 
                        SERV_ID *pServId );

### csServInitN

	#include <csLib.h>
    STATUS csServInitN ( int maxRqstDataSize, int maxReplyDataSize, 
                         int nbRqstFunc, SERV_ID *pServId );

### csServReplySend

	#include <csLib.h>
    STATUS csServReplySend ( SERV_ID servId, int rqstId, int replyType, 
                             int replyBilan, char *replyDataAdrs,
							 int replyDataSize, FUNCPTR codFunc );

### csServRqstExec

	#include <csLib.h>
    STATUS csServRqstExec ( SERV_ID servId );

### csServRqstIdFree

	#include <csLib.h>
    STATUS csServRqstIdFree ( SERV_ID servId, int rqstId );

### csServRqstParamsGet

	#include <csLib.h>
    STATUS csServRqstParamsGet ( SERV_ID servId, int rqstId, 
                                 char *rqstDataAdrs, int rqstDataSize,
								 FUNCPTR decodFunc );



Events
------

### h2evnSusp

    #include <h2evnLib.h>
	h2evnSusp(int timeout)

### h2evnSignal

    #include <h2evnLib.h>
	h2evnSignal(long taskId);

### h2evnClear

    #include <h2evnLib.h>
	h2evnClear(void)

Posters
-------

### posterCreate

	#include <posterLib.h>
    STATUS posterCreate (const char *name, int size, POSTER_ID *pPosterId );

### posterMemCreate

	#include <posterLib.h>
    STATUS posterMemCreate (const char *name, int busSpace,
                            void *pPool, int size, POSTER_ID *pPosterId );

### posterDelete

	#include <posterLib.h>
    STATUS posterDelete ( POSTER_ID dev );

### posterFind

    #include <posterLib.h>
    STATUS posterFind (const char *name, POSTER_ID *pPosterId );

### posterWrite

	#include <posterLib.h>
    int posterWrite ( POSTER_ID posterId, int offset, void *buf, int nbytes );

### posterRead

	#include <posterLib.h>
    int posterRead ( POSTER_ID posterId, int offset, void *buf, int nbytes );

### posterTake

	#include <posterLib.h>
    STATUS posterTake ( POSTER_ID posterId, POSTER_OP op );

### posterGive

    #include <posterLib.h>
    STATUS posterGive ( POSTER_ID posterId );

### posterAddr

	#include <posterLib.h>
    void * posterAddr ( POSTER_ID posterId );

### posterIoctl

	#include <posterLib.h>
    STATUS posterIoctl(POSTER_ID posterId, int code, void *parg);

### posterName

	#include <posterLib.h>
    char* posterName(POSTER_ID posterId);

### posterForget

	#include <posterLib.h>
    STATUS posterForget(POSTER_ID posterId);
