/* $LAAS$ */
/*
 * Copyright (c) 1999, 2003 CNRS/LAAS
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

#ifndef _H2TIMERLIBPROTO_H
#define _H2TIMERLIBPROTO_H
#ifdef __STDC__

extern STATUS h2timerInit ( void );
extern H2TIMER_ID h2timerAlloc ( void );
extern STATUS h2timerStart ( H2TIMER_ID timerId, int periode, int delay );
extern STATUS h2timerPause ( H2TIMER_ID timerId );
extern STATUS h2timerPauseReset ( H2TIMER_ID timerId );
extern STATUS h2timerStop ( H2TIMER_ID timerId );
extern STATUS h2timerChangePeriod ( H2TIMER_ID timerId, int periode );
extern STATUS h2timerFree ( H2TIMER_ID timerId );

#else /* __STDC__ */

extern STATUS h2timerInit (/* void */);
extern H2TIMER_ID h2timerAlloc (/* void */);
extern STATUS h2timerStart (/* H2TIMER_ID timerId, int periode, int delay */);
extern STATUS h2timerPause (/* H2TIMER_ID timerId */);
extern STATUS h2timerPauseReset (/* H2TIMER_ID timerId */);
extern STATUS h2timerStop (/* H2TIMER_ID timerId */);
extern STATUS h2timerChangePeriod (/* H2TIMER_ID timerId, int periode */);
extern STATUS h2timerFree (/* H2TIMER_ID timerId */);

#endif /* __STDC__ */
#endif /* _H2TIMERLIBPROTO_H */
