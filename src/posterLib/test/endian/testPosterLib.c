/*
 * Copyright (c) 2003 CNRS/LAAS
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
__RCSID("$LAAS$");

#include <stdio.h>

#include <portLib.h>
#include "testPosterLib.h"
#include "h2endianness.h"

void printTestStr(char *who, TESTPOST_STR *test)
{
  printf ("testPoster%s : TESTPOST_STR =\n", who);
  printf ("  tabc[4] %c %c %c %c\n", 
	  test->tabc[0], test->tabc[1], test->tabc[2], test->tabc[3]);
  printf ("  si[4] %d %d %d %d\n", 
	  test->si[0], test->si[1], test->si[2], test->si[3]);
  printf ("  i[2] %d %d\n", test->i[0], test->i[1]);
  printf ("  li %ld\n", test->li);
  printf ("  ll %lld\n", test->ll);
  printf ("  f[2] %f %f\n", test->f[0], test->f[1]);
  printf ("  d %g\n", test->d);
  printf ("\n");
}


void endianswap_TESTPOST_STR (TESTPOST_STR *test) 
{
  int dims2[1]={2};
  int dims4[1]={4};

  printf ("change endianness\n");

  endianswap_char(test->tabc, 1, dims4);
  endianswap_short_int(test->si, 1, dims4);
  endianswap_int(test->i, 1, dims2);
  endianswap_long_int(&test->li, 0, NULL);
  endianswap_longlong(&test->ll, 0, NULL);
  endianswap_float(test->f, 1, dims2);
  endianswap_double(&test->d, 0, NULL);
}
