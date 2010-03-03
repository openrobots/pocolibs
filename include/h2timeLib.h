/* $LAAS$ */
/*
 * Copyright (c) 1993, 2003-2004 CNRS/LAAS
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

#ifndef _H2TIMELIB_H
#define _H2TIMELIB_H

#include <portLib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Def. type structure de temps */
typedef struct H2TIME_STR {
  unsigned long ntick;               /* ticks number */
  unsigned short msec;               /* milli-seconds */
  unsigned short sec;                /* seconds */
  unsigned short minute;             /* minutes */
  unsigned short hour;               /* hours */
  unsigned short day;                /* week day number*/
  unsigned short date;               /* month day number */
  unsigned short month;              /* month */
  unsigned short year;               /* year */
} H2TIME;

/* Structure compatible with POSIX clock functions */
typedef struct H2TIMESPEC_STR {
	long tv_sec;
	long tv_nsec;
} H2TIMESPEC;

/* Prototypes */
struct timeval;

extern STATUS h2timeAdj ( H2TIME *pTimeStr );
extern STATUS h2timeGet ( H2TIME *pTimeStr );
extern STATUS h2GetTimeSpec(H2TIMESPEC *pTs);
extern void h2timeFromTimeval(H2TIME* pTimeStr, const struct timeval* tv);
extern void timevalFromH2time(struct timeval* tv, const H2TIME* pTimeStr);
extern void h2timeFromTimespec(H2TIME* pTimeStr, const H2TIMESPEC* ts);
extern void timespecFromH2time(H2TIMESPEC* ts, const H2TIME* pTimeStr);
extern void h2timeInit ( void );
extern STATUS h2timeInterval ( H2TIME *pOldTime, unsigned long *pNmsec );
extern STATUS h2timespecInterval ( const H2TIMESPEC *pOldTime, unsigned long *pNmsec );
extern STATUS h2timeSet ( H2TIME *pTimeStr );
extern void h2timeShow ( void );

#ifdef __cplusplus
};
#endif

#endif /* _H2TIMELIB_H */
