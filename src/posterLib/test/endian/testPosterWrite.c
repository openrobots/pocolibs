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

#include "config.h"
__RCSID("$LAAS$");

#ifdef UNIX
#include "portLib.h"
#else
#include <vxWorks.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#include "h2errorLib.h"

#include "testPosterLib.h"

static int testEnd(POSTER_ID posterId)
{
  if(posterDelete(posterId) == ERROR) {
    h2perror ("testPosterWrite: posterDelete failed\n");
  }
  printf ("testPosterWrite: poster deleted, Bye\n");
  return 1;
}

#ifdef UNIX
int main()
#else
int testPosterWrite ()
#endif
{
  POSTER_ID posterIdC;
  POSTER_ID posterIdF;
  TESTPOST_STR testIn = INIT_TESTPOST_STR;
  TESTPOST_STR testOut;
  int size = sizeof(TESTPOST_STR);
  int sizeOut;
  H2_ENDIANNESS endianness;
  char c;

  printf ("testPosterWrite: LOCAL ENDIANNESS = %d\n", h2localEndianness());

  /* create */
  if (posterCreate(TEST_POSTER_NAME, size, &posterIdC) == ERROR) {
    printf ("testPosterWrite: posterCreate(%s) failed : %s\n", 
	    TEST_POSTER_NAME, h2getMsgErrno(errnoGet()));
    return 1;
  }
  printf ("testPosterWrite: poster %s created. Id = %0x\n", 
	  TEST_POSTER_NAME, (int)posterIdC);
  posterEndianness(posterIdC, &endianness);
  printf ("testPosterWrite: posterEndianness(%s) = %d\n\n", 
	  TEST_POSTER_NAME, endianness);

  /* write */
  if ((sizeOut=posterWrite(posterIdC, 0, (void *)&testIn, size)) != size) {
    printf ("testPosterWrite: posterWrite size = %d != %d \n", sizeOut, size);
    return testEnd(posterIdC);
  }
  printf ("testPosterWrite: poster %s writen\n", TEST_POSTER_NAME);

  /* read after create */
  if ((sizeOut=posterRead(posterIdC, 0, (void *)&testOut, size)) != size) {
    printf ("testPosterWrite: posterRead size = %d != %d \n", sizeOut, size);
    return testEnd(posterIdC);
  }
  printf ("testPosterWrite: poster %s read OK\n", TEST_POSTER_NAME);

  /* display */
  if (endianness != h2localEndianness()) 
    endianswap_TESTPOST_STR(&testOut);
  printTestStr("Write", &testOut);
  printf ("\n");

  /* find */
  if (posterFind(TEST_POSTER_NAME, &posterIdF) == ERROR) {
    printf ("testPosterWrite: posterFind failed\n");
    return testEnd(posterIdC);
  }
/*   printf ("posterFind TEST_POSTER_NAME posterIdF = %d\n", (int)posterIdF); */
  posterEndianness(posterIdF, &endianness);
  printf ("   endianness = %d\n", endianness);

  /* read after find */
  if ((sizeOut=posterRead(posterIdF, 0, (void *)&testOut, size)) != size) {
    printf ("testPosterWrite: posterRead size = %d != %d \n", sizeOut, size);
    return testEnd(posterIdC);
  }

  /* display */
  if (endianness != h2localEndianness()) 
    endianswap_TESTPOST_STR(&testOut);
  printTestStr("Write", &testOut);

  /* find again */
  if (posterFind(TEST_POSTER_NAME, &posterIdF) == ERROR) {
    printf ("testPosterWrite: posterFind failed\n");
    return testEnd(posterIdC);
  }
  printf ("PosterIdF %s = %d\n", TEST_POSTER_NAME, (int)posterIdF);

  
  posterEndianness(posterIdF, &endianness);
  printf ("   endianness = %d\n", endianness);

  printf ("Return to end ....");
  fflush(stdout);
  scanf("%c", &c);

  /* end */
  return testEnd(posterIdF);
}

