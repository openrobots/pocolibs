/*
 * Copyright (c) 2003 CNRS/LAAS
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
