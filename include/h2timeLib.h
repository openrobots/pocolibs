/* $LAAS$ */
/*
 * Copyright (c) 1993, 2003 CNRS/LAAS
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

#include <sys/types.h>


/* Def. type structure de temps */
typedef struct {
  u_long ntick;               /* nombre de ticks */
  u_short msec;               /* milli-secondes */
  u_short sec;                /* secondes */
  u_short minute;             /* minutes */
  u_short hour;               /* heures */
  u_short day;                /* jour de la semaine */
  u_short date;               /* jour du mois */
  u_short month;              /* mois */
  u_short year;               /* annee */
} H2TIME;

/* Prototypes */

extern STATUS h2timeAdj ( H2TIME *pTimeStr );
extern STATUS h2timeGet ( H2TIME *pTimeStr );
extern void h2timeInit ( void );
extern STATUS h2timeInterval ( H2TIME *pOldTime, u_long *pNmsec );
extern STATUS h2timeSet ( H2TIME *pTimeStr );
extern void h2timeShow ( void );

#endif /* _H2TIMELIB_H */
