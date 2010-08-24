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

#include "pocolibs-config.h"

#include <stdio.h>

#include "portLib.h"
#include "testPosterLib.h"

static int testEnd(POSTER_ID posterId)
{
  return 1;
}

int main(int argc, char *argv[])
{
  POSTER_ID posterIdF;
  TESTPOST_STR testOut;
  int size = sizeof(TESTPOST_STR);
  int sizeOut;
  H2_ENDIANNESS endianness;

  printf ("LOCAL ENDIANNESS = %d\n", h2localEndianness());

  /* find */
  if (posterFind(TEST_POSTER_NAME, &posterIdF) == ERROR) {
    printf ("testPostersRead: posterFind failed\n");
    return testEnd(posterIdF);
  }
  printf ("testPosterRead: posterFind TEST_POSTER_NAME = %d\n", 
	  (int)posterIdF);

  /* endianness */
  posterEndianness(posterIdF, &endianness);
  printf ("testPosterRead: endianness = %d\n", endianness);

  /* read */
  if ((sizeOut=posterRead(posterIdF, 0, (void *)&testOut, size)) != size) {
    printf ("testPostersRead: posterRead size = %d != %d \n", sizeOut, size);
    return testEnd(posterIdF);
  }
  if (endianness != h2localEndianness()) 
    endianswap_TESTPOST_STR(&testOut);
  printTestStr("Read", &testOut);

  posterEndianness(posterIdF, &endianness);
  printf ("   endianness = %d\n", endianness);

  /* end */
  return testEnd(posterIdF);
}

