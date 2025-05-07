/*
 * Copyright (c) 1990, 2003, 2025 CNRS/LAAS
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

#include "pocolibs-config.h"

/***
 *** Configuration des modules inclus dans le shell
 ***/
#include "portLib.h"
#include "mboxLib.h"
#include "shellConfig.h"

#ifdef INCLUDE_MBOX
extern void mboxEssay(void);
#endif
#ifdef INCLUDE_TIMER
extern void h2timerTest(int);
#endif
#ifdef INCLUDE_POSTER
extern void posterShow(void);
#endif

/* unused varniable, to pull symmbols from libraries */
void *shellTabFunc[] = {
#ifdef INCLUDE_MBOX
    mboxEssay,
#endif
#ifdef INCLUDE_TIMER
    h2timerTest,
#endif
#ifdef INCLUDE_POSTER
    posterShow,
#endif
};

