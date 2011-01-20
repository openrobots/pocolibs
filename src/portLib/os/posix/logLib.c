/*
 * Copyright (c) 2004 CNRS/LAAS
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

#include <stdio.h>
#include <stdarg.h>

#define LOGLIB_C
#include "portLib.h"
#include "logLib.h"

/*
 * Message logging facilities
 */

STATUS
logInit(int fd, int maxMsgs)
{
   return OK;
}

void
logEnd(void)
{
   ;
}

int
logMsg(const char *fmt, ...)
{
	int retval;
	va_list ap;

	va_start(ap, fmt);
	
	retval = vfprintf(stderr, fmt, ap);
	va_end(ap);
	return retval;
}
