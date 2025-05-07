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

#ifndef _TASKHOOKLIB_H
#define _TASKHOOKLIB_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*TASKHOOKFUNC)(OS_TCB *);

void taskHookInit(void);
STATUS taskCreateHookAdd(TASKHOOKFUNC createHook);
STATUS taskCreateHookDelete(TASKHOOKFUNC createHook);
STATUS taskSwitchHookAdd(TASKHOOKFUNC switchHook);
STATUS taskSwitchHookDelete(TASKHOOKFUNC switchHook);
STATUS taskDeleteHookAdd(TASKHOOKFUNC deleteHook);
STATUS taskDeleteHookDelete(TASKHOOKFUNC deleteHook);

#ifdef __cplusplus
}
#endif

#endif
