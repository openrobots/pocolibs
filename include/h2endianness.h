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

/** 
 ** Allows to get the local endianness using the function
 ** h2localEndianness().
 **
 **  Remark: To get the endianness at compilation time (instead of
 **  execution time), one can use the macro H2_LOCAL_ENDIANNESS
 ** that returns the same thing.
 **
 **/

#ifndef H2_ENDIANNESS_H
#define H2_ENDIANNESS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum H2_ENDIANNESS {
  H2_BIG_ENDIAN,
  H2_LITTLE_ENDIAN
} H2_ENDIANNESS;

/* Returns the local endianness */
extern H2_ENDIANNESS h2localEndianness (void);

#ifdef WORDS_BIGENDIAN
#define H2_LOCAL_ENDIANNESS H2_BIG_ENDIAN
#else
#define H2_LOCAL_ENDIANNESS H2_LITTLE_ENDIAN
#endif

/* If the macro is not defined (h2endiannessMacro.h) then call C function */
#ifndef H2_LOCAL_ENDIANNESS
#define H2_LOCAL_ENDIANNESS (h2localEndianness())
#endif

#ifdef __cplusplus
}
#endif

#endif /* H2_ENDIANNESS_H */

