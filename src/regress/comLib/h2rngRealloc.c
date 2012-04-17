/*
 * Copyright (c) 2012 CNRS/LAAS
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
#include <string.h>

#include <portLib.h>
#include <h2errorLib.h>
#include <h2rngLib.h>

char strtest[] = "123qwerty890";

int
pocoregress_init()
{
  int n;
  H2RNG_ID r, s;
  char buf[sizeof(strtest)];

  r = h2rngCreate(H2RNG_TYPE_BYTE, sizeof(strtest));
  if (!r) { h2perror("h2rngCreate"); return 1; }
  n = h2rngBufPut(r, strtest, sizeof(strtest));
  if (n != sizeof(strtest)) {
    logMsg("cannot put content\n"); return 1;
  }
  logMsg("put %s\n", strtest);

  s = h2rngRealloc(r, 1);
  if (s) { logMsg("resize to an invalid size works\n"); return 1; }

  s = h2rngRealloc(r, 1.5 * sizeof(strtest));
  if (!s) { h2perror("h2rngRealloc"); return 1; }
  r = s;
  logMsg("resize +50%%\n", buf);

  n = h2rngBufGet(r, buf, sizeof(buf));
  if (n != sizeof(buf) || strcmp(buf, strtest)) {
    logMsg("resize #1 lost content: %s\n", buf); return 1;
  }
  logMsg("got %s\n", buf);

  n = h2rngBufPut(r, strtest, sizeof(strtest));
  if (n != sizeof(strtest)) {
    logMsg("cannot put content\n"); return 1;
  }
  logMsg("put (wrapping) %s\n", strtest);

  s = h2rngRealloc(r, sizeof(strtest));
#ifdef allow_shrinking
  logMsg("resize -50%%\n", buf);
#else
  if (s) { logMsg("shrinking should not work\n"); return 1; }
  s = h2rngRealloc(r, 2 * sizeof(strtest));
  logMsg("resize +100%%\n", buf);
#endif /* allow_shrinking */
  if (!s) { h2perror("h2rngRealloc"); return 1; }
  r = s;

  n = h2rngBufGet(r, buf, sizeof(buf));
  if (n != sizeof(buf) || strcmp(buf, strtest)) {
    logMsg("resize #1 lost content: %s\n", buf); return 1;
  }
  logMsg("got %s\n", buf);

  return 0;
}
