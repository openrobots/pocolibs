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

#ifndef _MEMLIB_H
#define _MEMLIB_H

#error "memLib.h to be removed"

/* -- ERRORS CODES ----------------------------------------------- */

#include "h2errorLib.h"
#define M_memLib        17

#define S_memLib_NOT_ENOUGH_MEMORY           H2_ENCODE_ERR(M_memLib, 1)

#define MEM_LIB_H2_ERR_MSGS { \
  {"NOT_ENOUGH_MEMORY",  H2_DECODE_ERR(S_memLib_NOT_ENOUGH_MEMORY)},  \
}


#endif
