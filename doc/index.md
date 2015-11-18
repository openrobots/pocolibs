pocoLibs documentation
======================

_pocoLibs_ stands for __Po__rtability and __Co__mmunication
__Lib__raries. It is a set of libraries written to support the
execution of Genom modules.

Historically these libraries have been written for the !VxWorks
operating system, which was the first environment of execution for
Genom at LAAS. Later they have been ported to Unix systems and more
specifically to Linux. The __po__ (portability) part is aimed a
providing some !VxWorks style APIs to Linux.

The __co__ (communication) part is made of two major components : one
based on mailboxes (or messages ''queues'') and one base on posters
(or shared memory). There are a few other components that provide
helper functions for various tasks (time management, watchdogs and
timers, math,...).

 
* [portLib](portLib)
* [comLib](comLib)
