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

#include "config.h"
__RCSID("$LAAS$");

#ifndef VXWORKS
#include "portLib.h"
#else
#include "vxWorks.h"
#endif

#include <stdio.h>

#include "h2endianness.h"

H2_ENDIANNESS h2localEndianness (void)
{

  union {
    char c[sizeof(int)];
    int i;
  } u;

  u.i  = 256; /* 0x10 */
  printf ("u.i = %d\n", u.i);
  printf ("u.c[1] = %d\n", u.c[1]);

  /* Little endian */
  if (u.c[1] == 1) {
#if defined(WORDS_BIGENDIAN)
    printf ("** ERROR h2localEndianness: this library was compiled with wrong endianness\n");
#endif
    return H2_LITTLE_ENDIAN;
  }
  
  /* Big endian */
#if !defined(WORDS_BIGENDIAN)
  printf ("** ERROR h2localEndianness: this library was compiled with wrong endianness\n");
#endif
  return H2_BIG_ENDIAN;
}
