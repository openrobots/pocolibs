/*
 * Copyright (C) 1994 LAAS/CNRS 
 *
 * Matthieu Herrb - April 1994
 *
 * $Source$
 * $Revision$
 * $Date$
 * 
 */
#ifndef _CONFIG_H
#define _CONFIG_H

/***
 *** Configuration options for xes
 ***/

/*----------------------------------------------------------------------*/

/*
 * Portability defines
 */
#if defined(sun) && defined(SVR4)

#define USE_SYSV_PTY
#define USE_STREAMS
#define USE_RLIMIT

#endif /* Solaris 2.x */

#if defined(sun) && !defined(SVR4)

#undef USE_SYSV_PTY
#undef USE_STREAMS
#undef USE_RLIMIT

#endif /* SunOS 4.1.x */

#ifdef linux

#undef USE_SYSV_PTY
#undef USE_STREAMS
#define USE_RLIMIT

#endif /* linux */

#if defined(__NetBSD__)

#undef USE_SYSV_PTY
#undef USE_STREAMS
#undef USE_RLIMIT

#endif /* __NetBSD__ */

#if defined(__sgi__)
#define USE_SYSV_PTY
#undef USE_STREAMS
#undef USE_RLIMIT
#endif

#ifndef USE_SYSV_PTY
#define MASTER  "/dev/ptyXY"
#define SLAVE   "/dev/ttyXY"
#endif


/*----------------------------------------------------------------------*/

/*
 * Other stuff
 */

#define BUFFER_SIZE 1024
#define RNG_SIZE (32 * BUFFER_SIZE)

#define PID_FILE "/tmp/xes-pid"


#endif
