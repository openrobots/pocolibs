/*
 * Copyright (c) 1990, 2003-2004 CNRS/LAAS
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

/***
 *** Global initialization function for an RTAI task using comLib
 ***
 *** ticksPerSec -> system clock frequency
 ***                0 -> no clock
 ***
 ***/

#include "pocolibs-config.h"
__RCSID("$LAAS$");

#include "h2initGlob.h"

/**
 ** comLib initialization for an RTAI module
 **/

STATUS
h2initGlob(int ticksPerSec)
{
   /* There is nothing to do here. comLib should already be present in
    * the kernel and configured properly. */

   /* Maybe we should however report an error if ticksPerSec is not the
    * same as the current one? */

   return OK;
}
