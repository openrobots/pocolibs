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

#ifndef _SYSLIB_H
#define _SYSLIB_H

#ifdef __cplusplus
extern "C" {
#endif

extern STATUS sysClkConnect ( FUNCPTR routine, int arg );
extern void sysClkEnable ( void );
extern void sysClkDisable ( void );
extern STATUS sysClkRateSet ( int ticksPerSecond );
extern int sysClkRateGet ( void );
 

#ifdef __cplusplus
};
#endif

#endif
