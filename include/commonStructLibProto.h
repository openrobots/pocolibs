/* $LAAS$ */
/*
 * Copyright (c) 1991, 2003 CNRS/LAAS
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

#ifndef _COMMON_STRUCT_LIB_PROTO_H
#define _COMMON_STRUCT_LIB_PROTO_H
#ifdef __STDC__

extern STATUS commonStructCopy ( void *pCommonStruct, int toFromFlag, void *pBuf );
extern STATUS commonStructCreate ( int len, void **pStructAdrs );
extern STATUS commonStructDelete ( void *pCommonStruct );
extern STATUS commonStructGive ( void *pCommonStruct );
extern STATUS commonStructTake ( void *pCommonStruct );

#else /* __STDC__ */

extern STATUS commonStructCopy (/* void *pCommonStruct, int toFromFlag, void *pBuf */);
extern STATUS commonStructCreate (/* int len, void **pStructAdrs */);
extern STATUS commonStructDelete (/* void *pCommonStruct */);
extern STATUS commonStructGive (/* void *pCommonStruct */);
extern STATUS commonStructTake (/* void *pCommonStruct */);

#endif /* __STDC__ */
#endif /* _COMMON_STRUCT_LIB_PROTO_H */
