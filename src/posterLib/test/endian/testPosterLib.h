/* $LAAS$ */
/*
 * Copyright (c) 1990, 2003 CNRS/LAAS
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

#ifndef VXWORKS
#include "portLib.h"
#else
#include <vxWorks.h>
#endif
#include <stdio.h>

#include "posterLib.h"
#include "h2mathLib.h"

typedef struct TESTPOST_STR {
  char tabc[4];
  short int si[4];
  int i[2];
  long int li;
  long long int ll;
  float f[2];
  double d;
} TESTPOST_STR;

#define INIT_TESTPOST_STR { \
{'a', 'b', 'c', '\0'}, \
{32, 33, 62, 63}, \
{10000, 20000}, \
1234, \
12345678, \
{1234.5678, -1234.5678}, \
PI}

void printTestStr(char *who, TESTPOST_STR *test);
void endianswap_TESTPOST_STR (TESTPOST_STR *test);

#define TEST_POSTER_NAME "test"


